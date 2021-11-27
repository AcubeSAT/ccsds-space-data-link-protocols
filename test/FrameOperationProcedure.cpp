#include <catch2/catch.hpp>
#include <CCSDSChannel.hpp>
#include <PacketTC.hpp>
#include <CCSDSServiceChannel.hpp>

TEST_CASE("Initiate FOP Directives") {
    PhysicalChannel phy_channel_fop = PhysicalChannel(1024, false, 12, 1024, 220000, 20);

    etl::flat_map<uint8_t, MAPChannel, MAX_MAP_CHANNELS> map_channels_fop = {
            {2, MAPChannel(2, DataFieldContent::PACKET)},
            {3, MAPChannel(3, DataFieldContent::PACKET)}};

    uint8_t data[] = {0x00, 0xDA, 0x42, 0x32, 0x43, 0x12, 0x77, 0xFA, 0x3C, 0xBB, 0x92};
    MasterChannel master_channel_fop = MasterChannel(true, 0);
    master_channel_fop.add_vc(3, true, 1024, 20, true, 32, 32, 32, map_channels_fop);

    ServiceChannel serv_channel_fop = ServiceChannel(master_channel_fop);

	serv_channel_fop.storeTC(data, 11, 3, 2, 10, ServiceType::TYPE_A);

    CHECK(serv_channel_fop.tx_available(3, 2) == MAX_RECEIVED_TC_IN_MAP_CHANNEL - 1);

    serv_channel_fop.mapp_request(3, 2);

    CHECK(serv_channel_fop.tx_available(3, 2) == MAX_RECEIVED_TC_IN_MAP_CHANNEL);

    CHECK(serv_channel_fop.fop_state(3) == FOPState::INITIAL);
    serv_channel_fop.initiate_ad_no_clcw(3);
    CHECK(serv_channel_fop.fop_state(3) == FOPState::ACTIVE);
    serv_channel_fop.terminate_ad_service(3);
    serv_channel_fop.set_vs(3, 6);
    CHECK(serv_channel_fop.expected_frame_seq_number(3) == 6);
    CHECK(serv_channel_fop.transmitter_frame_seq_number(3) == 6);
    CHECK(serv_channel_fop.fop_state(3) == FOPState::INITIAL);
    serv_channel_fop.initiate_ad_unlock(3);

    CHECK(serv_channel_fop.timeout_type(3) == 0);
    serv_channel_fop.set_timeout_type(3, 1);
    CHECK(serv_channel_fop.timeout_type(3) == 1);

    CHECK(serv_channel_fop.t1_timer(3) == FOP_TIMER_INITIAL);
    serv_channel_fop.set_t1_initial(3, 55);
    CHECK(serv_channel_fop.t1_timer(3) == 55);

    CHECK(serv_channel_fop.fop_sliding_window_width(3) == FOP_SLIDING_WINDOW_INITIAL);
    serv_channel_fop.set_fop_width(3, 100);
    CHECK(serv_channel_fop.fop_sliding_window_width(3) == 100);
}