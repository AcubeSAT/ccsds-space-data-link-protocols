#include <CCSDSServiceChannel.hpp>
#include <TransferFrameTM.hpp>
#include <etl/iterator.h>
#include "CLCW.hpp"

ServiceChannelNotification ServiceChannel::storeTC(uint8_t* packet, uint16_t packetLength) {
	if (masterChannel.rxMasterCopyTC.full()) {
		ccsdsLogNotice(Rx, TypeServiceChannelNotif, RX_IN_MC_FULL);
		return ServiceChannelNotification::RX_IN_MC_FULL;
	}

	if (masterChannel.rxInFramesBeforeAllFramesReceptionListTC.full()) {
		ccsdsLogNotice(Rx, TypeServiceChannelNotif, RX_IN_BUFFER_FULL);
		return ServiceChannelNotification::RX_IN_BUFFER_FULL;
	}

	TransferFrameTC pckt = TransferFrameTC(packet, packetLength);

	if (pckt.getFrameLength() != packetLength) {
		ccsdsLogNotice(Rx, TypeServiceChannelNotif, RX_INVALID_LENGTH);
		return ServiceChannelNotification::RX_INVALID_LENGTH;
	}

	uint8_t vid = pckt.virtualChannelId();
	uint8_t mapid = pckt.mapId();

	// Check if Virtual Channel Id does not exist in the relevant Virtual Channels map
	if (masterChannel.virtualChannels.find(vid) == masterChannel.virtualChannels.end()) {
		// If it doesn't, abort operation
		ccsdsLogNotice(Tx, TypeServiceChannelNotif, INVALID_VC_ID);
		return ServiceChannelNotification::INVALID_VC_ID;
	}

	VirtualChannel* vchan = &(masterChannel.virtualChannels.at(vid));

	// If Segment Header present, check if MAP channel Id does not exist in the relevant MAP Channels map
	if (vchan->segmentHeaderPresent && (vchan->mapChannels.find(mapid) == vchan->mapChannels.end())) {
		// If it doesn't, abort the operation
		ccsdsLogNotice(Tx, TypeServiceChannelNotif, INVALID_MAP_ID);
		return ServiceChannelNotification::INVALID_MAP_ID;
	}

	masterChannel.rxMasterCopyTC.push_back(pckt);
	TransferFrameTC* masterPckt = &(masterChannel.rxMasterCopyTC.back());

	masterChannel.rxInFramesBeforeAllFramesReceptionListTC.push_back(masterPckt);
	return ServiceChannelNotification::NO_SERVICE_EVENT;
}

ServiceChannelNotification ServiceChannel::packetExtractionTM(uint8_t vid, uint8_t* packet_target) {
	VirtualChannel* virtualChannel = &(masterChannel.virtualChannels.at(vid));

	if (virtualChannel->rxInFramesAfterMCReception.full()) {
		ccsdsLogNotice(Rx, TypeServiceChannelNotif, RX_IN_BUFFER_FULL);
		return ServiceChannelNotification::RX_IN_BUFFER_FULL;
	}

	if (virtualChannel->rxInFramesAfterMCReception.empty()) {
		ccsdsLogNotice(Rx, TypeServiceChannelNotif, NO_RX_PACKETS_TO_PROCESS);
		return ServiceChannelNotification::NO_RX_PACKETS_TO_PROCESS;
	}
	TransferFrameTM* packet = virtualChannel->rxInFramesAfterMCReception.front();

	uint16_t frameSize = packet->getPacketLength();
	uint8_t headerSize = 5 + virtualChannel->secondaryHeaderTMLength;
	uint8_t trailerSize =
	    4 * packet->operationalControlFieldExists() + 2 * virtualChannel->frameErrorControlFieldPresent;
	memcpy(packet_target, packet->packetData() + headerSize + 1, frameSize - headerSize - trailerSize);

	virtualChannel->rxInFramesAfterMCReception.pop_front();
	masterChannel.removeMasterRx(packet);

	return ServiceChannelNotification::NO_SERVICE_EVENT;
}

ServiceChannelNotification ServiceChannel::vcGenerationServiceTM(uint16_t transferFrameDataLength, uint8_t vid) {
	VirtualChannel& vchan = masterChannel.virtualChannels.at(vid);

	uint16_t currentTransferFrameDataLength = 0;
	if (vchan.packetLengthBufferTmTx.empty()) {
		return PACKET_BUFFER_EMPTY;
	}
	uint16_t packetLength = vchan.packetLengthBufferTmTx.front();
	uint16_t numberOfTransferFrames =
	    packetLength / transferFrameDataLength + (packetLength % transferFrameDataLength != 0);
	if (masterChannel.txMasterCopyTM.available() < numberOfTransferFrames) {
		return MASTER_CHANNEL_FRAME_BUFFER_FULL;
	}
	if (masterChannel.txProcessedPacketListBufferTM.available() < numberOfTransferFrames) {
		return TX_MC_FRAME_BUFFER_FULL;
	}
	if (numberOfTransferFrames <= 1) {
		return blockingTM(transferFrameDataLength, packetLength, vid);
	}
	return segmentationTM(numberOfTransferFrames, packetLength, transferFrameDataLength, vid);
}

ServiceChannelNotification ServiceChannel::storePacketTc(uint8_t *packet, uint16_t packetLength, uint8_t vid, uint8_t mapid,
                                                         ServiceType service_type) {
    if (masterChannel.virtualChannels.find(vid) == masterChannel.virtualChannels.end()) {
        ccsdsLogNotice(Tx, TypeServiceChannelNotif, INVALID_VC_ID);
        return ServiceChannelNotification::INVALID_VC_ID;
    }

	VirtualChannel *vchan = &(masterChannel.virtualChannels.at(vid));

    if (vchan->mapChannels.find(mapid) == vchan->mapChannels.end()) {
        ccsdsLogNotice(Tx, TypeServiceChannelNotif, INVALID_MAP_ID);
        return ServiceChannelNotification::INVALID_MAP_ID;
    }

	MAPChannel* mapChannel = &(vchan->mapChannels.at(mapid));

    etl::queue<uint16_t, PacketBufferTmSize> *packetLengthBufferTcTx;
    etl::queue<uint8_t, PacketBufferTmSize> *packetBufferTcTx;

	if (service_type == ServiceType::TYPE_AD){
		packetLengthBufferTcTx = &mapChannel->packetLengthBufferTcTxTypeA;
        packetBufferTcTx = &mapChannel->packetBufferTcTxTypeA;
	}
	else {
        packetLengthBufferTcTx = &mapChannel->packetLengthBufferTcTxTypeB;
        packetBufferTcTx = &mapChannel->packetBufferTcTxTypeB;
	}

    if (packetLength <= packetBufferTcTx->available()) {
        packetLengthBufferTcTx->push(packetLength);
        for (uint16_t i = 0; i < packetLength; i++) {
            packetBufferTcTx->push(packet[i]);
        }
        return NO_SERVICE_EVENT;
    }
    return VC_MC_FRAME_BUFFER_FULL;
}


