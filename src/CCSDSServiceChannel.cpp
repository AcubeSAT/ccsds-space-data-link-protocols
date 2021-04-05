#include <CCSDSServiceChannel.hpp>
#include <Packet.hpp>
#include <etl/iterator.h>
#include <Alert.hpp>


ServiceChannelNotif
ServiceChannel::store(uint8_t *packet, uint16_t packet_length, uint8_t gvcid, uint8_t mapid, uint16_t sduid,
                      ServiceType service_type) {
    uint8_t vid = gvcid & 0x3F;
    MAPChannel *map_channel = &(masterChannel.virtChannels.at(vid).mapChannels.at(mapid));

    if (map_channel->packetList.full()) {
        return ServiceChannelNotif::MAP_CHANNEL_FRAME_BUFFER_FULL;
    }

    Packet packet_s = Packet(packet, packet_length, 0, gvcid, mapid, sduid, service_type);
    map_channel->packetList.push_back(packet_s);
    return ServiceChannelNotif::NO_SERVICE_EVENT;
}

ServiceChannelNotif ServiceChannel::mapp_request(uint8_t vid, uint8_t mapid) {
    VirtualChannel *virt_channel = &(masterChannel.virtChannels.at(vid));
    MAPChannel *map_channel = &(virt_channel->mapChannels.at(mapid));

    if (map_channel->packetList.empty()) {
        return ServiceChannelNotif::NO_PACKETS_TO_PROCESS;
    }

    if (virt_channel->waitQueue.full()) {
        return ServiceChannelNotif::VIRTUAL_CHANNEL_FRAME_BUFFER_FULL;
    }

    Packet packet = map_channel->packetList.front();

    const uint16_t max_frame_length = virt_channel->maxFrameLength;
    bool segmentation_enabled = virt_channel->segmentHeaderPresent;
    bool blocking_enabled = virt_channel->blocking;

    const uint16_t max_packet_length = max_frame_length - (TC_PRIMARY_HEADER_SIZE + segmentation_enabled * 1U);

    if (packet.packetLength > max_packet_length) {
        if (segmentation_enabled) {
            // Check if there is enough space in the buffer of the virtual channel to store all the segments
            uint8_t tf_n = (packet.packetLength / max_packet_length) + (packet.packetLength % max_packet_length != 0);

            if (virt_channel->waitQueue.capacity() >= tf_n) {
                // Break up packet
                map_channel->packetList.pop_front();

                // First portion
                uint16_t seg_header = mapid || 0x40;

                Packet t_packet = Packet(packet.packet, max_packet_length, seg_header, packet.gvcid, packet.mapid,
                                         packet.sduid,
                                         packet.serviceType);
                virt_channel->store(t_packet);

                // Middle portion
                t_packet.segHdr = mapid || 0x00;
                for (uint8_t i = 1; i < (tf_n - 1); i++) {
                    t_packet.packet = &packet.packet[i * max_packet_length];
                    virt_channel->store(t_packet);
                }

                // Last portion
                t_packet.segHdr = mapid || 0x80;
                t_packet.packet = &packet.packet[(tf_n - 1) * max_packet_length];
                t_packet.packetLength = packet.packetLength % max_packet_length;
                virt_channel->store(t_packet);
            }
        } else {
            return ServiceChannelNotif::PACKET_EXCEEDS_MAX_SIZE;
        }
    } else {
        // We've already checked whether there is enough space in the buffer so we can simply remove the packet from
        // the buffer.
        map_channel->packetList.pop_front();

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
            virt_channel->store(packet);
        } else {
            if (segmentation_enabled) {
                packet.segHdr = (0xc0) || (mapid && 0x3F);
            }
            virt_channel->store(packet);
        }
    }
    return ServiceChannelNotif::NO_SERVICE_EVENT;
}

#if MAX_RECEIVED_UNPROCESSED_TC_IN_VIRT_BUFFER > 0

ServiceChannelNotif ServiceChannel::vcpp_request(uint8_t vid) {
    VirtualChannel *virt_channel = &(masterChannel.virtChannels.at(vid));

    if (virt_channel->unprocessedPacketList.empty()) {
        return ServiceChannelNotif::NO_PACKETS_TO_PROCESS;
    }

    if (virt_channel->waitQueue.full()) {
        return ServiceChannelNotif::VIRTUAL_CHANNEL_FRAME_BUFFER_FULL;;
    }
    Packet packet = virt_channel->unprocessedPacketList.front();

    const uint16_t max_frame_length = virt_channel->maxFrameLength;
    bool segmentation_enabled = virt_channel->segmentHeaderPresent;
    bool blocking_enabled = virt_channel->blocking;

    const uint16_t max_packet_length = max_frame_length - (TC_PRIMARY_HEADER_SIZE + segmentation_enabled * 1U);

    if (packet.packetLength > max_packet_length) {
        if (segmentation_enabled) {
            // Check if there is enough space in the buffer of the virtual channel to store all the segments
            uint8_t tf_n = (packet.packetLength / max_packet_length) + (packet.packetLength % max_packet_length != 0);

            if (virt_channel->waitQueue.capacity() >= tf_n) {
                // Break up packet
                virt_channel->unprocessedPacketList.pop_front();

                // First portion
                uint16_t seg_header = 0x40;

                Packet t_packet = Packet(packet.packet, max_packet_length, seg_header, packet.gvcid, packet.mapid,
                                         packet.sduid,
                                         packet.serviceType);
                virt_channel->store(t_packet);

                // Middle portion
                t_packet.segHdr = 0x00;
                for (uint8_t i = 1; i < (tf_n - 1); i++) {
                    t_packet.packet = &packet.packet[i * max_packet_length];
                    virt_channel->store(t_packet);
                }

                // Last portion
                t_packet.segHdr = 0x80;
                t_packet.packet = &packet.packet[(tf_n - 1) * max_packet_length];
                t_packet.packetLength = packet.packetLength % max_packet_length;
                virt_channel->store(t_packet);
            }
        } else {
            return ServiceChannelNotif::PACKET_EXCEEDS_MAX_SIZE;
        }
    } else {
        // We've already checked whether there is enough space in the buffer so we can simply remove the packet from
        // the buffer.
        virt_channel->unprocessedPacketList.pop_front();

        if (blocking_enabled) {
            virt_channel->store(packet);
        } else {
            if (segmentation_enabled) {
                packet.segHdr = 0xc0;
            }
            virt_channel->store(packet);
        }
    }
    return ServiceChannelNotif::NO_SERVICE_EVENT;
}

