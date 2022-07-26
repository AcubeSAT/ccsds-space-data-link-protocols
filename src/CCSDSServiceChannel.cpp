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

	VirtualChannel<256>* vchan = &(masterChannel.virtualChannels.at(vid));

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

ServiceChannelNotification ServiceChannel::storeTC(uint8_t* packet, uint16_t packetLength, uint8_t gvcid, uint8_t mapid,
                                                   uint16_t sduid, ServiceType serviceType) {
	uint8_t vid = gvcid & 0x3F;

	// Check if Virtual Channel Id does not exist in the relevant Virtual Channels map
	if (masterChannel.virtualChannels.find(vid) == masterChannel.virtualChannels.end()) {
		// If it doesn't, abort operation
		ccsdsLogNotice(Tx, TypeServiceChannelNotif, INVALID_VC_ID);
		return ServiceChannelNotification::INVALID_VC_ID;
	}

	VirtualChannel<256>* virtualChannel = &(masterChannel.virtualChannels.at(vid));

	// Check if MAP Channel Id does not exist in the relevant MAP channels map
	if (virtualChannel->mapChannels.find(mapid) == virtualChannel->mapChannels.end()) {
		// If it doesn't, abort the operation
		ccsdsLogNotice(Tx, TypeServiceChannelNotif, INVALID_MAP_ID);
		return ServiceChannelNotification::INVALID_MAP_ID;
	}

	MAPChannel<128>* mapChannel = &(virtualChannel->mapChannels.at(mapid));

	if (mapChannel->unprocessedPacketListBufferTC.full()) {
		ccsdsLogNotice(Tx, TypeServiceChannelNotif, MAP_CHANNEL_FRAME_BUFFER_FULL);
		return ServiceChannelNotification::MAP_CHANNEL_FRAME_BUFFER_FULL;
	}

	TransferFrameTC packet_s =
	    TransferFrameTC(packet, packetLength, gvcid, serviceType, virtualChannel->segmentHeaderPresent);

	if (serviceType == ServiceType::TYPE_AD) {
		packet_s.setRepetitions(virtualChannel->repetitionTypeAFrame);
	} else if ((serviceType == ServiceType::TYPE_BC) || (serviceType == ServiceType::TYPE_BD)) {
		packet_s.setRepetitions(virtualChannel->repetitionCOPCtrl);
	}

	masterChannel.txMasterCopyTC.push_back(packet_s);
	mapChannel->unprocessedPacketListBufferTC.push_back(&(masterChannel.txMasterCopyTC.back()));
	return ServiceChannelNotification::NO_SERVICE_EVENT;
}

// It is expected for space to be reserved for the Primary and Secondary header (if present) beforehand
ServiceChannelNotification ServiceChannel::storeTM(uint8_t* packet, uint16_t packetLength, uint8_t gvcid) {
	uint8_t vid = gvcid & 0x3F;

	// Check if Virtual Channel Id does not exist in the relevant Virtual chanels map
	if (masterChannel.virtualChannels.find(vid) == masterChannel.virtualChannels.end()) {
		// If it doesn't, abort operation
		ccsdsLogNotice(Tx, TypeServiceChannelNotif, INVALID_VC_ID);
		return ServiceChannelNotification::INVALID_VC_ID;
	}
	VirtualChannel<256>* vchan = &(masterChannel.virtualChannels.at(vid));

	if (masterChannel.txMasterCopyTM.full()) {
		ccsdsLogNotice(Tx, TypeServiceChannelNotif, MASTER_CHANNEL_FRAME_BUFFER_FULL);
		return ServiceChannelNotification::MASTER_CHANNEL_FRAME_BUFFER_FULL;
	}

	if (masterChannel.txProcessedPacketListBufferTM.full()) {
		ccsdsLogNotice(Tx, TypeServiceChannelNotif, TX_MC_FRAME_BUFFER_FULL);
		return ServiceChannelNotification::TX_MC_FRAME_BUFFER_FULL;
	}

	auto hdr = TransferFrameHeaderTM(packet);

	uint8_t* secondaryHeader = 0;

	if (vchan->secondaryHeaderTMPresent) {
		secondaryHeader = &packet[7];
	}

	TransferFrameTM packet_s = TransferFrameTM(
	    packet, packetLength, vchan->frameCountTM, vid, vchan->operationalControlFieldTMPresent,
	    vchan->frameErrorControlFieldPresent, vchan->secondaryHeaderTMPresent, vchan->synchronization);

	// Increment VC frame count. The MC counter is incremented in the Master Channel
	vchan->frameCountTM = vchan->frameCountTM < 255 ? vchan->frameCountTM + 1 : 0;

	masterChannel.txMasterCopyTM.push_back(packet_s);
	masterChannel.txProcessedPacketListBufferTM.push_back(&(masterChannel.txMasterCopyTM.back()));

	ccsdsLogNotice(Tx, TypeServiceChannelNotif, NO_SERVICE_EVENT);
	return ServiceChannelNotification::NO_SERVICE_EVENT;
}