ServiceChannelNotification ServiceChannel::storePacketTm(uint8_t* packet, uint16_t packetLength, uint8_t vid) {
    if (masterChannel.virtualChannels.find(vid) == masterChannel.virtualChannels.end()) {
        ccsdsLogNotice(Tx, TypeServiceChannelNotif, INVALID_VC_ID);
        return ServiceChannelNotification::INVALID_VC_ID;
    }

    VirtualChannel* vchan = &(masterChannel.virtualChannels.at(vid));

	if (packetLength <= vchan->packetBufferTmTx.available()) {
		vchan->packetLengthBufferTmTx.push(packetLength);
		for (uint16_t i = 0; i < packetLength; i++) {
			vchan->packetBufferTmTx.push(packet[i]);
		}
		return NO_SERVICE_EVENT;
	}
	return VC_MC_FRAME_BUFFER_FULL;
}

ServiceChannelNotification ServiceChannel::mappRequest(uint8_t vid, uint8_t mapid, uint8_t transferFrameDataLength, ServiceType serviceType) {
    if (masterChannel.virtualChannels.find(vid) == masterChannel.virtualChannels.end()) {
        ccsdsLogNotice(Tx, TypeServiceChannelNotif, INVALID_VC_ID);
        return ServiceChannelNotification::INVALID_VC_ID;
    }

    VirtualChannel& virtualChannel = masterChannel.virtualChannels.at(vid);

    if (virtualChannel.mapChannels.find(mapid) == virtualChannel.mapChannels.end()) {
        ccsdsLogNotice(Tx, TypeServiceChannelNotif, INVALID_MAP_ID);
        return ServiceChannelNotification::INVALID_MAP_ID;
    }

    MAPChannel& mapChannel = virtualChannel.mapChannels.at(mapid);

    etl::queue<uint16_t, PacketBufferTmSize> *packetLengthBufferTcTx;
    etl::queue<uint8_t, PacketBufferTmSize> *packetBufferTcTx;

    if (serviceType == ServiceType::TYPE_AD){
        packetLengthBufferTcTx = &mapChannel.packetLengthBufferTcTxTypeA;
        packetBufferTcTx = &mapChannel.packetBufferTcTxTypeA;
    }
    else {
        packetLengthBufferTcTx = &mapChannel.packetLengthBufferTcTxTypeB;
        packetBufferTcTx = &mapChannel.packetBufferTcTxTypeB;
    }

	if (virtualChannel.waitQueueTxTC.full()) {
		ccsdsLogNotice(Tx, TypeServiceChannelNotif, VC_MC_FRAME_BUFFER_FULL);

		return ServiceChannelNotification::VC_MC_FRAME_BUFFER_FULL;
	}

    static uint8_t tmpData[MaxTcTransferFrameSize] = {0, 0, 0, 0, 0};
	TransferFrameTC* packet = mapChannel.unprocessedPacketListBufferTC.front();

    uint16_t packetLength = packetLengthBufferTcTx->front();
    uint16_t numberOfTransferFrames =
        packetLength / transferFrameDataLength + (packetLength % transferFrameDataLength != 0);

    const uint16_t maxFrameLength = virtualChannel.maxFrameLengthTC;
	bool segmentationEnabled = virtualChannel.segmentHeaderPresent;

	const uint16_t maxPacketLength = maxFrameLength - (TcPrimaryHeaderSize + segmentationEnabled * 1U);

    if (numberOfTransferFrames >= 1) {
        return segmentationTC(numberOfTransferFrames, maxPacketLength, transferFrameDataLength, vid, mapid, serviceType);
    }

    blockingTC(transferFrameDataLength, packetLength, vid, mapid, serviceType);
	return ServiceChannelNotification::NO_SERVICE_EVENT;
}

ServiceChannelNotification ServiceChannel::mcGenerationTMRequest() {
	if (masterChannel.txProcessedPacketListBufferTM.empty()) {
		ccsdsLogNotice(Rx, TypeServiceChannelNotif, NO_RX_PACKETS_TO_PROCESS);
		return ServiceChannelNotification::NO_RX_PACKETS_TO_PROCESS;
	}
	if (masterChannel.txToBeTransmittedFramesAfterMCGenerationListTM.full()) {
		ccsdsLogNotice(Tx, TypeServiceChannelNotif, TX_MC_FRAME_BUFFER_FULL);
		return ServiceChannelNotification::TX_MC_FRAME_BUFFER_FULL;
	}
	TransferFrameTM* packet = masterChannel.txProcessedPacketListBufferTM.front();

	// Check if need to add secondary header and act accordingly
	// TODO: Process secondary headers

	// set master channel frame counter
	packet->setMasterChannelFrameCount(masterChannel.currFrameCountTM);

	// increment master channel frame counter
	masterChannel.currFrameCountTM = masterChannel.currFrameCountTM <= 254 ? masterChannel.currFrameCountTM : 0;
	masterChannel.txToBeTransmittedFramesAfterMCGenerationListTM.push_back(packet);
	masterChannel.txProcessedPacketListBufferTM.pop_front();

	ccsdsLogNotice(Rx, TypeServiceChannelNotif, NO_SERVICE_EVENT);
	return ServiceChannelNotification::NO_SERVICE_EVENT;
}

ServiceChannelNotification ServiceChannel::vcGenerationRequestTC(uint8_t vid) {
	VirtualChannel& virt_channel = masterChannel.virtualChannels.at(vid);
	if (virt_channel.txUnprocessedPacketListBufferTC.empty()) {
		ccsdsLogNotice(Tx, TypeServiceChannelNotif, NO_TX_PACKETS_TO_PROCESS);
		return ServiceChannelNotification::NO_TX_PACKETS_TO_PROCESS;
	}

	if (masterChannel.txOutFramesBeforeAllFramesGenerationListTC.full()) {
		ccsdsLogNotice(Tx, TypeServiceChannelNotif, TX_MC_FRAME_BUFFER_FULL);
		return ServiceChannelNotification::TX_MC_FRAME_BUFFER_FULL;
	}

	TransferFrameTC& frame = *virt_channel.txUnprocessedPacketListBufferTC.front();
	COPDirectiveResponse err = COPDirectiveResponse::ACCEPT;

	err = virt_channel.fop.transferFdu();

	MasterChannelAlert mc = virt_channel.master_channel().storeOut(&frame);
	if (mc != MasterChannelAlert::NO_MC_ALERT) {
		ccsdsLogNotice(Tx, TypeCOPDirectiveResponse, REJECT);
		return ServiceChannelNotification::FOP_REQUEST_REJECTED;
	}

	if (err == COPDirectiveResponse::REJECT) {
		ccsdsLogNotice(Tx, TypeServiceChannelNotif, FOP_REQUEST_REJECTED);
		return ServiceChannelNotification::FOP_REQUEST_REJECTED;
	}
	virt_channel.txUnprocessedPacketListBufferTC.pop_front();
	return ServiceChannelNotification::NO_SERVICE_EVENT;
}

