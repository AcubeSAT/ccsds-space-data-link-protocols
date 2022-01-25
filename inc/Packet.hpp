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

	/**
     * @brief Application Process Identifier
	 */

	const uint16_t applicationProcessIdentifier() const {
		return (static_cast<uint16_t>(packet_header[0] & 0x07) << 8U) | (static_cast<uint16_t>(packet_header[1]));
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
	uint8_t *packet_header;

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

	// the following may be unnecessary
	const uint8_t  packetsVersionNumber() const{
		return packetVersionNumber;
	}

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
	PacketPrimaryHeader hdr;
	uint16_t packetDataLength;
	uint8_t packetVersionNumber;
	uint16_t packetIdentification;
	uint16_t packetSequenceControl;
};
#endif // CCSDS_TM_PACKETS_PACKET_HPP
