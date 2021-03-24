#include <catch2/catch.hpp>
#include <CCSDSChannel.hpp>
#include <Packet.hpp>
#include <CCSDSServiceChannel.hpp>

TEST_CASE("CCSDS TC Channel Model") {
    // @todo add more and better test cases :)

    PhysicalChannel phy_channel = PhysicalChannel(1024, false, 12,
                                                  1024, 220000, 20);

    etl::map<uint8_t, MAPChannel, MAX_MAP_CHANNELS> map_channels = {
            {2, MAPChannel(2, DataFieldContent::PACKET)},
            {3, MAPChannel(3, DataFieldContent::VCA_SDU)}
    };


    etl::map<uint8_t, VirtualChannel, MAX_VIRTUAL_CHANNELS> virt_channels = {
            {3, VirtualChannel(3, true, 1024, 20,
                               true, 32, 32, map_channels)}
    };

    uint8_t data[] = {0x00, 0xDA, 0x42, 0x32, 0x43, 0x12, 0x77, 0xFA, 0x3C, 0xBB, 0x92};
    MasterChannel master_channel = MasterChannel(virt_channels);

    CHECK(master_channel.virtChannels.at(3).VCID == 0x03);
    ServiceChannel serv_channel = ServiceChannel(master_channel);

    // serv_channel.store(data, 11, 3, 2, 10, 0);
   // serv_channel.mapp_request(3, 2);
}


TEST_CASE("MAPP blocking") {

    etl::map<uint8_t, MAPChannel, MAX_MAP_CHANNELS> map_channels = {
            {2,  MAPChannel(2, DataFieldContent::PACKET)}
    };


    etl::map<uint8_t, VirtualChannel, MAX_VIRTUAL_CHANNELS> virt_channels = {
            {3, VirtualChannel(3, true, 8, 20,
                               true, 32, 32, map_channels)}
    };


    MasterChannel master_channel = MasterChannel(virt_channels);
    CHECK(master_channel.virtChannels.at(3).VCID == 3);
    ServiceChannel serv_channel = ServiceChannel(master_channel);

    uint8_t data[] = {0x00, 0x01, 0x02, 0x30, 0x40, 0x05, 0x06, 0x07, 0x80, 0x90, 0xA0};

    serv_channel.store(data, 11, 3, 2, 10, 0);
    CHECK(serv_channel.available(3, 2) == MAX_RECEIVED_TC_IN_MAP_BUFFER - 1);

    serv_channel.mapp_request(3, 2);

    CHECK(serv_channel.available(3) == MAX_RECEIVED_TC_IN_WAIT_QUEUE - 6);
    CHECK(serv_channel.available(3, 2) == MAX_RECEIVED_TC_IN_MAP_BUFFER);
}
