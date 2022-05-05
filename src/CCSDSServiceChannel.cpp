#include <CCSDSServiceChannel.hpp>
#include <TransferFrameTM.hpp>
#include <etl/iterator.h>
#include <CCSDSLoggerImpl.h>


ServiceChannelNotification ServiceChannel::storeTC(uint8_t* packet, uint16_t packetLength) {
	if (masterChannel.rxMasterCopyTC.full()) {
		ccsdsLog(Rx, TypeServiceChannelNotif, RX_IN_MC_FULL);
		return ServiceChannelNotification::RX_IN_MC_FULL;
	}

	if (masterChannel.rxInFramesBeforeAllFramesReceptionListTC.full()) {
		ccsdsLog(Rx, TypeServiceChannelNotif, RX_IN_BUFFER_FULL);
		return ServiceChannelNotification::RX_IN_BUFFER_FULL;
	}

	TransferFrameTC pckt = TransferFrameTC(packet, packetLength);

	if (pckt.getPacketLength() != packetLength) {
		ccsdsLog(Rx, TypeServiceChannelNotif, RX_INVALID_LENGTH);
		return ServiceChannelNotification::RX_INVALID_LENGTH;
	}

	masterChannel.rxMasterCopyTC.push_back(pckt);
	TransferFrameTC* masterPckt = &(masterChannel.rxMasterCopyTC.front());
	masterChannel.rxInFramesBeforeAllFramesReceptionListTC.push_back(masterPckt);
	ccsdsLog(Rx, TypeServiceChannelNotif, NO_SERVICE_EVENT);
	return ServiceChannelNotification::NO_SERVICE_EVENT;
}

ServiceChannelNotification ServiceChannel::storeTC(uint8_t* packet, uint16_t packetLength, uint8_t gvcid, uint8_t mapid,
                                                   uint16_t sduid, ServiceType serviceType) {
	uint8_t vid = gvcid & 0x3F;
	VirtualChannel* vchan = &(masterChannel.virtChannels.at(vid));
	MAPChannel* mapChannel = &(vchan->mapChannels.at(mapid));

	if (mapChannel->unprocessedPacketListBufferTC.full()) {
		ccsdsLog(Tx, TypeServiceChannelNotif, MAP_CHANNEL_FRAME_BUFFER_FULL);
		return ServiceChannelNotification::MAP_CHANNEL_FRAME_BUFFER_FULL;
	}

	TransferFrameTC packet_s =
	    TransferFrameTC(packet, packetLength, 0, gvcid, mapid, sduid, serviceType, vchan->segmentHeaderPresent);

	if (serviceType == ServiceType::TYPE_A) {
		packet_s.setRepetitions(vchan->repetitionTypeAFrame);
	} else if (serviceType == ServiceType::TYPE_B) {
		packet_s.setRepetitions(vchan->repetitionCOPCtrl);
	}

	masterChannel.txMasterCopyTC.push_back(packet_s);
	mapChannel->unprocessedPacketListBufferTC.push_back(&(masterChannel.txMasterCopyTC.back()));
	ccsdsLog(Tx, TypeServiceChannelNotif, NO_SERVICE_EVENT);
	return ServiceChannelNotification::NO_SERVICE_EVENT;
}

