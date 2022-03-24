//
// Created by xrist on 23-Mar-22.
//
#include "iostream"
#include "MemoryPool.h"

MemoryPool::MemoryPool(){
    for(unsigned int i=0 ; i<sizeof(mem)/sizeof (bool) ; i++){
        map[i] = false;
    }
}


uint8_t *MemoryPool::allocatePacket(uint8_t *packet, uint16_t packetLength) {
    int start = findFit(packetLength);
    if(start == -1) {
        std::cout<<"Not enough space";
        return NULL;
    }
    else{
        for(unsigned int i =0 ; i< packetLength ; i++){
            mem[start+i] = packet[i];
            map[start+i] = true;
        }
        return &mem[start];
    }
}

bool MemoryPool::deletePacket(uint8_t *packet, uint16_t packetLength) {
    for(unsigned int i =0 ; i< sizeof(mem)/sizeof(uint8_t); i++){
        if(packet==&mem[i]){
            for(unsigned int j = i ; j<packetLength ; j++){
                map[j] = false;
            }
            return true;
        }

    }
    return false;
}


int MemoryPool::findFit(uint16_t packetLength) {
    bool fit;
    int start = -1;
    for(unsigned int i =0 ; i<sizeof(mem)/sizeof(bool) - packetLength ; i++){
        fit = true;
        for(unsigned int j = i ; j<packetLength; j++){
            if (map[j]){
                fit = false;
                break;
            }
        }
        if (fit){
           start = i;
           return start;
        }
    }
    return start;
}