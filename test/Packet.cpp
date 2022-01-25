#include <catch2/catch.hpp>
#include <TransferFrameTC.hpp>
#include <TransferFrameTM.hpp>
#include "Packet.hpp"

TEST_CASE("Transfer Frame TC Generation") {
    uint8_t data[] = {0, 11, 128, 33, 4, 5};
	TransferFrameTC pckt = TransferFrameTC(data, 5);

    CHECK(pckt.segmentationHeader() == 5);
    CHECK(pckt.transfer_frame_data()[0] == 0);
    CHECK(pckt.transfer_frame_data()[1] == 11);
    CHECK(pckt.transfer_frame_data()[2] == 128);
    CHECK(pckt.transfer_frame_data()[3] == 33);
    CHECK(pckt.transfer_frame_data()[4] == 4);
    CHECK(pckt.globalVirtualChannelId() == 32);
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
    CHECK(transfer_frame_header_tm.firstHeaderPointer() == 0xBE);
}

TEST_CASE("Transfer Frame TΜ Generation") {
    uint8_t data[] = {200, 185, 99, 23, 40, 6, 0, 11, 128, 33, 4, 5, 9, 10};
	TransferFrameTM transfer_frame = TransferFrameTM(data, 14);
    CHECK(transfer_frame.transfer_frame_data()[0] == 200);
    CHECK(transfer_frame.transfer_frame_data()[1] == 185);
    CHECK(transfer_frame.transfer_frame_data()[2] == 99);
    CHECK(transfer_frame.transfer_frame_data()[3] == 23);
    CHECK(transfer_frame.transfer_frame_data()[4] == 40);
    CHECK(transfer_frame.transfer_frame_data()[5] == 6);
    CHECK(transfer_frame.transfer_frame_pl_data()[0] == 0);
    CHECK(transfer_frame.transfer_frame_pl_data()[1] == 11);
    CHECK(pckt.getOperationalControlField()[0] == 128);
    CHECK(pckt.getOperationalControlField()[1] == 33);
    CHECK(pckt.getOperationalControlField()[2] == 4);
    CHECK(pckt.getOperationalControlField()[3] == 5);
//	CHECK(pckt.global_virtual_channel_id() == 32);
}