ServiceChannelNotification ServiceChannel::vcReceptionTC(uint8_t vid) {
	VirtualChannel& virtChannel = masterChannel.virtualChannels.at(vid);

	if (virtChannel.waitQueueRxTC.empty()) {
		ccsdsLogNotice(Rx, TypeServiceChannelNotif, NO_PACKETS_TO_PROCESS_IN_VC_RECEPTION_BEFORE_FARM);
		return ServiceChannelNotification::NO_PACKETS_TO_PROCESS_IN_VC_RECEPTION_BEFORE_FARM;
	}

	if (virtChannel.rxInFramesAfterVCReception.full()) {
		ccsdsLogNotice(Rx, TypeServiceChannelNotif, VC_RECEPTION_BUFFER_AFTER_FARM_FULL);
		return ServiceChannelNotification::VC_RECEPTION_BUFFER_AFTER_FARM_FULL;
	}

	TransferFrameTC* frame = virtChannel.waitQueueRxTC.front();

	// FARM procedures
	virtChannel.farm.frameArrives();

	CLCW clcw =
	    CLCW(0, 0, 0, 1, vid, 0, 0, 1, virtChannel.farm.lockout, virtChannel.farm.wait, virtChannel.farm.retransmit,
	         virtChannel.farm.farmBCount, 0, virtChannel.farm.receiverFrameSeqNumber);

	// add idle data
	for (uint8_t i = TmPrimaryHeaderSize; i < TmTransferFrameSize - 2 * virtChannel.frameErrorControlFieldPresent;
	     i++) {
		// add idle data
		clcwTransferFrameDataBuffer[i] = idle_data[i];
	}
	TransferFrameTM clcwTransferFrame =
	    TransferFrameTM(clcwTransferFrameDataBuffer, TmTransferFrameSize, virtChannel.frameCountTM, vid,
	                    virtChannel.frameErrorControlFieldPresent, virtChannel.secondaryHeaderTMPresent, NoSegmentation,
	                    virtChannel.synchronization, clcw.clcw, TM);
	if (!clcwTransferFrameBuffer.empty()) {
		clcwTransferFrameBuffer.pop_front();
	}
	clcwTransferFrameBuffer.push_back(clcwTransferFrame);
	clcwWaitingToBeTransmitted = true;

	// If MAP channels are implemented in this specific VC, write to the MAP buffer
	if (virtChannel.segmentHeaderPresent) {
		uint8_t mapid = frame->mapId();
		MAPChannel& mapChannel = virtChannel.mapChannels.at(mapid);
		mapChannel.rxInFramesAfterVCReception.push_back(frame);
	} else {
		virtChannel.rxInFramesAfterVCReception.push_back(frame);
	}
	return ServiceChannelNotification::NO_SERVICE_EVENT;
}

ServiceChannelNotification ServiceChannel::packetExtractionTC(uint8_t vid, uint8_t mapid, uint8_t* packet) {
	VirtualChannel& virtualChannel = masterChannel.virtualChannels.at(vid);
	// We can't call the MAP Packet Extraction service if no segmentation header is present
	if (!virtualChannel.segmentHeaderPresent) {
		ccsdsLogNotice(Rx, TypeServiceChannelNotif, INVALID_SERVICE_CALL);
		return ServiceChannelNotification::INVALID_SERVICE_CALL;
	}

	MAPChannel& mapChannel = virtualChannel.mapChannels.at(mapid);

	if (mapChannel.rxInFramesAfterVCReception.empty()) {
		ccsdsLogNotice(Rx, TypeServiceChannelNotif, NO_RX_PACKETS_TO_PROCESS);
		return ServiceChannelNotification::NO_RX_PACKETS_TO_PROCESS;
	}

	TransferFrameTC* frame = mapChannel.rxInFramesAfterVCReception.front();

	uint16_t frameSize = frame->packetLength();
	uint8_t headerSize = TcPrimaryHeaderSize + 1; // Segment header is present

	uint8_t trailerSize = 2 * virtualChannel.frameErrorControlFieldPresent;

	memcpy(packet, frame->packetData() + headerSize, frameSize - headerSize - trailerSize);

	mapChannel.rxInFramesAfterVCReception.pop_front();
	masterChannel.removeMasterRx(frame);

	return ServiceChannelNotification::NO_SERVICE_EVENT;
}

ServiceChannelNotification ServiceChannel::packetExtractionTC(uint8_t vid, uint8_t* packet) {
	VirtualChannel* virtualChannel = &(masterChannel.virtualChannels.at(vid));

	// If segmentation header exists, then the MAP packet extraction service needs to be called
	if (virtualChannel->segmentHeaderPresent) {
		ccsdsLogNotice(Rx, TypeServiceChannelNotif, INVALID_SERVICE_CALL);
		return ServiceChannelNotification::INVALID_SERVICE_CALL;
	}

	if (virtualChannel->rxInFramesAfterVCReception.empty()) {
		ccsdsLogNotice(Rx, TypeServiceChannelNotif, NO_RX_PACKETS_TO_PROCESS);
		return ServiceChannelNotification::NO_RX_PACKETS_TO_PROCESS;
	}

	TransferFrameTC* frame = virtualChannel->rxInFramesAfterVCReception.front();

	uint16_t frameSize = frame->packetLength();
	uint8_t headerSize = TcPrimaryHeaderSize; // Segment header is present
	uint8_t trailerSize = 2 * virtualChannel->frameErrorControlFieldPresent;

	memcpy(packet, frame->packetData() + headerSize, frameSize - headerSize - trailerSize);

	virtualChannel->rxInFramesAfterVCReception.pop_front();

	return ServiceChannelNotification::NO_SERVICE_EVENT;
}

