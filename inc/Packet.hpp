#ifndef CCSDS_TM_PACKETS_PACKET_HPP
#define CCSDS_TM_PACKETS_PACKET_HPP

#include <cstdint>
struct PacketPrimaryHeader {
public:
	PacketPrimaryHeader(uint8_t * packet){
		packet_header = packet;
	}
	/**
     * @brief The Packet Version Number of the packet
	 */
	const uint8_t packetVersionNumber() const {
		return (packet_header[0] >> 5U);
	}

	/**
     * @brief Packet Type of packet.
	 */
	const uint8_t packetType() const {
		return (packet_header[0] >> 4U) & 0x01;
	}

	/**
	 * @brief Indicates the presence of the secondary header.
	 */
	const uint8_t secHeaderFlag() const {
		return (packet_header[0] >> 3U) & 0x01;
	}

struct TransferFrameHeader {
    TransferFrameHeader(uint8_t *pckt) {
		packetHeader = pckt;
    }
	/**
     * @brief Application Process Identifier
	 */

	const uint16_t applicationProcessIdentifier() const {
		return (static_cast<uint16_t>(packet_header[0] & 0x07) << 8U) | (static_cast<uint16_t>(packet_header[1]));
	}

    /**
     * @brief The ID of the spacecraft
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
     * @brief Packet Identification contains the packet type, the secondary header flag and the application process identifier
	 */

	const uint16_t packetIdentification() const {
		return (static_cast<uint16_t>(packet_header[0] & 0x1F) << 8U) | (static_cast<uint16_t>(packet_header[1]));
	}

	/**
	 * @brief Sequence flags
	 */
	const uint8_t sequenceFlags() const {
		return (packet_header[2] >> 6U);
	}

    /**
     * @brief The virtual channel ID this channel is transferred in
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
	/**
	 * @brief Packet Name or packet sequence count
	 */
	const uint16_t packetSequnceCount() const {
		return (static_cast<uint16_t>((packet_header[2]) & 0x3F)) << 8U | (static_cast<uint16_t>((packet_header[3])));
	}

	/**
	 * @brief Packet sequence control consists of sequnce flags and packet sequence count
	 */
	const uint16_t packetSequenceControl() const {
		return (static_cast<uint16_t>((packet_header[2])) << 8U | (static_cast<uint16_t>((packet_header[3]))));
	}

	/**
	 * @brief Packet Data Length
	 */
	 const uint16_t packetDataLength() const{
		 return (static_cast<uint16_t>((packet_header[4])) << 8U | (static_cast<uint16_t>((packet_header[5]))));
	 }
protected:
    uint8_t * packetHeader;
};

struct Packet {

	const uint8_t * packetData() const {
		return data;
	}

	const PacketPrimaryHeader packet_header() const {
		return hdr;
	}

	const uint16_t packetLength() const {
		return packetDataLength;
	}

public:
	Packet(PacketType t, uint16_t packetLength, uint8_t *packet):
	      type(t), packetLength(packetLength), packet(packet), data(nullptr){};
	// the following may be unnecessary
	const uint8_t  packetsVersionNumber() const{
		return packetVersionNumber;
	}

	/**
     * @brief Appends the CRC code (given that the corresponding Error Correction field is present in the given
     * virtual channel)
     * @see p. 4.1.4.2 from TC SPACE DATA LINK PROTOCOL
	 */
	const uint16_t packetsIdentification() const{
		return packetIdentification;
	}

	const uint16_t packetsSequenceControl() const{
		return packetSequenceControl;
	}

	Packet(uint8_t *packet_data, uint16_t packet_length, PacketPrimaryHeader packet_header,
	       uint8_t packet_version_number, uint16_t packet_identification, uint16_t packet_sequence_control):
		data(packet_data), hdr(packet_header),packetDataLength(packet_length), packetVersionNumber(packet_version_number),
	     packetIdentification(packet_identification), packetSequenceControl(packet_sequence_control) {};

protected:
	uint8_t *data;

	/**
     * @brief Calculates the CRC code
     * @see p. 4.1.4.2 from TC SPACE DATA LINK PROTOCOL
	 */

	static uint16_t calculateCRC(const uint8_t *packet, uint16_t len);

	PacketPrimaryHeader hdr;
	uint16_t packetDataLength;
	uint8_t packetVersionNumber;
	uint16_t packetIdentification;
	uint16_t packetSequenceControl;
};
#endif // CCSDS_TM_PACKETS_PACKET_HPP
