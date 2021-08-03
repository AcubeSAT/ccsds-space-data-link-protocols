#include <CCSDSChannel.hpp>

// Virtual Channel

VirtualChannelAlert VirtualChannel::store(PacketTC *packet) {
    // Limit the amount of packets that can be stored at any given time
    if (txUnprocessedPacketList.full()) {
        return VirtualChannelAlert::TX_WAIT_QUEUE_FULL;
    }
    txUnprocessedPacketList.push_back(packet);
    return VirtualChannelAlert::NO_VC_ALERT;
}

// Master Channel

// Technically not a packet, but it has identical information
// @todo consider another data structure

MasterChannelAlert MasterChannel::store_out(PacketTC *packet) {
    if (txOutFramesList.full()) {
        // Log that buffer is full
        return MasterChannelAlert::OUT_FRAMES_LIST_FULL;
    }
    txOutFramesList.push_back(packet);
    uint8_t vid = packet->global_virtual_channel_id();
    // virtChannels.at(0).fop.

    return MasterChannelAlert::NO_MC_ALERT;
}

MasterChannelAlert MasterChannel::store_transmitted_out(PacketTC *packet) {
    if (txToBeTransmittedFramesList.full()) {
        return MasterChannelAlert::TO_BE_TRANSMITTED_FRAMES_LIST_FULL;
    }
    txToBeTransmittedFramesList.push_back(packet);
    return MasterChannelAlert::NO_MC_ALERT;
}

MasterChannelAlert MasterChannel::add_vc(const uint8_t vcid, const bool segment_header_present,
                                         const uint16_t max_frame_length, const uint8_t clcw_rate, const bool blocking,
                                         const uint8_t repetition_type_a_frame, const uint8_t repetition_cop_ctrl,
                                         etl::flat_map<uint8_t, MAPChannel, max_map_channels> map_chan) {
    if (virtChannels.full()) {
        return MasterChannelAlert::MAX_AMOUNT_OF_VIRT_CHANNELS;
    }

    virtChannels.emplace(vcid, VirtualChannel(*this, vcid, segment_header_present, max_frame_length, clcw_rate,
                                              blocking, repetition_type_a_frame, repetition_cop_ctrl, map_chan));
}
