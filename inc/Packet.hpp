#ifndef CCSDS_PACKET_H
#define CCSDS_PACKET_H

#include <CCSDS_Definitions.hpp>

#include <algorithm>
#include <cstring>

struct Packet{
    uint8_t* packet;
    uint16_t packetLength;
    uint8_t segHdr;
    uint8_t gvcid;
    uint8_t mapid;
    uint16_t sduid;
    bool serviceType;

    Packet(uint8_t* packet, uint16_t packet_length, uint8_t seg_hdr, uint8_t gvcid, uint8_t mapid, uint16_t sduid,
            bool service_type):
    packet(packet), packetLength(packet_length), segHdr(seg_hdr), gvcid(gvcid), mapid(mapid), sduid(sduid),
    serviceType(service_type){}
};


#endif //CCSDS_PACKET_H
