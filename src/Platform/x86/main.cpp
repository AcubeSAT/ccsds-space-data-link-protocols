
#include "FrameSender.hpp"
#include "CCSDSServiceChannel.hpp"


int main(){
	PhysicalChannel phy_channel_fop = PhysicalChannel(1024, false, 12, 1024, 220000, 20);

	etl::flat_map<uint8_t, MAPChannel, MaxMapChannels> map_channels = {
	    {0, MAPChannel(0, true, true)},
	    {1, MAPChannel(1, false, false)},
	    {2, MAPChannel(2, true, false)},
	};

	MasterChannel master_channel = MasterChannel();
	master_channel.addVC(0, 128, true, 2, 2, true, true, true, 8, SynchronizationFlag::FORWARD_ORDERED, 255, 10, 10, 3,
	                     map_channels);

	ServiceChannel serv_channel = ServiceChannel(master_channel, phy_channel_fop);
	ServiceChannelNotification err;

	uint8_t packet1[] = {1, 54, 32, 49, 12, 23};
	uint8_t packet2[] = {47, 31, 65, 81, 25, 44, 76, 99, 13};
	uint8_t packet3[] = {41, 91, 68, 10};
	uint8_t vid = 0 & 0x3F;

	serv_channel.storePacketTm(packet1, 6, 0);
	serv_channel.storePacketTm(packet2, 9, 0);
	serv_channel.storePacketTm(packet3, 4, 0);
	uint16_t maxTransferFrameData = 15;
	err = serv_channel.vcGenerationService(maxTransferFrameData, 0);

	const TransferFrameTM* transferFrame = serv_channel.packetMasterChannel();

	FrameSender frameSender = FrameSender();

	const TransferFrameTM& transferFramep = *transferFrame;
	for(uint8_t i = 0; i < 4 ; i++) {
		frameSender.sendFrameToYamcs(transferFramep);
		sleep(2);
	}
}