ServiceChannelNotification ServiceChannel::allFramesReceptionTCRequest() {
	if (masterChannel.rxInFramesBeforeAllFramesReceptionListTC.empty()) {
		ccsdsLogNotice(Rx, TypeServiceChannelNotif, NO_RX_PACKETS_TO_PROCESS);
		return ServiceChannelNotification::NO_RX_PACKETS_TO_PROCESS;
	}

	if (masterChannel.rxToBeTransmittedFramesAfterAllFramesReceptionListTC.full()) {
		ccsdsLogNotice(Rx, TypeServiceChannelNotif, RX_OUT_BUFFER_FULL);
		return ServiceChannelNotification::RX_OUT_BUFFER_FULL;
	}

	TransferFrameTC* frame = masterChannel.rxInFramesBeforeAllFramesReceptionListTC.front();
	VirtualChannel& virtualChannel = masterChannel.virtualChannels.at(frame->virtualChannelId());

	if (virtualChannel.waitQueueRxTC.full()) {
		ccsdsLogNotice(Rx, TypeServiceChannelNotif, VC_RX_WAIT_QUEUE_FULL);
		return ServiceChannelNotification::VC_RX_WAIT_QUEUE_FULL;
	}

	// Frame Delimiting and Fill Removal supposedly aren't implemented here

	/*
	 * Frame validation checks
	 */

	// Check for valid TFVN
	if (frame->getTransferFrameVersionNumber() != 0) {
		ccsdsLogNotice(Rx, TypeServiceChannelNotif, RX_INVALID_TFVN);
		return ServiceChannelNotification::RX_INVALID_TFVN;
	}

	// Check for valid SCID
	if (frame->spacecraftId() != SpacecraftIdentifier) {
		ccsdsLogNotice(Rx, TypeServiceChannelNotif, RX_INVALID_SCID);
		return ServiceChannelNotification::RX_INVALID_SCID;
	}

	// TransferFrameTC length is checked upon storing the packet in the MC

	// If present in channel, check if CRC is valid
	bool eccFieldExists = virtualChannel.frameErrorControlFieldPresent;

	if (eccFieldExists) {
		uint16_t len = frame->packetLength() - 2;
		uint16_t crc = frame->calculateCRC(frame->packetData(), len);

		uint16_t packet_crc = (static_cast<uint16_t>(frame->packetData()[len]) << 8) | frame->packetData()[len + 1];
		if (crc != packet_crc) {
			ccsdsLogNotice(Rx, TypeServiceChannelNotif, RX_INVALID_CRC);
			// Invalid packet is discarded and service aborted
			masterChannel.removeMasterRx(frame);
			masterChannel.rxInFramesBeforeAllFramesReceptionListTC.pop_front();
			return ServiceChannelNotification::RX_INVALID_CRC;
		}
		virtualChannel.waitQueueRxTC.push_back(frame);
		masterChannel.rxInFramesBeforeAllFramesReceptionListTC.pop_front();
		return ServiceChannelNotification::NO_SERVICE_EVENT;
	}
	virtualChannel.waitQueueRxTC.push_back(frame);
	masterChannel.rxInFramesBeforeAllFramesReceptionListTC.pop_front();
	return ServiceChannelNotification::NO_SERVICE_EVENT;
}

std::optional<TransferFrameTC> ServiceChannel::getTxProcessedPacket() {
	if (masterChannel.txOutFramesBeforeAllFramesGenerationListTC.empty()) {
		return {};
	}

	TransferFrameTC packet = *masterChannel.txOutFramesBeforeAllFramesGenerationListTC.front();

	// TODO: Here the packet should probably be deleted from the master buffer
	return packet;
}

ServiceChannelNotification ServiceChannel::allFramesGenerationTCRequest() {
    if (masterChannel.txOutFramesBeforeAllFramesGenerationListTC.empty()) {
        ccsdsLogNotice(Tx, TypeServiceChannelNotif, NO_TX_PACKETS_TO_PROCESS);
        return ServiceChannelNotification::NO_TX_PACKETS_TO_PROCESS;
    }

    if (masterChannel.txToBeTransmittedFramesAfterAllFramesGenerationListTC.full()) {
        ccsdsLogNotice(Tx, TypeServiceChannelNotif, TX_TO_BE_TRANSMITTED_FRAMES_LIST_FULL);
        return ServiceChannelNotification::TX_TO_BE_TRANSMITTED_FRAMES_LIST_FULL;
    }

    TransferFrameTC* packet = masterChannel.txOutFramesBeforeAllFramesGenerationListTC.front();
    masterChannel.txOutFramesBeforeAllFramesGenerationListTC.pop_front();

    uint8_t vid = packet->virtualChannelId();
    VirtualChannel& vchan = masterChannel.virtualChannels.at(vid);

    if (vchan.frameErrorControlFieldPresent) {
        packet->appendCRC();
    }

    masterChannel.storeTransmittedOut(packet);
    ccsdsLogNotice(Tx, TypeServiceChannelNotif, NO_SERVICE_EVENT);
    return ServiceChannelNotification::NO_SERVICE_EVENT;
}


ServiceChannelNotification ServiceChannel::allFramesGenerationTMRequest(uint8_t* packet_data, uint16_t packet_length) {
	if (masterChannel.txToBeTransmittedFramesAfterMCGenerationListTM.empty()) {
		return ServiceChannelNotification::NO_TX_PACKETS_TO_PROCESS;
	}

	TransferFrameTM* packet = masterChannel.txToBeTransmittedFramesAfterMCGenerationListTM.front();

	uint8_t vid = packet->virtualChannelId();
	VirtualChannel& vchan = masterChannel.virtualChannels.at(vid);

	if (vchan.frameErrorControlFieldPresent) {
		packet->appendCRC();
	}

	if (packet->getPacketLength() > packet_length) {
		return ServiceChannelNotification::RX_INVALID_LENGTH;
	}

	uint16_t frameSize = packet->getPacketLength();
	uint16_t idleDataSize = TmTransferFrameSize - frameSize;
	uint8_t trailerSize = 4 * packet->operationalControlFieldExists() + 2 * vchan.frameErrorControlFieldPresent;

	// Copy frame without the trailer
	memcpy(packet_data, packet->packetData(), frameSize - trailerSize);

	// Append idle data
	memcpy(packet_data + frameSize - trailerSize, idle_data, idleDataSize);

	// Append trailer
	memcpy(packet_data + TmTransferFrameSize - trailerSize, packet->packetData() + packet_length - trailerSize,
	       trailerSize);

	masterChannel.txToBeTransmittedFramesAfterMCGenerationListTM.pop_front();
	// Finally, remove master copy
	masterChannel.removeMasterTx(packet);
	masterChannel.masterChannelPoolTM.deletePacket(packet->packetData(), packet->getPacketLength());

	return ServiceChannelNotification::NO_SERVICE_EVENT;
}

