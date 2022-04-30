
#include "iostream"
#include "MemoryPool.hpp"
#include "cstring"
#include "Logger.hpp"
#include "Alert.hpp"


MemoryPool::MemoryPool() {

}


uint8_t *MemoryPool::allocatePacket(uint8_t *packet, uint16_t packetLength) {
    std::pair<uint16_t, MasterChannelAlert> index = findFit(packetLength);
    uint16_t start = index.first;
    if (index.second == NO_SPACE) {
        LOG_ERROR << "There is no room in memory pool for the packet";
        return NULL;
    } else {
        std::memcpy(&memory[start], packet, packetLength * sizeof(uint8_t));
        memset(&usedMemory[start], 1, packetLength);
        return &memory[start];
    }
}


bool MemoryPool::deletePacket(uint8_t *packet, uint16_t packetLength) {
    int i = packet - &memory[0];
    if (i >= 0 && i <= memorySize - 1) {
        memset(&usedMemory[i], 0, packetLength);
        return true;
    }
    LOG<Logger::error>() << "Packet not found, index is out of bounds";
    return false;
}


std::pair<uint16_t, MasterChannelAlert> MemoryPool::findFit(uint16_t packetLength) {
    std::pair<uint16_t, MasterChannelAlert> index;
    uint16_t currentWindow = 0;
    for (unsigned int i = 0; i < sizeof(memory) / sizeof(uint8_t); i++) {
        if (!usedMemory[i]) {
            currentWindow++;
        } else if (usedMemory[i]) {
            currentWindow = 0;
        }
        if (currentWindow == packetLength) {
            index.first = i - packetLength + 1;
            index.second = NO_MC_ALERT;
            return index;
        }

    }
    index.first = 0;
    index.second = NO_SPACE;
    return index;
}

uint8_t *MemoryPool::getMemory() {
    return &memory[0];
}

bool *MemoryPool::getUsedMemory() {
    return &usedMemory[0];
}