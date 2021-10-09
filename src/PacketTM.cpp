#include <PacketTM.hpp>
#include <Packet.hpp>

PacketTM::PacketTM(uint8_t *packet, uint16_t packet_length, PacketType t): Packet(t,packet_length,packet), hdr(packet) {

	if(hdr.transfer_frame_secondary_header_flag()==1) {
		secondaryHeader = &packet[7];
	}
	firstHeaderPointer = hdr.first_header_pointer();

	data = &packet[firstHeaderPointer];

	if(hdr.operational_control_field_flag()==1)
	{
		operationalControlField=&packet[packet_length-6];
	}

}
