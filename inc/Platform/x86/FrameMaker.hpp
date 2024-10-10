#pragma once

#include "CCSDSServiceChannel.hpp"
#include "TransferFrameTM.hpp"
#include <queue>
#include <array>

/**
 * Class that creates a frame by passing data through all the CCSDS Services
 */
class FrameMaker{
private:
    // Auxiliary members used internally
    uint8_t frameTarget[TmTransferFrameSize] = {0};
    uint16_t frameLength;

    // Configuration members, initialized upon class construction
    uint16_t transferFrameDataFieldLength;
    ServiceChannel* serviceChannel;

    // Output and statistics members (accessed via methods)
    std::queue<std::array<uint8_t , TmTransferFrameSize>> frameQueue;
    uint8_t numberOfPacketsSent = 0;  // Does not count any generated idle packet
    uint8_t numberOfFramesSent = 0;

public:
    FrameMaker(ServiceChannel* serviceChannel, uint16_t transferFrameDataFieldLength) :
               serviceChannel(serviceChannel), transferFrameDataFieldLength(transferFrameDataFieldLength) {};

    void clcwFeeder(uint32_t clcw) {
        serviceChannel->pushClcwInBuffer(CLCW(clcw));
    }

    void packetFeeder(uint8_t* packet, uint16_t packetLength, uint8_t vid) {
        serviceChannel->storePacketTxTM(packet, packetLength, vid);
    }

    ServiceChannelNotification transmitChain(uint8_t vid);


    void popWaitingFrame(uint8_t* frameDest);

    uint16_t getFrameLength() {
        return frameLength;
    }

    uint8_t getNumberOfPacketsSent() const{
        return numberOfPacketsSent;
    }

    uint8_t getNumberOfFramesSent() const{
        return numberOfFramesSent;
    }

    void resetNumberOfPacketsSent() {
        numberOfPacketsSent = 0;
    }

    void resetNumberOfFramesSent() {
        numberOfFramesSent = 0;
    }

};
