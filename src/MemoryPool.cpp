
#include "iostream"
#include "MemoryPool.h"
#include "cstring"
#include "Logger.hpp"


MemoryPool::MemoryPool() {

}

/**
 * Method that copies the packet data to the first available chunk of memory of the memory pool.
 * Calls the findFit method in order to find the index of the array that is first available.
 * @param packet pointer to the packet data.
 * @param packetLength the length of the packet data.
 * @return an uint8_t pointer to the packet data in the memory pool.
 */

uint8_t *MemoryPool::allocatePacket(uint8_t *packet, uint16_t packetLength) {
    int start = findFit(packetLength);
    if (start == -1) {
        LOG_ERROR << "There is no room in memory pool for the packet";
        return NULL;
    } else {
        std::memcpy(&memory[start], packet, packetLength * sizeof(uint8_t));
        memset(&usedMemory[start], 1, packetLength);
        return &memory[start];
    }
}

/**
 * This method is called when we want to delete the data of a packet.
 * @param packet pointer to the packet data in the pool.
 * @param packetLength length of the data.
 * @return true if the delete was successful and false if the packet was not found.
 */
bool MemoryPool::deletePacket(uint8_t *packet, uint16_t packetLength) {
    int i = packet - &memory[0];
    if (i >= 0 && i <= memorySize - 1) {
        memset(&usedMemory[i], 0, packetLength);
        return true;
    }
    LOG<Logger::error>() << "Packet not found, index is out of bounds";
    return false;
}

/**
 * This methods finds the first empty space in the memory pool that can fit the data
 * @param packetLength length of the data.
 * @return -1 if there was not enough space for the data else returns the index of the first memory
 * where the data will be stored.
 */
int MemoryPool::findFit(uint16_t packetLength) {
    int start = -1;
    unsigned int currentWindow = 0;
    for (unsigned int i = 0; i < sizeof(memory) / sizeof(uint8_t); i++) {
        if (!usedMemory[i]) {
            currentWindow++;
        } else if (usedMemory[i]) {
            currentWindow = 0;
        }
        if (currentWindow == packetLength) {
            start = i - packetLength + 1;
            return start;
        }

    }
    return start;
}

uint8_t *MemoryPool::getMemory() {
    return &memory[0];
}

bool *MemoryPool::getUsedMemory() {
    return &usedMemory[0];
}