#pragma once

#include <cstdint>
#include <cstring>
#include "CCSDS_Definitions.hpp"

namespace CCSDSDataLinkLayer {
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

        void copyNewFrameData(uint8_t *dataSource, uint16_t dataLength) {
            std::memcpy(transferFrameData, dataSource, dataLength * sizeof(uint8_t));
        }

        void setNewFrameDataPointer(uint8_t *newFrameDataPointer) {
            transferFrameData = newFrameDataPointer;
        }

        /**
         * Utility function for calculating the CRC code
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
         * Appends the CRC code (given that the corresponding Error Correction field is present in the given
         * virtual channel)
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
         *  Auxiliary variable that indicates the position of the first empty octet in the transfer frame data field.
         *  The first octet of the data field is defined as position 0. A value equal of the data field size indicates a filled frame.
         */
        uint16_t firstDataFieldEmptyOctet;

    };
} // namespace CCSDSDataLinkLayer