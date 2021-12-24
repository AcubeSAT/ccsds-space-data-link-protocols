#include <CCSDSServiceChannel.hpp>
#include <PacketTC.hpp>
#include <PacketTM.hpp>
#include <etl/iterator.h>
#include <Alert.hpp>
#include <CCSDS_Log.h>

ServiceChannelNotification ServiceChannel::store(uint8_t *packet, uint16_t packet_length) {
    if (masterChannel.rxMasterCopyTC.full()) {
		ccsds_log(Rx, TypeServiceChannelNotif, RX_IN_MC_FULL);
        return ServiceChannelNotification::RX_IN_MC_FULL;
    }

    if (masterChannel.rxInFramesBeforeAllFramesReceptionList.full()) {
		ccsds_log(Rx, TypeServiceChannelNotif, RX_IN_BUFFER_FULL);
        return ServiceChannelNotification::RX_IN_BUFFER_FULL;
    }

    PacketTC pckt = PacketTC(packet, packet_length);

    if (pckt.getPacketLength() != packet_length) {
		ccsds_log(Rx, TypeServiceChannelNotif, RX_INVALID_LENGTH);
        return ServiceChannelNotification::RX_INVALID_LENGTH;
    }

    masterChannel.rxMasterCopyTC.push_back(pckt);
    PacketTC *master_pckt = &(masterChannel.rxMasterCopyTC.front());
    masterChannel.rxInFramesBeforeAllFramesReceptionList.push_back(master_pckt);
	ccsds_log(Rx, TypeServiceChannelNotif, NO_SERVICE_EVENT);
    return ServiceChannelNotification::NO_SERVICE_EVENT;
}

ServiceChannelNotification ServiceChannel::storeTC(uint8_t *packet, uint16_t packet_length, uint8_t gvcid, uint8_t mapid,
                                          uint16_t sduid, ServiceType service_type) {
    uint8_t vid = gvcid & 0x3F;
    VirtualChannel *vchan = &(masterChannel.virtChannels.at(vid));
    MAPChannel *map_channel = &(vchan->mapChannels.at(mapid));

    if (map_channel->unprocessedPacketListBufferTC.full()) {
		ccsds_log(Tx, TypeServiceChannelNotif, MAP_CHANNEL_FRAME_BUFFER_FULL);
        return ServiceChannelNotification::MAP_CHANNEL_FRAME_BUFFER_FULL;
    }

    PacketTC packet_s =
            PacketTC(packet, packet_length, 0, gvcid, mapid, sduid, service_type, vchan->segmentHeaderPresent);

    if (service_type == ServiceType::TYPE_A) {
		packet_s.setRepetitions(vchan->repetitionTypeAFrame);
    } else if (service_type == ServiceType::TYPE_B) {
		packet_s.setRepetitions(vchan->repetitionCOPCtrl);
    }

    masterChannel.txMasterCopyTC.push_back(packet_s);
    map_channel->unprocessedPacketListBufferTC.push_back(&(masterChannel.txMasterCopyTC.back()));
	ccsds_log(Tx, TypeServiceChannelNotif, NO_SERVICE_EVENT);
    return ServiceChannelNotification::NO_SERVICE_EVENT;
}

ServiceChannelNotification ServiceChannel::storeTM(uint8_t *packet, uint16_t packet_length, uint8_t gvcid, uint16_t scid) {
    uint8_t vid = gvcid & 0x3F;
    VirtualChannel *vchan = &(masterChannel.virtChannels.at(vid));

    if (masterChannel.txMasterCopyTM.full()) {
		ccsds_log(Tx, TypeServiceChannelNotif, MASTER_CHANNEL_FRAME_BUFFER_FULL);
        return ServiceChannelNotification::MASTER_CHANNEL_FRAME_BUFFER_FULL;
    }

    if (masterChannel.txMasterCopyTM.full()) {
		ccsds_log(Tx, TypeServiceChannelNotif, MASTER_CHANNEL_FRAME_BUFFER_FULL);
        return ServiceChannelNotification::MASTER_CHANNEL_FRAME_BUFFER_FULL;
    }

    TransferFrameHeaderTM hdr = TransferFrameHeaderTM(packet);

    uint8_t *secondaryHeader = 0;
    if (hdr.transferFrameSecondaryHeaderFlag() == 1) {
        secondaryHeader = &packet[7];
    }

    PacketTM packet_s =
            PacketTM(packet, packet_length, vchan->frameCount, scid, vid, masterChannel.frameCount,
                     secondaryHeader, hdr.transferFrameDataFieldStatus(), TM);


    masterChannel.txMasterCopyTM.push_back(packet_s);
	ccsds_log(Tx, TypeServiceChannelNotif, NO_SERVICE_EVENT);
    return ServiceChannelNotification::NO_SERVICE_EVENT;
}

