#include <CCSDSChannel.hpp>

// MAP Channel

void MAPChannel::store(Packet packet) {
    if (packetList.full()) {
        // Log that buffer is full
        return;
    }
    packetList.push_back(packet);
}

// Virtual Channel

// @todo rename to something that makes more sense
VirtualChannelAlert VirtualChannel::store_unprocessed(Packet packet) {
    // Limit the amount of packets that can be stored at any given time
    if (unprocessedPacketList.full()) {
        return VirtualChannelAlert::UNPROCESSED_PACKET_LIST_FULL;
    }
    unprocessedPacketList.push_back(packet);
    return VirtualChannelAlert::NO_VC_ALERT;
}

VirtualChannelAlert VirtualChannel::store(Packet* packet) {
    // Limit the amount of packets that can be stored at any given time
    if (waitQueue.full()) {
        return VirtualChannelAlert::WAIT_QUEUE_FULL;
    }
    waitQueue.push_back(packet);
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