ServiceChannelNotification ServiceChannel::allFramesReceptionTMRequest(uint8_t* packet, uint16_t packetLength) {
	if (masterChannel.rxMasterCopyTM.full()) {
		ccsdsLogNotice(Rx, TypeServiceChannelNotif, RX_IN_MC_FULL);
		return ServiceChannelNotification::RX_IN_MC_FULL;
	}

	uint8_t vid = (packet[1] >> 1) & 0x7;
	// Check if Virtual channel Id does not exist in the relevant Virtual Channels map
	if (masterChannel.virtualChannels.find(vid) == masterChannel.virtualChannels.end()) {
		// If it doesn't, abort operation
		ccsdsLogNotice(Rx, TypeServiceChannelNotif, INVALID_VC_ID);
		return ServiceChannelNotification::INVALID_VC_ID;
	}

	VirtualChannel* virtualChannel = &(masterChannel.virtualChannels.at(vid));
	TransferFrameTM frame = TransferFrameTM(packet, packetLength, virtualChannel->frameErrorControlFieldPresent);
	bool eccFieldExists = virtualChannel->frameErrorControlFieldPresent;

	if (virtualChannel->rxInFramesAfterMCReception.full()) {
		ccsdsLogNotice(Rx, TypeServiceChannelNotif, RX_IN_BUFFER_FULL);
		return ServiceChannelNotification::RX_IN_BUFFER_FULL;
	}

	if (eccFieldExists) {
		uint16_t len = frame.getPacketLength() - 2;
		uint16_t crc = frame.calculateCRC(frame.packetData(), len);

		uint16_t packet_crc =
		    ((static_cast<uint16_t>(frame.packetData()[len]) << 8) & 0xFF00) | frame.packetData()[len + 1];
		if (crc != packet_crc) {
			ccsdsLogNotice(Rx, TypeServiceChannelNotif, RX_INVALID_CRC);
			// Invalid packet is discarded and service aborted
			return ServiceChannelNotification::RX_INVALID_CRC;
		}
	}
	// Master Channel Reception
	uint8_t mc_lost_frames = frame.getMasterChannelFrameCount();

	// Check if master channel frames have been lost
	uint8_t mc_counter_diff = (mc_lost_frames - masterChannel.currFrameCountTM) % 0xFF;

	if (mc_counter_diff > 1) {
		// Log error that frames have been lost, but don't abort processing
		ccsdsLogNotice<uint8_t>(Rx, TypeServiceChannelNotif, MC_RX_INVALID_COUNT, mc_counter_diff);
	}
	// CLCW extraction
	std::optional<uint32_t> operationalControlField = frame.getOperationalControlField();
	if (operationalControlField.has_value() && operationalControlField.value() >> 31 == 0) {
		CLCW clcw = CLCW(operationalControlField.value());
		virtualChannel->currentlyProcessedCLCW = CLCW(clcw.getClcw());
		virtualChannel->fop.validClcwArrival();
		virtualChannel->fop.acknowledgePreviousFrames(clcw.getReportValue());
	}
	// TODO: Will we use secondary headers? If so they need to be processed here and forward to the respective service
	masterChannel.rxMasterCopyTM.push_back(frame);
	TransferFrameTM* masterPacket = &(masterChannel.rxMasterCopyTM.back());
	virtualChannel->rxInFramesAfterMCReception.push_back(masterPacket);

	return ServiceChannelNotification::NO_SERVICE_EVENT;
}

ServiceChannelNotification ServiceChannel::frameTransmission(uint8_t* tframe) {
    if (masterChannel.txToBeTransmittedFramesAfterAllFramesGenerationListTC.empty()) {
        ccsdsLogNotice(Tx, TypeServiceChannelNotif, TX_TO_BE_TRANSMITTED_FRAMES_LIST_EMPTY);
        return ServiceChannelNotification::TX_TO_BE_TRANSMITTED_FRAMES_LIST_EMPTY;
    }

    TransferFrameTC* frame = masterChannel.txToBeTransmittedFramesAfterAllFramesGenerationListTC.front();
    frame->setRepetitions(frame->repetitions() - 1);
    frame->setToTransmitted();

    if (frame->repetitions() == 0) {
        masterChannel.txToBeTransmittedFramesAfterAllFramesGenerationListTC.pop_front();
        ccsdsLogNotice(Tx, TypeServiceChannelNotif, NO_SERVICE_EVENT);
    }
    memcpy(tframe, frame, frame->getFrameLength());
    return ServiceChannelNotification::NO_SERVICE_EVENT;
}


ServiceChannelNotification ServiceChannel::transmitAdFrame(uint8_t vid) {
	VirtualChannel* virt_channel = &(masterChannel.virtualChannels.at(vid));
	FOPNotification req;
	req = virt_channel->fop.transmitAdFrame();
	if (req == FOPNotification::NO_FOP_EVENT) {
		ccsdsLogNotice(Tx, TypeServiceChannelNotif, NO_SERVICE_EVENT);
		return ServiceChannelNotification::NO_SERVICE_EVENT;

	} else {
		// TODO
	}
	return ServiceChannelNotification::NO_SERVICE_EVENT;
}

// TODO: Probably not needed. Refactor sentQueueTC
ServiceChannelNotification ServiceChannel::pushSentQueue(uint8_t vid) {
	VirtualChannel* virt_channel = &(masterChannel.virtualChannels.at(vid));
	COPDirectiveResponse req;
	req = virt_channel->fop.pushSentQueue();

	if (req == COPDirectiveResponse::ACCEPT) {
		ccsdsLogNotice(Tx, TypeServiceChannelNotif, NO_SERVICE_EVENT);
		return ServiceChannelNotification::NO_SERVICE_EVENT;
	}
	ccsdsLogNotice(Tx, TypeServiceChannelNotif, TX_FOP_REJECTED);
	return ServiceChannelNotification::TX_FOP_REJECTED;
}

void ServiceChannel::acknowledgeFrame(uint8_t vid, uint8_t frameSeqNumber) {
	VirtualChannel* virt_channel = &(masterChannel.virtualChannels.at(vid));
	virt_channel->fop.acknowledgeFrame(frameSeqNumber);
}

void ServiceChannel::clearAcknowledgedFrames(uint8_t vid) {
	VirtualChannel* virtualChannel = &(masterChannel.virtualChannels.at(vid));
	virtualChannel->fop.removeAcknowledgedFrames();
}

void ServiceChannel::initiateAdNoClcw(uint8_t vid) {
	VirtualChannel* virtualChannel = &(masterChannel.virtualChannels.at(vid));
	FDURequestType req;
	req = virtualChannel->fop.initiateAdNoClcw();
}

void ServiceChannel::initiateAdClcw(uint8_t vid) {
	VirtualChannel* virtualChannel = &(masterChannel.virtualChannels.at(vid));
	FDURequestType req;
	req = virtualChannel->fop.initiateAdClcw();
}

void ServiceChannel::initiateAdUnlock(uint8_t vid) {
	VirtualChannel* virtualChannel = &(masterChannel.virtualChannels.at(vid));
	FDURequestType req;
	req = virtualChannel->fop.initiateAdUnlock();
}

