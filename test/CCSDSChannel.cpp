#include <catch2/catch.hpp>
#include <CCSDSChannel.hpp>
#include <TransferFrameTC.hpp>
#include <CCSDSServiceChannel.hpp>
#include "CLCW.hpp"

TEST_CASE("CCSDS TC Channel Model") {
	// @todo add more and better test cases :)

	PhysicalChannel phy_channel = PhysicalChannel(1024, false, 12, 1024, 220000, 20);

	etl::flat_map<uint8_t, MAPChannel, MaxMapChannels> map_channels = {{2, MAPChannel(2, false, false)},
	                                                                   {3, MAPChannel(3, true, true)}};

	uint8_t data[] = {0x00, 0xDA, 0x42, 0x32, 0x43, 0x12, 0x77, 0xFA, 0x3C, 0xBB, 0x92};
	MasterChannel master_channel = MasterChannel(true);
	master_channel.addVC(3, true, 1024, true, 32, 32, true, true, true, 8, SynchronizationFlag::FORWARD_ORDERED,
                         255, 10, 10, map_channels);

	CHECK(master_channel.virtChannels.at(3).VCID == 0x03);
	PhysicalChannel physical_channel =
	    PhysicalChannel(TmTransferFrameSize, TcErrorControlFieldExists, 100, 50, 20000, 5);
	ServiceChannel serv_channel = ServiceChannel(std::move(master_channel), std::move(physical_channel));
}

TEST_CASE("MAPP blocking") {
	PhysicalChannel physical_channel =
	    PhysicalChannel(TmTransferFrameSize, TcErrorControlFieldExists, 100, 50, 20000, 5);

	etl::flat_map<uint8_t, MAPChannel, MaxMapChannels> map_channels = {{2, MAPChannel(2, true, true)}};

	MasterChannel master_channel = MasterChannel(true);
	master_channel.addVC(3, true, 8, true, 32, 32, true, true, true, 11, SynchronizationFlag::FORWARD_ORDERED,
                         255, 10, 10, map_channels);

	CHECK(master_channel.virtChannels.at(3).VCID == 3);
	ServiceChannel serv_channel = ServiceChannel(std::move(master_channel), std::move(physical_channel));

	uint8_t data[] = {0x00, 0x01, 0x02, 0x30, 0x40, 0x05, 0x06, 0x07, 0x80, 0x90, 0xA0};

	serv_channel.storeTC(data, 11, 3, 2, 10, ServiceType::TYPE_A);
	CHECK(serv_channel.txAvailableTC(3, 2) == MaxReceivedTcInMapChannel - 1);

	serv_channel.mappRequest(3, 2);

	CHECK(serv_channel.txAvailableTC(3) == MaxReceivedUnprocessedTxTcInVirtBuffer - 6);
	CHECK(serv_channel.txAvailableTC(3, 2) == MaxReceivedTcInMapChannel);
}

TEST_CASE("Virtual Channel Generation") {}