#endif

ServiceChannelNotif ServiceChannel::vc_generation_request(uint8_t vid) {
    VirtualChannel virt_channel = std::move(masterChannel.virtChannels.at(vid));
    if (virt_channel.waitQueue.empty()) {
        return ServiceChannelNotif::NO_PACKETS_TO_PROCESS;
    }

    if (masterChannel.framesList.full()) {
        return ServiceChannelNotif::MASTER_CHANNEL_FRAME_BUFFER_FULL;
    }

    Packet packet = virt_channel.waitQueue.front();
    FOPNotif err;

    if (packet.serviceType == ServiceType::TYPE_A) {
        err = virt_channel.fop.transmit_ad_frame(packet);
    } else{
        err = virt_channel.fop.transmit_bc_frame(packet);
    }

    if (err == FOPNotif::SENT_QUEUE_FULL){
        return ServiceChannelNotif::FOP_SENT_QUEUE_FULL;
    }

    return ServiceChannelNotif::NO_SERVICE_EVENT;
}

void ServiceChannel::initiate_ad_no_clcw(uint8_t vid){
    VirtualChannel *virt_channel = &(masterChannel.virtChannels.at(vid));
    FDURequestType req;
    req = virt_channel->fop.initiate_ad_no_clcw();
}

void ServiceChannel::initiate_ad_clcw(uint8_t vid){
    VirtualChannel *virt_channel = &(masterChannel.virtChannels.at(vid));
    FDURequestType req;
    req = virt_channel->fop.initiate_ad_clcw();
}

void ServiceChannel::initiate_ad_unlock(uint8_t vid){
    VirtualChannel *virt_channel = &(masterChannel.virtChannels.at(vid));
    FDURequestType req;
    req = virt_channel->fop.initiate_ad_unlock();
}

void ServiceChannel::initiate_ad_vr(uint8_t vid, uint8_t vr){
    VirtualChannel *virt_channel = &(masterChannel.virtChannels.at(vid));
    FDURequestType req;
    req = virt_channel->fop.initiate_ad_vr(vr);
}

void ServiceChannel::terminate_ad_service(uint8_t vid){
    VirtualChannel *virt_channel = &(masterChannel.virtChannels.at(vid));
    FDURequestType req;
    req = virt_channel->fop.terminate_ad_service();
}

void ServiceChannel::resume_ad_service(uint8_t vid){
    VirtualChannel *virt_channel = &(masterChannel.virtChannels.at(vid));
    FDURequestType req;
    req = virt_channel->fop.resume_ad_service();
}

void ServiceChannel::set_vs(uint8_t vid, uint8_t vs){
    VirtualChannel *virt_channel = &(masterChannel.virtChannels.at(vid));
    virt_channel->fop.set_vs(vs);
}

void ServiceChannel::set_fop_width(uint8_t vid, uint8_t width){
    VirtualChannel *virt_channel = &(masterChannel.virtChannels.at(vid));
    virt_channel->fop.set_fop_width(width);
}

void ServiceChannel::set_t1_initial(uint8_t vid, uint16_t t1_init){
    VirtualChannel *virt_channel = &(masterChannel.virtChannels.at(vid));
    virt_channel->fop.set_t1_initial(t1_init);
}

void ServiceChannel::set_transmission_limit(uint8_t vid, uint8_t vr){
    VirtualChannel *virt_channel = &(masterChannel.virtChannels.at(vid));
    virt_channel->fop.set_transmission_limit(vr);
}

void ServiceChannel::set_timeout_type(uint8_t vid, bool vr){
    VirtualChannel *virt_channel = &(masterChannel.virtChannels.at(vid));
    virt_channel->fop.set_timeout_type(vr);
}

//todo: this may not be needed since it doesn't affect lower procedures and doesn't change the state in any way
void ServiceChannel::invalid_directive(uint8_t vid){
    VirtualChannel *virt_channel = &(masterChannel.virtChannels.at(vid));
    virt_channel->fop.invalid_directive();
}

const FOPState ServiceChannel::fop_state(uint8_t vid) const{
    return masterChannel.virtChannels.at(vid).fop.state;
}

const uint16_t ServiceChannel::t1_timer(uint8_t vid) const{
    return masterChannel.virtChannels.at(vid).fop.tiInitial;
}

const uint8_t ServiceChannel::fop_sliding_window_width(uint8_t vid) const{
    return masterChannel.virtChannels.at(vid).fop.fopSlidingWindow;
}

const bool ServiceChannel::timeout_type(uint8_t vid) const {
    return masterChannel.virtChannels.at(vid).fop.timeoutType;
}

const uint8_t ServiceChannel::transmitter_frame_seq_number(uint8_t vid) const{
    return masterChannel.virtChannels.at(vid).fop.transmitterFrameSeqNumber;
}

const uint8_t ServiceChannel::expected_frame_seq_number(uint8_t vid) const{
    return masterChannel.virtChannels.at(vid).fop.expectedAcknowledgementSeqNumber;
}