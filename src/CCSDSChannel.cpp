#include <CCSDSChannel.hpp>


// Virtual Channel

VirtualChannelAlert VirtualChannel::store(Packet* packet) {
    // Limit the amount of packets that can be stored at any given time
    if (unprocessedPacketList.full()) {
        return VirtualChannelAlert::WAIT_QUEUE_FULL;
    }
    unprocessedPacketList.push_back(packet);
    return VirtualChannelAlert::NO_VC_ALERT;
}

// Master Channel

// Technically not a packet, but it has identical information
// @todo consider another data structure

MasterChannelAlert MasterChannel::store_out(Packet* packet) {
    if (outFramesList.full()) {
        // Log that buffer is full
        return MasterChannelAlert::OUT_FRAMES_LIST_FULL;
    }
    outFramesList.push_back(packet);
    return MasterChannelAlert::NO_MC_ALERT;
}

MasterChannelAlert MasterChannel::store_transmitted_out(Packet* packet){
    if (toBeTransmittedFramesList.full()){
        return MasterChannelAlert::TO_BE_TRANSMITTED_FRAMES_LIST_FULL;
    }
    toBeTransmittedFramesList.push_back(packet);
    return MasterChannelAlert::NO_MC_ALERT;
}