// It is expected for space to be reserved for the Primary and Secondary header (if present) beforehand
ServiceChannelNotification ServiceChannel::storeTM(uint8_t* packet, uint16_t packetLength, uint8_t gvcid) {
	uint8_t vid = gvcid & 0x3F;
	VirtualChannel* vchan = &(masterChannel.virtChannels.at(vid));

	if (masterChannel.txMasterCopyTM.full()) {
		ccsdsLog(Tx, TypeServiceChannelNotif, MASTER_CHANNEL_FRAME_BUFFER_FULL);
		return ServiceChannelNotification::MASTER_CHANNEL_FRAME_BUFFER_FULL;
	}

	if (masterChannel.txProcessedPacketListBufferTM.full()) {
		ccsdsLog(Tx, TypeServiceChannelNotif, TX_MC_FRAME_BUFFER_FULL);
		return ServiceChannelNotification::TX_MC_FRAME_BUFFER_FULL;
	}

	auto hdr = TransferFrameHeaderTM(packet);

	uint8_t* secondaryHeader = 0;

	if (masterChannel.secondaryHeaderTMPresent) {
		secondaryHeader = &packet[7];
	}

	TransferFrameTM packet_s = TransferFrameTM(packet, packetLength, vchan->frameCountTM, vid, secondaryHeader,
	                             hdr.transferFrameDataFieldStatus(), vchan->operationalControlFieldTMPresent,
	                             masterChannel.secondaryHeaderTMPresent, vchan->synchronization);

	// Increment VC frame count. The MC counter is incremented in the Master Channel
	vchan->frameCountTM = vchan->frameCountTM < 255 ? vchan->frameCountTM + 1 : 0;

	masterChannel.txMasterCopyTM.push_back(packet_s);
	masterChannel.txProcessedPacketListBufferTM.push_back(&(masterChannel.txMasterCopyTM.back()));

	ccsdsLog(Tx, TypeServiceChannelNotif, NO_SERVICE_EVENT);
	return ServiceChannelNotification::NO_SERVICE_EVENT;
}

ServiceChannelNotification ServiceChannel::storeTM(uint8_t* packet, uint16_t packetLength) {
	if (masterChannel.rxMasterCopyTM.full()) {
		ccsdsLog(Rx, TypeServiceChannelNotif, RX_IN_MC_FULL);
		return ServiceChannelNotification::RX_IN_MC_FULL;
	}

	if (masterChannel.rxInFramesBeforeAllFramesReceptionListTM.full()) {
		ccsdsLog(Rx, TypeServiceChannelNotif, RX_IN_BUFFER_FULL);
		return ServiceChannelNotification::RX_IN_BUFFER_FULL;
	}

	TransferFrameTM pckt = TransferFrameTM(packet, packetLength);

	if (pckt.getPacketLength() != packetLength) {
		ccsdsLog(Rx, TypeServiceChannelNotif, RX_INVALID_LENGTH);
		return ServiceChannelNotification::RX_INVALID_LENGTH;
	}

	masterChannel.rxMasterCopyTM.push_back(pckt);
	TransferFrameTM* masterPckt = &(masterChannel.rxMasterCopyTM.back());
	masterChannel.rxInFramesBeforeAllFramesReceptionListTM.push_back(masterPckt);
	ccsdsLog(Rx, TypeServiceChannelNotif, NO_SERVICE_EVENT);
	return ServiceChannelNotification::NO_SERVICE_EVENT;
}

