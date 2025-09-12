/**
 * @file TransferFrameTM.hpp
 * @brief Defines a specialized transfer frame for carrying telemetry packets.
 */

#pragma once

#include "etl/optional.h"
#include "TransferFrame.hpp"
#include "CcsdsDefinitions.hpp"

namespace CCSDSDataLinkLayer {
    class TransferFrameTM : public TransferFrame {
    public:
        TransferFrameTM() = default;

        /**
         * @brief Constructor for frame creation within the data link (operational control field remains uninitialized).
         */
        TransferFrameTM(uint8_t *frameData, const uint16_t frameLength, const Defs::Vcid vcid, const Defs::Scid scid,
                        const bool operationalControlFieldPresent,
                        const uint8_t virtualChannelFrameCount, const bool transferFrameSecondaryHeaderPresent,
                        const uint8_t transferFrameSecondaryHeaderLength,
                        const Defs::SynchronizationFlag syncFlag, const bool packetOrder,
                        const uint8_t segmentLengthIdentifier,
                        const uint16_t firstHeaderPointer, const bool eccFieldPresent,
                        const uint16_t firstEmptyOctet = 0)
            : TransferFrame(Defs::FrameType::TM, frameLength, frameData, firstEmptyOctet),
              eccFieldPresent(eccFieldPresent) {
            // Transfer Frame Version Number + Spacecraft Id
            frameData[0] = (static_cast<uint8_t>(
                                Defs::TransferFrameVersionNumber::TM_TC_SYNCHRONOUS_TRANSFER_FRAME_V1)
                            << 6U) |
                           static_cast<uint8_t>((scid & 0x3F0) >> 4U);
            // Spacecraft  Id + Virtual Channel ID + Operational Control Field
            frameData[1] = static_cast<uint8_t>((scid & 0x0F) << 4U) | ((vcid & 0x7) << 1U) |
                           static_cast<uint8_t>(operationalControlFieldPresent);
            // Master Channel Frame Count is set by the MC Generation Service
            frameData[2] = 0;
            frameData[3] = virtualChannelFrameCount;
            // Data field status
            frameData[4] = (transferFrameSecondaryHeaderPresent << 7U) | (static_cast<uint8_t>(syncFlag) << 6U) |
                           ((packetOrder & 0x1) << 5U) |
                           ((segmentLengthIdentifier & 0x3) << 3U) |
                           static_cast<uint8_t>((firstHeaderPointer & 0x700) >> 8U);
            frameData[5] = static_cast<uint8_t>(firstHeaderPointer & 0xFF);

            if (transferFrameSecondaryHeaderPresent) {
                // Note: The secondary header length must store the actual length, reduced my one
                frameData[6] = (static_cast<uint8_t>(Defs::SecondaryHeaderVersionNumber::VERSION_1) << 6U) |
                    (transferFrameSecondaryHeaderLength - Defs::TmSecondaryHeaderIdLength);
            }
        }

        /**
         * @brief Constructor for frame creation within the data link (operational control field is initialized).
         */
        TransferFrameTM(uint8_t *frameData, const uint16_t frameLength, const Defs::Vcid vcid, const Defs::Scid scid,
                        const uint32_t operationalControlField,
                        const uint8_t virtualChannelFrameCount, const bool transferFrameSecondaryHeaderPresent,
                        const uint8_t transferFrameSecondaryHeaderLength,
                        Defs::SynchronizationFlag syncFlag, const bool packetOrder,
                        const uint8_t segmentationLengthId,
                        const uint16_t firstHeaderPointer, const bool eccFieldExists,
                        const uint16_t firstEmptyOctet = 0)
            : TransferFrame(Defs::FrameType::TM, frameLength, frameData, firstEmptyOctet),
              eccFieldPresent(eccFieldExists) {
            // Transfer Frame Version Number + Spacecraft Id
            frameData[0] = (static_cast<uint8_t>(
                                Defs::TransferFrameVersionNumber::TM_TC_SYNCHRONOUS_TRANSFER_FRAME_V1)
                            << 6U) |
                           static_cast<uint8_t>((scid & 0x3F0) >> 4U);
            // Spacecraft  Id + Virtual Channel ID + Operational Control Field
            frameData[1] = static_cast<uint8_t>((scid & 0x0F) << 4U) | ((vcid & 0x7) << 1U) | 0x1;
            // Master Channel Frame Count is set by the MC Generation Service
            frameData[2] = 0;
            frameData[3] = virtualChannelFrameCount;
            // Data field status
            frameData[4] = (transferFrameSecondaryHeaderPresent << 7U) | (static_cast<uint8_t>(syncFlag) << 6U) |
                           ((packetOrder & 0x1) << 5U) |
                           ((segmentationLengthId & 0x3) << 3U) |
                           static_cast<uint8_t>((firstHeaderPointer & 0x700) >> 8U);
            frameData[5] = static_cast<uint8_t>(firstHeaderPointer & 0xFF);

            if (transferFrameSecondaryHeaderPresent) {
                // Note: The secondary header length must store the actual length, reduced my one
                frameData[6] = (static_cast<uint8_t>(Defs::SecondaryHeaderVersionNumber::VERSION_1) << 6U) |
                    (transferFrameSecondaryHeaderLength - Defs::TmSecondaryHeaderIdLength);
            }

            uint8_t *ocfPointer = frameData + transferFrameLength -
                                  Defs::TmOperationalControlFieldSize
                                  - Defs::ErrorControlFieldSize * eccFieldExists;
            ocfPointer[0] = static_cast<uint8_t>(operationalControlField >> 24U);
            ocfPointer[1] = static_cast<uint8_t>((operationalControlField >> 16U) & 0xFF);
            ocfPointer[2] = static_cast<uint8_t>((operationalControlField >> 8U) & 0xFF);
            ocfPointer[3] = static_cast<uint8_t>(operationalControlField & 0xFF);
        }

