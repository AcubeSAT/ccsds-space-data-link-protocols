#include "MemoryPool.hpp"
#include "cstring"
#include "Logger.hpp"
#include "Alert.hpp"

MemoryPool::MemoryPool() {}

uint8_t *MemoryPool::allocatePacket(uint8_t *packet, uint16_t packetLength) {
    std::pair<uint16_t, MasterChannelAlert> index = findFit(packetLength);
    uint16_t start = index.first;
    if (index.second == NO_SPACE) {
        LOG_ERROR << "There is no space in memory pool for the packet.";
        return nullptr;
    }
	std::memcpy(memory + start, packet, packetLength * sizeof(uint8_t));
	memset(&usedMemory[start], 1, packetLength);
	return memory + start;
}

bool MemoryPool::deletePacket(uint8_t *packet, uint16_t packetLength) {
	int32_t indexInMemory = packet - &memory[0];
    if (indexInMemory >= 0 && indexInMemory <= memorySize - 1) {
        memset(&usedMemory[indexInMemory], false, packetLength);
        return true;
    }
    LOG<Logger::error>() << "Packet not found, index is out of bounds";
    return false;
}

std::pair<uint16_t, MasterChannelAlert> MemoryPool::findFit(uint16_t packetLength) {
    std::pair<uint16_t, MasterChannelAlert> fit;
    uint16_t windowLength = 0;
    for (uint16_t memoryIndex = 0; memoryIndex < memorySize; memoryIndex++) {
        if (!usedMemory[memoryIndex]) {
			windowLength++;
        } else if (usedMemory[memoryIndex]) {
			windowLength = 0;
        }
        if (windowLength == packetLength) {
            fit.first = memoryIndex - packetLength + 1;
            fit.second = NO_MC_ALERT;
            return fit;
        }

    }
    fit.first = 0;
    fit.second = NO_SPACE;
    return fit;
}

uint8_t *MemoryPool::getMemory() {
    return &memory[0];
}

bool *MemoryPool::getUsedMemory() {
    return &usedMemory[0];
}