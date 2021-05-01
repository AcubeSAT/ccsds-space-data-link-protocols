#include <catch2/catch.hpp>
#include <CCSDSChannel.hpp>
#include <Packet.hpp>
#include <CCSDSServiceChannel.hpp>
#include <iostream>

TEST_CASE("Service Channel") {
    // Set up Service Channel
    PhysicalChannel phy_channel_fop = PhysicalChannel(1024, false, 12,
                                                      1024, 220000, 20);

    etl::map<uint8_t, MAPChannel, MAX_MAP_CHANNELS> map_channels = {
            {0, MAPChannel(0, DataFieldContent::PACKET)},
            {1, MAPChannel(1, DataFieldContent::PACKET)},
            {2, MAPChannel(2, DataFieldContent::PACKET)},
    };


    etl::map<uint8_t, VirtualChannel, MAX_VIRTUAL_CHANNELS> virt_channels = {
            {0, VirtualChannel(0, true, 128, 20,
                               true, 2, 2, map_channels)}
    };

    MasterChannel master_channel = MasterChannel(virt_channels, true);

    ServiceChannel serv_channel = ServiceChannel(master_channel);

    // TODO: Test getters (once implemented)

    // MAPP Generation Service

    uint8_t pckt_type_a[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0xA2, 0xB3, 0x21, 0xA1};
    uint8_t pckt_type_b[] = {0x10, 0xB1, 0xFF, 0x33, 0x08, 0xA5, 0x15, 0x1C, 0x21, 0X40};
    uint8_t pckt_type_a2[] = {0xE1, 0x32, 0x12};

    // Initialize service
    CHECK(serv_channel.fop_state(0) == FOPState::INITIAL);
    serv_channel.initiate_ad_no_clcw(0);
    CHECK(serv_channel.fop_state(0) == FOPState::ACTIVE);

    CHECK(serv_channel.available(0, 0) == MAX_RECEIVED_TC_IN_MAP_BUFFER);
    CHECK(serv_channel.available(0, 1) == MAX_RECEIVED_TC_IN_MAP_BUFFER);
    CHECK(serv_channel.available(0, 2) == MAX_RECEIVED_TC_IN_MAP_BUFFER);

    serv_channel.store(pckt_type_a, 9, 0, 0, 0, ServiceType::TYPE_A);
    CHECK(serv_channel.available(0, 0) == MAX_RECEIVED_TC_IN_MAP_BUFFER - 1);
    const Packet* packet_a = serv_channel.packet().second;
    CHECK(packet_a->packetLength == 9);
    CHECK(packet_a->serviceType == ServiceType::TYPE_A);
    CHECK((serv_channel.packet(0, 0).second == packet_a));

    serv_channel.store(pckt_type_b, 10, 0, 0, 0, ServiceType::TYPE_B);
    CHECK(serv_channel.available(0, 0) == MAX_RECEIVED_TC_IN_MAP_BUFFER - 2);
    const Packet* packet_b = serv_channel.packet().second;
    CHECK(packet_b->packetLength == 10);
    CHECK(packet_b->serviceType == ServiceType::TYPE_B);

    serv_channel.store(pckt_type_a2, 3, 0, 0, 0, ServiceType::TYPE_A);
    CHECK(serv_channel.available(0, 0) == MAX_RECEIVED_TC_IN_MAP_BUFFER - 3);
    const Packet* packet_c = serv_channel.packet().second;
    CHECK(packet_c->packetLength == 3);
    CHECK(packet_c->serviceType == ServiceType::TYPE_A);
    CHECK((serv_channel.packet(0, 0).second == packet_a));

    CHECK(serv_channel.available(0) == MAX_RECEIVED_UNPROCESSED_TC_IN_VIRT_BUFFER);
    ServiceChannelNotif err;
    err = serv_channel.mapp_request(0, 0);
    CHECK(err == ServiceChannelNotif::NO_SERVICE_EVENT);
    CHECK(serv_channel.available(0, 0) == MAX_RECEIVED_TC_IN_MAP_BUFFER - 2);
    CHECK(serv_channel.available(0) == MAX_RECEIVED_UNPROCESSED_TC_IN_VIRT_BUFFER - 1);

    CHECK((serv_channel.packet(0, 0).second == packet_b));

    err = serv_channel.mapp_request(0, 0);
    CHECK(err == ServiceChannelNotif::NO_SERVICE_EVENT);
    CHECK(serv_channel.available(0, 0) == MAX_RECEIVED_TC_IN_MAP_BUFFER - 1);

    CHECK((serv_channel.packet(0, 0).second == packet_c));

    err = serv_channel.mapp_request(0, 0);
    CHECK(err == ServiceChannelNotif::NO_SERVICE_EVENT);
    CHECK(serv_channel.available(0, 0) == MAX_RECEIVED_TC_IN_MAP_BUFFER);
    CHECK(serv_channel.mapp_request(0, 0) == ServiceChannelNotif::NO_PACKETS_TO_PROCESS);

    // VC Generation Service
    CHECK(serv_channel.packet(0).second == packet_a);
    CHECK(serv_channel.available(0) == MAX_RECEIVED_UNPROCESSED_TC_IN_VIRT_BUFFER - 3U);

    // Process first type-A packet
    err = serv_channel.vc_generation_request(0);
    CHECK(err == ServiceChannelNotif::NO_SERVICE_EVENT);
    CHECK(serv_channel.packet(0).second == packet_b);
    CHECK(serv_channel.available(0) == MAX_RECEIVED_UNPROCESSED_TC_IN_VIRT_BUFFER - 2U);

    // Process first type-B  packet
    err = serv_channel.vc_generation_request(0);
    CHECK(serv_channel.available(0) == MAX_RECEIVED_UNPROCESSED_TC_IN_VIRT_BUFFER - 1U);
    CHECK(err == ServiceChannelNotif::NO_SERVICE_EVENT);
    CHECK(serv_channel.packet(0).second == packet_c);

    // Process second type-A  packet
    err = serv_channel.vc_generation_request(0);
    CHECK(serv_channel.available(0) == MAX_RECEIVED_UNPROCESSED_TC_IN_VIRT_BUFFER);
    CHECK(err == ServiceChannelNotif::NO_SERVICE_EVENT);
    CHECK(serv_channel.packet(0).second == nullptr);
}
