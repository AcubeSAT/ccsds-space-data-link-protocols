#pragma once

#include <cstdint>
#include <CCSDS_Definitions.hpp>

enum FrameType { TC, TM };

struct TransferFrameHeader {
	TransferFrameHeader(uint8_t* frameData) {
        transferFrameHeader = frameData;
	}

    /**
     *  Frame version
     */
    uint8_t getTransferFrameVersionNumber() const {
        return (transferFrameHeader[0] & 0xC0) >> 6;
    }

	/**
	 * The ID of the spacecraft
	 * 			TC: Bits  6–15  of  the  Transfer  Frame  Primary  Header
	 * 			TM: Bits  2–11  of  the  Transfer  Frame  Primary  Header
	 */
	uint16_t getSpacecraftId(enum FrameType frameType) const {
		if (frameType == TC) {
			return (static_cast<uint16_t>(transferFrameHeader[0] & 0x03) << 8U) |
                   (static_cast<uint16_t>(transferFrameHeader[1]));
		} else {
			return (static_cast<uint16_t>(transferFrameHeader[0] & 0x3F) << 4U) |
                   (static_cast<uint16_t>(transferFrameHeader[1] & 0xF0) >> 4U);
		}
	}

	/**
	 * The virtual channel ID this channel is transferred in
	 * 			TC: Bits 16–21 of the Transfer Frame Primary Header
	 * 			TM: Bits 12–14 of the Transfer Frame Primary Header
	 */
	uint8_t getVirtualChannelId(enum FrameType frameType) const {
		if (frameType == TC) {
			return (transferFrameHeader[2] >> 2U) & 0x3F;
		} else {
			return ((transferFrameHeader[1] & 0x0E)) >> 1U;
		}
	}

protected:
	uint8_t* transferFrameHeader;
};

class TransferFrame {
private:
	FrameType type;

public:
	TransferFrame(FrameType t, uint16_t transferFrameLength, uint8_t* frameData, uint16_t firstEmptyOctet = 0)
	    : type(t), transferFrameLength(transferFrameLength), transferFrameData(frameData), firstDataFieldEmptyOctet(firstEmptyOctet){};

	virtual ~TransferFrame() {}

    uint16_t getFrameLength() const {
        return transferFrameLength;
    }

    uint16_t getFirstDataFieldEmptyOctet() const {
        return firstDataFieldEmptyOctet;
    }

    void setFirstDataFieldEmptyOctet(uint16_t firstEmptyOctet) {
        firstDataFieldEmptyOctet = firstEmptyOctet;
    }

    uint8_t* getFrameData() const {
        return transferFrameData;
    }

    void setFrameData(uint8_t* dataSource, uint16_t dataLength) {
        std::memcpy(transferFrameData, dataSource, dataLength * sizeof(uint8_t));
    }

    /**
     * Calculates the CRC code
     * @see p. 4.1.4.2 from TC SPACE DATA LINK PROTOCOL
     */
    static uint16_t calculateCRC(const uint8_t* data, uint16_t len) {
        uint16_t crc = 0xFFFF;

        // calculate remainder of binary polynomial division
        for (uint16_t i = 0; i < len; i++) {
            crc = crc_16_ccitt_table[(data[i] ^ (crc >> 8)) & 0xFF] ^ (crc << 8);
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
		transferFrameData[transferFrameLength - 2] = (crc >> 8) & 0xFF;
        transferFrameData[transferFrameLength - 1] = crc & 0xFF;
	}

protected:
	uint16_t transferFrameLength;
	uint8_t* transferFrameData;
    /**
     *  Auxiliary variable that shows the position of the first empty octet in the transfer frame data field.
     *  The first octet of the data field is defined as position 0. A value equal of the data field size indicates a filled frame.
     */
    uint16_t firstDataFieldEmptyOctet;
};