void ServiceChannel::initiateAdVr(uint8_t vid, uint8_t vr) {
	VirtualChannel* virtualChannel = &(masterChannel.virtualChannels.at(vid));
	FDURequestType req;
	req = virtualChannel->fop.initiateAdVr(vr);
}

void ServiceChannel::terminateAdService(uint8_t vid) {
	VirtualChannel* virtualChannel = &(masterChannel.virtualChannels.at(vid));
	FDURequestType req;
	req = virtualChannel->fop.terminateAdService();
}

void ServiceChannel::resumeAdService(uint8_t vid) {
	VirtualChannel* virtualChannel = &(masterChannel.virtualChannels.at(vid));
	FDURequestType req;
	req = virtualChannel->fop.resumeAdService();
}

void ServiceChannel::setVs(uint8_t vid, uint8_t vs) {
	VirtualChannel* virtualChannel = &(masterChannel.virtualChannels.at(vid));
	virtualChannel->fop.setVs(vs);
}

void ServiceChannel::setFopWidth(uint8_t vid, uint8_t width) {
	VirtualChannel* virtualChannel = &(masterChannel.virtualChannels.at(vid));
	virtualChannel->fop.setFopWidth(width);
}

void ServiceChannel::setT1Initial(uint8_t vid, uint16_t t1Init) {
	VirtualChannel* virtualChannel = &(masterChannel.virtualChannels.at(vid));
	virtualChannel->fop.setT1Initial(t1Init);
}

void ServiceChannel::setTransmissionLimit(uint8_t vid, uint8_t vr) {
	VirtualChannel* virtualChannel = &(masterChannel.virtualChannels.at(vid));
	virtualChannel->fop.setTransmissionLimit(vr);
}

void ServiceChannel::setTimeoutType(uint8_t vid, bool vr) {
	VirtualChannel* virtualChannel = &(masterChannel.virtualChannels.at(vid));
	virtualChannel->fop.setTimeoutType(vr);
}
CLCW ServiceChannel::getClcwInBuffer() {
	CLCW clcw = CLCW(clcwTransferFrameBuffer.front().getOperationalControlField().value());
	return clcw;
}

// todo: this may not be needed since it doesn't affect lower procedures and doesn't change the state in any way
void ServiceChannel::invalidDirective(uint8_t vid) {
	VirtualChannel* virtualChannel = &(masterChannel.virtualChannels.at(vid));
	virtualChannel->fop.invalidDirective();
}

FOPState ServiceChannel::fopState(uint8_t vid) const {
	return masterChannel.virtualChannels.at(vid).fop.state;
}

uint16_t ServiceChannel::t1Timer(uint8_t vid) const {
	return masterChannel.virtualChannels.at(vid).fop.tiInitial;
}

uint8_t ServiceChannel::fopSlidingWindowWidth(uint8_t vid) const {
	return masterChannel.virtualChannels.at(vid).fop.fopSlidingWindow;
}

bool ServiceChannel::timeoutType(uint8_t vid) const {
	return masterChannel.virtualChannels.at(vid).fop.timeoutType;
}

uint8_t ServiceChannel::transmitterFrameSeqNumber(uint8_t vid) const {
	return masterChannel.virtualChannels.at(vid).fop.transmitterFrameSeqNumber;
}

uint8_t ServiceChannel::expectedFrameSeqNumber(uint8_t vid) const {
	return masterChannel.virtualChannels.at(vid).fop.expectedAcknowledgementSeqNumber;
}

uint8_t ServiceChannel::getFrameCountTM(uint8_t vid) {
	return masterChannel.virtualChannels.at(vid).frameCountTM;
}

uint8_t ServiceChannel::getFrameCountTM() {
	return masterChannel.currFrameCountTM;
}

std::pair<ServiceChannelNotification, const TransferFrameTC*> ServiceChannel::txOutPacketTC(uint8_t vid,
                                                                                            uint8_t mapid) const {
	const etl::list<TransferFrameTC*, MaxReceivedTcInMapChannel>* mc =
	    &(masterChannel.virtualChannels.at(vid).mapChannels.at(mapid).unprocessedPacketListBufferTC);
	if (mc->empty()) {
		ccsdsLogNotice(Tx, TypeServiceChannelNotif, NO_TX_PACKETS_TO_PROCESS);
		return std::pair(ServiceChannelNotification::NO_TX_PACKETS_TO_PROCESS, nullptr);
	}

	return std::pair(ServiceChannelNotification::NO_SERVICE_EVENT, mc->front());
}

std::pair<ServiceChannelNotification, const TransferFrameTC*> ServiceChannel::txOutPacketTC(uint8_t vid) const {
	const etl::list<TransferFrameTC*, MaxReceivedUnprocessedTxTcInVirtBuffer>* vc =
	    &(masterChannel.virtualChannels.at(vid).txUnprocessedPacketListBufferTC);
	if (vc->empty()) {
		ccsdsLogNotice(Tx, TypeServiceChannelNotif, NO_TX_PACKETS_TO_PROCESS);
		return std::pair(ServiceChannelNotification::NO_TX_PACKETS_TO_PROCESS, nullptr);
	}
	ccsdsLogNotice(Tx, TypeServiceChannelNotif, NO_SERVICE_EVENT);
	return std::pair(ServiceChannelNotification::NO_SERVICE_EVENT, vc->front());
}

std::pair<ServiceChannelNotification, const TransferFrameTC*> ServiceChannel::txOutPacketTC() const {
	if (masterChannel.txMasterCopyTC.empty()) {
		ccsdsLogNotice(Tx, TypeServiceChannelNotif, NO_TX_PACKETS_TO_PROCESS);
		return std::pair(ServiceChannelNotification::NO_TX_PACKETS_TO_PROCESS, nullptr);
	}
	return std::pair(ServiceChannelNotification::NO_SERVICE_EVENT, &(masterChannel.txMasterCopyTC.back()));
}

std::pair<ServiceChannelNotification, const TransferFrameTM*> ServiceChannel::txOutPacketTM() const {
	if (masterChannel.txMasterCopyTM.empty()) {
		ccsdsLogNotice(Tx, TypeServiceChannelNotif, NO_TX_PACKETS_TO_PROCESS);
		return std::pair(ServiceChannelNotification::NO_TX_PACKETS_TO_PROCESS, nullptr);
	}
	ccsdsLogNotice(Tx, TypeServiceChannelNotif, NO_SERVICE_EVENT);
	return std::pair(ServiceChannelNotification::NO_SERVICE_EVENT, &(masterChannel.txMasterCopyTM.back()));
}

