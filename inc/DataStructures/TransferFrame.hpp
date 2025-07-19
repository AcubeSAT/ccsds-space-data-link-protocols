/**
 * @file TransferFrame.hpp
 * @brief Defines the fundamental structure of the data link. It encapsulates packets sent from higher layers.
 */

#pragma once
#include <cstdint>
#include <cstring>
#include "CcsdsDefinitions.hpp"
#include "CRC16CCITT.hpp"

namespace CCSDSDataLinkLayer {
    class TransferFrame {
    public:
        TransferFrame() = default;

        TransferFrame(Defs::FrameType t, uint16_t transferFrameLength, uint8_t *frameData, uint16_t firstEmptyOctet = 0)
                : type(t), transferFrameLength(transferFrameLength), transferFrameData(frameData),
                  timesSequentiallyTransmitted(0), firstDataFieldEmptyOctet(firstEmptyOctet) {}

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

        [[nodiscard]] uint16_t getTimesSequentiallyTransmitted() const {
            return timesSequentiallyTransmitted;
        }

        void incrementTimesSequentiallyTransmitted() {
            timesSequentiallyTransmitted++;
        }

        /**
         * @brief Partially or completely replace frame data of this transfer frame.
         * @param dataSource Source of new data.
         * @param dataLength Amount of bytes to copy.
         * @param offset Offset from the start of the frame
         */
        void modifyFrameData(const uint8_t *dataSource, const uint16_t dataLength, const uint16_t offset = 0) {
            std::memcpy(transferFrameData + offset, dataSource, dataLength * sizeof(uint8_t));
        }

        /**
         * @brief Replace the frame data pointer with a new one.
         */
        void setNewFrameDataPointer(uint8_t *newFrameDataPointer) {
            transferFrameData = newFrameDataPointer;
        }

        /**
         * @brief Appends the CRC code to the end of the frame (error control field). It should be called only if the
         * frame actually contains an error control field.
         * @see p. 4.1.4.2 from TC SPACE DATA LINK PROTOCOL
         */
        void appendCRC() {
            const uint16_t len = transferFrameLength - Defs::ErrorControlFieldSize;
            const uint16_t crc = calculateCRC16CCITT(etl::span{transferFrameData, len});

            // append CRC
            transferFrameData[transferFrameLength - 2] = (crc >> 8U) & 0xFF;
            transferFrameData[transferFrameLength - 1] = crc & 0xFF;
        }

    protected:
        Defs::FrameType type;

        uint16_t transferFrameLength;
        uint8_t *transferFrameData;
        uint16_t timesSequentiallyTransmitted;

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