ServiceChannelNotification ServiceChannel::packetExtractionTM(uint8_t vid, uint8_t* packet_target) {
	VirtualChannel<256>* virtualChannel = &(masterChannel.virtualChannels.at(vid));

	if (virtualChannel->rxInFramesAfterMCReception.full()) {
		ccsdsLogNotice(Rx, TypeServiceChannelNotif, RX_IN_BUFFER_FULL);
		return ServiceChannelNotification::RX_IN_BUFFER_FULL;
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

ServiceChannelNotification ServiceChannel::vcGenerationService(uint16_t transferFrameDataLength, uint8_t gvcid) {
	uint8_t vid = gvcid & 0x3F;
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
		return blockingTm(transferFrameDataLength, packetLength, gvcid);
	}
	return segmentationTm(numberOfTransferFrames, packetLength, transferFrameDataLength, gvcid);
}

ServiceChannelNotification ServiceChannel::storePacketTm(uint8_t* packet, uint16_t packetLength, uint8_t gvcid) {
	uint8_t vid = gvcid & 0x3F;
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

// TODO: MAP Request service shall be rewritten to support allocation in the Memory Pool
// TODO: It shall also be decided based on the virtual channel whether this or vc request will be called based on the VC
ServiceChannelNotification ServiceChannel::mappRequest(uint8_t vid, uint8_t mapid) {
	VirtualChannel<256>* virtualChannel = &(masterChannel.virtualChannels.at(vid));
	MAPChannel<128>* mapChannel = &(virtualChannel->mapChannels.at(mapid));

	if (mapChannel->unprocessedPacketListBufferTC.empty()) {
		ccsdsLogNotice(Tx, TypeServiceChannelNotif, NO_TX_PACKETS_TO_PROCESS);
		return ServiceChannelNotification::NO_TX_PACKETS_TO_PROCESS;
	}

	if (virtualChannel->waitQueueTxTC.full()) {
		ccsdsLogNotice(Tx, TypeServiceChannelNotif, VC_MC_FRAME_BUFFER_FULL);

		return ServiceChannelNotification::VC_MC_FRAME_BUFFER_FULL;
	}

	TransferFrameTC* packet = mapChannel->unprocessedPacketListBufferTC.front();

	const uint16_t maxFrameLength = virtualChannel->maxFrameLengthTC;
	bool segmentationEnabled = virtualChannel->segmentHeaderPresent;
	bool blocking_enabled = virtualChannel->blockingTC;

	const uint16_t maxPacketLength = maxFrameLength - (TcPrimaryHeaderSize + segmentationEnabled * 1U);

	if (packet->getFrameLength() > maxPacketLength) {
		if (segmentationEnabled) {
			// Check if there is enough space in the buffer of the virtual channel to store_out all the segments
			uint8_t tf_n =
			    (packet->getFrameLength() / maxPacketLength) + (packet->getFrameLength() % maxPacketLength != 0);

			if (virtualChannel->waitQueueTxTC.available() >= tf_n) {
				// Break up packet
				mapChannel->unprocessedPacketListBufferTC.pop_front();

				// First portion
				TransferFrameTC t_packet =
				    TransferFrameTC(packet->packetData(), maxPacketLength, packet->virtualChannelId(),
				                    packet->getServiceType(), virtualChannel->segmentHeaderPresent);
				// t_packet.setSegmentationHeader(mapid | 0x40);
				virtualChannel->storeVC(&t_packet);

				// Middle portion
				// t_packet.setSegmentationHeader(mapid | 0x00);
				for (uint8_t i = 1; i < (tf_n - 1); i++) {
					t_packet.setPacketData(&packet->packetData()[i * maxPacketLength]);
					virtualChannel->storeVC(&t_packet);
				}

				// Last portion
				// t_packet.setSegmentationHeader(mapid | 0x80);
				t_packet.setPacketData(&packet->packetData()[(tf_n - 1) * maxPacketLength]);

				t_packet.setPacketLength(packet->getFrameLength() % maxPacketLength);
				virtualChannel->storeVC(&t_packet);
			}
		} else {
			ccsdsLogNotice(Tx, TypeServiceChannelNotif, PACKET_EXCEEDS_MAX_SIZE);
			return ServiceChannelNotification::PACKET_EXCEEDS_MAX_SIZE;
		}
	} else {
		// We've already checked whether there is enough space in the buffer so we can simply remove the packet from
		// the buffer.
		mapChannel->unprocessedPacketListBufferTC.pop_front();

		if (blocking_enabled) {
			// See if we can block it with other packets
			// @todo There are a few things I'm unsure about regarding blocking:
			// - You need to block neighbouring packets because otherwise there's a significant increase in complexity
			// when searching for the most space-efficient combination of packets (though it is still manageable for
			// fairly small buffer size) and you'd also need a linked list instead of a double queue
			// - Neighboring packets also need to share a primary header
			// - You'll need to allocate a new chunk of memory (unless you go with data structures that don't require
			// contiguous memory but I'm also against that)

			// for now just send packet as-is
			virtualChannel->storeVC(packet);
		} else {
			if (segmentationEnabled) {
				packet->setSegmentationHeader((0xc0) | (static_cast<bool>(mapid) && 0x3F));
			}
			virtualChannel->storeVC(packet);
		}
	}
	ccsdsLogNotice(Tx, TypeServiceChannelNotif, NO_SERVICE_EVENT);
	return ServiceChannelNotification::NO_SERVICE_EVENT;
}

#if MaxReceivedUnprocessedTxTcInVirtBuffer > 0

ServiceChannelNotif ServiceChannel::vcpp_request(uint8_t vid) {
	VirtualChannel* virt_channel = &(masterChannel.virtChannels.at(vid));

	if (virt_channel->txUnprocessedPacketList.empty()) {
		return ServiceChannelNotif::NO_TX_PACKETS_TO_PROCESS;
	}

	if (virt_channel->txWaitQueueTC.full()) {
		return ServiceChannelNotif::VC_MC_FRAME_BUFFER_FULL;
		;
	}
	TransferFrameTC* packet = virt_channel->txUnprocessedPacketList.front();

	const uint16_t max_frame_length = virt_channel->maxFrameLength;
	bool segmentation_enabled = virt_channel->segmentHeaderPresent;
	bool blocking_enabled = virt_channel->blocking;

	const uint16_t max_packet_length = max_frame_length - (tc_primary_header_size + segmentation_enabled * 1U);

	if (packet->packet_length() > max_packet_length) {
		if (segmentation_enabled) {
			// Check if there is enough space in the buffer of the virtual channel to store_out all the segments
			uint8_t tf_n =
			    (packet->packet_length() / max_packet_length) + (packet->packet_length() % max_packet_length != 0);

			if (virt_channel->txWaitQueueTC.capacity() >= tf_n) {
				// Break up packet
				virt_channel->txUnprocessedPacketList.pop_front();

				// First portion
				uint16_t seg_header = 0x40;

				TransferFrameTC t_packet = TransferFrameTC(packet->packet_data(), max_packet_length, seg_header,
				                                           packet->global_virtual_channel_id(), packet->map_id(),
				                                           packet->spacecraft_id(), packet->service_type());
				virt_channel->store(&t_packet);

				// Middle portion
				t_packet.set_segmentation_header(0x00);
				for (uint8_t i = 1; i < (tf_n - 1); i++) {
					t_packet.set_packet_data(&packet->packet_data()[i * max_packet_length]);
					virt_channel->store(&t_packet);
				}

				// Last portion
				t_packet.set_segmentation_header(0x80);
				t_packet.set_packet_data(&packet->packet_data()[(tf_n - 1) * max_packet_length]);
				t_packet.set_packet_length(packet->packet_length() % max_packet_length);
				virt_channel->store(&t_packet);
			}
		} else {
			return ServiceChannelNotif::PACKET_EXCEEDS_MAX_SIZE;
		}
	} else {
		// We've already checked whether there is enough space in the buffer so we can simply remove the packet from
		// the buffer.
		virt_channel->txUnprocessedPacketList.pop_front();

		if (blocking_enabled) {
			virt_channel->store(packet);
		} else {
			if (segmentation_enabled) {
				packet->set_segmentation_header(0xc0);
			}
			virt_channel->storeTC(packet);
		}
	}
	return ServiceChannelNotif::NO_SERVICE_EVENT;
}

#endif

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

	if (frame.transferFrameHeader().ctrlAndCmdFlag() == 0) {
		err = virt_channel.fop.transferFdu();
	} else {
		err = virt_channel.fop.validClcwArrival();
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

	CLCW clcw = CLCW(0, 0, 0, 1, vid, 0, 1, virtChannel.farm.lockout, virtChannel.farm.wait,
	                 virtChannel.farm.retransmit, virtChannel.farm.farmBCount, virtChannel.farm.receiverFrameSeqNumber);

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
	VirtualChannel<256>& virtualChannel = masterChannel.virtualChannels.at(frame->virtualChannelId());

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
		packet->append_crc();
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
		packet->append_crc();
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
	memcpy(packet_data + TmTransferFrameSize - trailerSize, packet->packetData() + packet_length - trailerSize + 1,
	       trailerSize);

	masterChannel.txToBeTransmittedFramesAfterMCGenerationListTM.pop_front();
	// Finally, remove master copy
	masterChannel.removeMasterTx(packet);

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
	}
	// TODO: Will we use secondary headers? If so they need to be processed here and forward to the respective service
	masterChannel.rxMasterCopyTM.push_back(frame);
	TransferFrameTM* masterPacket = &(masterChannel.rxMasterCopyTM.back());
	virtualChannel->rxInFramesAfterMCReception.push_back(masterPacket);

	return ServiceChannelNotification::NO_SERVICE_EVENT;
}

ServiceChannelNotification ServiceChannel::transmitFrame(uint8_t* pack) {
	if (masterChannel.txToBeTransmittedFramesAfterAllFramesGenerationListTC.empty()) {
		ccsdsLogNotice(Tx, TypeServiceChannelNotif, TX_TO_BE_TRANSMITTED_FRAMES_LIST_EMPTY);
		return ServiceChannelNotification::TX_TO_BE_TRANSMITTED_FRAMES_LIST_EMPTY;
	}

	TransferFrameTC* packet = masterChannel.txToBeTransmittedFramesAfterAllFramesGenerationListTC.front();
	packet->setRepetitions(packet->repetitions() - 1);
	if (packet->repetitions() == 0) {
		masterChannel.txToBeTransmittedFramesAfterAllFramesGenerationListTC.pop_front();
		ccsdsLogNotice(Tx, TypeServiceChannelNotif, NO_SERVICE_EVENT);
	}
	memcpy(pack, packet, packet->getFrameLength());
	return ServiceChannelNotification::NO_SERVICE_EVENT;
}

ServiceChannelNotification ServiceChannel::transmitAdFrame(uint8_t vid) {
	VirtualChannel<256>* virt_channel = &(masterChannel.virtualChannels.at(vid));
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
	VirtualChannel<256>* virt_channel = &(masterChannel.virtualChannels.at(vid));
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
	VirtualChannel<256>* virt_channel = &(masterChannel.virtualChannels.at(vid));
	virt_channel->fop.acknowledgeFrame(frameSeqNumber);
}

void ServiceChannel::clearAcknowledgedFrames(uint8_t vid) {
	VirtualChannel<256>* virtualChannel = &(masterChannel.virtualChannels.at(vid));
	virtualChannel->fop.removeAcknowledgedFrames();
}

void ServiceChannel::initiateAdNoClcw(uint8_t vid) {
	VirtualChannel<256>* virtualChannel = &(masterChannel.virtualChannels.at(vid));
	FDURequestType req;
	req = virtualChannel->fop.initiateAdNoClcw();
}

void ServiceChannel::initiateAdClcw(uint8_t vid) {
	VirtualChannel<256>* virtualChannel = &(masterChannel.virtualChannels.at(vid));
	FDURequestType req;
	req = virtualChannel->fop.initiateAdClcw();
}

void ServiceChannel::initiateAdUnlock(uint8_t vid) {
	VirtualChannel<256>* virtualChannel = &(masterChannel.virtualChannels.at(vid));
	FDURequestType req;
	req = virtualChannel->fop.initiateAdUnlock();
}

void ServiceChannel::initiateAdVr(uint8_t vid, uint8_t vr) {
	VirtualChannel<256>* virtualChannel = &(masterChannel.virtualChannels.at(vid));
	FDURequestType req;
	req = virtualChannel->fop.initiateAdVr(vr);
}

void ServiceChannel::terminateAdService(uint8_t vid) {
	VirtualChannel<256>* virtualChannel = &(masterChannel.virtualChannels.at(vid));
	FDURequestType req;
	req = virtualChannel->fop.terminateAdService();
}

void ServiceChannel::resumeAdService(uint8_t vid) {
	VirtualChannel<256>* virtualChannel = &(masterChannel.virtualChannels.at(vid));
	FDURequestType req;
	req = virtualChannel->fop.resumeAdService();
}

void ServiceChannel::setVs(uint8_t vid, uint8_t vs) {
	VirtualChannel<256>* virtualChannel = &(masterChannel.virtualChannels.at(vid));
	virtualChannel->fop.setVs(vs);
}

void ServiceChannel::setFopWidth(uint8_t vid, uint8_t width) {
	VirtualChannel<256>* virtualChannel = &(masterChannel.virtualChannels.at(vid));
	virtualChannel->fop.setFopWidth(width);
}

void ServiceChannel::setT1Initial(uint8_t vid, uint16_t t1Init) {
	VirtualChannel<256>* virtualChannel = &(masterChannel.virtualChannels.at(vid));
	virtualChannel->fop.setT1Initial(t1Init);
}

void ServiceChannel::setTransmissionLimit(uint8_t vid, uint8_t vr) {
	VirtualChannel<256>* virtualChannel = &(masterChannel.virtualChannels.at(vid));
	virtualChannel->fop.setTransmissionLimit(vr);
}

void ServiceChannel::setTimeoutType(uint8_t vid, bool vr) {
	VirtualChannel<256>* virtualChannel = &(masterChannel.virtualChannels.at(vid));
	virtualChannel->fop.setTimeoutType(vr);
}
CLCW ServiceChannel::getClcwInBuffer() {
	CLCW clcw = CLCW(clcwTransferFrameBuffer.front().getOperationalControlField().value());
	return clcw;
}

// todo: this may not be needed since it doesn't affect lower procedures and doesn't change the state in any way
void ServiceChannel::invalidDirective(uint8_t vid) {
	VirtualChannel<256>* virtualChannel = &(masterChannel.virtualChannels.at(vid));
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
	const etl::list<TransferFrameTC*, 256>* vc =
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

ServiceChannelNotification ServiceChannel::blockingTm(uint16_t transferFrameDataLength, uint16_t packetLength,
                                                      uint8_t gvcid) {
	uint8_t vid = gvcid & 0x3F;
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
	}
	for (uint8_t i = 0; i < TmTrailerSize; i++) {
		if (currentTransferFrameDataLength + TmPrimaryHeaderSize + i < TmTransferFrameSize) {
			tmpData[currentTransferFrameDataLength + TmPrimaryHeaderSize + i] = 0;
		}
	}
	uint8_t* transferFrameData = masterChannel.masterChannelPool.allocatePacket(
	    tmpData, currentTransferFrameDataLength + TmPrimaryHeaderSize + TmTrailerSize);
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

ServiceChannelNotification ServiceChannel::segmentationTm(uint8_t numberOfTransferFrames, uint16_t packetLength,
                                                          uint16_t transferFrameDataLength, uint8_t gvcid) {
	uint8_t vid = gvcid & 0x3F;
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
		uint8_t* transferFrameData = masterChannel.masterChannelPool.allocatePacket(
		    tmpData, currentTransferFrameDataLength + TmPrimaryHeaderSize + TmTrailerSize);
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