std::pair<ServiceChannelNotification, const TransferFrameTC*> ServiceChannel::txOutProcessedPacketTC() const {
    if (masterChannel.txToBeTransmittedFramesAfterAllFramesGenerationListTC.empty()) {
        ccsdsLogNotice(Tx, TypeServiceChannelNotif, NO_TX_PACKETS_TO_PROCESS);
        return std::pair(ServiceChannelNotification::NO_TX_PACKETS_TO_PROCESS, nullptr);
    }
    ccsdsLogNotice(Tx, TypeServiceChannelNotif, NO_SERVICE_EVENT);
    return std::pair(ServiceChannelNotification::NO_SERVICE_EVENT,
                     masterChannel.txToBeTransmittedFramesAfterAllFramesGenerationListTC.front());
}

std::pair<ServiceChannelNotification, const TransferFrameTM*> ServiceChannel::txOutProcessedPacketTM() const {
	if (masterChannel.txToBeTransmittedFramesAfterAllFramesGenerationListTM.empty()) {
		ccsdsLogNotice(Tx, TypeServiceChannelNotif, NO_TX_PACKETS_TO_PROCESS);
		return std::pair(ServiceChannelNotification::NO_TX_PACKETS_TO_PROCESS, nullptr);
	}
	ccsdsLogNotice(Tx, TypeServiceChannelNotif, NO_SERVICE_EVENT);
	return std::pair(ServiceChannelNotification::NO_SERVICE_EVENT,
	                 masterChannel.txToBeTransmittedFramesAfterAllFramesGenerationListTM.front());
}
uint8_t* ServiceChannel::getClcwTransferFrameDataBuffer() {
	return clcwTransferFrameDataBuffer;
}

ServiceChannelNotification ServiceChannel::blockingTC(uint16_t transferFrameDataLength, uint16_t packetLength,
                                                      uint8_t vid, uint8_t mapid, ServiceType serviceType){
    uint16_t currentTransferFrameDataLength = 0;
    static uint8_t tmpData[MaxTcTransferFrameSize] = {0};

    if (masterChannel.virtualChannels.find(vid) == masterChannel.virtualChannels.end()) {
        ccsdsLogNotice(Tx, TypeServiceChannelNotif, INVALID_VC_ID);
        return ServiceChannelNotification::INVALID_VC_ID;
    }

    VirtualChannel& vchan = masterChannel.virtualChannels.at(vid);

    if (vchan.segmentHeaderPresent && (vchan.mapChannels.find(mapid) == vchan.mapChannels.end())) {
        ccsdsLogNotice(Tx, TypeServiceChannelNotif, INVALID_MAP_ID);
        return ServiceChannelNotification::INVALID_MAP_ID;
    }

    MAPChannel& mapChannel = vchan.mapChannels.at(mapid);

    etl::queue<uint16_t, PacketBufferTcSize> *packetLengthBufferTcTx;
    etl::queue<uint8_t, PacketBufferTcSize> *packetBufferTcTx;

    if (serviceType == ServiceType::TYPE_AD){
        packetLengthBufferTcTx = &mapChannel.packetLengthBufferTcTxTypeA;
        packetBufferTcTx = &mapChannel.packetBufferTcTxTypeA;
    }
    else {
        packetLengthBufferTcTx = &mapChannel.packetLengthBufferTcTxTypeB;
        packetBufferTcTx = &mapChannel.packetBufferTcTxTypeB;
    }

    while (currentTransferFrameDataLength + packetLength <= transferFrameDataLength &&
           !packetLengthBufferTcTx->empty()) {
        for (uint16_t i = currentTransferFrameDataLength; i < currentTransferFrameDataLength + packetLength; i++) {
            tmpData[i + TcPrimaryHeaderSize] = packetBufferTcTx->front();
            packetBufferTcTx->pop();
        }
        currentTransferFrameDataLength += packetLength;
        packetLengthBufferTcTx->pop();
        packetLength = packetLengthBufferTcTx->front();
        // If blocking is disabled, stop the operation on the first packet
        if (!vchan.blocking) {
            break;
        }
    }

    uint8_t tcTrailerSize = 2 * vchan.frameErrorControlFieldPresent;

    uint8_t* transferFrameData = masterChannel.masterChannelPoolTC.allocatePacket(
        tmpData, currentTransferFrameDataLength + TcPrimaryHeaderSize + tcTrailerSize);
    if (transferFrameData == nullptr) {
        return MEMORY_POOL_FULL;
    }
    TransferFrameTC transferFrameTc =
        TransferFrameTC(transferFrameData, currentTransferFrameDataLength + TcPrimaryHeaderSize + tcTrailerSize,
                        vid, serviceType, true);
    masterChannel.txMasterCopyTC.push_back(transferFrameTc);
    vchan.txUnprocessedPacketListBufferTC.push_back(&(masterChannel.txMasterCopyTC.back()));
    return NO_SERVICE_EVENT;
}

ServiceChannelNotification ServiceChannel::blockingTM(uint16_t transferFrameDataLength, uint16_t packetLength,
                                                      uint8_t vid) {
	VirtualChannel& vchan = masterChannel.virtualChannels.at(vid);

	uint16_t currentTransferFrameDataLength = 0;
	static uint8_t tmpData[TmTransferFrameSize] = {0};

	while (currentTransferFrameDataLength + packetLength <= transferFrameDataLength &&
	       !vchan.packetLengthBufferTmTx.empty()) {
		for (uint16_t i = currentTransferFrameDataLength; i < currentTransferFrameDataLength + packetLength; i++) {
			tmpData[i + TmPrimaryHeaderSize] = vchan.packetBufferTmTx.front();
			vchan.packetBufferTmTx.pop();
		}
		currentTransferFrameDataLength += packetLength;
		vchan.packetLengthBufferTmTx.pop();
		packetLength = vchan.packetLengthBufferTmTx.front();
		// If blocking is disabled, stop the operation on the first packet
		if (!vchan.blocking) {
			break;
		}
	}

	for (uint8_t i = 0; i < TmTrailerSize; i++) {
		if (currentTransferFrameDataLength + TmPrimaryHeaderSize + i < TmTransferFrameSize) {
			tmpData[currentTransferFrameDataLength + TmPrimaryHeaderSize + i] = 0;
		}
	}
	uint8_t* transferFrameData = masterChannel.masterChannelPoolTM.allocatePacket(
	    tmpData, currentTransferFrameDataLength + TmPrimaryHeaderSize + TmTrailerSize);
	if (transferFrameData == nullptr) {
		return MEMORY_POOL_FULL;
	}
	vchan.frameCountTM = (vchan.frameCountTM + 1) % 256;
	SegmentLengthID segmentLengthId = NoSegmentation;
	TransferFrameTM transferFrameTm =
	    TransferFrameTM(transferFrameData, currentTransferFrameDataLength + TmPrimaryHeaderSize + TmTrailerSize,
	                    vchan.frameCountTM, vid, vchan.frameErrorControlFieldPresent, vchan.segmentHeaderPresent,
	                    segmentLengthId, vchan.synchronization, TM);
	masterChannel.txMasterCopyTM.push_back(transferFrameTm);
	masterChannel.txProcessedPacketListBufferTM.push_back(&(masterChannel.txMasterCopyTM.back()));
	return NO_SERVICE_EVENT;
}

