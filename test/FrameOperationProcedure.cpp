//#include <catch2/catch.hpp>
#include <catch2/catch_test_macros.hpp>
#include <CCSDSChannel.hpp>
#include <TransferFrameTC.hpp>
#include <CCSDSServiceChannel.hpp>

TEST_CASE("Initiate FOP Directives") {
	PhysicalChannel phy_channel_fop = PhysicalChannel(1024, 12, 1024, 220000, 20);

	etl::flat_map<uint8_t, MAPChannel, MaxMapChannels> map_channels_fop = {{2, MAPChannel(2, true, false)},
	                                                                       {3, MAPChannel(3, false, false)}};

	uint8_t data[] = {0x00, 0xDA, 0x42, 0x32, 0x43, 0x12, 0x77, 0xFA, 0x3C, 0xBB, 0x92};
	MasterChannel master_channel_fop = MasterChannel();
	master_channel_fop.addVC(3, 1024, true, 32, 32, true, true, true, 32, SynchronizationFlag::FORWARD_ORDERED, 255, 10,
	                         10, 3, map_channels_fop);

	ServiceChannel serv_channel_fop = ServiceChannel(master_channel_fop, phy_channel_fop);

    serv_channel_fop.storePacketTxTC(data, 11, 3, 2, ServiceType::TYPE_AD);
    serv_channel_fop.mappRequestTxTC(3, 2, 11, ServiceType::TYPE_AD);

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
TEST_CASE("Retransmission"){
	PhysicalChannel phy_channel_fop = PhysicalChannel(1024, 12, 1024, 220000, 20);

	etl::flat_map<uint8_t, MAPChannel, MaxMapChannels> map_channels = {
	    {0, MAPChannel(0, true, true)},
	    {1, MAPChannel(1, false, false)},
	    {2, MAPChannel(2, true, false)},
	};

	MasterChannel master_channel = MasterChannel();
	master_channel.addVC(0, 128, true, 2, 2, false, false, 0, 8, SynchronizationFlag::FORWARD_ORDERED, 255, 10, 10, 3,
	                     map_channels);
	master_channel.addVC(1, 128, true, 2, 2, false, false, 0, 8, SynchronizationFlag::FORWARD_ORDERED, 255, 10, 10, 3,
	                     map_channels);

	ServiceChannel serv_channel = ServiceChannel(master_channel, phy_channel_fop);
	VirtualChannel virtualChannel = master_channel.virtualChannels.at(0);

	uint8_t packet1[] = {0x10, 0xB1, 0x00, 0x0A, 0x00, 0x00, 0x00, 0x1C, 0xD3, 0x8C};
	uint8_t packet2[] = {0x10, 0xB1, 0x00, 0x0A, 0x01, 0x00, 0x00, 0x1C, 0xD3, 0x8C};
	uint8_t packet3[] = {0x10, 0xB1, 0x00, 0x0A, 0x12, 0x00, 0x00, 0x1C, 0xD3, 0x8C};


	//Send 3 frames
	serv_channel.initiateAdClcw(0);
    serv_channel.storePacketTxTC(packet1, 9, 0, 0, ServiceType::TYPE_AD);
    serv_channel.storePacketTxTC(packet2, 9, 0, 0, ServiceType::TYPE_AD);
    serv_channel.storePacketTxTC(packet3, 9, 0, 0, ServiceType::TYPE_AD);
	for (uint8_t i = 0; i < 3; i++){
        serv_channel.mappRequestTxTC(0, 0, 9, ServiceType::TYPE_AD);
		serv_channel.vcGenerationRequestTC(0);
        serv_channel.allFramesGenerationRequestTxTC();
	}
	CHECK(serv_channel.getLastMasterCopyTcFrame().transferFrameSequenceNumber() == 2);
	CHECK(serv_channel.frontUnprocessedFrameMcCopyTxTC().transferFrameSequenceNumber() == 0);

	//Create a CLCW  that indicates that retransmission is needed aka a negative acknowledgement
	CLCW clcw = CLCW(0,0,0,1,0,0,1,0,0,0,1,0,0,0);
	uint8_t clcwData[TmTransferFrameSize] = {0};
	for (uint8_t i = TmPrimaryHeaderSize; i < TmTransferFrameSize ; i++) {
		// add idle data
		clcwData[i] = idle_data[i];
	}
	//Create a transfer frame that carries the above CLCW
	TransferFrameTM clcwTransferFrame =
	    TransferFrameTM(clcwData, TmTransferFrameSize, 0, 0,
	                    false, false, NoSegmentation,
	                    FORWARD_ORDERED, clcw.clcw, TM);
	//Receive the CLCW frame
    serv_channel.allFramesReceptionRequestRxTM(clcwData, TmTransferFrameSize);
	//E10 enters
	CHECK(serv_channel.fopState(0) ==  RETRANSMIT_WITHOUT_WAIT);
}
