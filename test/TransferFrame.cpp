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
	CHECK(transfer_frame_header_tc.getVirtualChannelId(TC) == 0x08);
	CHECK(transfer_frame_header_tc.getSpacecraftId(TC) == 0x9B);
	CHECK(transfer_frame_header_tc.ctrlAndCmdFlag() == 0x0);
	CHECK(transfer_frame_header_tc.transferFrameLength() == 0x28);
}


TEST_CASE("TM Header Generation - Raw Data Constructor") {
    uint8_t data[] = {200, 184, 99, 23, 40, 190};
    TransferFrameTM frame = TransferFrameTM(data, 6, true, 0);
    TransferFrameHeaderTM hdr = frame.getTransferFrameHeader();
    CHECK(hdr.getSpacecraftId(TM) == 0x8B);
    CHECK(hdr.getOperationalControlFieldFlag() == 0x0);
    CHECK(hdr.getVirtualChannelId(TM) == 0x04);
    CHECK(hdr.getMasterChannelFrameCount() == 0x63);
    CHECK(hdr.getVirtualChannelFrameCount() == 0x17);
    CHECK(hdr.getTransferFrameSecondaryHeaderFlag() == 0x0);
    CHECK(hdr.getSynchronizationFlag() == 0x0);
    CHECK(hdr.getPacketOrderFlag() == 0x01);
    CHECK(hdr.getFirstHeaderPointer() == 0xBE);
}

TEST_CASE("TM Header Tests - Field Constructor") {
    uint8_t data[TmTransferFrameSize] = {0};
    TransferFrameTM frame = TransferFrameTM(data, 10, 0x17, 4, true, false, false, SegmentLengthIdentifier, PacketOrder, OCTET_SYNCHRONIZED_FORWARD_ORDERED, 0xBE, 0, TM );
    TransferFrameHeaderTM hdr = frame.getTransferFrameHeader();

    CHECK(hdr.getTransferFrameVersionNumber() == 0);
    CHECK(hdr.getSpacecraftId(TM) == SpacecraftIdentifier);
    CHECK(hdr.getVirtualChannelId(TM) == 0x04);
    CHECK(hdr.getOperationalControlFieldFlag() == false);
    CHECK(hdr.getMasterChannelFrameCount() == 0x0);
    CHECK(hdr.getVirtualChannelFrameCount() == 0x17);
    CHECK(hdr.getTransferFrameSecondaryHeaderFlag() == false);
    CHECK(hdr.getSynchronizationFlag() == OCTET_SYNCHRONIZED_FORWARD_ORDERED);
    CHECK(hdr.getPacketOrderFlag() == PacketOrder);
    CHECK(hdr.getSegmentLengthId() == SegmentLengthIdentifier);
    CHECK(hdr.getFirstHeaderPointer() == 0xBE);
}