ServiceChannelNotification ServiceChannel::mappRequest(uint8_t vid, uint8_t mapid) {
	VirtualChannel* virtChannel = &(masterChannel.virtChannels.at(vid));
	MAPChannel* mapChannel = &(virtChannel->mapChannels.at(mapid));

	if (mapChannel->unprocessedPacketListBufferTC.empty()) {
		ccsdsLog(Tx, TypeServiceChannelNotif, NO_TX_PACKETS_TO_PROCESS);
		return ServiceChannelNotification::NO_TX_PACKETS_TO_PROCESS;
	}

	if (virtChannel->txWaitQueueTC.full()) {
		ccsdsLog(Tx, TypeServiceChannelNotif, VC_MC_FRAME_BUFFER_FULL);
		return ServiceChannelNotification::VC_MC_FRAME_BUFFER_FULL;
	}

	TransferFrameTC* packet = mapChannel->unprocessedPacketListBufferTC.front();

	const uint16_t maxFrameLength = virtChannel->maxFrameLength;
	bool segmentationEnabled = virtChannel->segmentHeaderPresent;
	bool blocking_enabled = virtChannel->blocking;

	const uint16_t maxPacketLength = maxFrameLength - (TcPrimaryHeaderSize + segmentationEnabled * 1U);

	if (packet->getPacketLength() > maxPacketLength) {
		if (segmentationEnabled) {
			// Check if there is enough space in the buffer of the virtual channel to store_out all the segments
			uint8_t tf_n =
			    (packet->getPacketLength() / maxPacketLength) + (packet->getPacketLength() % maxPacketLength != 0);

			if (virtChannel->txWaitQueueTC.available() >= tf_n) {
				// Break up packet
				mapChannel->unprocessedPacketListBufferTC.pop_front();

				// First portion
				uint16_t seg_header = mapid | 0x40;

				TransferFrameTC t_packet =
				    TransferFrameTC(packet->packetData(), maxPacketLength, seg_header,
				                             packet->globalVirtualChannelId(), packet->mapId(), packet->spacecraftId(),
				                             packet->getServiceType(), virtChannel->segmentHeaderPresent);
				virtChannel->storeVC(&t_packet);

				// Middle portion
				t_packet.setSegmentationHeader(mapid | 0x00);
				for (uint8_t i = 1; i < (tf_n - 1); i++) {
					t_packet.setPacketData(&packet->packetData()[i * maxPacketLength]);
					virtChannel->storeVC(&t_packet);
				}

				// Last portion
				t_packet.setSegmentationHeader(mapid | 0x80);
				t_packet.setPacketData(&packet->packetData()[(tf_n - 1) * maxPacketLength]);
				t_packet.setPacketLength(packet->getPacketLength() % maxPacketLength);
				virtChannel->storeVC(&t_packet);
			}
		} else {
			ccsdsLog(Tx, TypeServiceChannelNotif, PACKET_EXCEEDS_MAX_SIZE);
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
			virtChannel->storeVC(packet);
		} else {
			if (segmentationEnabled) {
				packet->setSegmentationHeader((0xc0) | (static_cast<bool>(mapid) && 0x3F));
			}
			virtChannel->storeVC(packet);
		}
	}
	ccsdsLog(Tx, TypeServiceChannelNotif, NO_SERVICE_EVENT);
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

				TransferFrameTC t_packet =
				    TransferFrameTC(packet->packet_data(), max_packet_length, seg_header, packet->global_virtual_channel_id(),
				             packet->map_id(), packet->spacecraft_id(), packet->service_type());
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
		ccsdsLog(Rx, TypeServiceChannelNotif, NO_RX_PACKETS_TO_PROCESS);
		return ServiceChannelNotification::NO_RX_PACKETS_TO_PROCESS;
	}
	if (masterChannel.txToBeTransmittedFramesAfterMCGenerationListTM.full()) {
		ccsdsLog(Tx, TypeServiceChannelNotif, TX_MC_FRAME_BUFFER_FULL);
		return ServiceChannelNotification::TX_MC_FRAME_BUFFER_FULL;
	}
	TransferFrameTM* packet = masterChannel.txProcessedPacketListBufferTM.front();

	// Check if need to add secondary header and act accordingly
	// TODO: Process secondary headers

	// set master channel frame counter
    packet->setMasterChannelFrameCount(masterChannel.frameCountTM);

	// increment master channel frame counter
	masterChannel.frameCountTM = masterChannel.frameCountTM <= 254 ? masterChannel.frameCountTM : 0;
	masterChannel.txToBeTransmittedFramesAfterMCGenerationListTM.push_back(packet);
	masterChannel.txProcessedPacketListBufferTM.pop_front();

	ccsdsLog(Rx, TypeServiceChannelNotif, NO_SERVICE_EVENT);
	return ServiceChannelNotification::NO_SERVICE_EVENT;
}

ServiceChannelNotification ServiceChannel::mcReceptionTMRequest() {
	if (masterChannel.rxInFramesBeforeAllFramesReceptionListTM.empty()) {
		ccsdsLog(Rx, TypeServiceChannelNotif, NO_RX_PACKETS_TO_PROCESS);
		return ServiceChannelNotification::NO_RX_PACKETS_TO_PROCESS;
	}

	if (masterChannel.txToBeTransmittedFramesAfterMCReceptionListTM.full()) {
		ccsdsLog(Rx, TypeServiceChannelNotif, RX_IN_MC_FULL);
		return ServiceChannelNotification::RX_IN_MC_FULL;
	}
	TransferFrameTM* packet = masterChannel.rxInFramesBeforeAllFramesReceptionListTM.front();

	// Check if need to add secondary header and act accordingly
	// TODO: Process secondary headers

	masterChannel.txToBeTransmittedFramesAfterMCReceptionListTM.push_back(packet);
	masterChannel.rxInFramesBeforeAllFramesReceptionListTM.pop_front();

	ccsdsLog(Rx, TypeServiceChannelNotif, NO_SERVICE_EVENT);
	return ServiceChannelNotification::NO_SERVICE_EVENT;
}

