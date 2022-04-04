#include <Packet.hpp>

Packet::Packet(uint8_t* packet_data, uint16_t packet_length, PacketPrimaryHeader hdr)
    : hdr(hdr) {

	packetVersionNumber = packet_data[0] >> 5U;
	packetIdentification = (static_cast<uint16_t>(packet_data[0] & 0x1F) << 8U) | (static_cast<uint16_t>(packet_data[1]));
	packetSequenceControl = (static_cast<uint16_t>(packet_data[2])) << 8U | (static_cast<uint16_t>((packet_data[3])));

}
