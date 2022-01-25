#include <CCSDSServiceChannel.hpp>
#include <TransferFrameTC.hpp>
#include <TransferFrameTM.hpp>
#include <etl/iterator.h>
#include <Alert.hpp>
#include <CCSDSLoggerImpl.h>

ServiceChannelNotification ServiceChannel::store(uint8_t *packet, uint16_t packetLength) {
    if (masterChannel.rxMasterCopyTC.full()) {
		ccsdsLog(Rx, TypeServiceChannelNotif, RX_IN_MC_FULL);
        return ServiceChannelNotification::RX_IN_MC_FULL;
    }

    if (masterChannel.rxInFramesBeforeAllFramesReceptionList.full()) {
		ccsdsLog(Rx, TypeServiceChannelNotif, RX_IN_BUFFER_FULL);
        return ServiceChannelNotification::RX_IN_BUFFER_FULL;
    }

	TransferFrameTC transfer_frame = TransferFrameTC(packet, packet_length);

    if (transfer_frame.transfer_frame_length() != packet_length) {
		ccsdsLog(Rx, TypeServiceChannelNotif, RX_INVALID_LENGTH);
        return ServiceChannelNotification::RX_INVALID_LENGTH;
    }

    masterChannel.rxMasterCopyTC.push_back(transfer_frame);
	TransferFrameTC * master_transfer_frame = &(masterChannel.rxMasterCopyTC.front());
    masterChannel.rxInFramesBeforeAllFramesReceptionList.push_back(master_transfer_frame);
	ccsdsLog(Rx, TypeServiceChannelNotif, NO_SERVICE_EVENT);
    return ServiceChannelNotification::NO_SERVICE_EVENT;
}

ServiceChannelNotification ServiceChannel::storeTC(uint8_t *transfer_frame, uint16_t transfer_frame_length, uint8_t gvcid, uint8_t mapid,
                                          uint16_t sduid, ServiceType serviceType) {
    uint8_t vid = gvcid & 0x3F;
    VirtualChannel *vchan = &(masterChannel.virtChannels.at(vid));
    MAPChannel * mapChannel = &(vchan->mapChannels.at(mapid));

    if (mapChannel->unprocessedTransferFrameListBufferTC.full()) {
		ccsdsLog(Tx, TypeServiceChannelNotif, MAP_CHANNEL_FRAME_BUFFER_FULL);
        return ServiceChannelNotification::MAP_CHANNEL_FRAME_BUFFER_FULL;
    }

	TransferFrameTC transfer_frame_s =
	    TransferFrameTC(transfer_frame, transfer_frame_length, 0, gvcid, mapid, sduid, serviceΤype, vchan->segmentHeaderPresent);

    if (serviceType == ServiceType::TYPE_A) {
		transfer_frame_s.setRepetitions(vchan->repetitionTypeAFrame);
    } else if (serviceType == ServiceType::TYPE_B) {
		transfer_frame_s.setRepetitions(vchan->repetitionCOPCtrl);
    }

    masterChannel.txMasterCopyTC.push_back(transfer_frame_s);
	mapChannel->unprocessedTransferFrameListBufferTC.push_back(&(masterChannel.txMasterCopyTC.back()));
	ccsdsLog(Tx, TypeServiceChannelNotif, NO_SERVICE_EVENT);
    return ServiceChannelNotification::NO_SERVICE_EVENT;
}

