#include <CCSDSChannel.hpp>

void MAPChannel::store(Packet packet){
    if (packetList.full()){
        // Log that buffer is full
        return;
    }
    packetList.push(packet);
}

void VirtualChannel::store(Packet packet){
    // Limit the amount of packets that can be stored at any given time
    if (packetList.full()){
        // Log that buffer is full
        return;
    }
    packetList.push(packet);
}

// Technically not a packet, but it has identical information
void MasterChannel::store(Packet packet){
    if (framesList.full()){
        // Log that buffer is full
        return;
    }
    framesList.push(packet);
}