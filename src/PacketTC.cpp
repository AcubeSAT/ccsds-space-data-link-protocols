#include <PacketTC.hpp>

PacketTC::PacketTC(uint8_t *packet, uint16_t packetLength, PacketType t): Packet(t, packetLength,packet), hdr(packet){
    // Segment header may be unavailable in virtual channels, here we treat it as if it's there sinc it's existence is
    // dependent on the channel. We deal with it's existence afterwards
    segHdr = packet[5];
    gvcid = packet[2] >> 2;

    // MAP IDs are relevant in case the transfer frame primary header is present. If it's not, this will be determined
    // upon processing the packet in the relevant channel since since this value will be essentially junk
    mapid = mapId() & 0x63;

    // todo: This is supposed to be a user-set number that will help with identification of a packet upon generating an
    // alert. However, I'm still unsure on the specifics so let's just leave this blank for now
    sduid = 0;
    serviceType = ServiceType::TYPE_A;
    transferFrameSeqNumber = packet[4];
    ack = 0;
    toBeRetransmitted = 0; // N/A
}