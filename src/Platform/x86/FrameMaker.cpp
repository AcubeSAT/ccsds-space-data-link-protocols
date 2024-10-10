#include "FrameMaker.hpp"
#include "stdlib.h"

ServiceChannelNotification FrameMaker::transmitChain(uint8_t vid) {
    ServiceChannelNotification ser;

    // block and segment packets into frames. Add idle space packets if needed
    uint8_t initialPacketsNum = PacketBufferTmSize - serviceChannel->availablePacketLengthBufferTxTM(vid);
    ser = serviceChannel->vcGenerationServiceTxTM(transferFrameDataFieldLength, vid);
    numberOfPacketsSent += initialPacketsNum - (PacketBufferTmSize - serviceChannel->availablePacketLengthBufferTxTM(vid));
    if(ser != NO_SERVICE_EVENT){
        return ser;
    }

    // (secondary header) and CLCW insertion for all created frames
    do {
        ser = serviceChannel->mcGenerationRequestTxTM();
    } while (ser == NO_SERVICE_EVENT);

    // error control encoding for all frames
    while (serviceChannel->allFramesGenerationRequestTxTM(frameTarget, frameLength) == NO_SERVICE_EVENT) {
        numberOfFramesSent += 1;
        std::array<uint8_t, TmTransferFrameSize> frameArray = {0};
        for (uint16_t i = 0; i < frameLength; ++i) {
            frameArray[i] = frameTarget[i];
        }
        frameQueue.push(frameArray);
    }


    return ser;
}

void FrameMaker::popWaitingFrame(uint8_t* frameDest) {
    if (!frameQueue.empty()) {
        for (uint16_t i = 0; i < TmTransferFrameSize; ++i) {
            frameDest[i] = frameQueue.front()[i];
        }
        frameQueue.pop();
    }
}
