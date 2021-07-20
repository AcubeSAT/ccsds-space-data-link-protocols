#include <catch2/catch.hpp>
#include <Packet.hpp>

TEST_CASE("Packet Generation") {
    uint8_t data[] = {0, 11, 128, 33, 4, 5};
    Packet pckt = Packet(data, 5);

    CHECK(pckt.segmentation_header() == 5);
    CHECK(pckt.packet_data()[0] == 0);
    CHECK(pckt.packet_data()[1] == 11);
    CHECK(pckt.packet_data()[2] == 128);
    CHECK(pckt.packet_data()[3] == 33);
    CHECK(pckt.packet_data()[4] == 4);
    CHECK(pckt.global_virtual_channel_id() == 32);
}