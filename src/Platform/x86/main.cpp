
#include "FrameSender.hpp"
#include "CCSDSServiceChannel.hpp"
#include "FrameMaker.hpp"

int main(){

	//Create Virtual, Master and Service Channels
	PhysicalChannel phy_channel_fop = PhysicalChannel(1024, 12, 1024, 220000, 20);

	etl::flat_map<uint8_t, MAPChannel, MaxMapChannels> map_channels = {
	    {0, MAPChannel(0, true, true)},
	    {1, MAPChannel(1, false, false)},
	    {2, MAPChannel(2, true, false)},
	};

	MasterChannel master_channel = MasterChannel();
	master_channel.addVC(0, 128, true, 2, 2, true, true, true, 8, SynchronizationFlag::FORWARD_ORDERED, 255, 10, 10, 3,
	                     map_channels);

	ServiceChannel serv_channel = ServiceChannel(master_channel, phy_channel_fop);

	//Define the packets you want to send
	uint8_t packet[] = {0, 0, 0, 0, 0xd, 0x92, 0xb7, 17, 2, 0x0, 0x0, 0xa, 0xd, 17, 0x2, 0x38,0x0};
	uint8_t packet2[] = {1,2,3,4,5,6,7,8,9};

	std::pair<uint8_t *, ServiceChannelNotification> frame;

	//Create Frame Maker class instance
	FrameMaker frameMaker = FrameMaker(&serv_channel);
	//Store the packets you want to make a frame out of
	frameMaker.packetFeeder(packet,17,0);
	frameMaker.packetFeeder(packet2,9,0);
	frame = frameMaker.transmitChain(30,0);
	if(frame.second != NO_SERVICE_EVENT){
		printf("Error in Transmit Chain");
	}

	//Print the sent frame
	printf("Sent Frame: ");
	for(uint8_t i = 0 ; i < frameMaker.getFrameLength() ; i++){
		printf(" %d ", (frame.first)[i]);
	}
	//Print number of packet sent
	printf("\nNumber of Packet Sent %d", frameMaker.getNumberOfPackets());

	FrameSender frameSender = FrameSender();
	frameSender.sendFrameToYamcs(frame.first, frameMaker.getFrameLength());

}
