/**
 * @file TransferFrameTC.hpp
 * @brief Defines a specialized transfer frame for carrying telecommand packets.
 */

#pragma once

#include "etl/optional.h"
#include "CCSDS_Definitions.hpp"
#include "TransferFrame.hpp"

namespace CCSDSDataLinkLayer {
   /**
    *  @brief The frame's service type.
    *  @see p. 2.2.2 from TC SPACE DATA LINK PROTOCOL
    */
    enum class ServiceType : uint8_t {
        TYPE_AD = 0x0,
        TYPE_RESERVED = 0x1,
        TYPE_BD = 0x2,
        TYPE_BC = 0x3,
    };

    /**
     * @brief Indicates whether a frame carries a segmented packet.
     */
    enum class SequenceFlags : uint8_t {
        SegmentationMiddle = 0x0,
        SegmentationStart = 0x1,
        SegmentationEnd = 0x2,
        NoSegmentation = 0x3
    };

    class TransferFrameTC : public TransferFrame {
    public:
        /**
         * @brief Constructor for frame creation within the data link.
         */
        TransferFrameTC(uint8_t *frameData, ServiceType serviceType, uint8_t vid, uint16_t frameLength,
                        bool segHdrPresent,
                        SequenceFlags sequenceFlag = SequenceFlags::NoSegmentation, uint8_t mapId = 0,
                        uint16_t firstEmptyOctet = 0)
                : TransferFrame(FrameType::TC, frameLength, frameData, firstEmptyOctet),
                  toBeRetransmitted(false), segmentationHeaderPresent(segHdrPresent) {
            uint8_t bypassFlag = ((serviceType == ServiceType::TYPE_AD) || (serviceType == ServiceType::TYPE_RESERVED))
                                 ? 0 : 1;
            uint8_t ctrlCmdFlag = ((serviceType == ServiceType::TYPE_BC) || (serviceType == ServiceType::TYPE_RESERVED))
                                  ? 1 : 0;
            frameData[0] = ((TransferFrameVersionNumber & 0x3) << 6U) | (bypassFlag << 5U) | (ctrlCmdFlag << 4U) | 0 |
                           static_cast<uint8_t>((SpacecraftIdentifier & 0x300) >> 8U);
            frameData[1] = static_cast<uint8_t>(SpacecraftIdentifier & 0xFF);
            frameData[2] = ((vid & 0x3F) << 2U) | static_cast<uint8_t>((frameLength & 0x300) >> 8U);
            frameData[3] = static_cast<uint8_t>(frameLength & 0xFF);

            if (segHdrPresent) {
                frameData[5] = (static_cast<uint8_t>(sequenceFlag) << 6U) | (mapId & 0x3F);
            }
        }

        /**
         * @brief Constructor for frame creation from received octets.
         */
        TransferFrameTC(uint8_t *frameData, uint16_t frameLength, uint16_t firstEmptyOctet = 0)
                : TransferFrame(FrameType::TC, frameLength, frameData, firstEmptyOctet) {};

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

        void setTransferFrameSequenceNumber(uint8_t frame_seq_number) {
            transferFrameData[4] = frame_seq_number;
        }

        /**
         * @brief Determined by the sequence and control&command flags.
         * @see p. 2.2.2 from TC SPACE DATA LINK PROTOCOL
         */
        [[nodiscard]] ServiceType getServiceType() const {
            bool bypass = getBypassFlag();
            bool ctrl = getCtrlAndCmdFlag();

            if (bypass && ctrl) {
                return ServiceType::TYPE_BC;
            } else if (bypass && !ctrl) {
                return ServiceType::TYPE_BD;
            } else if (!bypass && !ctrl) {
                return ServiceType::TYPE_AD;
            }
            // Reserved type not normally used as per the standard
            return ServiceType::TYPE_RESERVED;
        }

        void setFrameLength(uint16_t frameLength) {
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
            } else {
                return etl::nullopt;
            }
        }

        [[nodiscard]] bool getSegmentationHeaderPresentFlag() const {
            return segmentationHeaderPresent;
        }

        void setSegmentationHeaderPresentFlag(bool segHdrPresent) {
            segmentationHeaderPresent = segHdrPresent;
        }

        [[nodiscard]] etl::optional<uint8_t> getMapId() const {
            if (segmentationHeaderPresent) {
                return transferFrameData[5] & 0x3F;
            }
            return etl::nullopt;
        }

        [[nodiscard]] etl::optional<SequenceFlags> getSequenceFlag() const {
            if (segmentationHeaderPresent) {
                return static_cast<SequenceFlags>((transferFrameData[5] >> 6U) & 0x03);
            }
            return etl::nullopt;
        }

        [[nodiscard]] etl::optional<bool> getToBeRetransmittedFlag() const {
            return toBeRetransmitted;
        }

        void setToBeRetransmittedFlag(bool retransmitFlag) {
            toBeRetransmitted = retransmitFlag;
        }

    private:
        /**
         * @brief Used by FOP-1 (sending side) to determine if a Type-AD transfer frame should be retransmitted.
         */
        etl::optional<bool> toBeRetransmitted;
        bool segmentationHeaderPresent;
    };
} // namespace CCSDSDataLinkLayer