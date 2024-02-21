//#include <catch2/catch.hpp>
#include <catch2/catch_test_macros.hpp>
#include <TransferFrameTC.hpp>
#include <TransferFrameTM.hpp>

TEST_CASE("TransferFrameTC Generation") {
	uint8_t data[] = {0, 11, 128, 33, 4, 5};
	TransferFrameTC pckt = TransferFrameTC(data, 5);

	CHECK(pckt.segmentationHeader() == 5);
	CHECK(pckt.packetData()[0] == 0);
	CHECK(pckt.packetData()[1] == 11);
	CHECK(pckt.packetData()[2] == 128);
	CHECK(pckt.packetData()[3] == 33);
	CHECK(pckt.packetData()[4] == 4);
}

TEST_CASE("TC Header Generation") {
	uint8_t data[] = {200, 155, 32, 40, 66};
	TransferFrameHeaderTC transfer_frame_header_tc = TransferFrameHeaderTC(data);
	CHECK(transfer_frame_header_tc.bypassFlag() == 0x0);
	CHECK(transfer_frame_header_tc.vcid() == 0x08);
	CHECK(transfer_frame_header_tc.spacecraftId() == 0x9B);
	CHECK(transfer_frame_header_tc.ctrlAndCmdFlag() == 0x0);
	CHECK(transfer_frame_header_tc.transferFrameLength() == 0x28);
}

TEST_CASE("TM Header Generation") {
	uint8_t data[] = {200, 184, 99, 23, 40, 190};
	TransferFrameHeaderTM transfer_frame_header_tm = TransferFrameHeaderTM(data);
	CHECK(transfer_frame_header_tm.spacecraftId() == 0x22);
	CHECK(transfer_frame_header_tm.operationalControlFieldFlag() == 0x0);
	CHECK(transfer_frame_header_tm.vcid() == 0x04);
	CHECK(transfer_frame_header_tm.getmasterChannelFrameCount() == 0x63);
	CHECK(transfer_frame_header_tm.virtualChannelFrameCount() == 0x17);
	CHECK(transfer_frame_header_tm.transferFrameSecondaryHeaderFlag() == 0x0);
	CHECK(transfer_frame_header_tm.synchronizationFlag() == 0x0);
	CHECK(transfer_frame_header_tm.packetOrderFlag() == 0x01);
	CHECK(transfer_frame_header_tm.firstHeaderPointer() == 0xBE);
}

TEST_CASE("PacketTΜ Generation") {
	uint8_t data[] = {200, 185, 99, 23, 40, 6, 0, 11, 128, 33, 4, 5, 9, 10};
	TransferFrameTM pckt = TransferFrameTM(data, 14, true);
	CHECK(pckt.packetData()[0] == 200);
	CHECK(pckt.packetData()[1] == 185);
	CHECK(pckt.packetData()[2] == 99);
	CHECK(pckt.packetData()[3] == 23);
	CHECK(pckt.packetData()[4] == 40);
	CHECK(pckt.packetData()[5] == 6);
	CHECK(pckt.getOperationalControlField() == 0x80210405);
	// CHECK(pckt.global_virtual_channel_id() == 32);
}