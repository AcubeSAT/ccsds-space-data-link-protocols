#pragma once

#include <cstdint>
#include <CCSDS_Definitions.hpp>

enum PacketType { TC, TM };

struct TransferFrameHeader {
	TransferFrameHeader(uint8_t* pckt) {
		packetHeader = pckt;
	}

	/**
	 * The ID of the spacecraft
	 * 			TC: Bits  6–15  of  the  Transfer  Frame  Primary  Header
	 * 			TM: Bits  2–11  of  the  Transfer  Frame  Primary  Header
	 */
	uint16_t spacecraftId(enum PacketType packetType) const {
		if (packetType == TC) {
			return (static_cast<uint16_t>(packetHeader[0] & 0x03) << 8U) | (static_cast<uint16_t>(packetHeader[1]));
		} else {
			return ((static_cast<uint16_t>(packetHeader[0]) & 0x3F) << 2U) |
			       ((static_cast<uint16_t>(packetHeader[1])) & 0xC0) >> 6U;
		}
	}

	/**
	 * The virtual channel ID this channel is transferred in
	 * 			TC: Bits 16–21 of the Transfer Frame Primary Header
	 * 			TM: Bits 12–14 of the Transfer Frame Primary Header
	 */
	uint8_t vcid(enum PacketType packetType) const {
		if (packetType == TC) {
			return (packetHeader[2] >> 2U) & 0x3F;
		} else {
			return ((packetHeader[1] & 0x0E)) >> 1U;
		}
	}

protected:
	uint8_t* packetHeader;
};

class TransferFrame {
private:
	PacketType type;

public:
	TransferFrame(PacketType t, uint16_t transferFrameLength, uint8_t* packet)
	    : type(t), transferFrameLength(transferFrameLength),
	      packet(packet){};

    virtual ~TransferFrame(){ }

    virtual uint16_t calculateCRC(const uint8_t* packet, uint16_t len) = 0;

    /**
     * Appends the CRC code (given that the corresponding Error Correction field is present in the given
     * virtual channel)
     * @see p. 4.1.4.2 from TC SPACE DATA LINK PROTOCOL
     */
    void appendCRC() {
        uint16_t len = transferFrameLength - 2;
        uint16_t crc = calculateCRC(packet, len);

		uint16_t frameLength = (type == PacketType::TC) ? transferFrameLength : TmTransferFrameSize;

        // append CRC
        packet[frameLength - 2] = (crc >> 8) & 0xFF;
        packet[frameLength - 1] = crc & 0xFF;
    }

protected:
	uint16_t transferFrameLength;
	uint8_t* packet;
};