ServiceChannelNotification ServiceChannel::vcGenerationRequestTC(uint8_t vid) {
	VirtualChannel* virt_channel = &(masterChannel.virtChannels.at(vid));
	if (virt_channel->txUnprocessedPacketListBufferTC.empty()) {
		ccsdsLog(Tx, TypeServiceChannelNotif, NO_TX_PACKETS_TO_PROCESS);
		return ServiceChannelNotification::NO_TX_PACKETS_TO_PROCESS;
	}

	if (masterChannel.txOutFramesBeforeAllFramesGenerationListTC.full()) {
		ccsdsLog(Tx, TypeServiceChannelNotif, TX_MC_FRAME_BUFFER_FULL);
		return ServiceChannelNotification::TX_MC_FRAME_BUFFER_FULL;
	}

	TransferFrameTC* frame = virt_channel->txUnprocessedPacketListBufferTC.front();
	COPDirectiveResponse err = COPDirectiveResponse::ACCEPT;

	if (frame->transferFrameHeader().ctrlAndCmdFlag() == 0) {
		err = virt_channel->fop.transferFdu();
	} else {
		err = virt_channel->fop.validClcwArrival();
	}

	if (err == COPDirectiveResponse::REJECT) {
		ccsdsLog(Tx, TypeServiceChannelNotif, FOP_REQUEST_REJECTED);
		return ServiceChannelNotification::FOP_REQUEST_REJECTED;
	}

	virt_channel->txUnprocessedPacketListBufferTC.pop_front();
	ccsdsLog(Tx, TypeServiceChannelNotif, NO_SERVICE_EVENT);
	return ServiceChannelNotification::NO_SERVICE_EVENT;
}

ServiceChannelNotification ServiceChannel::allFramesReceptionTCRequest() {
	if (masterChannel.rxInFramesBeforeAllFramesReceptionListTC.empty()) {
		ccsdsLog(Rx, TypeServiceChannelNotif, NO_RX_PACKETS_TO_PROCESS);
		return ServiceChannelNotification::NO_RX_PACKETS_TO_PROCESS;
	}

	if (masterChannel.rxToBeTransmittedFramesAfterAllFramesReceptionListTC.full()) {
		ccsdsLog(Rx, TypeServiceChannelNotif, RX_OUT_BUFFER_FULL);
		return ServiceChannelNotification::RX_OUT_BUFFER_FULL;
	}

	TransferFrameTC* packet = masterChannel.rxInFramesBeforeAllFramesReceptionListTC.front();
	VirtualChannel* virt_channel = &(masterChannel.virtChannels.at(packet->virtualChannelId()));

	if (virt_channel->rxWaitQueueTC.full()) {
		ccsdsLog(Rx, TypeServiceChannelNotif, VC_RX_WAIT_QUEUE_FULL);
		return ServiceChannelNotification::VC_RX_WAIT_QUEUE_FULL;
	}

	// Frame Delimiting and Fill Removal supposedly aren't implemented here

	/*
	 * Frame validation checks
	 */

	// Check for valid TFVN
	if (packet->getTransferFrameVersionNumber() != 0) {
		ccsdsLog(Rx, TypeServiceChannelNotif, RX_INVALID_TFVN);
		return ServiceChannelNotification::RX_INVALID_TFVN;
	}

	// Check for valid SCID
	if (packet->spacecraftId() == SpacecraftIdentifier) {
		ccsdsLog(Rx, TypeServiceChannelNotif, RX_INVALID_SCID);
		return ServiceChannelNotification::RX_INVALID_SCID;
	}

	// TransferFrameTC length is checked upon storing the packet in the MC

	// If present in channel, check if CRC is valid
#if tc_error_control_field_exists
	uint16_t len = packet->packet_length() - 2;
	uint16_t crc = calculate_crc(packet->packet_data(), packet->packet_length() - 2);

	uint16_t packet_crc =
	    ((static_cast<uint16_t>(packet->packet_data()[len]) << 8) & 0xFF00) | packet->packet_data()[len + 1];
	if (crc != packet_crc) {
		return ServiceChannelNotif::RX_INVALID_CRC;
	}
#endif
	virt_channel->rxWaitQueueTC.push_back(packet);
	masterChannel.rxInFramesBeforeAllFramesReceptionListTC.pop_front();
	ccsdsLog(Rx, TypeServiceChannelNotif, NO_SERVICE_EVENT);
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
		ccsdsLog(Tx, TypeServiceChannelNotif, NO_TX_PACKETS_TO_PROCESS);
		return ServiceChannelNotification::NO_TX_PACKETS_TO_PROCESS;
	}

	if (masterChannel.txToBeTransmittedFramesAfterAllFramesGenerationListTC.full()) {
		ccsdsLog(Tx, TypeServiceChannelNotif, TX_TO_BE_TRANSMITTED_FRAMES_LIST_FULL);
		return ServiceChannelNotification::TX_TO_BE_TRANSMITTED_FRAMES_LIST_FULL;
	}

	TransferFrameTC* packet = masterChannel.txOutFramesBeforeAllFramesGenerationListTC.front();
	masterChannel.txOutFramesBeforeAllFramesGenerationListTC.pop_front();

	if (masterChannel.errorCtrlField) {
		packet->append_crc();
	}

	masterChannel.storeTransmittedOut(packet);
	ccsdsLog(Tx, TypeServiceChannelNotif, NO_SERVICE_EVENT);
	return ServiceChannelNotification::NO_SERVICE_EVENT;
}

