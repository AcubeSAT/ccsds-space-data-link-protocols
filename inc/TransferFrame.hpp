/**
 * @file TransferFrame.hpp
 * @brief Defines the fundamental block of the data link. It encapsulates packets sent from higher layers.
 */

#pragma once

#include <cstdint>
#include <cstring>
#include "CCSDS_Definitions.hpp"

namespace CCSDSDataLinkLayer {
    /**
     * @brief Indicates whether a frame is of type TC (telecommand) or TM (telemetry)
     */
    enum class FrameType : bool {
        TC = false,
        TM = true
    };

    class TransferFrame {
    public:
        TransferFrame(FrameType t, uint16_t transferFrameLength, uint8_t *frameData, uint16_t firstEmptyOctet = 0)
                : type(t), transferFrameLength(transferFrameLength), transferFrameData(frameData),
                  firstDataFieldEmptyOctet(firstEmptyOctet) {};

        [[nodiscard]] uint16_t getFrameLength() const {
            return transferFrameLength;
        }

        [[nodiscard]] uint16_t getFirstDataFieldEmptyOctet() const {
            return firstDataFieldEmptyOctet;
        }

        void setFirstDataFieldEmptyOctet(uint16_t firstEmptyOctet) {
            firstDataFieldEmptyOctet = firstEmptyOctet;
        }

        [[nodiscard]] uint8_t *getFrameData() const {
            return transferFrameData;
        }

        /**
         * @brief Partially or completely replace the frame data of this transfer frame.
         * @param dataSource Source of new data.
         * @param dataLength Amount of bytes to copy.
         */
        void setNewFrameData(uint8_t *dataSource, uint16_t dataLength) {
            std::memcpy(transferFrameData, dataSource, dataLength * sizeof(uint8_t));
        }

        /**
         * @brief Replace the frame data pointer with a new one.
         */
        void setNewFrameDataPointer(uint8_t *newFrameDataPointer) {
            transferFrameData = newFrameDataPointer;
        }

        /**
         * @brief Utility function for calculating the CRC-16 code of an data sequence.
         * @param data Start of data sequence.
         * @param len  Length of data sequence.
         * @see p. 4.1.4.2 from TC SPACE DATA LINK PROTOCOL
         */
        static uint16_t calculateCRC(const uint8_t *data, uint16_t len) {
            uint16_t crc = 0xFFFF;

            // calculate remainder of binary polynomial division
            for (uint16_t i = 0; i < len; i++) {
                crc = crc_16_ccitt_table[(data[i] ^ (crc >> 8U)) & 0xFF] ^ (crc << 8U);
            }

            return crc;
        }

        /**
         * @brief Appends the CRC code to the end of the frame (error control field). It should be called only if the
         * frame actually contains an error control field.
         * @see p. 4.1.4.2 from TC SPACE DATA LINK PROTOCOL
         */
        void appendCRC() {
            uint16_t len = transferFrameLength - ErrorControlFieldSize;
            uint16_t crc = calculateCRC(transferFrameData, len);

            // append CRC
            transferFrameData[transferFrameLength - 2] = (crc >> 8U) & 0xFF;
            transferFrameData[transferFrameLength - 1] = crc & 0xFF;
        }

    protected:
        FrameType type;

        uint16_t transferFrameLength;
        uint8_t *transferFrameData;

        /**
         *  @brief Auxiliary variable that indicates the position of the first empty octet in the transfer
         *  frame data field. It used in the process of contructing transfer frame data fields, when there is
         *  "blocking" involved.
         *
         *  @detais The value "0" is defined as the first octet of the transfer frame's data field.
         *  A value equal of the data field size shall indicate a filled frame.
         */
        uint16_t firstDataFieldEmptyOctet;

    };
} // namespace CCSDSDataLinkLayer