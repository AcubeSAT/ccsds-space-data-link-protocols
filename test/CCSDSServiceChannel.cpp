#include <catch2/catch.hpp>
#include <CCSDSChannel.hpp>
#include <Packet.hpp>
#include <CCSDSServiceChannel.hpp>
#include <iostream>

TEST_CASE("Service Channel") {
    // Set up Service Channel
    PhysicalChannel phy_channel_fop = PhysicalChannel(1024, false, 12,
                                                      1024, 220000, 20);

    etl::flat_map<uint8_t, MAPChannel, max_map_channels> map_channels = {
            {0, MAPChannel(0, DataFieldContent::PACKET)},
            {1, MAPChannel(1, DataFieldContent::PACKET)},
            {2, MAPChannel(2, DataFieldContent::PACKET)},
    };

    MasterChannel master_channel = MasterChannel(true);
    master_channel.add_vc(0, true, 128, 20,
                          true, 2, 2, map_channels);

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

    CHECK(serv_channel.tx_available(0, 0) == max_received_tc_in_map_channel);
    CHECK(serv_channel.tx_available(0, 1) == max_received_tc_in_map_channel);
    CHECK(serv_channel.tx_available(0, 2) == max_received_tc_in_map_channel);

    serv_channel.store(pckt_type_a, 9, 0, 0, 0, ServiceType::TYPE_A);
    CHECK(serv_channel.tx_available(0, 0) == max_received_tc_in_map_channel - 1);
    const Packet *packet_a = serv_channel.tx_out_packet().second;
    CHECK(packet_a->packet_length() == 9);
    CHECK(packet_a->service_type() == ServiceType::TYPE_A);
    CHECK((serv_channel.out_packet(0, 0).second == packet_a));

    serv_channel.store(pckt_type_b, 10, 0, 0, 0, ServiceType::TYPE_B);
    CHECK(serv_channel.tx_available(0, 0) == max_received_tc_in_map_channel - 2);
    const Packet *packet_b = serv_channel.tx_out_packet().second;
    CHECK(packet_b->packet_length() == 10);
    CHECK(packet_b->service_type() == ServiceType::TYPE_B);

    serv_channel.store(pckt_type_a2, 3, 0, 0, 0, ServiceType::TYPE_A);
    CHECK(serv_channel.tx_available(0, 0) == max_received_tc_in_map_channel - 3);
    const Packet *packet_c = serv_channel.tx_out_packet().second;
    CHECK(packet_c->packet_length() == 3);
    CHECK(packet_c->service_type() == ServiceType::TYPE_A);
    CHECK((serv_channel.out_packet(0, 0).second == packet_a));

    CHECK(serv_channel.tx_available(0) == max_received_unprocessed_tc_in_virt_buffer);
    ServiceChannelNotif err;
    err = serv_channel.mapp_request(0, 0);
    CHECK(err == ServiceChannelNotif::NO_SERVICE_EVENT);
    CHECK(serv_channel.tx_available(0, 0) == max_received_tc_in_map_channel - 2);
    CHECK(serv_channel.tx_available(0) == max_received_unprocessed_tc_in_virt_buffer - 1);

    CHECK((serv_channel.out_packet(0, 0).second == packet_b));

    err = serv_channel.mapp_request(0, 0);
    CHECK(err == ServiceChannelNotif::NO_SERVICE_EVENT);
    CHECK(serv_channel.tx_available(0, 0) == max_received_tc_in_map_channel - 1);

    CHECK((serv_channel.out_packet(0, 0).second == packet_c));

    err = serv_channel.mapp_request(0, 0);
    CHECK(err == ServiceChannelNotif::NO_SERVICE_EVENT);
    CHECK(serv_channel.tx_available(0, 0) == max_received_tc_in_map_channel);
    CHECK(serv_channel.mapp_request(0, 0) == ServiceChannelNotif::NO_PACKETS_TO_PROCESS);

    // VC Generation Service
    CHECK(serv_channel.tx_out_packet(0).second == packet_a);
    CHECK(serv_channel.tx_available(0) == max_received_unprocessed_tc_in_virt_buffer - 3U);

    // Process first type-A packet
    err = serv_channel.vc_generation_request(0);
    CHECK(err == ServiceChannelNotif::NO_SERVICE_EVENT);
    CHECK(serv_channel.tx_out_packet(0).second == packet_b);
    CHECK(serv_channel.tx_available(0) == max_received_unprocessed_tc_in_virt_buffer - 2U);

    // Process first type-B  packet
    err = serv_channel.vc_generation_request(0);
    CHECK(serv_channel.tx_available(0) == max_received_unprocessed_tc_in_virt_buffer - 1U);
    CHECK(err == ServiceChannelNotif::NO_SERVICE_EVENT);
    CHECK(serv_channel.tx_out_packet(0).second == packet_c);

    // Process second type-A  packet
    err = serv_channel.vc_generation_request(0);
    CHECK(serv_channel.tx_available(0) == max_received_unprocessed_tc_in_virt_buffer);
    CHECK(err == ServiceChannelNotif::NO_SERVICE_EVENT);
    CHECK(serv_channel.tx_out_packet(0).second == nullptr);
}