        /**
         * @brief Constructor for frame creation from received octets.
         */
        TransferFrameTM(uint8_t *frameData, const uint16_t frameLength, const bool eccFieldExists,
                        const uint16_t firstEmptyOctet = 0)
            : TransferFrame(Defs::FrameType::TM, frameLength, frameData, firstEmptyOctet),
              eccFieldPresent(eccFieldExists) {}

        /**
         * @brief Transfer frame version number.
         * @details Bits 0-1 of the Transfer Frame Primary Header
         * @see p. 4.1.2.2.2 from TM SPACE DATA LINK PROTOCOL
         */
        [[nodiscard]] uint8_t getTransferFrameVersionNumber() const {
            return (transferFrameData[0] & 0xC0) >> 6U;
        }

        /**
         * @brief The ID of the spacecraft.
         * @details Bits  2–11  of  the  Transfer  Frame  Primary  Header
         * @see p. 4.1.2.2.3 from TM SPACE DATA LINK PROTOCOL
         */
        [[nodiscard]] uint16_t getSpacecraftId() const {
            return (static_cast<uint16_t>(transferFrameData[0] & 0x3F) << 4U) |
                   (static_cast<uint16_t>(transferFrameData[1] & 0xF0) >> 4U);
        }

        /**
         * @brief The virtual channel ID this frame is transferred in
         * @details Bits 12–14 of the Transfer Frame Primary Header
         * @see p. 4.1.2.3 from TM SPACE DATA LINK PROTOCOL
        */
        [[nodiscard]] uint8_t getVirtualChannelId() const {
            return ((transferFrameData[1] & 0x0E)) >> 1U;
        }

        /**
         * @brief The Operational Control Field Flag indicates the presence or absence of the Operational Control Field
         * @details Bit  15  of  the  Transfer  Frame  Primary  Header
         * @see p. 4.1.2.4 from TM SPACE DATA LINK PROTOCOL
         */
        [[nodiscard]] bool getOperationalControlFieldFlag() const {
            return (transferFrameData[1]) & 0x01;
        }

        /**
         * @brief Provides  a  running  count  of  the  Transfer  Frames  which  have  been  transmitted  through  the
         * same  Master  Channel.
         * @details Bits  16–23  of  the  Transfer  Frame  Primary  Header
         * @see p. 4.1.2.5 from TM SPACE DATA LINK PROTOCOL
         */
        [[nodiscard]] uint8_t getMasterChannelFrameCount() const {
            return transferFrameData[2];
        }

        void setMasterChannelFrameCount(const uint8_t masterChannelFrameCount) const {
            transferFrameData[2] = masterChannelFrameCount;
        }

        /**
         * @brief contain  a  sequential  binary  count (modulo-256) of each Transfer Frame transmitted within a
         * specific Virtual Channel.
         * @details Bits  24–31  of  the  Transfer  Frame  Primary  Header
         * @see p. 4.1.2.6 from TM SPACE DATA LINK PROTOCOL
         */
        [[nodiscard]] uint8_t getVirtualChannelFrameCount() const {
            return transferFrameData[3];
        }

        /**
         * @brief Indicates the presence of the secondary header.
         * @details Bit  32  of  the Transfer  Frame  Primary  Header
         * @see p. 4.1.2.7.2 from TM SPACE DATA LINK PROTOCOL
         */
        [[nodiscard]] bool getTransferFrameSecondaryHeaderFlag() const {
            return (transferFrameData[4] & 0x80) >> 7U;
        }

        /**
         * @brief Signals the type of data which are inserted into the Transfer Frame Data Field (VCA_SDU or Packets).
         * @details Bit 33 of the Transfer Frame Primary Header
         * @see p. 4.1.2.7.3 from TM SPACE DATA LINK PROTOCOL
         */
        [[nodiscard]] Defs::SynchronizationFlag getSynchronizationFlag() const {
            return static_cast<Defs::SynchronizationFlag>((transferFrameData[4] & 0x40) >> 6U);
        }