ServiceChannelNotification ServiceChannel::storeTM(uint8_t *transfer_frame, uint16_t transfer_frame_length, uint8_t gvcid, uint16_t scid) {
    uint8_t vid = gvcid & 0x3F;
    VirtualChannel *vchan = &(masterChannel.virtChannels.at(vid));

    if (masterChannel.txMasterCopyTM.full()) {
		ccsdsLog(Tx, TypeServiceChannelNotif, MASTER_CHANNEL_FRAME_BUFFER_FULL);
        return ServiceChannelNotification::MASTER_CHANNEL_FRAME_BUFFER_FULL;
    }

    if (masterChannel.txMasterCopyTM.full()) {
		ccsdsLog(Tx, TypeServiceChannelNotif, MASTER_CHANNEL_FRAME_BUFFER_FULL);
        return ServiceChannelNotification::MASTER_CHANNEL_FRAME_BUFFER_FULL;
    }

    auto hdr = TransferFrameHeaderTM(transfer_frame);

    uint8_t *secondaryHeader = 0;
    if (hdr.transferFrameSecondaryHeaderFlag() == 1) {
        secondaryHeader = &transfer_frame[7];
    }

    PacketTM transfer_frame_s =
	    TransferFrameTM(transfer_frame, transfer_frame_length, vchan->frameCount, scid, vid, masterChannel.frameCount,
                     secondaryHeader, hdr.transferFrameDataFieldStatus(), TM);


    masterChannel.txMasterCopyTM.push_back(transfer_frame_s);
	ccsdsLog(Tx, TypeServiceChannelNotif, NO_SERVICE_EVENT);
    return ServiceChannelNotification::NO_SERVICE_EVENT;
}

ServiceChannelNotification ServiceChannel::mappRequest(uint8_t vid, uint8_t mapid) {
    VirtualChannel * virtChannel = &(masterChannel.virtChannels.at(vid));
    MAPChannel * mapChannel = &(virtChannel->mapChannels.at(mapid));

    if (mapChannel->unprocessedTransferFrameListBufferTC.empty()) {
		ccsdsLog(Tx, TypeServiceChannelNotif, NO_TX_PACKETS_TO_PROCESS);
        return ServiceChannelNotification::NO_TX_PACKETS_TO_PROCESS;
    }

    if (virtChannel->txWaitQueue.full()) {
		ccsdsLog(Tx, TypeServiceChannelNotif, VC_MC_FRAME_BUFFER_FULL);
        return ServiceChannelNotification::VC_MC_FRAME_BUFFER_FULL;
    }

	TransferFrameTC* transfer_frame = mapChannel->unprocessedTransferFrameListBufferTC.front();

    const uint16_t maxFrameLength = virtChannel->maxFrameLength;
    bool segmentationEnabled = virtChannel->segmentHeaderPresent;
    bool blocking_enabled = virtChannel->blocking;

    const uint16_t maxPacketLength = maxFrameLength - (TC_PRIMARY_HEADER_SIZE + segmentationEnabled * 1U);

    if (transfer_frame->transfer_frame_length() > maxPacketLength) {
        if (segmentationEnabled) {
            // Check if there is enough space in the buffer of the virtual channel to store_out all the segments
            uint8_t tf_n =
                    (transfer_frame->transfer_frame_length() / maxPacketLength) + (transfer_frame->transfer_frame_length() % maxPacketLength != 0);

            if (virtChannel->txWaitQueue.available() >= tf_n) {
                // Break up packet
				mapChannel->unprocessedTransferFrameListBufferTC.pop_front();

                // First portion
                uint16_t seg_header = mapid | 0x40;

				TransferFrameTC t_transfer_frame =
				    TransferFrameTC(transfer_frame->transfer_frame_data(), maxPacketLength, seg_header, transfer_frame->globalVirtualChannelId(), transfer_frame->mapId(), transfer_frame->spacecraftId(),
				                    transfer_frame->getServiceType(), virtChannel->segmentHeaderPresent);
				virtChannel->storeVC(&transfer_frame);

                // Middle portion
				t_transfer_frame.setSegmentationHeader(mapid | 0x00);
                for (uint8_t i = 1; i < (tf_n - 1); i++) {
					t_transfer_frame.setPacketData(&transfer_frame->transfer_frame_data()[i * maxPacketLength]);
					virtChannel->storeVC(&t_transfer_frame);
                }

                // Last portion
				t_transfer_frame.setSegmentationHeader(mapid | 0x80);
				t_transfer_frame.setPacketData(&transfer_frame->transfer_frame_data()[(tf_n - 1) * maxPacketLength]);
				t_transfer_frame.setPacketLength(transfer_frame->transfer_frame_length() % maxPacketLength);
				virtChannel->storeVC(&t_transfer_frame);
            }
        } else {
			ccsdsLog(Tx, TypeServiceChannelNotif, PACKET_EXCEEDS_MAX_SIZE);
            return ServiceChannelNotification::PACKET_EXCEEDS_MAX_SIZE;
        }
    } else {
        // We've already checked whether there is enough space in the buffer so we can simply remove the packet from
        // the buffer.
		mapChannel->unprocessedTransferFrameListBufferTC.pop_front();

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
			virtChannel->storeVC(transfer_frame);
        } else {
            if (segmentationEnabled) {
				transfer_frame->setSegmentationHeader((0xc0) | (static_cast<bool>(mapid) && 0x3F));
            }
			virtChannel->storeVC(transfer_frame);
        }
    }
	ccsdsLog(Tx, TypeServiceChannelNotif, NO_SERVICE_EVENT);
    return ServiceChannelNotification::NO_SERVICE_EVENT;
}