ServiceChannelNotification ServiceChannel::mappRequest(uint8_t vid, uint8_t mapid) {
    VirtualChannel *virt_channel = &(masterChannel.virtChannels.at(vid));
    MAPChannel *map_channel = &(virt_channel->mapChannels.at(mapid));

    if (map_channel->unprocessedPacketListBufferTC.empty()) {
		ccsds_log(Tx, TypeServiceChannelNotif, NO_TX_PACKETS_TO_PROCESS);
        return ServiceChannelNotification::NO_TX_PACKETS_TO_PROCESS;
    }

    if (virt_channel->txWaitQueue.full()) {
		ccsds_log(Tx, TypeServiceChannelNotif, VC_MC_FRAME_BUFFER_FULL);
        return ServiceChannelNotification::VC_MC_FRAME_BUFFER_FULL;
    }

    PacketTC *packet = map_channel->unprocessedPacketListBufferTC.front();

    const uint16_t max_frame_length = virt_channel->maxFrameLength;
    bool segmentation_enabled = virt_channel->segmentHeaderPresent;
    bool blocking_enabled = virt_channel->blocking;

    const uint16_t max_packet_length = max_frame_length - (TC_PRIMARY_HEADER_SIZE + segmentation_enabled * 1U);

    if (packet->getPacketLength() > max_packet_length) {
        if (segmentation_enabled) {
            // Check if there is enough space in the buffer of the virtual channel to store_out all the segments
            uint8_t tf_n =
                    (packet->getPacketLength() / max_packet_length) + (packet->getPacketLength() % max_packet_length != 0);

            if (virt_channel->txWaitQueue.available() >= tf_n) {
                // Break up packet
                map_channel->unprocessedPacketListBufferTC.pop_front();

                // First portion
                uint16_t seg_header = mapid | 0x40;

                PacketTC t_packet =
                        PacketTC(packet->packetData(), max_packet_length, seg_header, packet->globalVirtualChannelId(), packet->mapId(), packet->spacecraftId(),
				                             packet->getServiceType(),
                                 virt_channel->segmentHeaderPresent);
                virt_channel->storeVC(&t_packet);

                // Middle portion
				t_packet.setSegmentationHeader(mapid | 0x00);
                for (uint8_t i = 1; i < (tf_n - 1); i++) {
					t_packet.setPacketData(&packet->packetData()[i * max_packet_length]);
                    virt_channel->storeVC(&t_packet);
                }

                // Last portion
				t_packet.setSegmentationHeader(mapid | 0x80);
				t_packet.setPacketData(&packet->packetData()[(tf_n - 1) * max_packet_length]);
				t_packet.setPacketLength(packet->getPacketLength() % max_packet_length);
                virt_channel->storeVC(&t_packet);
            }
        } else {
			ccsds_log(Tx, TypeServiceChannelNotif, PACKET_EXCEEDS_MAX_SIZE);
            return ServiceChannelNotification::PACKET_EXCEEDS_MAX_SIZE;
        }
    } else {
        // We've already checked whether there is enough space in the buffer so we can simply remove the packet from
        // the buffer.
        map_channel->unprocessedPacketListBufferTC.pop_front();

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
            virt_channel->storeVC(packet);
        } else {
            if (segmentation_enabled) {
				packet->setSegmentationHeader((0xc0) | (mapid && 0x3F));
            }
            virt_channel->storeVC(packet);
        }
    }
	ccsds_log(Tx, TypeServiceChannelNotif, NO_SERVICE_EVENT);
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
    if (virt_channel->txUnprocessedPacketListBufferTC.empty()) {
		ccsds_log(Tx, TypeServiceChannelNotif, NO_TX_PACKETS_TO_PROCESS);
        return ServiceChannelNotification::NO_TX_PACKETS_TO_PROCESS;
    }

    if (masterChannel.txOutFramesBeforeAllFramesGenerationList.full()) {
		ccsds_log(Tx, TypeServiceChannelNotif, TX_MC_FRAME_BUFFER_FULL);
        return ServiceChannelNotification::TX_MC_FRAME_BUFFER_FULL;
    }

    PacketTC *frame = virt_channel->txUnprocessedPacketListBufferTC.front();
    COPDirectiveResponse err = COPDirectiveResponse::ACCEPT;

    if (frame->transferFrameHeader().ctrlAndCmdFlag() == 0) {
        err = virt_channel->fop.transferFdu();
    } else {
        err = virt_channel->fop.validClcwArrival();
    }

    if (err == COPDirectiveResponse::REJECT) {
		ccsds_log(Tx, TypeServiceChannelNotif, FOP_REQUEST_REJECTED);
        return ServiceChannelNotification::FOP_REQUEST_REJECTED;
    }

    virt_channel->txUnprocessedPacketListBufferTC.pop_front();
	ccsds_log(Tx, TypeServiceChannelNotif, NO_SERVICE_EVENT);
    return ServiceChannelNotification::NO_SERVICE_EVENT;
}