ServiceChannelNotification ServiceChannel::allFramesGenerationTMRequest(uint8_t* packet_data, uint16_t packet_length) {
	if (masterChannel.txToBeTransmittedFramesAfterMCGenerationListTM.empty()) {
		return ServiceChannelNotification::NO_TX_PACKETS_TO_PROCESS;
	}

	TransferFrameTM* packet = masterChannel.txToBeTransmittedFramesAfterMCGenerationListTM.front();

	if (masterChannel.errorCtrlField) {
		packet->append_crc();
	}

	if (packet->getPacketLength() > packet_length){
		return ServiceChannelNotification::RX_INVALID_LENGTH;
	}

	uint8_t vid = packet->virtualChannelId();
    VirtualChannel* vchan = &(masterChannel.virtChannels.at(vid));

	uint16_t frameSize = packet->getPacketLength();
	uint16_t idleDataSize = TmTransferFrameSize - frameSize;
    uint8_t trailerSize = 4*packet->operationalControlFieldExists() + 2*vchan->frameErrorControlFieldTMPresent;

    masterChannel.txToBeTransmittedFramesAfterMCGenerationListTM.pop_front();

	// Copy frame without the trailer
	memcpy(packet_data, packet->packetData(), frameSize - trailerSize);

	// Append idle data
	memcpy(packet_data + frameSize - trailerSize, idle_data, idleDataSize);

	// Append trailer
    memcpy(packet_data + TmTransferFrameSize - trailerSize, packet->packetData() + packet_length - trailerSize + 1, trailerSize);

    // Finally, remove master copy
	masterChannel.removeMasterTx(packet);

	return ServiceChannelNotification::NO_SERVICE_EVENT;
}