ServiceChannelNotification ServiceChannel::segmentationTC(uint8_t numberOfTransferFrames, uint16_t packetLength,
                                                          uint16_t transferFrameDataLength, uint8_t vid, uint8_t mapid,
                                                          ServiceType service_type) {
    VirtualChannel& virtualChannel = masterChannel.virtualChannels.at(vid);
    MAPChannel& mapChannel = virtualChannel.mapChannels.at(mapid);

    etl::queue<uint16_t, PacketBufferTcSize> *packetLengthBufferTcTx;
    etl::queue<uint8_t, PacketBufferTcSize> *packetBufferTcTx;

    if (service_type == ServiceType::TYPE_AD){
        packetLengthBufferTcTx = &mapChannel.packetLengthBufferTcTxTypeA;
        packetBufferTcTx = &mapChannel.packetBufferTcTxTypeA;
    }
    else {
        packetLengthBufferTcTx = &mapChannel.packetLengthBufferTcTxTypeB;
        packetBufferTcTx = &mapChannel.packetBufferTcTxTypeB;
    }

    uint16_t currentTransferFrameDataLength = 0;
	uint8_t tcTrailerSize = 2*virtualChannel.frameErrorControlFieldPresent;

    static uint8_t tmpData[MaxTcTransferFrameSize] = {0};
    for (uint16_t i = 0; i < numberOfTransferFrames; i++) {
        currentTransferFrameDataLength =
            packetLength > transferFrameDataLength ? transferFrameDataLength : packetLength;
        SegmentLengthID segmentLengthId = SegmentationMiddle;
        for (uint16_t j = 0; j < currentTransferFrameDataLength; j++) {
            tmpData[j + TmPrimaryHeaderSize] = packetBufferTcTx->front();
            packetBufferTcTx->pop();
        }
        if (i == 0) {
            segmentLengthId = SegmentationStart;
        } else if (i == numberOfTransferFrames - 1) {
            segmentLengthId = SegmentaionEnd;
        }
        for (uint8_t j = 0; j < TmTrailerSize; j++) {
            if (currentTransferFrameDataLength + TcPrimaryHeaderSize + i < MaxTcTransferFrameSize) {
                tmpData[currentTransferFrameDataLength + TcPrimaryHeaderSize + i] = 0;
            }
        }
        uint8_t* transferFrameData = masterChannel.masterChannelPoolTC.allocatePacket(
            tmpData, currentTransferFrameDataLength + TcPrimaryHeaderSize + tcTrailerSize);
        if (transferFrameData == nullptr) {
            return MEMORY_POOL_FULL;
        }
        TransferFrameTC transferFrameTc =
            TransferFrameTC(transferFrameData, currentTransferFrameDataLength + TcPrimaryHeaderSize + tcTrailerSize,
		                    vid, service_type, true);
        masterChannel.txMasterCopyTC.push_back(transferFrameTc);
        virtualChannel.txUnprocessedPacketListBufferTC.push_back(&(masterChannel.txMasterCopyTC.back()));
        packetLength -= transferFrameDataLength;
    }
	packetLengthBufferTcTx->pop();
    return NO_SERVICE_EVENT;
}

ServiceChannelNotification ServiceChannel::segmentationTM(uint8_t numberOfTransferFrames, uint16_t packetLength,
                                                          uint16_t transferFrameDataLength, uint8_t vid) {
	VirtualChannel& vchan = masterChannel.virtualChannels.at(vid);

	uint16_t currentTransferFrameDataLength = 0;
	static uint8_t tmpData[TmTransferFrameSize] = {0};
	for (uint16_t i = 0; i < numberOfTransferFrames; i++) {
		currentTransferFrameDataLength =
		    packetLength > transferFrameDataLength ? transferFrameDataLength : packetLength;
		SegmentLengthID segmentLengthId = SegmentationMiddle;
		for (uint16_t j = 0; j < currentTransferFrameDataLength; j++) {
			tmpData[j + TmPrimaryHeaderSize] = vchan.packetBufferTmTx.front();
			vchan.packetBufferTmTx.pop();
		}
		if (i == 0) {
			segmentLengthId = SegmentationStart;
		} else if (i == numberOfTransferFrames - 1) {
			segmentLengthId = SegmentaionEnd;
		}
		for (uint8_t j = 0; j < TmTrailerSize; j++) {
			if (currentTransferFrameDataLength + TmPrimaryHeaderSize + i < TmTransferFrameSize) {
				tmpData[currentTransferFrameDataLength + TmPrimaryHeaderSize + i] = 0;
			}
		}
		uint8_t* transferFrameData = masterChannel.masterChannelPoolTM.allocatePacket(
		    tmpData, currentTransferFrameDataLength + TmPrimaryHeaderSize + TmTrailerSize);
		if (transferFrameData == nullptr) {
			return MEMORY_POOL_FULL;
		}
		vchan.frameCountTM = (vchan.frameCountTM + 1) % 256;
		TransferFrameTM transferFrameTm =
		    TransferFrameTM(transferFrameData, currentTransferFrameDataLength + TmPrimaryHeaderSize + TmTrailerSize,
		                    vchan.frameCountTM, vid, vchan.frameErrorControlFieldPresent, vchan.segmentHeaderPresent,
		                    segmentLengthId, vchan.synchronization, TM);
		masterChannel.txMasterCopyTM.push_back(transferFrameTm);
		masterChannel.txProcessedPacketListBufferTM.push_back(&(masterChannel.txMasterCopyTM.back()));
		packetLength -= transferFrameDataLength;
	}
	vchan.packetLengthBufferTmTx.pop();
	return NO_SERVICE_EVENT;
}

uint16_t ServiceChannel::availableInPacketLengthBufferTmTx(uint8_t gvcid) {
	uint8_t vid = gvcid & 0x3F;
	VirtualChannel& vchan = masterChannel.virtualChannels.at(vid);

	return vchan.packetLengthBufferTmTx.available();
}

uint16_t ServiceChannel::availableInPacketBufferTmTx(uint8_t gvcid) {
	uint8_t vid = gvcid & 0x3F;
	VirtualChannel& vchan = masterChannel.virtualChannels.at(vid);

	return vchan.packetBufferTmTx.available();
}

TransferFrameTC ServiceChannel::getLastMasterCopyTcFrame() {
	return masterChannel.getLastTxMasterCopyTcFrame();
}

TransferFrameTC ServiceChannel::getFirstMasterCopyTcFrame() {
	return masterChannel.geFirstTxMasterCopyTcFrame();
}