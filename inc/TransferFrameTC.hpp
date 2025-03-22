/**
 * @file TransferFrameTC.hpp
 * @brief Defines a specialized transfer frame for carrying telecommand packets.
 */

#pragma once

#include "etl/optional.h"
#include "CCSDSDefinitionsAndUtilities.hpp"
#include "TransferFrame.hpp"

namespace CCSDSDataLinkLayer {
    class TransferFrameTC : public TransferFrame {
    public:
        /**
         * @brief Constructor for frame creation within the data link.
         */
        TransferFrameTC(uint8_t *frameData, const DefsAndUtils::ServiceType serviceType, const uint8_t vid,
                        const uint16_t scid, const uint16_t frameLength,
                        const bool segHdrPresent,
                        DefsAndUtils::SequenceFlag sequenceFlag =
                                DefsAndUtils::SequenceFlag::NoSegmentation, const uint8_t mapId = 0,
                        const uint16_t firstEmptyOctet = 0)
            : TransferFrame(DefsAndUtils::FrameType::TC, frameLength, frameData, firstEmptyOctet),
              toBeRetransmitted(false), segmentationHeaderPresent(segHdrPresent),
              processingStage(DefsAndUtils::TcFrameProcessingStage::PROCESSED_BY_VC_GENERATION_TX) {
            const uint8_t bypassFlag = ((serviceType == DefsAndUtils::ServiceType::TYPE_AD) ||
                                        (serviceType == DefsAndUtils::ServiceType::TYPE_RESERVED))
                                           ? 0
                                           : 1;
            const uint8_t ctrlCmdFlag = ((serviceType == DefsAndUtils::ServiceType::TYPE_BC) ||
                                         (serviceType == DefsAndUtils::ServiceType::TYPE_RESERVED))
                                            ? 1
                                            : 0;
            frameData[0] = (static_cast<uint8_t>(
                                DefsAndUtils::TransferFrameVersionNumber::TM_TC_SYNCHRONOUS_TRANSFER_FRAME_V1)
                            << 6U) |
                           (bypassFlag << 5U) | (ctrlCmdFlag << 4U) | 0 |
                           static_cast<uint8_t>((scid & 0x300) >> 8U);
            frameData[1] = static_cast<uint8_t>(scid & 0xFF);
            frameData[2] = ((vid & 0x3F) << 2U) | static_cast<uint8_t>((frameLength & 0x300) >> 8U);
            frameData[3] = static_cast<uint8_t>(frameLength & 0xFF);

            if (segHdrPresent) {
                frameData[5] = (static_cast<uint8_t>(sequenceFlag) << 6U) | (mapId & 0x3F);
            }
        }

        /**
         * @brief Constructor for frame creation from received octets.
         */
        TransferFrameTC(uint8_t *frameData, const uint16_t frameLength, const uint16_t firstEmptyOctet = 0)
            : TransferFrame(DefsAndUtils::FrameType::TC, frameLength, frameData, firstEmptyOctet),
              processingStage(DefsAndUtils::TcFrameProcessingStage::PROCESSED_BY_ALL_FRAMES_RECEPTION) {
        };

        /**
         * @brief Compares two frames.
         */
        friend bool operator==(const TransferFrameTC &frame1, const TransferFrameTC &frame2) {
            if (frame1.transferFrameLength != frame2.transferFrameLength) {
                return false;
            }
            for (uint16_t i = 0; i < frame1.transferFrameLength; i++) {
                if (frame1.getFrameData()[i] != frame2.getFrameData()[i]) {
                    return false;
                }
            }
            return true;
        }

        /** === PRIMARY HEADER === **/

        /**
         *  @brief Transfer frame version number.
         * @details Bits 0-1 of the Transfer Frame Primary Header
         * @see p. 4.1.2.2 from TC SPACE DATA LINK PROTOCOL
         */
        [[nodiscard]] uint8_t getTransferFrameVersionNumber() const {
            return (transferFrameData[0] & 0xC0) >> 6U;
        }

        /**
         * @brief The bypass Flag determines whether the transferFrameData will bypass FARM checks.
         * @details Bit 2 of the Transfer Frame Primary Header
         * @see p. 4.1.2.3.1 from TC SPACE DATA LINK PROTOCOL
         */
        [[nodiscard]] bool getBypassFlag() const {
            return (transferFrameData[0] >> 5U) & 0x01;
        }

        /**
         * @brief The control and command Flag determines whether the transferFrameData carries control commands (Type-C) or
         * data (Type-D).
         * @details Bit 3 of the Transfer Frame Primary Header
         * @see p. 4.1.2.3.2 from TC SPACE DATA LINK PROTOCOL
         */
        [[nodiscard]] bool getCtrlAndCmdFlag() const {
            return (transferFrameData[0] >> 4U) & 0x01;
        }

