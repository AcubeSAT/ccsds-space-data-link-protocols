#include <CCSDSChannel.hpp>

// MAP Channel

void MAPChannel::store(Packet packet){
    if (packetList.full()){
        // Log that buffer is full
        return;
    }
    packetList.push(packet);
}

// Virtual Channel

// @todo rename to something that makes more sense
void VirtualChannel::store_unprocessed(Packet packet){
    // Limit the amount of packets that can be stored at any given time
    if (unprocessedPacketList.full()){
        // Log that buffer is full
        return;
    }
    unprocessedPacketList.push(packet);
}

void VirtualChannel::store(Packet packet){
    // Limit the amount of packets that can be stored at any given time
    if (packetList.full()){
        // Log that buffer is full
        return;
    }
    packetList.push(packet);
}

// Master Channel

// Technically not a packet, but it has identical information
// @todo consider another data structure

void MasterChannel::store(Packet packet){
    if (framesList.full()){
        // Log that buffer is full
        return;
    }
    framesList.push(packet);
}