        /**
         * @brief Reserved value for future protocol versions.
         * @details If the Synchronization Flag is set to ‘0’,t he TransferFrame Order Flag is reserved for
         * future use by the CCSDS and shall be set to ‘0’. If the Synchronization Flag is
         * set to ‘1’, the use of the TransferFrame Order Flag is undefined.
         * Bit 34 of the Transfer Frame Primary Header
         * @see p. 4.1.2.7.4 from TM SPACE DATA LINK PROTOCOL
         */
        [[nodiscard]] bool getPacketOrderFlag() const {
            return (transferFrameData[4] & 0x20) >> 5U;
        }

        /**
         * @brief Segment Length Id indicates the order of the segmented packets
         * @details Bits 35 and 36 of the Transfer Frame Primary Header.
         * @see p. 4.1.2.7.5 from TM SPACE DATA LINK PROTOCOL
         */
        [[nodiscard]] uint8_t getSegmentLengthId() const {
            return (transferFrameData[4] >> 3U) & 0x3;
        }

        /**
         * @brief If the Synchronization Flag is set to ‘0’, the First Header Pointer shall contain
         *		the position of the first octet of the first TransferFrame that starts in the Transfer Frame Data Field,
         *		otherwise it is undefined.
         * @details Bits 37–47 of the Transfer Frame Primary Header
         * @see p. 4.1.2.7.6 from TM SPACE DATA LINK PROTOCOL
         */
        [[nodiscard]] uint16_t getFirstHeaderPointer() const {
            return ((static_cast<uint16_t>(((transferFrameData[4]) & 0x07)) << 8U) |
                    (static_cast<uint16_t>((transferFrameData[5]))));
        }

        /**
         * @brief The version of the secondary header. Currently, only version 1 '00' is supported.
         * @details Bits 0-1 of the Transfer Frame Secondary Header
         * @see p. 4.1.3.2.2 from TM SPACE DATA LINK PROTOCOL
         */
        [[nodiscard]] Defs::SecondaryHeaderVersionNumber getSecondaryHeaderVersionNumber() const {
            return static_cast<Defs::SecondaryHeaderVersionNumber>(transferFrameData[Defs::TmPrimaryHeaderSize] >> 6U);
        }

        /**
         * @details Bits 2-7 of the Transfer Frame Secondary Header
         * @see p. 4.1.3.2.3 from TM SPACE DATA LINK PROTOCOL
         * @note The returned result is incremented by one, since this field actually contains the length of the secondary
         *       header, minus one.
         */
        [[nodiscard]] uint8_t getSecondaryHeaderLength() const {
            return (transferFrameData[Defs::TmPrimaryHeaderSize] & 0x3F) + Defs::TmSecondaryHeaderIdLength;
        }

        [[nodiscard]] uint16_t getFirstEmptyOctet() const {}

        /**
         * @details Contains the 	a)Transfer Frame Secondary Header Flag (1 bit)
         *							b) Synchronization Flag (1 bit)
         *							c) TransferFrame Order Flag (1 bit)
         *							d) Segment Length Identifier (2 bits)
         *							e) First Header Pointer (11 bits)
         * Bits  32–47  of  the  Transfer  Frame  Primary  Header.
         * @see p. 4.1.2.7 from TM SPACE DATA LINK PROTOCOL
         */
        [[nodiscard]] uint16_t getTransferFrameDataFieldStatus() const {
            return (static_cast<uint16_t>((transferFrameData[4])) << 8U |
                    (static_cast<uint16_t>((transferFrameData[5]))));
        }

        /**
         * @brief Carry a report for the receiver.
         *
         * @details In this implementation, the operational control field is occupied by the CLCW,
         * which is defined in the TC SPACE DATA LINK PROTOCOL.
         *
         * @see p. 4.1.5 from TM SPACE DATA LINK PROTOCOL
         */
        [[nodiscard]] etl::optional<uint32_t> getOperationalControlField() const {
            if (!getOperationalControlFieldFlag()) {
                return etl::nullopt;
            }

            const uint8_t *operationalControlFieldPointer = transferFrameData + transferFrameLength -
                                                            Defs::TmOperationalControlFieldSize
                                                            - Defs::ErrorControlFieldSize *
                                                            eccFieldPresent;
            uint32_t operationalControlField = (operationalControlFieldPointer[0] << 24U) |
                                               (operationalControlFieldPointer[1] << 16U) |
                                               (operationalControlFieldPointer[2] << 8U) |
                                               operationalControlFieldPointer[3];
            return operationalControlField;
        }

        void setOperationalControlField(const uint32_t operationalControlField) const {
            uint8_t *ocfPointer = transferFrameData + transferFrameLength -
                                  Defs::TmOperationalControlFieldSize -
                                  Defs::ErrorControlFieldSize * eccFieldPresent;
            ocfPointer[0] = operationalControlField >> 24U;
            ocfPointer[1] = (operationalControlField >> 16U) & 0xFF;
            ocfPointer[2] = (operationalControlField >> 8U) & 0xFF;
            ocfPointer[3] = operationalControlField & 0xFF;
        }

    private:
        /**
         * @brief Indicates the presence of the error control field.
         *
         * @details This parameter is used in combination with the transfer frame length to determine the position of
         * the operational control field.
         */
        bool eccFieldPresent;
    };
} // namespace CCSDSDataLinkLayer
