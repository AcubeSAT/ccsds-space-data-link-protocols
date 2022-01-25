#include <catch2/catch.hpp>
#include <CCSDSChannel.hpp>
#include <TransferFrameTC.hpp>
#include <TransferFrameTM.hpp>
#include <CCSDSServiceChannel.hpp>
#include <iostream>

TEST_CASE("Service Channel") {
    // Set up Service Channel
    PhysicalChannel phy_channel_fop = PhysicalChannel(1024, false, 12, 1024, 220000, 20);

    etl::flat_map<uint8_t, MAPChannel, MAX_MAP_CHANNELS> map_channels = {
            {0, MAPChannel(0, DataFieldContent::PACKET)},
            {1, MAPChannel(1, DataFieldContent::PACKET)},
            {2, MAPChannel(2, DataFieldContent::PACKET)},
    };

    MasterChannel master_channel = MasterChannel(true, 0);
	master_channel.addVC(0, true, 128, 20, true, 2, 2, 2, map_channels);

    ServiceChannel serv_channel = ServiceChannel(master_channel, phy_channel_fop);

//    memset(reinterpret_cast<uint8_t*>(&master_channel), 0xff, sizeof(master_channel));

    // TODO: Test getters (once implemented)

    // MAPP Generation Service

    uint8_t pckt_type_a[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0xA2, 0xB3, 0x21, 0xA1};
    uint8_t pckt_type_b[] = {0x10, 0xB1, 0x00, 0x03, 0x08, 0xA5, 0x15, 0x1C, 0x21, 0X40};
    uint8_t pckt_type_a2[] = {0xE1, 0x32, 0x12};

    // Initialize service
    CHECK(serv_channel.fopState(0) == FOPState::INITIAL);
	serv_channel.initiateAdNoClcw(0);
    CHECK(serv_channel.fopState(0) == FOPState::ACTIVE);

    CHECK(serv_channel.txAvailable(0, 0) == MAX_RECEIVED_TC_IN_MAP_CHANNEL);
    CHECK(serv_channel.txAvailable(0, 1) == MAX_RECEIVED_TC_IN_MAP_CHANNEL);
    CHECK(serv_channel.txAvailable(0, 2) == MAX_RECEIVED_TC_IN_MAP_CHANNEL);

	serv_channel.storeTC(pckt_type_a, 9, 0, 0, 0, ServiceType::TYPE_A);
    CHECK(serv_channel.txAvailable(0, 0) == MAX_RECEIVED_TC_IN_MAP_CHANNEL - 1);
    const TransferFrameTC *packet_a = serv_channel.txOutPacketTC().second;
    CHECK(packet_a->getTransferFrameLength() == 9);
    CHECK(packet_a->getServiceType() == ServiceType::TYPE_A);
    CHECK((serv_channel.txOutPacket(0, 0).second == packet_a));

	serv_channel.storeTC(pckt_type_b, 10, 0, 0, 0, ServiceType::TYPE_B);
    CHECK(serv_channel.txAvailable(0, 0) == MAX_RECEIVED_TC_IN_MAP_CHANNEL - 2);
    const TransferFrameTC *packet_b = serv_channel.txOutPacketTC().second;
    CHECK(packet_b->getTransferFrameLength() == 10);
    CHECK(packet_b->getServiceType() == ServiceType::TYPE_B);

	serv_channel.storeTC(pckt_type_a2, 3, 0, 0, 0, ServiceType::TYPE_A);
    CHECK(serv_channel.txAvailable(0, 0) == MAX_RECEIVED_TC_IN_MAP_CHANNEL - 3);
    const TransferFrameTC *packet_c = serv_channel.txOutPacketTC().second;
    CHECK(packet_c->getTransferFrameLength() == 3);
    CHECK(packet_c->getServiceType() == ServiceType::TYPE_A);
    CHECK((serv_channel.txOutPacket(0, 0).second == packet_a));

    CHECK(serv_channel.txAvailable(0) == MAX_RECEIVED_UNPROCESSED_TX_TC_IN_VIRT_BUFFER);
	ServiceChannelNotification err;
    err = serv_channel.mappRequest(0, 0);
    CHECK(err == ServiceChannelNotification::NO_SERVICE_EVENT);
    CHECK(serv_channel.txAvailable(0, 0) == MAX_RECEIVED_TC_IN_MAP_CHANNEL - 2);
    CHECK(serv_channel.txAvailable(0) == MAX_RECEIVED_UNPROCESSED_TX_TC_IN_VIRT_BUFFER - 1);

    CHECK((serv_channel.txOutPacket(0, 0).second == packet_b));

    err = serv_channel.mappRequest(0, 0);
    CHECK(err == ServiceChannelNotification::NO_SERVICE_EVENT);
    CHECK(serv_channel.txAvailable(0, 0) == MAX_RECEIVED_TC_IN_MAP_CHANNEL - 1);

    CHECK((serv_channel.txOutPacket(0, 0).second == packet_c));

    err = serv_channel.mappRequest(0, 0);
    CHECK(err == ServiceChannelNotification::NO_SERVICE_EVENT);
    CHECK(serv_channel.txAvailable(0, 0) == MAX_RECEIVED_TC_IN_MAP_CHANNEL);
    CHECK(serv_channel.mappRequest(0, 0) == ServiceChannelNotification::NO_TX_PACKETS_TO_PROCESS);

    // VC Generation Service
    CHECK(serv_channel.txOutPacket(0).second == packet_a);
    CHECK(serv_channel.txAvailable(0) == MAX_RECEIVED_UNPROCESSED_TX_TC_IN_VIRT_BUFFER - 3U);

    // Process first type-A packet
    err = serv_channel.vcGenerationRequest(0);
    CHECK(err == ServiceChannelNotification::NO_SERVICE_EVENT);
    CHECK(serv_channel.txOutPacket(0).second == packet_b);
    CHECK(serv_channel.txAvailable(0) == MAX_RECEIVED_UNPROCESSED_TX_TC_IN_VIRT_BUFFER - 2U);
    err = serv_channel.pushSentQueue(0);

    // All Frames Generation Service
//    CHECK(serv_channel.tx_out_processed_packet().second == nullptr);
    err = serv_channel.allFramesGenerationRequest();
    CHECK(err == ServiceChannelNotification::NO_SERVICE_EVENT);

	TransferFrameTC packet = *serv_channel.getTxProcessedPacket();

    CHECK(serv_channel.txOutProcessedPacket().second == packet_a);

    CHECK(packet_a->acknowledged() == false);
    CHECK(packet_a->transferFrameSequenceNumber() == 0);
	serv_channel.acknowledgeFrame(0, 0);
    CHECK(packet_a->acknowledged() == true);

    CHECK(err == ServiceChannelNotification::NO_SERVICE_EVENT);
    // Process first type-B packet
    err = serv_channel.vcGenerationRequest(0);
    CHECK(serv_channel.txAvailable(0) == MAX_RECEIVED_UNPROCESSED_TX_TC_IN_VIRT_BUFFER - 1U);
    CHECK(err == ServiceChannelNotification::NO_SERVICE_EVENT);
    CHECK(serv_channel.txOutPacket(0).second == packet_c);


    // Process second type-A packet
    err = serv_channel.vcGenerationRequest(0);
    CHECK(serv_channel.txAvailable(0) == MAX_RECEIVED_UNPROCESSED_TX_TC_IN_VIRT_BUFFER);
    CHECK(err == ServiceChannelNotification::NO_SERVICE_EVENT);

	/**
	 * the next commented lines are duplicated (line 93) . I don't know why. It won't work if uncommented
	 */
//    // All Frames Generation Service
//    CHECK(serv_channel.tx_out_processed_packet().second == nullptr);
//    err = serv_channel.all_frames_generation_request();
//    CHECK(err == ServiceChannelNotif::NO_SERVICE_EVENT);
//
//    //PacketTC packet = *serv_channel.get_tx_processed_packet();
//
//    CHECK(serv_channel.tx_out_processed_packet().second == packet_a);

    //TM store function
    uint8_t pckt_TM[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0xA2, 0xB3, 0x21, 0xA1};

	serv_channel.storeTM(pckt_TM, 9, 0, 0);

    const TransferFrameTM *transfer_frame_TM = serv_channel.txOutTransferFrameTM().second;
    CHECK(transfer_frame_TM->getTransferFrameLength() == 9);
    CHECK(packet_a->getTransferFrameVersionNumber() == 0);
    CHECK(transfer_frame_TM->getMasterChannelFrameCount() == 0);
    CHECK(transfer_frame_TM->getVirtualChannelFrameCount() == 2);
    CHECK(transfer_frame_TM->getTransferFrameDataFieldStatus() ==
          (static_cast<uint16_t>(0x04) << 8U | (static_cast<uint16_t>(0xA2))));

}
