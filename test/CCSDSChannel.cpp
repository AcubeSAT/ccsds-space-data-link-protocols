#include <catch2/catch.hpp>
#include <CCSDSChannel.hpp>
#include <TransferFrameTC.hpp>
#include <CCSDSServiceChannel.hpp>

TEST_CASE("CCSDS TC Channel Model") {
    // @todo add more and better test cases :)

    PhysicalChannel phy_channel = PhysicalChannel(1024, false, 12, 1024, 220000, 20);

    etl::flat_map<uint8_t, MAPChannel, MAX_MAP_CHANNELS> map_channels = {{2, MAPChannel(2, DataFieldContent::PACKET)},
                                                                         {3, MAPChannel(3, DataFieldContent::VCA_SDU)}};

    uint8_t data[] = {0x00, 0xDA, 0x42, 0x32, 0x43, 0x12, 0x77, 0xFA, 0x3C, 0xBB, 0x92};
    MasterChannel master_channel = MasterChannel(true, 0);
	master_channel.addVC(3, true, 1024, 20, true, 32, 32, 32, map_channels);

    CHECK(master_channel.virtChannels.at(3).VCID == 0x03);
	PhysicalChannel physical_channel = PhysicalChannel(TM_TRANSFER_FRAME_SIZE, TC_ERROR_CONTROL_FIELD_EXISTS,
	                                                   100, 50, 20000, 5);
    ServiceChannel serv_channel = ServiceChannel(std::move(master_channel), std::move(physical_channel));
}

TEST_CASE("MAPP blocking") {
    PhysicalChannel physical_channel = PhysicalChannel(TM_TRANSFER_FRAME_SIZE, TC_ERROR_CONTROL_FIELD_EXISTS,
                                                       100, 50, 20000, 5);

    etl::flat_map<uint8_t, MAPChannel, MAX_MAP_CHANNELS> map_channels = {{2, MAPChannel(2, DataFieldContent::PACKET)}};

    MasterChannel master_channel = MasterChannel(true, 0);
	master_channel.addVC(3, true, 8, 20, true, 32, 32, 32, map_channels);

    CHECK(master_channel.virtChannels.at(3).VCID == 3);
    ServiceChannel serv_channel = ServiceChannel(std::move(master_channel), std::move(physical_channel));

    uint8_t data[] = {0x00, 0x01, 0x02, 0x30, 0x40, 0x05, 0x06, 0x07, 0x80, 0x90, 0xA0};

	serv_channel.storeTC(data, 11, 3, 2, 10, ServiceType::TYPE_A);
    CHECK(serv_channel.txAvailable(3, 2) == MAX_RECEIVED_TC_IN_MAP_CHANNEL - 1);

	serv_channel.mappRequest(3, 2);

    CHECK(serv_channel.txAvailable(3) == MAX_RECEIVED_UNPROCESSED_TX_TC_IN_VIRT_BUFFER - 6);
    CHECK(serv_channel.txAvailable(3, 2) == MAX_RECEIVED_TC_IN_MAP_CHANNEL);

	ServiceChannelNotif err;
	err = serv_channel.mappRequest(3, 2);
	CHECK(err == ServiceChannelNotif::NO_TX_PACKETS_TO_PROCESS);

	uint8_t data2[] = {0x00, 0x01, 0x02, 0x30, 0x40, 0x05, 0x06, 0x07, 0x80, 0x90, 0xA0,0x01, 0x02, 0x30, 0x40, 0x05, 0x06, 0x07, 0x80, 0x90, 0xA0};

	serv_channel.store(data2, 11, 3, 2, 10, ServiceType::TYPE_A);
	CHECK(serv_channel.txAvailable(3, 2) == MAX_RECEIVED_TC_IN_MAP_CHANNEL - 1);

	err = serv_channel.mappRequest(3, 2);
	CHECK(err == ServiceChannelNotif::PACKET_EXCEEDS_MAX_SIZE);
}

TEST_CASE("Virtual Channel Generation") {}

TEST_CASE("CLCW parsing") {
    // Parse CLCW from raw data
    uint8_t clcw_data[] = {0x09, 0xA8, 0xAA, 0x52};
    CLCW clcw = CLCW(clcw_data);

    const uint8_t field_status = clcw.getFieldStatus();
    const uint8_t cop_in_effect = clcw.getCopInEffect();
    const uint8_t virtual_channel = clcw.vcid();
    const bool no_rf_avail = clcw.getNoRFAvail();
    const bool no_bit_lock = clcw.getNoBitLock();
    const bool lockout = clcw.lockout();
    const bool wait = clcw.wait();
    const bool retransmit = clcw.retransmission();
    const uint8_t farm_b_counter = clcw.getFarmBCounter();
    const uint8_t report_value = clcw.getReportValue();

    CHECK(field_status == 2);
    CHECK(cop_in_effect == 1);
    CHECK(virtual_channel == 42);
    CHECK(no_rf_avail == true);
    CHECK(no_bit_lock == false);
    CHECK(lockout == true);
    CHECK(wait == false);
    CHECK(retransmit == true);
    CHECK(farm_b_counter == 1);
    CHECK(report_value == 0x52);

    // Construct CLCW from the individual fields
    CLCW parsed_clcw = CLCW(field_status, cop_in_effect, virtual_channel, no_rf_avail, no_bit_lock, lockout, wait,
                            retransmit, farm_b_counter, report_value);

    CHECK(parsed_clcw.getFieldStatus() == clcw.getFieldStatus());
    CHECK(parsed_clcw.getCopInEffect() == clcw.getCopInEffect());
    CHECK(parsed_clcw.vcid() == clcw.vcid());
    CHECK(parsed_clcw.getNoRFAvail() == clcw.getNoRFAvail());
    CHECK(parsed_clcw.getNoBitLock() == clcw.getNoBitLock());
    CHECK(parsed_clcw.lockout() == clcw.lockout());
    CHECK(parsed_clcw.wait() == clcw.wait());
    CHECK(parsed_clcw.retransmission() == clcw.retransmission());
    CHECK(parsed_clcw.getFarmBCounter() == clcw.getFarmBCounter());
    CHECK(parsed_clcw.getReportValue() == clcw.getReportValue());
}
