#include <catch2/catch.hpp>
#include <PacketTC.hpp>
#include <PacketTM.hpp>

TEST_CASE("PacketTC Generation") {
    uint8_t data[] = {0, 11, 128, 33, 4, 5};
    PacketTC pckt = PacketTC(data, 5);

    CHECK(pckt.segmentation_header() == 5);
    CHECK(pckt.packet_data()[0] == 0);
    CHECK(pckt.packet_data()[1] == 11);
    CHECK(pckt.packet_data()[2] == 128);
    CHECK(pckt.packet_data()[3] == 33);
    CHECK(pckt.packet_data()[4] == 4);
    CHECK(pckt.global_virtual_channel_id() == 32);
}

TEST_CASE("TC Header Generation") {
    uint8_t data[] = {200, 155, 32, 40, 66};
    TransferFrameHeaderTC transfer_frame_header_tc = TransferFrameHeaderTC(data);
    CHECK(transfer_frame_header_tc.bypass_flag() == 0x0);
    CHECK(transfer_frame_header_tc.vcid(TC) == 0x08);
    CHECK(transfer_frame_header_tc.spacecraft_id(TC) == 0x9B);
    CHECK(transfer_frame_header_tc.ctrl_and_cmd_flag() == 0x0);
    CHECK(transfer_frame_header_tc.transfer_frame_length() == 0x28);
}

TEST_CASE("TM Header Generation") {
    uint8_t data[] = {200, 184, 99, 23, 40, 190};
    TransferFrameHeaderTM transfer_frame_header_tm = TransferFrameHeaderTM(data);
    CHECK(transfer_frame_header_tm.spacecraft_id(TM) == 0x22);
    CHECK(transfer_frame_header_tm.operational_control_field_flag() == 0x0);
    CHECK(transfer_frame_header_tm.vcid(TM) == 0x04);
    CHECK(transfer_frame_header_tm.master_channel_frame_count() == 0x63);
    CHECK(transfer_frame_header_tm.virtual_channel_frame_count() == 0x17);
    CHECK(transfer_frame_header_tm.transfer_frame_secondary_header_flag() == 0x0);
    CHECK(transfer_frame_header_tm.synchronization_flag() == 0x0);
    CHECK(transfer_frame_header_tm.packet_order_flag() == 0x01);
    CHECK(transfer_frame_header_tm.first_header_pointer() == 0xBE);
}
