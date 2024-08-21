//#include <catch2/catch.hpp>
#include <catch2/catch_test_macros.hpp>
#include <TransferFrameTC.hpp>
#include <TransferFrameTM.hpp>

TEST_CASE("TransferFrameTC Generation") {
	uint8_t data[] = {0, 11, 128, 33, 4, 5};
	TransferFrameTC frame = TransferFrameTC(data, 5);

	CHECK(frame.segmentationHeader() == 5);
	CHECK(frame.getFrameData()[0] == 0);
	CHECK(frame.getFrameData()[1] == 11);
	CHECK(frame.getFrameData()[2] == 128);
	CHECK(frame.getFrameData()[3] == 33);
	CHECK(frame.getFrameData()[4] == 4);
}

TEST_CASE("TC Header Generation") {
	uint8_t data[] = {200, 155, 32, 40, 66};
	TransferFrameHeaderTC transfer_frame_header_tc = TransferFrameHeaderTC(data);
	CHECK(transfer_frame_header_tc.bypassFlag() == 0x0);
	CHECK(transfer_frame_header_tc.vcid(TC) == 0x08);
	CHECK(transfer_frame_header_tc.spacecraftId(TC) == 0x9B);
	CHECK(transfer_frame_header_tc.ctrlAndCmdFlag() == 0x0);
	CHECK(transfer_frame_header_tc.transferFrameLength() == 0x28);
}

TEST_CASE("TM Header Generation") {
	uint8_t data[] = {200, 184, 99, 23, 40, 190};
	TransferFrameHeaderTM transfer_frame_header_tm = TransferFrameHeaderTM(data);
	CHECK(transfer_frame_header_tm.spacecraftId(TM) == 0x22);
	CHECK(transfer_frame_header_tm.operationalControlFieldFlag() == 0x0);
	CHECK(transfer_frame_header_tm.vcid(TM) == 0x04);
	CHECK(transfer_frame_header_tm.masterChannelFrameCount() == 0x63);
	CHECK(transfer_frame_header_tm.virtualChannelFrameCount() == 0x17);
	CHECK(transfer_frame_header_tm.transferFrameSecondaryHeaderFlag() == 0x0);
	CHECK(transfer_frame_header_tm.synchronizationFlag() == 0x0);
	CHECK(transfer_frame_header_tm.packetOrderFlag() == 0x01);
	CHECK(transfer_frame_header_tm.getFirstHeaderPointer() == 0xBE);
}

TEST_CASE("PacketTΜ Generation") {
	uint8_t data[] = {200, 185, 99, 23, 40, 6, 0, 11, 128, 33, 4, 5, 9, 10};
	TransferFrameTM frame = TransferFrameTM(data, 14, true, TmTransferFrameSize - TmPrimaryHeaderSize - 6);
	CHECK(frame.getFrameData()[0] == 200);
	CHECK(frame.getFrameData()[1] == 185);
	CHECK(frame.getFrameData()[2] == 99);
	CHECK(frame.getFrameData()[3] == 23);
	CHECK(frame.getFrameData()[4] == 40);
	CHECK(frame.getFrameData()[5] == 6);
	CHECK(frame.getOperationalControlField() == 0x80210405);
	// CHECK(frame.global_virtual_channel_id() == 32);
}