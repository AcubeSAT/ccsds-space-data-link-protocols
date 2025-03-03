#include <stdlib.h>
#include "FrameMaker.hpp"

namespace CCSDSDataLinkLayer {
    etl::expected<void, ServiceChannelNotification> FrameMaker::transmitChain(uint8_t vid) {
        etl::expected<void, ServiceChannelNotification> ser;
        auto packetLenBuffer =
                serviceChannel->getMasterChannel().getVirtualChannels().at(vid).getPacketLengthBufferTxTM();

        // block and segment packets into frames. Add idle space packets if needed
        uint8_t initialPacketsNum = PacketBufferTmSize - packetLenBuffer.size();
        ser = serviceChannel->vcGenerationServiceTxTM(transferFrameDataFieldLength, vid);
        numberOfPacketsSent +=
                initialPacketsNum - (PacketBufferTmSize - packetLenBuffer.size());

        // (secondary header) and CLCW insertion for all created frames
        do {
            ser = serviceChannel->mcGenerationRequestTxTM();
        } while (ser.has_value());

        // error control encoding for all frames
        while (serviceChannel->allFramesGenerationRequestTxTM(frameTarget).has_value()) {
            numberOfFramesSent += 1;
            std::array<uint8_t, TmTransferFrameSize> frameArray = {0};
            for (uint16_t i = 0; i < frameLength; ++i) {
                frameArray[i] = frameTarget[i];
            }
            frameQueue.push(frameArray);
        }

        return ser;
    }

    void FrameMaker::popWaitingFrame(uint8_t *frameDest) {
        if (!frameQueue.empty()) {
            for (uint16_t i = 0; i < TmTransferFrameSize; ++i) {
                frameDest[i] = frameQueue.front()[i];
            }
            frameQueue.pop();
        }
    }
} // namespace CCSDSDataLinkLayer