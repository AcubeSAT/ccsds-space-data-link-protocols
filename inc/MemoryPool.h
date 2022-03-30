
#include <cstdint>
#include "etl/vector.h"


#ifndef CCSDS_TM_PACKETS_MEMORYPOOL_H
#define CCSDS_TM_PACKETS_MEMORYPOOL_H


class MemoryPool {
private:
    static constexpr unsigned int memorySize = 8;   //a random power of 2
    uint8_t memory[memorySize];                        //array that allocates statically memory to be used for the packet data
    bool usedMemory[memorySize] = {0};                 //boolean array that show if each memory slot is used. False->empty and true-> used


public:
    MemoryPool();
    int findFit(uint16_t packetLength);
    uint8_t *allocatePacket(uint8_t *packet, uint16_t packetLength);
    bool deletePacket(uint8_t* packet, uint16_t packetLength);

};

#endif //CCSDS_TM_PACKETS_MEMORYPOOL_H
