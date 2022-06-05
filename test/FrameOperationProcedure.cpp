#include <catch2/catch.hpp>
#include <CCSDSChannel.hpp>
#include <TransferFrameTC.hpp>
#include <CCSDSServiceChannel.hpp>

TEST_CASE("Initiate FOP Directives") {
	PhysicalChannel phy_channel_fop = PhysicalChannel(1024, false, 12, 1024, 220000, 20);

	etl::flat_map<uint8_t, MAPChannel, MaxMapChannels> map_channels_fop = {
	    {2, MAPChannel(2, true, false)}, {3, MAPChannel(3, false, false)}};

	uint8_t data[] = {0x00, 0xDA, 0x42, 0x32, 0x43, 0x12, 0x77, 0xFA, 0x3C, 0xBB, 0x92};
	MasterChannel master_channel_fop = MasterChannel(true);
	master_channel_fop.addVC(3, true, 1024, true, 32, 32, true, true, true, 32, SynchronizationFlag::FORWARD_ORDERED,
                             255, 10, 10, map_channels_fop);

	ServiceChannel serv_channel_fop = ServiceChannel(master_channel_fop, phy_channel_fop);

	serv_channel_fop.storeTC(data, 11, 3, 2, 10, ServiceType::TYPE_AD);

	CHECK(serv_channel_fop.txAvailableTC(3, 2) == MaxReceivedTcInMapChannel - 1);

	serv_channel_fop.mappRequest(3, 2);

	CHECK(serv_channel_fop.txAvailableTC(3, 2) == MaxReceivedTcInMapChannel);

	CHECK(serv_channel_fop.fopState(3) == FOPState::INITIAL);
	serv_channel_fop.initiateAdNoClcw(3);
	CHECK(serv_channel_fop.fopState(3) == FOPState::ACTIVE);
	serv_channel_fop.terminateAdService(3);
	serv_channel_fop.setVs(3, 6);
	CHECK(serv_channel_fop.expectedFrameSeqNumber(3) == 6);
	CHECK(serv_channel_fop.transmitterFrameSeqNumber(3) == 6);
	CHECK(serv_channel_fop.fopState(3) == FOPState::INITIAL);
	serv_channel_fop.initiateAdUnlock(3);

	CHECK(serv_channel_fop.timeoutType(3) == 0);
	serv_channel_fop.setTimeoutType(3, 1);
	CHECK(serv_channel_fop.timeoutType(3) == 1);

	CHECK(serv_channel_fop.t1Timer(3) == FopTimerInitial);
	serv_channel_fop.setT1Initial(3, 55);
	CHECK(serv_channel_fop.t1Timer(3) == 55);

	CHECK(serv_channel_fop.fopSlidingWindowWidth(3) == FopSlidingWindowInitial);
	serv_channel_fop.setFopWidth(3, 100);
	CHECK(serv_channel_fop.fopSlidingWindowWidth(3) == 100);
}