ServiceChannelNotification ServiceChannel::allFramesReceptionTMRequest() {
	if (masterChannel.rxInFramesBeforeAllFramesReceptionListTM.empty()) {
		ccsdsLog(Rx, TypeServiceChannelNotif, NO_RX_PACKETS_TO_PROCESS);
		return ServiceChannelNotification::NO_RX_PACKETS_TO_PROCESS;
	}

	if (masterChannel.rxToBeTransmittedFramesAfterAllFramesReceptionListTM.full()) {
		ccsdsLog(Rx, TypeServiceChannelNotif, RX_OUT_BUFFER_FULL);
		return ServiceChannelNotification::RX_OUT_BUFFER_FULL;
	}

	TransferFrameTM* packet = masterChannel.rxInFramesBeforeAllFramesReceptionListTM.front();

#if tm_error_control_field_exists
	uint16_t len = packet->getPacketLength() - 2;
	uint16_t crc = packet->calculateCRC(packet->packetData(), len);

	uint16_t packet_crc =
	    ((static_cast<uint16_t>(packet->packetData()[len]) << 8) & 0xFF00) | packet->packetData()[len + 1];
	if (crc != packet_crc) {
		ccsdsLog(Rx, TypeServiceChannelNotif, RX_INVALID_CRC);
		// Invalid packet is discarded
		masterChannel.rxInFramesBeforeAllFramesReceptionListTM.pop_front();
		masterChannel.removeMasterRx(packet);

		return ServiceChannelNotification::RX_INVALID_CRC;
	}
#endif

	masterChannel.rxToBeTransmittedFramesAfterAllFramesReceptionListTM.push_back(packet);
	masterChannel.rxInFramesBeforeAllFramesReceptionListTM.pop_front();
	ccsdsLog(Rx, TypeServiceChannelNotif, NO_SERVICE_EVENT);
	return ServiceChannelNotification::NO_SERVICE_EVENT;
}

ServiceChannelNotification ServiceChannel::transmitFrame(uint8_t* pack) {
	if (masterChannel.txToBeTransmittedFramesAfterAllFramesGenerationListTC.empty()) {
		ccsdsLog(Tx, TypeServiceChannelNotif, TX_TO_BE_TRANSMITTED_FRAMES_LIST_EMPTY);
		return ServiceChannelNotification::TX_TO_BE_TRANSMITTED_FRAMES_LIST_EMPTY;
	}

	TransferFrameTC* packet = masterChannel.txToBeTransmittedFramesAfterAllFramesGenerationListTC.front();
	packet->setRepetitions(packet->repetitions() - 1);
	if (packet->repetitions() == 0) {
		masterChannel.txToBeTransmittedFramesAfterAllFramesGenerationListTC.pop_front();
		ccsdsLog(Tx, TypeServiceChannelNotif, NO_SERVICE_EVENT);
	}
	memcpy(pack, packet, packet->getPacketLength());
	return ServiceChannelNotification::NO_SERVICE_EVENT;
}

ServiceChannelNotification ServiceChannel::transmitAdFrame(uint8_t vid) {
	VirtualChannel* virt_channel = &(masterChannel.virtChannels.at(vid));
	FOPNotification req;
	req = virt_channel->fop.transmitAdFrame();
	if (req == FOPNotification::NO_FOP_EVENT) {
		ccsdsLog(Tx, TypeServiceChannelNotif, NO_SERVICE_EVENT);
		return ServiceChannelNotification::NO_SERVICE_EVENT;

	} else {
		// TODO
	}
	return ServiceChannelNotification::NO_SERVICE_EVENT;
}

// TODO: Probably not needed. Refactor sentQueueTC
ServiceChannelNotification ServiceChannel::pushSentQueue(uint8_t vid) {
	VirtualChannel* virt_channel = &(masterChannel.virtChannels.at(vid));
	COPDirectiveResponse req;
	req = virt_channel->fop.pushSentQueue();

	if (req == COPDirectiveResponse::ACCEPT) {
		ccsdsLog(Tx, TypeServiceChannelNotif, NO_SERVICE_EVENT);
		return ServiceChannelNotification::NO_SERVICE_EVENT;
	}
	ccsdsLog(Tx, TypeServiceChannelNotif, TX_FOP_REJECTED);
	return ServiceChannelNotification::TX_FOP_REJECTED;
}

void ServiceChannel::acknowledgeFrame(uint8_t vid, uint8_t frameSeqNumber) {
	VirtualChannel* virt_channel = &(masterChannel.virtChannels.at(vid));
	virt_channel->fop.acknowledgeFrame(frameSeqNumber);
}

void ServiceChannel::clearAcknowledgedFrames(uint8_t vid) {
	VirtualChannel* virtChannel = &(masterChannel.virtChannels.at(vid));
	virtChannel->fop.removeAcknowledgedFrames();
}

void ServiceChannel::initiateAdNoClcw(uint8_t vid) {
	VirtualChannel* virtChannel = &(masterChannel.virtChannels.at(vid));
	FDURequestType req;
	req = virtChannel->fop.initiateAdNoClcw();
}