        /**
         * @brief The ID of the spacecraft.
         * @details Bits  6–15 of  the  Transfer  Frame  Primary  Header
         * @see p. 4.1.2.5 from TC SPACE DATA LINK PROTOCOL
         */
        [[nodiscard]] uint16_t getSpacecraftId() const {
            return (static_cast<uint16_t>(transferFrameData[0] & 0x03) << 8U) |
                   (static_cast<uint16_t>(transferFrameData[1]));
        }

        /**
         * @brief The virtual channel ID this frame is transferred in.
         * @details Bits 16–21 of the Transfer Frame Primary Header
         * @see p. 4.1.2.6 from TC SPACE DATA LINK PROTOCOL
         */
        [[nodiscard]] uint8_t getVirtualChannelId() const {
            return (transferFrameData[2] >> 2U) & 0x3F;
        }

        /**
         * @brief The length of the transfer frame.
         * @details Bits 22–31 of the Transfer Frame Primary Header
         * @see p. 4.1.2.7 from TC SPACE DATA LINK PROTOCOL
         */
        [[nodiscard]] uint16_t getTransferFrameLength() const {
            return (static_cast<uint16_t>(transferFrameData[2] & 0x03) << 8U) |
                   (static_cast<uint16_t>(transferFrameData[3]));
        }

        /**
         * @brief The sequence number of the frame. It is used by COP-1 to determine if a frame did not
         * arrive to the sending end.
         * @details Bits 32-39 of the Transfer Frame Primary Header
         * @see p. 4.1.2.8 from TC SPACE DATA LINK PROTOCOL
         */
        [[nodiscard]] uint8_t getTransferFrameSequenceNumber() const {
            return transferFrameData[4];
        }

        void setTransferFrameSequenceNumber(const uint8_t frame_seq_number) const {
            transferFrameData[4] = frame_seq_number;
        }

        /**
         * @brief Determined by the sequence and control&command flags.
         * @see p. 2.2.2 from TC SPACE DATA LINK PROTOCOL
         */
        [[nodiscard]] DefsAndUtils::ServiceType getServiceType() const {
            const bool bypass = getBypassFlag();
            const bool ctrl = getCtrlAndCmdFlag();

            if (bypass && ctrl) {
                return DefsAndUtils::ServiceType::TYPE_BC;
            }
            if (bypass && !ctrl) {
                return DefsAndUtils::ServiceType::TYPE_BD;
            }
            if (!bypass && !ctrl) {
                return DefsAndUtils::ServiceType::TYPE_AD;
            }
            // Reserved type not normally used as per the standard
            return DefsAndUtils::ServiceType::TYPE_RESERVED;
        }

        void setFrameLength(const uint16_t frameLength) {
            transferFrameLength = frameLength;
            transferFrameData[2] =
                    ((getVirtualChannelId() & 0x3F) << 2) | static_cast<uint8_t>((transferFrameLength & 0x300) >> 8);
            transferFrameData[3] = static_cast<uint8_t>(transferFrameLength & 0xFF);
        }

        /** === SEGMENTATION HEADER === **/

        /** @brief The segmentation header stores.
         * @see p. 4.1.3.2.2 from TC SPACE DATA LINK PROTOCOL
         */
        [[nodiscard]] etl::optional<uint8_t> getSegmentationHeader() const {
            if (segmentationHeaderPresent) {
                return transferFrameData[5];
            }
            return etl::nullopt;
        }

        [[nodiscard]] bool getSegmentationHeaderPresentFlag() const {
            return segmentationHeaderPresent;
        }

        void setSegmentationHeaderPresentFlag(const bool segHdrPresent) {
            segmentationHeaderPresent = segHdrPresent;
        }

        [[nodiscard]] etl::optional<uint8_t> getMapId() const {
            if (segmentationHeaderPresent) {
                return transferFrameData[5] & 0x3F;
            }
            return etl::nullopt;
        }

        [[nodiscard]] etl::optional<DefsAndUtils::SequenceFlag> getSequenceFlag() const {
            if (segmentationHeaderPresent) {
                return static_cast<DefsAndUtils::SequenceFlag>((transferFrameData[5] >> 6U) & 0x03);
            }
            return etl::nullopt;
        }

        [[nodiscard]] etl::optional<bool> getToBeRetransmittedFlag() const {
            return toBeRetransmitted;
        }

        void setToBeRetransmittedFlag(const bool retransmitFlag) {
            toBeRetransmitted = retransmitFlag;
        }

        [[nodiscard]] DefsAndUtils::TcFrameProcessingStage getProcessingStage() const {
            return processingStage;
        }

        void updateProcessingStage(const DefsAndUtils::TcFrameProcessingStage newProcessingStage) {
            processingStage = newProcessingStage;
        }

    private:
        /**
         * @brief Used by FOP-1 (sending side) to determine if a Type-AD transfer frame should be retransmitted.
         */
        etl::optional<bool> toBeRetransmitted;
        bool segmentationHeaderPresent;
        DefsAndUtils::TcFrameProcessingStage processingStage;
    };
} // namespace CCSDSDataLinkLayer