ServiceChannelNotification ServiceChannel::allFramesReceptionRequest() {
    if (masterChannel.rxInFramesBeforeAllFramesReceptionList.empty()) {
		ccsds_log(Rx, TypeServiceChannelNotif, NO_RX_PACKETS_TO_PROCESS);
        return ServiceChannelNotification::NO_RX_PACKETS_TO_PROCESS;
    }

    if (masterChannel.rxToBeTransmittedFramesAfterAllFramesReceptionList.full()) {
		ccsds_log(Rx, TypeServiceChannelNotif, RX_OUT_BUFFER_FULL);
        return ServiceChannelNotification::RX_OUT_BUFFER_FULL;
    }

    PacketTC *packet = masterChannel.rxInFramesBeforeAllFramesReceptionList.front();
    VirtualChannel *virt_channel = &(masterChannel.virtChannels.at(packet->virtualChannelId()));

    if (virt_channel->rxWaitQueue.full()) {
		ccsds_log(Rx, TypeServiceChannelNotif, VC_RX_WAIT_QUEUE_FULL);
        return ServiceChannelNotification::VC_RX_WAIT_QUEUE_FULL;
    }

    // Frame Delimiting and Fill Removal supposedly aren't implemented here

    /*
     * Frame validation checks
     */

    // Check for valid TFVN
    if (packet->getTransferFrameVersionNumber() != 0) {
		ccsds_log(Rx, TypeServiceChannelNotif, RX_INVALID_TFVN);
        return ServiceChannelNotification::RX_INVALID_TFVN;
    }

    // Check for valid SCID
    if (packet->spacecraftId() == SPACECRAFT_IDENTIFIER) {
		ccsds_log(Rx, TypeServiceChannelNotif, RX_INVALID_SCID);
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
	ccsds_log(Rx, TypeServiceChannelNotif, NO_SERVICE_EVENT);
    return ServiceChannelNotification::NO_SERVICE_EVENT;
}

std::optional<PacketTC> ServiceChannel::getTxProcessedPacket() {
    if (masterChannel.txOutFramesBeforeAllFramesGenerationList.empty()) {
        return {};
    }

    PacketTC packet = *masterChannel.txOutFramesBeforeAllFramesGenerationList.front();
    // TODO: Here the packet should probably be deleted from the master buffer
    return packet;
}

ServiceChannelNotification ServiceChannel::allFramesGenerationRequest() {
    if (masterChannel.txOutFramesBeforeAllFramesGenerationList.empty()) {
		ccsds_log(Tx, TypeServiceChannelNotif, NO_TX_PACKETS_TO_PROCESS);
        return ServiceChannelNotification::NO_TX_PACKETS_TO_PROCESS;
    }

    if (masterChannel.txToBeTransmittedFramesAfterAllFramesGenerationList.full()) {
		ccsds_log(Tx, TypeServiceChannelNotif, TX_TO_BE_TRANSMITTED_FRAMES_LIST_FULL);
        return ServiceChannelNotification::TX_TO_BE_TRANSMITTED_FRAMES_LIST_FULL;
    }

    PacketTC *packet = masterChannel.txOutFramesBeforeAllFramesGenerationList.front();
    masterChannel.txOutFramesBeforeAllFramesGenerationList.pop_front();

    if (masterChannel.errorCtrlField) {
        packet->append_crc();
    }

	masterChannel.storeTransmittedOut(packet);
	ccsds_log(Tx, TypeServiceChannelNotif, NO_SERVICE_EVENT);
    return ServiceChannelNotification::NO_SERVICE_EVENT;
}

ServiceChannelNotification ServiceChannel::transmitFrame(uint8_t *pack) {
    if (masterChannel.txToBeTransmittedFramesAfterAllFramesGenerationList.empty()) {
		ccsds_log(Tx, TypeServiceChannelNotif, TX_TO_BE_TRANSMITTED_FRAMES_LIST_EMPTY);
        return ServiceChannelNotification::TX_TO_BE_TRANSMITTED_FRAMES_LIST_EMPTY;
    }

    PacketTC *packet = masterChannel.txToBeTransmittedFramesAfterAllFramesGenerationList.front();
	packet->setRepetitions(packet->repetitions() - 1);
    if (packet->repetitions() == 0) {
        masterChannel.txToBeTransmittedFramesAfterAllFramesGenerationList.pop_front();
		ccsds_log(Tx, TypeServiceChannelNotif, NO_SERVICE_EVENT);
    }
    memcpy(pack, packet, packet->getPacketLength());
    return ServiceChannelNotification::NO_SERVICE_EVENT;
}

ServiceChannelNotification ServiceChannel::transmitAdFrame(uint8_t vid) {
    VirtualChannel *virt_channel = &(masterChannel.virtChannels.at(vid));
	FOPNotification req;
    req = virt_channel->fop.transmitAdFrame();
    if (req == FOPNotification::NO_FOP_EVENT) {
		ccsds_log(Tx, TypeServiceChannelNotif, NO_SERVICE_EVENT);
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
		ccsds_log(Tx, TypeServiceChannelNotif, NO_SERVICE_EVENT);
        return ServiceChannelNotification::NO_SERVICE_EVENT;
    }
	ccsds_log(Tx, TypeServiceChannelNotif, TX_FOP_REJECTED);
    return ServiceChannelNotification::TX_FOP_REJECTED;
}

void ServiceChannel::acknowledgeFrame(uint8_t vid, uint8_t frame_seq_number){
    VirtualChannel *virt_channel = &(masterChannel.virtChannels.at(vid));
	virt_channel->fop.acknowledgeFrame(frame_seq_number);
}

void ServiceChannel::clearAcknowledgedFrames(uint8_t vid){
    VirtualChannel *virt_channel = &(masterChannel.virtChannels.at(vid));
	virt_channel->fop.removeAcknowledgedFrames();
}

void ServiceChannel::initiateAdNoClcw(uint8_t vid) {
    VirtualChannel *virt_channel = &(masterChannel.virtChannels.at(vid));
    FDURequestType req;
    req = virt_channel->fop.initiateAdNoClcw();
}

void ServiceChannel::initiateAdClcw(uint8_t vid) {
    VirtualChannel *virt_channel = &(masterChannel.virtChannels.at(vid));
    FDURequestType req;
    req = virt_channel->fop.initiateAdClcw();
}

void ServiceChannel::initiateAdUnlock(uint8_t vid) {
    VirtualChannel *virt_channel = &(masterChannel.virtChannels.at(vid));
    FDURequestType req;
    req = virt_channel->fop.initiateAdUnlock();
}

void ServiceChannel::initiateAdVr(uint8_t vid, uint8_t vr) {
    VirtualChannel *virt_channel = &(masterChannel.virtChannels.at(vid));
    FDURequestType req;
    req = virt_channel->fop.initiateAdVr(vr);
}

void ServiceChannel::terminateAdService(uint8_t vid) {
    VirtualChannel *virt_channel = &(masterChannel.virtChannels.at(vid));
    FDURequestType req;
    req = virt_channel->fop.terminateAdService();
}

void ServiceChannel::resumeAdService(uint8_t vid) {
    VirtualChannel *virt_channel = &(masterChannel.virtChannels.at(vid));
    FDURequestType req;
    req = virt_channel->fop.resumeAdService();
}

void ServiceChannel::setVs(uint8_t vid, uint8_t vs) {
    VirtualChannel *virt_channel = &(masterChannel.virtChannels.at(vid));
	virt_channel->fop.setVs(vs);
}

void ServiceChannel::setFopWidth(uint8_t vid, uint8_t width) {
    VirtualChannel *virt_channel = &(masterChannel.virtChannels.at(vid));
	virt_channel->fop.setFopWidth(width);
}

void ServiceChannel::setT1Initial(uint8_t vid, uint16_t t1_init) {
    VirtualChannel *virt_channel = &(masterChannel.virtChannels.at(vid));
	virt_channel->fop.setT1Initial(t1_init);
}

void ServiceChannel::setTransmissionLimit(uint8_t vid, uint8_t vr) {
    VirtualChannel *virt_channel = &(masterChannel.virtChannels.at(vid));
	virt_channel->fop.setTransmissionLimit(vr);
}

void ServiceChannel::setTimeoutType(uint8_t vid, bool vr) {
    VirtualChannel *virt_channel = &(masterChannel.virtChannels.at(vid));
	virt_channel->fop.setTimeoutType(vr);
}

// todo: this may not be needed since it doesn't affect lower procedures and doesn't change the state in any way
void ServiceChannel::invalidDirective(uint8_t vid) {
    VirtualChannel *virt_channel = &(masterChannel.virtChannels.at(vid));
	virt_channel->fop.invalidDirective();
}

const FOPState ServiceChannel::fopState(uint8_t vid) const {
    return masterChannel.virtChannels.at(vid).fop.state;
}

const uint16_t ServiceChannel::t1Timer(uint8_t vid) const {
    return masterChannel.virtChannels.at(vid).fop.tiInitial;
}

const uint8_t ServiceChannel::fopSlidingWindowWidth(uint8_t vid) const {
    return masterChannel.virtChannels.at(vid).fop.fopSlidingWindow;
}

const bool ServiceChannel::timeoutType(uint8_t vid) const {
    return masterChannel.virtChannels.at(vid).fop.timeoutType;
}

const uint8_t ServiceChannel::transmitterFrameSeqNumber(uint8_t vid) const {
    return masterChannel.virtChannels.at(vid).fop.transmitterFrameSeqNumber;
}

const uint8_t ServiceChannel::expectedFrameSeqNumber(uint8_t vid) const {
    return masterChannel.virtChannels.at(vid).fop.expectedAcknowledgementSeqNumber;
}

std::pair<ServiceChannelNotification, const PacketTC *> ServiceChannel::txOutPacket(const uint8_t vid,
                                                                               const uint8_t mapid) const {
    const etl::list<PacketTC *, MAX_RECEIVED_TC_IN_MAP_CHANNEL> *mc =
            &(masterChannel.virtChannels.at(vid).mapChannels.at(mapid).unprocessedPacketListBufferTC);
    if (mc->empty()) {
		ccsds_log(Tx, TypeServiceChannelNotif, NO_TX_PACKETS_TO_PROCESS);
        return std::pair(ServiceChannelNotification::NO_TX_PACKETS_TO_PROCESS, nullptr);
    }

    return std::pair(ServiceChannelNotification::NO_SERVICE_EVENT, mc->front());
}

std::pair<ServiceChannelNotification, const PacketTC *> ServiceChannel::txOutPacket(const uint8_t vid) const {
    const etl::list<PacketTC *, MAX_RECEIVED_UNPROCESSED_TX_TC_IN_VIRT_BUFFER> *vc =
            &(masterChannel.virtChannels.at(vid).txUnprocessedPacketListBufferTC);
    if (vc->empty()) {
		ccsds_log(Tx, TypeServiceChannelNotif, NO_TX_PACKETS_TO_PROCESS);
        return std::pair(ServiceChannelNotification::NO_TX_PACKETS_TO_PROCESS, nullptr);
    }
	ccsds_log(Tx, TypeServiceChannelNotif, NO_SERVICE_EVENT);
    return std::pair(ServiceChannelNotification::NO_SERVICE_EVENT, vc->front());
}

std::pair<ServiceChannelNotification, const PacketTC *> ServiceChannel::txOutPacketTC() const {
    if (masterChannel.txMasterCopyTC.empty()) {
		ccsds_log(Tx, TypeServiceChannelNotif, NO_TX_PACKETS_TO_PROCESS);
        return std::pair(ServiceChannelNotification::NO_TX_PACKETS_TO_PROCESS, nullptr);
    }
    return std::pair(ServiceChannelNotification::NO_SERVICE_EVENT, &(masterChannel.txMasterCopyTC.back()));
}

std::pair<ServiceChannelNotification, const PacketTM *> ServiceChannel::txOutPacketTM() const {
    if (masterChannel.txMasterCopyTM.empty()) {
		ccsds_log(Tx, TypeServiceChannelNotif, NO_TX_PACKETS_TO_PROCESS);
        return std::pair(ServiceChannelNotification::NO_TX_PACKETS_TO_PROCESS, nullptr);
    }
	ccsds_log(Tx, TypeServiceChannelNotif, NO_SERVICE_EVENT);
    return std::pair(ServiceChannelNotification::NO_SERVICE_EVENT, &(masterChannel.txMasterCopyTM.back()));
}

std::pair<ServiceChannelNotification, const PacketTC *> ServiceChannel::txOutProcessedPacket() const {
    if (masterChannel.txToBeTransmittedFramesAfterAllFramesGenerationList.empty()) {
		ccsds_log(Tx, TypeServiceChannelNotif, NO_TX_PACKETS_TO_PROCESS);
        return std::pair(ServiceChannelNotification::NO_TX_PACKETS_TO_PROCESS, nullptr);
    }
	ccsds_log(Tx, TypeServiceChannelNotif, NO_SERVICE_EVENT);
    return std::pair(ServiceChannelNotification::NO_SERVICE_EVENT, masterChannel.txToBeTransmittedFramesAfterAllFramesGenerationList.front());
}