void ServiceChannel::initiateAdClcw(uint8_t vid) {
	VirtualChannel* virtChannel = &(masterChannel.virtChannels.at(vid));
	FDURequestType req;
	req = virtChannel->fop.initiateAdClcw();
}

void ServiceChannel::initiateAdUnlock(uint8_t vid) {
	VirtualChannel* virtChannel = &(masterChannel.virtChannels.at(vid));
	FDURequestType req;
	req = virtChannel->fop.initiateAdUnlock();
}

void ServiceChannel::initiateAdVr(uint8_t vid, uint8_t vr) {
	VirtualChannel* virtChannel = &(masterChannel.virtChannels.at(vid));
	FDURequestType req;
	req = virtChannel->fop.initiateAdVr(vr);
}

void ServiceChannel::terminateAdService(uint8_t vid) {
	VirtualChannel* virtChannel = &(masterChannel.virtChannels.at(vid));
	FDURequestType req;
	req = virtChannel->fop.terminateAdService();
}

void ServiceChannel::resumeAdService(uint8_t vid) {
	VirtualChannel* virtChannel = &(masterChannel.virtChannels.at(vid));
	FDURequestType req;
	req = virtChannel->fop.resumeAdService();
}

void ServiceChannel::setVs(uint8_t vid, uint8_t vs) {
	VirtualChannel* virtChannel = &(masterChannel.virtChannels.at(vid));
	virtChannel->fop.setVs(vs);
}

void ServiceChannel::setFopWidth(uint8_t vid, uint8_t width) {
	VirtualChannel* virtChannel = &(masterChannel.virtChannels.at(vid));
	virtChannel->fop.setFopWidth(width);
}

void ServiceChannel::setT1Initial(uint8_t vid, uint16_t t1Init) {
	VirtualChannel* virtChannel = &(masterChannel.virtChannels.at(vid));
	virtChannel->fop.setT1Initial(t1Init);
}

void ServiceChannel::setTransmissionLimit(uint8_t vid, uint8_t vr) {
	VirtualChannel* virtChannel = &(masterChannel.virtChannels.at(vid));
	virtChannel->fop.setTransmissionLimit(vr);
}

void ServiceChannel::setTimeoutType(uint8_t vid, bool vr) {
	VirtualChannel* virtChannel = &(masterChannel.virtChannels.at(vid));
	virtChannel->fop.setTimeoutType(vr);
}

// todo: this may not be needed since it doesn't affect lower procedures and doesn't change the state in any way
void ServiceChannel::invalidDirective(uint8_t vid) {
	VirtualChannel* virtChannel = &(masterChannel.virtChannels.at(vid));
	virtChannel->fop.invalidDirective();
}

FOPState ServiceChannel::fopState(uint8_t vid) const {
	return masterChannel.virtChannels.at(vid).fop.state;
}

uint16_t ServiceChannel::t1Timer(uint8_t vid) const {
	return masterChannel.virtChannels.at(vid).fop.tiInitial;
}

uint8_t ServiceChannel::fopSlidingWindowWidth(uint8_t vid) const {
	return masterChannel.virtChannels.at(vid).fop.fopSlidingWindow;
}

bool ServiceChannel::timeoutType(uint8_t vid) const {
	return masterChannel.virtChannels.at(vid).fop.timeoutType;
}

uint8_t ServiceChannel::transmitterFrameSeqNumber(uint8_t vid) const {
	return masterChannel.virtChannels.at(vid).fop.transmitterFrameSeqNumber;
}

uint8_t ServiceChannel::expectedFrameSeqNumber(uint8_t vid) const {
	return masterChannel.virtChannels.at(vid).fop.expectedAcknowledgementSeqNumber;
}


uint8_t ServiceChannel::getFrameCountTM(uint8_t vid){
    return masterChannel.virtChannels.at(vid).frameCountTM;
}

uint8_t ServiceChannel::getFrameCountTM(){
    return masterChannel.frameCountTM;
}

