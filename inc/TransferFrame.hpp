#pragma once

#include <cstdint>
#include <CCSDS_Definitions.hpp>

enum FrameType { TC, TM };

struct TransferFrameHeader {
	TransferFrameHeader(uint8_t* frameData) {
        transferFrameHeader = frameData;
	}

	/**
	 * The ID of the spacecraft
	 * 			TC: Bits  6–15  of  the  Transfer  Frame  Primary  Header
	 * 			TM: Bits  2–11  of  the  Transfer  Frame  Primary  Header
	 */
	uint16_t spacecraftId(enum FrameType frameType) const {
		if (frameType == TC) {
			return (static_cast<uint16_t>(transferFrameHeader[0] & 0x03) << 8U) | (static_cast<uint16_t>(transferFrameHeader[1]));
		} else {
			return ((static_cast<uint16_t>(transferFrameHeader[0]) & 0x3F) << 2U) |
                   ((static_cast<uint16_t>(transferFrameHeader[1])) & 0xC0) >> 6U;
		}
	}

	/**
	 * The virtual channel ID this channel is transferred in
	 * 			TC: Bits 16–21 of the Transfer Frame Primary Header
	 * 			TM: Bits 12–14 of the Transfer Frame Primary Header
	 */
	uint8_t vcid(enum FrameType frameType) const {
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
	TransferFrame(FrameType t, uint16_t transferFrameLength, uint8_t* frameData)
	    : type(t), transferFrameLength(transferFrameLength), transferFrameData(frameData){};

	virtual ~TransferFrame() {}

	virtual uint16_t calculateCRC(const uint8_t* data, uint16_t len) = 0;

	/**
	 * Appends the CRC code (given that the corresponding Error Correction field is present in the given
	 * virtual channel)
	 * @see p. 4.1.4.2 from TC SPACE DATA LINK PROTOCOL
	 */
	void appendCRC() {
		uint16_t len = transferFrameLength - 2;
		uint16_t crc = calculateCRC(transferFrameData, len);

		uint16_t frameLength = (type == FrameType::TC) ? transferFrameLength : TmTransferFrameSize;

		// append CRC
		transferFrameData[frameLength - 2] = (crc >> 8) & 0xFF;
        transferFrameData[frameLength - 1] = crc & 0xFF;
	}

protected:
	uint16_t transferFrameLength;
	uint8_t* transferFrameData;
};
