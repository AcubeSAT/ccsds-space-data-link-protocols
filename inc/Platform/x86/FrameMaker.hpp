#pragma once

#include "CCSDSServiceChannel.hpp"
#include "TransferFrameTM.hpp"


/**
 * Class that creates a frame by passing data through all the CCSDS Services
 */
class FrameMaker{
private:
    uint8_t frame[TmTransferFrameSize] = {0};
    uint8_t numberOfPackets = 0;
    uint16_t frameLength = 6;
    uint8_t sentFrameNumberOfPackets = 0;
    uint16_t sentFrameLegth = 0;
    ServiceChannel* serviceChannel;
public:
    FrameMaker(ServiceChannel* servChannel);
    void packetFeeder(uint8_t* packet, uint16_t packetLength,uint8_t gvid);
    std::pair<uint8_t*, ServiceChannelNotification> transmitChain(uint16_t maximumDataLength, uint8_t gcvid);
    uint16_t getFrameLength();
    uint8_t getNumberOfPackets();
};
