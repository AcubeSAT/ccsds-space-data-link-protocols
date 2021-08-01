
#include <cstdint>
#ifndef CCSDS_TM_PACKETS_PACKET_H
#define CCSDS_TM_PACKETS_PACKET_H

enum PacketType { TC, TM };

struct TransferFrameHeader {
	TransferFrameHeader(uint8_t* pckt) {
		packet_header = pckt;
	}
	/**
	 * @brief The ID of the spacecraft
	 */
	const uint16_t spacecraft_id(enum PacketType T) const {
		if (T == TC) {
			return (static_cast<uint16_t>(packet_header[0] & 0x03) << 8U) |
			(static_cast<uint16_t>(packet_header[1]));
		}

		else {
			return ((static_cast<uint16_t>(packet_header[0]) & 0x3F) << 2U) |
			       ((static_cast<uint16_t>(packet_header[1])) & 0xF0);
		}
	}
	/**
	 * @brief The virtual channel ID this channel is transferred in
	 */
	const uint8_t vcid(enum PacketType T) const {
		if (T == TC) {
			return (packet_header[2] >> 2U) & 0x3F;
		} else {
			return ((packet_header[1] & 0x07));
		}
	}

protected:
	uint8_t* packet_header;
};

class Packet {
private:
	PacketType type;
	TransferFrameHeader transfer_frame_header;
};

#endif // CCSDS_TM_PACKETS_PACKET_H
