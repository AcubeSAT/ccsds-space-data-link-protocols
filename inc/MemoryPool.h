//
// Created by xrist on 23-Mar-22.
//
#include <cstdint>
#include "etl_profile.h"


#ifndef CCSDS_TM_PACKETS_MEMORYPOOL_H
#define CCSDS_TM_PACKETS_MEMORYPOOL_H


class MemoryPool {
private:
    uint8_t mem[4096];
    bool map[4096];
    bool isFull;

public:
    MemoryPool();
    int findFit(uint16_t packetLength);
    uint8_t *allocatePacket(uint8_t *packet, uint16_t packetLength);
    bool deletePacket(uint8_t* packet, uint16_t packetLength);

};


#endif //CCSDS_TM_PACKETS_MEMORYPOOL_H
