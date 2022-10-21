
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

	uint8_t packet[] = {0, 0, 0, 0, 0xd, 0x92, 0xb7, 17, 2, 0x0, 0x0, 0xa, 0xd, 17, 0x2, 0x38,0x0};

	serv_channel.storePacketTm(packet, 17, 0);
	uint16_t maxTransferFrameData = 20;
	serv_channel.vcGenerationService(maxTransferFrameData, 0);

	const TransferFrameTM* transferFrame = serv_channel.packetMasterChannel();

	FrameSender frameSender = FrameSender();

	TransferFrameTM transferFrameTm = TransferFrameTM(packet, 17, 0,0, false, false, 0, FORWARD_ORDERED, TM);

	for(uint8_t i = 0; i < 6 ; i++) {
		frameSender.sendFrameToYamcs(transferFrameTm);
		sleep(2);
	}
}