std::pair<ServiceChannelNotification, const TransferFrameTC*> ServiceChannel::txOutPacketTC(uint8_t vid, uint8_t mapid) const {
	const etl::list<TransferFrameTC*, MaxReceivedTcInMapChannel>* mc =
	    &(masterChannel.virtChannels.at(vid).mapChannels.at(mapid).unprocessedPacketListBufferTC);
	if (mc->empty()) {
		ccsdsLog(Tx, TypeServiceChannelNotif, NO_TX_PACKETS_TO_PROCESS);
		return std::pair(ServiceChannelNotification::NO_TX_PACKETS_TO_PROCESS, nullptr);
	}

	return std::pair(ServiceChannelNotification::NO_SERVICE_EVENT, mc->front());
}

std::pair<ServiceChannelNotification, const TransferFrameTC*> ServiceChannel::txOutPacketTC(uint8_t vid) const {
	const etl::list<TransferFrameTC*, MaxReceivedUnprocessedTxTcInVirtBuffer>* vc =
	    &(masterChannel.virtChannels.at(vid).txUnprocessedPacketListBufferTC);
	if (vc->empty()) {
		ccsdsLog(Tx, TypeServiceChannelNotif, NO_TX_PACKETS_TO_PROCESS);
		return std::pair(ServiceChannelNotification::NO_TX_PACKETS_TO_PROCESS, nullptr);
	}
	ccsdsLog(Tx, TypeServiceChannelNotif, NO_SERVICE_EVENT);
	return std::pair(ServiceChannelNotification::NO_SERVICE_EVENT, vc->front());
}

std::pair<ServiceChannelNotification, const TransferFrameTC*> ServiceChannel::txOutPacketTC() const {
	if (masterChannel.txMasterCopyTC.empty()) {
		ccsdsLog(Tx, TypeServiceChannelNotif, NO_TX_PACKETS_TO_PROCESS);
		return std::pair(ServiceChannelNotification::NO_TX_PACKETS_TO_PROCESS, nullptr);
	}
	return std::pair(ServiceChannelNotification::NO_SERVICE_EVENT, &(masterChannel.txMasterCopyTC.back()));
}

std::pair<ServiceChannelNotification, const TransferFrameTM*> ServiceChannel::txOutPacketTM() const {
	if (masterChannel.txMasterCopyTM.empty()) {
		ccsdsLog(Tx, TypeServiceChannelNotif, NO_TX_PACKETS_TO_PROCESS);
		return std::pair(ServiceChannelNotification::NO_TX_PACKETS_TO_PROCESS, nullptr);
	}
	ccsdsLog(Tx, TypeServiceChannelNotif, NO_SERVICE_EVENT);
	return std::pair(ServiceChannelNotification::NO_SERVICE_EVENT, &(masterChannel.txMasterCopyTM.back()));
}

std::pair<ServiceChannelNotification, const TransferFrameTC*> ServiceChannel::txOutProcessedPacketTC() const {
	if (masterChannel.txToBeTransmittedFramesAfterAllFramesGenerationListTC.empty()) {
		ccsdsLog(Tx, TypeServiceChannelNotif, NO_TX_PACKETS_TO_PROCESS);
		return std::pair(ServiceChannelNotification::NO_TX_PACKETS_TO_PROCESS, nullptr);
	}
	ccsdsLog(Tx, TypeServiceChannelNotif, NO_SERVICE_EVENT);
	return std::pair(ServiceChannelNotification::NO_SERVICE_EVENT,
	                 masterChannel.txToBeTransmittedFramesAfterAllFramesGenerationListTC.front());
}

std::pair<ServiceChannelNotification, const TransferFrameTM*> ServiceChannel::txOutProcessedPacketTM() const {
	if (masterChannel.txToBeTransmittedFramesAfterAllFramesGenerationListTM.empty()) {
		ccsdsLog(Tx, TypeServiceChannelNotif, NO_TX_PACKETS_TO_PROCESS);
		return std::pair(ServiceChannelNotification::NO_TX_PACKETS_TO_PROCESS, nullptr);
	}
	ccsdsLog(Tx, TypeServiceChannelNotif, NO_SERVICE_EVENT);
	return std::pair(ServiceChannelNotification::NO_SERVICE_EVENT,
	                 masterChannel.txToBeTransmittedFramesAfterAllFramesGenerationListTM.front());
}