#if MAX_RECEIVED_UNPROCESSED_TX_TC_IN_VIRT_BUFFER > 0

ServiceChannelNotif ServiceChannel::vcpp_request(uint8_t vid) {
    VirtualChannel* virt_channel = &(masterChannel.virtChannels.at(vid));

    if (virt_channel->txUnprocessedPacketList.empty()) {
        return ServiceChannelNotif::NO_TX_PACKETS_TO_PROCESS;
    }

    if (virt_channel->txWaitQueue.full()) {
        return ServiceChannelNotif::VC_MC_FRAME_BUFFER_FULL;
        ;
    }
    PacketTC* packet = virt_channel->txUnprocessedPacketList.front();

    const uint16_t max_frame_length = virt_channel->maxFrameLength;
    bool segmentation_enabled = virt_channel->segmentHeaderPresent;
    bool blocking_enabled = virt_channel->blocking;

    const uint16_t max_packet_length = max_frame_length - (tc_primary_header_size + segmentation_enabled * 1U);

    if (packet->packet_length() > max_packet_length) {
        if (segmentation_enabled) {
            // Check if there is enough space in the buffer of the virtual channel to store_out all the segments
            uint8_t tf_n =
                (packet->packet_length() / max_packet_length) + (packet->packet_length() % max_packet_length != 0);

            if (virt_channel->txWaitQueue.capacity() >= tf_n) {
                // Break up packet
                virt_channel->txUnprocessedPacketList.pop_front();

                // First portion
                uint16_t seg_header = 0x40;

                PacketTC t_packet =
                    PacketTC(packet->packet_data(), max_packet_length, seg_header, packet->global_virtual_channel_id(),
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
            virt_channel->store(packet);
        }
    }
    return ServiceChannelNotif::NO_SERVICE_EVENT;
}

#endif

ServiceChannelNotification ServiceChannel::vcGenerationRequest(uint8_t vid) {
    VirtualChannel *virt_channel = &(masterChannel.virtChannels.at(vid));
    if (virt_channel->txUnprocessedTransferFrameListBufferTC.empty()) {
		ccsdsLog(Tx, TypeServiceChannelNotif, NO_TX_PACKETS_TO_PROCESS);
        return ServiceChannelNotification::NO_TX_PACKETS_TO_PROCESS;
    }

    if (masterChannel.txOutFramesBeforeAllFramesGenerationList.full()) {
		ccsdsLog(Tx, TypeServiceChannelNotif, TX_MC_FRAME_BUFFER_FULL);
        return ServiceChannelNotification::TX_MC_FRAME_BUFFER_FULL;
    }

	TransferFrameTC*frame = virt_channel->txUnprocessedTransferFrameListBufferTC.front();
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

    virt_channel->txUnprocessedTransferFrameListBufferTC.pop_front();
	ccsdsLog(Tx, TypeServiceChannelNotif, NO_SERVICE_EVENT);
    return ServiceChannelNotification::NO_SERVICE_EVENT;
}

ServiceChannelNotification ServiceChannel::allFramesReceptionRequest() {
    if (masterChannel.rxInFramesBeforeAllFramesReceptionList.empty()) {
		ccsdsLog(Rx, TypeServiceChannelNotif, NO_RX_PACKETS_TO_PROCESS);
        return ServiceChannelNotification::NO_RX_PACKETS_TO_PROCESS;
    }

    if (masterChannel.rxToBeTransmittedFramesAfterAllFramesReceptionList.full()) {
		ccsdsLog(Rx, TypeServiceChannelNotif, RX_OUT_BUFFER_FULL);
        return ServiceChannelNotification::RX_OUT_BUFFER_FULL;
    }

	TransferFrameTC *packet = masterChannel.rxInFramesBeforeAllFramesReceptionList.front();
    VirtualChannel *virt_channel = &(masterChannel.virtChannels.at(packet->virtualChannelId()));

    if (virt_channel->rxWaitQueue.full()) {
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
    if (packet->spacecraftId() == SPACECRAFT_IDENTIFIER) {
		ccsdsLog(Rx, TypeServiceChannelNotif, RX_INVALID_SCID);
        return ServiceChannelNotification::RX_INVALID_SCID;
    }

    // PacketTC length is checked upon storing the packet in the MC

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

    virt_channel->rxWaitQueue.push_back(packet);
    masterChannel.rxInFramesBeforeAllFramesReceptionList.pop_front();
	ccsdsLog(Rx, TypeServiceChannelNotif, NO_SERVICE_EVENT);
    return ServiceChannelNotification::NO_SERVICE_EVENT;
}

std::optional<TransferFrameTC> ServiceChannel::getTxProcessedPacket() {
    if (masterChannel.txOutFramesBeforeAllFramesGenerationList.empty()) {
        return {};
    }

	TransferFrameTC packet = *masterChannel.txOutFramesBeforeAllFramesGenerationList.front();
    // TODO: Here the packet should probably be deleted from the master buffer
    return packet;
}

ServiceChannelNotification ServiceChannel::allFramesGenerationRequest() {
    if (masterChannel.txOutFramesBeforeAllFramesGenerationList.empty()) {
		ccsdsLog(Tx, TypeServiceChannelNotif, NO_TX_PACKETS_TO_PROCESS);
        return ServiceChannelNotification::NO_TX_PACKETS_TO_PROCESS;
    }

    if (masterChannel.txToBeTransmittedFramesAfterAllFramesGenerationList.full()) {
		ccsdsLog(Tx, TypeServiceChannelNotif, TX_TO_BE_TRANSMITTED_FRAMES_LIST_FULL);
        return ServiceChannelNotification::TX_TO_BE_TRANSMITTED_FRAMES_LIST_FULL;
    }

	TransferFrameTC *packet = masterChannel.txOutFramesBeforeAllFramesGenerationList.front();
    masterChannel.txOutFramesBeforeAllFramesGenerationList.pop_front();

    if (masterChannel.errorCtrlField) {
        packet->append_crc();
    }

	masterChannel.storeTransmittedOut(packet);
	ccsdsLog(Tx, TypeServiceChannelNotif, NO_SERVICE_EVENT);
    return ServiceChannelNotification::NO_SERVICE_EVENT;
}

ServiceChannelNotification ServiceChannel::transmitFrame(uint8_t *pack) {
    if (masterChannel.txToBeTransmittedFramesAfterAllFramesGenerationList.empty()) {
		ccsdsLog(Tx, TypeServiceChannelNotif, TX_TO_BE_TRANSMITTED_FRAMES_LIST_EMPTY);
        return ServiceChannelNotification::TX_TO_BE_TRANSMITTED_FRAMES_LIST_EMPTY;
    }

	TransferFrameTC *packet = masterChannel.txToBeTransmittedFramesAfterAllFramesGenerationList.front();
	packet->setRepetitions(packet->repetitions() - 1);
    if (packet->repetitions() == 0) {
        masterChannel.txToBeTransmittedFramesAfterAllFramesGenerationList.pop_front();
		ccsdsLog(Tx, TypeServiceChannelNotif, NO_SERVICE_EVENT);
    }
    memcpy(pack, packet, packet->transfer_frame_length());
    return ServiceChannelNotification::NO_SERVICE_EVENT;
}

ServiceChannelNotification ServiceChannel::
    transmitAdFrame(uint8_t vid) {
    VirtualChannel *virt_channel = &(masterChannel.virtChannels.at(vid));
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

// TODO: Probably not needed. Refactor sentQueue
ServiceChannelNotification ServiceChannel::pushSentQueue(uint8_t vid) {
    VirtualChannel *virt_channel = &(masterChannel.virtChannels.at(vid));
    COPDirectiveResponse req;
    req = virt_channel->fop.pushSentQueue();

    if (req == COPDirectiveResponse::ACCEPT) {
		ccsdsLog(Tx, TypeServiceChannelNotif, NO_SERVICE_EVENT);
        return ServiceChannelNotification::NO_SERVICE_EVENT;
    }
	ccsdsLog(Tx, TypeServiceChannelNotif, TX_FOP_REJECTED);
    return ServiceChannelNotification::TX_FOP_REJECTED;
}

void ServiceChannel::acknowledgeFrame(uint8_t vid, uint8_t frameSeqNumber){
    VirtualChannel *virt_channel = &(masterChannel.virtChannels.at(vid));
	virt_channel->fop.acknowledgeFrame(frameSeqNumber);
}

void ServiceChannel::clearAcknowledgedFrames(uint8_t vid){
    VirtualChannel * virtChannel = &(masterChannel.virtChannels.at(vid));
	virtChannel->fop.removeAcknowledgedFrames();
}

void ServiceChannel::initiateAdNoClcw(uint8_t vid) {
    VirtualChannel * virtChannel = &(masterChannel.virtChannels.at(vid));
    FDURequestType req;
    req = virtChannel->fop.initiateAdNoClcw();
}

void ServiceChannel::initiateAdClcw(uint8_t vid) {
    VirtualChannel * virtChannel = &(masterChannel.virtChannels.at(vid));
    FDURequestType req;
    req = virtChannel->fop.initiateAdClcw();
}

void ServiceChannel::initiateAdUnlock(uint8_t vid) {
    VirtualChannel *virtChannel = &(masterChannel.virtChannels.at(vid));
    FDURequestType req;
    req = virtChannel->fop.initiateAdUnlock();
}

void ServiceChannel::initiateAdVr(uint8_t vid, uint8_t vr) {
    VirtualChannel *virtChannel = &(masterChannel.virtChannels.at(vid));
    FDURequestType req;
    req = virtChannel->fop.initiateAdVr(vr);
}

void ServiceChannel::terminateAdService(uint8_t vid) {
    VirtualChannel *virtChannel = &(masterChannel.virtChannels.at(vid));
    FDURequestType req;
    req = virtChannel->fop.terminateAdService();
}

void ServiceChannel::resumeAdService(uint8_t vid) {
    VirtualChannel *virtChannel = &(masterChannel.virtChannels.at(vid));
    FDURequestType req;
    req = virtChannel->fop.resumeAdService();
}

void ServiceChannel::setVs(uint8_t vid, uint8_t vs) {
    VirtualChannel *virtChannel = &(masterChannel.virtChannels.at(vid));
    virtChannel->fop.setVs(vs);
}

void ServiceChannel::setFopWidth(uint8_t vid, uint8_t width) {
    VirtualChannel *virtChannel = &(masterChannel.virtChannels.at(vid));
    virtChannel->fop.setFopWidth(width);
}

void ServiceChannel::setT1Initial(uint8_t vid, uint16_t t1Init) {
    VirtualChannel *virtChannel = &(masterChannel.virtChannels.at(vid));
    virtChannel->fop.setT1Initial(t1Init);
}

void ServiceChannel::setTransmissionLimit(uint8_t vid, uint8_t vr) {
    VirtualChannel *virtChannel = &(masterChannel.virtChannels.at(vid));
    virtChannel->fop.setTransmissionLimit(vr);
}

void ServiceChannel::setTimeoutType(uint8_t vid, bool vr) {
    VirtualChannel *virtChannel = &(masterChannel.virtChannels.at(vid));
    virtChannel->fop.setTimeoutType(vr);
}

// todo: this may not be needed since it doesn't affect lower procedures and doesn't change the state in any way
void ServiceChannel::invalidDirective(uint8_t vid) {
    VirtualChannel *virtChannel = &(masterChannel.virtChannels.at(vid));
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

std::pair<ServiceChannelNotification, const TransferFrameTC *> ServiceChannel::txOutPacket(const uint8_t vid,
                                                                               const uint8_t mapid) const {
    const etl::list<TransferFrameTC *, MAX_RECEIVED_TC_IN_MAP_CHANNEL> *mc =
            &(masterChannel.virtChannels.at(vid).mapChannels.at(mapid).unprocessedTransferFrameListBufferTC);
    if (mc->empty()) {
		ccsdsLog(Tx, TypeServiceChannelNotif, NO_TX_PACKETS_TO_PROCESS);
        return std::pair(ServiceChannelNotification::NO_TX_PACKETS_TO_PROCESS, nullptr);
    }

    return std::pair(ServiceChannelNotification::NO_SERVICE_EVENT, mc->front());
}

std::pair<ServiceChannelNotification, const TransferFrameTC *> ServiceChannel::txOutPacket(const uint8_t vid) const {
    const etl::list<TransferFrameTC *, MAX_RECEIVED_UNPROCESSED_TX_TC_IN_VIRT_BUFFER> *vc =
            &(masterChannel.virtChannels.at(vid).txUnprocessedTransferFrameListBufferTC);
    if (vc->empty()) {
		ccsdsLog(Tx, TypeServiceChannelNotif, NO_TX_PACKETS_TO_PROCESS);
        return std::pair(ServiceChannelNotification::NO_TX_PACKETS_TO_PROCESS, nullptr);
    }
	ccsdsLog(Tx, TypeServiceChannelNotif, NO_SERVICE_EVENT);
    return std::pair(ServiceChannelNotification::NO_SERVICE_EVENT, vc->front());
}

std::pair<ServiceChannelNotification, const TransferFrameTC *> ServiceChannel::txOutPacketTC() const {
    if (masterChannel.txMasterCopyTC.empty()) {
		ccsdsLog(Tx, TypeServiceChannelNotif, NO_TX_PACKETS_TO_PROCESS);
        return std::pair(ServiceChannelNotification::NO_TX_PACKETS_TO_PROCESS, nullptr);
    }
    return std::pair(ServiceChannelNotification::NO_SERVICE_EVENT, &(masterChannel.txMasterCopyTC.back()));
}

std::pair<ServiceChannelNotification, const TransferFrameTM *> ServiceChannel::txOutPacketTM() const {
    if (masterChannel.txMasterCopyTM.empty()) {
		ccsdsLog(Tx, TypeServiceChannelNotif, NO_TX_PACKETS_TO_PROCESS);
        return std::pair(ServiceChannelNotification::NO_TX_PACKETS_TO_PROCESS, nullptr);
    }
	ccsdsLog(Tx, TypeServiceChannelNotif, NO_SERVICE_EVENT);
    return std::pair(ServiceChannelNotification::NO_SERVICE_EVENT, &(masterChannel.txMasterCopyTM.back()));
}

std::pair<ServiceChannelNotification, const TransferFrameTC *> ServiceChannel::txOutProcessedPacket() const {
    if (masterChannel.txToBeTransmittedFramesAfterAllFramesGenerationList.empty()) {
		ccsdsLog(Tx, TypeServiceChannelNotif, NO_TX_PACKETS_TO_PROCESS);
        return std::pair(ServiceChannelNotification::NO_TX_PACKETS_TO_PROCESS, nullptr);
    }
	ccsdsLog(Tx, TypeServiceChannelNotif, NO_SERVICE_EVENT);
    return std::pair(ServiceChannelNotification::NO_SERVICE_EVENT, masterChannel.txToBeTransmittedFramesAfterAllFramesGenerationList.front());
}