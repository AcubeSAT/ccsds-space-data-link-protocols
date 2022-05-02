#include <PacketTM.hpp>
#include <Packet.hpp>

PacketTM::PacketTM(uint8_t* packet, uint16_t packet_length, PacketType t)
    : Packet(t, packet_length, packet), hdr(packet) {
	if (hdr.transferFrameSecondaryHeaderFlag() == 1) {
		secondaryHeader = &packet[7];
	}
	firstHeaderPointer = hdr.firstHeaderPointer();

	data = &packet[firstHeaderPointer];

	if (hdr.operationalControlFieldFlag() == 1) {
		operationalControlField = &packet[packet_length - 6];
	}
}
