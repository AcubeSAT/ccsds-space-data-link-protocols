
#include <cstdint>
#include "etl/vector.h"
#include "Alert.hpp"


#ifndef CCSDS_TM_PACKETS_MEMORYPOOL_H
#define CCSDS_TM_PACKETS_MEMORYPOOL_H


/**
 * @class MemoryPool This class defines a custom memory pool which is a statically allocated block of memory,
 * where the TM and TC packet data will be stored.
 * This helps better keep track of the packet data and reduce the memory needed for storing them.
 */
class MemoryPool {
private:
    /**
     * @var The size of the block of memory, a random power of 2
     */
    static constexpr uint8_t memorySize = 8;

    /**
     * @var An array that allocates statically memory to be used for the packet data
     */
    uint8_t memory[memorySize];

    /**
     * @var boolean array that shows if each memory slot is used. False -> empty and true-> used
     */
    bool usedMemory[memorySize] = {0};


public:
    MemoryPool();

    /**
    * This methods finds the first empty space in the memory pool that can fit the data
    * @param packetLength length of the data.
    * @return -1 if there was not enough space for the data else returns the index of the first memory
    * where the data will be stored.
    */
    std::pair<uint16_t, MasterChannelAlert> findFit(uint16_t packetLength);

    /**
    * Method that copies the packet data to the first available chunk of memory of the memory pool.
    * Calls the findFit method in order to find the index of the array that is first available.
    * @param packet pointer to the packet data.
    * @param packetLength the length of the packet data.
    * @return an uint8_t pointer to the packet data in the memory pool.
    */
    uint8_t *allocatePacket(uint8_t *packet, uint16_t packetLength);

    /**
    * This method is called when we want to delete the data of a packet.
    * @param packet pointer to the packet data in the pool.
    * @param packetLength length of the data.
    * @return true if the delete was successful and false if the packet was not found.
    */
    bool deletePacket(uint8_t *packet, uint16_t packetLength);

    uint8_t *getMemory();

    bool *getUsedMemory();

};

#endif //CCSDS_TM_PACKETS_MEMORYPOOL_H
