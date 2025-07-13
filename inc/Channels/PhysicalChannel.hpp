/**
 * @file PhysicalChannel.hpp
 */

#pragma once
#include <cstdint>
#include "DefinitionsAndUtilities.hpp"
#include "etl/span.h"

namespace CCSDSDataLinkLayer {
    /**
     * @see Table 5-1 from TC SPACE DATA LINK PROTOCOL
     */
    class PhysicalChannel {
    public:
        PhysicalChannel(const uint8_t pcid, const DefsAndUtils::TransferFrameVersionNumber tfvn,
                        const uint16_t maxTcFrameLength, const uint16_t tmFrameLength,
                        const uint16_t maxFramesPdu, const uint16_t maxPduLength,
                        const uint32_t bitrate, const bool frameErrorControlFieldPresent)
            : tfvn(tfvn), pcid(pcid), maxTcFrameLength(maxTcFrameLength), tmFrameLength(tmFrameLength),
              maxFramePdu(maxFramesPdu), maxPDULength(maxPduLength), bitrate(bitrate),
              frameErrorControlFieldPresent(frameErrorControlFieldPresent) {}

        /**
         * @brief Defines the type of protocol used
         * @see SANA transfer frame version numbers registry
         */
        [[nodiscard]] DefsAndUtils::TransferFrameVersionNumber getTFVN() const {
            return tfvn;
        }

        [[nodiscard]] uint8_t getPcid() const {
            return pcid;
        }

        /**
         * @brief Get the maximum allowed TC frame length in this physical channel.
         */
        [[nodiscard]] uint16_t getMaxTcFrameLength() const {
            return maxTcFrameLength;
        }

        /**
         * @brief Get length of TM frames in this virtual channel
         */
        [[nodiscard]] uint16_t getTMFrameLength() const {
            return tmFrameLength;
        }

        /**
         * @brief Sets the maximum number of transfer frames that can be transferred in a single data unit.
         */
        [[nodiscard]] uint16_t getMaxFramePdu() const {
            return maxFramePdu;
        }

        /**
         * @brief Maximum length of a data unit.
         */
        [[nodiscard]] uint16_t getMaxPDULength() const {
            return maxPDULength;
        }

        /**
         * @brief Maximum bit rate (bits per second).
         */
        [[nodiscard]] uint32_t getBitrate() const {
            return bitrate;
        }

        /**
         * @brief Defines whether the ECF service is present in transfer frames.
         */
        [[nodiscard]] bool getFrameErrorControlFieldPresent() const {
            return frameErrorControlFieldPresent;
        }

    private:
        const DefsAndUtils::TransferFrameVersionNumber tfvn;
        const uint8_t pcid; // Physical channel id. Not part of the standard, added for consistency with the other channels
        const uint16_t maxTcFrameLength;
        const uint16_t tmFrameLength;
        const uint16_t maxFramePdu;
        const uint16_t maxPDULength;
        const uint32_t bitrate;
        const bool frameErrorControlFieldPresent;
    };
} // namespace CCSDSDataLinkLayer