#include <catch2/catch.hpp>
#include <CCSDSChannel.hpp>
#include <TransferFrameTC.hpp>
#include <TransferFrameTM.hpp>
#include <CCSDSServiceChannel.hpp>
#include <iostream>

TEST_CASE("Service Channel") {
	ServiceChannelNotification err;

	// Set up Service Channel
	PhysicalChannel phy_channel_fop = PhysicalChannel(1024, false, 12, 1024, 220000, 20);

	etl::flat_map<uint8_t, MAPChannel, MaxMapChannels> map_channels = {
	    {0, MAPChannel(0, true, true)},
	    {1, MAPChannel(1, false, false)},
	    {2, MAPChannel(2, true, false)},
	};

	MasterChannel master_channel = MasterChannel(true);
	master_channel.addVC(0, 128, true, 2, 2, true, true, true, 8, SynchronizationFlag::FORWARD_ORDERED, 255, 10,
	                     10, map_channels);

    master_channel.addVC(1, 128, false, 2, 2, true, true, true, 8, SynchronizationFlag::FORWARD_ORDERED, 20, 3,
                         3);

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

	CHECK(serv_channel.txAvailableTC(0, 0) == MaxReceivedTcInMapChannel);
	CHECK(serv_channel.txAvailableTC(0, 1) == MaxReceivedTcInMapChannel);
	CHECK(serv_channel.txAvailableTC(0, 2) == MaxReceivedTcInMapChannel);

	err = serv_channel.storeTC(pckt_type_a, 9, 8, 0, 0, ServiceType::TYPE_AD);
	CHECK(err == ServiceChannelNotification::INVALID_VC_ID);

	serv_channel.storeTC(pckt_type_a, 9, 0, 0, 0, ServiceType::TYPE_AD);
	CHECK(serv_channel.txAvailableTC(0, 0) == MaxReceivedTcInMapChannel - 1);
	const TransferFrameTC* packet_a = serv_channel.txOutPacketTC().second;
	CHECK(packet_a->getFrameLength() == 9);
	CHECK(packet_a->getServiceType() == ServiceType::TYPE_AD);
	CHECK((serv_channel.txOutPacketTC(0, 0).second == packet_a));

	serv_channel.storeTC(pckt_type_b, 10, 0, 0, 0, ServiceType::TYPE_BC);
	CHECK(serv_channel.txAvailableTC(0, 0) == MaxReceivedTcInMapChannel - 2);
	const TransferFrameTC* packet_b = serv_channel.txOutPacketTC().second;
	CHECK(packet_b->getFrameLength() == 10);
	CHECK(packet_b->getServiceType() == ServiceType::TYPE_BC);

	serv_channel.storeTC(pckt_type_a2, 3, 0, 0, 0, ServiceType::TYPE_AD);
	CHECK(serv_channel.txAvailableTC(0, 0) == MaxReceivedTcInMapChannel - 3);
	const TransferFrameTC* packet_c = serv_channel.txOutPacketTC().second;
	CHECK(packet_c->getFrameLength() == 3);
	CHECK(packet_c->getServiceType() == ServiceType::TYPE_AD);
	CHECK((serv_channel.txOutPacketTC(0, 0).second == packet_a));

	CHECK(serv_channel.txAvailableTC(0) == MaxReceivedUnprocessedTxTcInVirtBuffer);
	err = serv_channel.mappRequest(0, 0);
	CHECK(err == ServiceChannelNotification::NO_SERVICE_EVENT);
	CHECK(serv_channel.txAvailableTC(0, 0) == MaxReceivedTcInMapChannel - 2);
	CHECK(serv_channel.txAvailableTC(0) == MaxReceivedUnprocessedTxTcInVirtBuffer - 1);

	CHECK((serv_channel.txOutPacketTC(0, 0).second == packet_b));

	err = serv_channel.mappRequest(0, 0);
	CHECK(err == ServiceChannelNotification::NO_SERVICE_EVENT);
	CHECK(serv_channel.txAvailableTC(0, 0) == MaxReceivedTcInMapChannel - 1);

	CHECK((serv_channel.txOutPacketTC(0, 0).second == packet_c));

	err = serv_channel.mappRequest(0, 0);
	CHECK(err == ServiceChannelNotification::NO_SERVICE_EVENT);
	CHECK(serv_channel.txAvailableTC(0, 0) == MaxReceivedTcInMapChannel);
	CHECK(serv_channel.mappRequest(0, 0) == ServiceChannelNotification::NO_TX_PACKETS_TO_PROCESS);

	// VC Generation Service
	CHECK(serv_channel.txOutPacketTC(0).second == packet_a);
	CHECK(serv_channel.txAvailableTC(0) == MaxReceivedUnprocessedTxTcInVirtBuffer - 3U);

	// Process first type-A packet
	err = serv_channel.vcGenerationRequestTC(0);
	CHECK(err == ServiceChannelNotification::NO_SERVICE_EVENT);
	CHECK(serv_channel.txOutPacketTC(0).second == packet_b);
	CHECK(serv_channel.txAvailableTC(0) == MaxReceivedUnprocessedTxTcInVirtBuffer - 2U);
	err = serv_channel.pushSentQueue(0);

	// All Frames Generation Service
	err = serv_channel.allFramesGenerationTCRequest();
	CHECK(err == ServiceChannelNotification::NO_SERVICE_EVENT);

	TransferFrameTC packet = *serv_channel.getTxProcessedPacket();

	CHECK(serv_channel.txOutProcessedPacketTC().second == packet_a);

	CHECK(packet_a->acknowledged() == false);
	CHECK(packet_a->transferFrameSequenceNumber() == 4);
	serv_channel.acknowledgeFrame(0, 0);

	CHECK(err == ServiceChannelNotification::NO_SERVICE_EVENT);

	// Process first type-B packet
	err = serv_channel.vcGenerationRequestTC(0);
	CHECK(serv_channel.txAvailableTC(0) == MaxReceivedUnprocessedTxTcInVirtBuffer - 1U);
	CHECK(err == ServiceChannelNotification::NO_SERVICE_EVENT);
	CHECK(serv_channel.txOutPacketTC(0).second == packet_c);

	// Process second type-A packet
	err = serv_channel.vcGenerationRequestTC(0);
	CHECK(serv_channel.txAvailableTC(0) == MaxReceivedUnprocessedTxTcInVirtBuffer);
	CHECK(err == ServiceChannelNotification::NO_SERVICE_EVENT);

	// Rx side
	// new packet
	uint8_t packet1[] = {0x10, 0xB1, 0x00, 0x0A, 0x00, 0x00, 0x00, 0x1C, 0x21, 0x33};
    uint8_t packet2[] = {0x10, 0xB4, 0x04, 0x0A, 0x00, 0x00, 0x00, 0x1C, 0x21, 0x33};
	uint8_t out_buffer[10] = {0};

	serv_channel.storeTC(packet1, 10);
    serv_channel.storeTC(packet2, 10);

	// All frames reception
	CHECK(serv_channel.getAvailableWaitQueueRxTC(0) == MaxReceivedTxTcInWaitQueue);
	err = serv_channel.allFramesReceptionTCRequest();
	CHECK(err == ServiceChannelNotification::NO_SERVICE_EVENT);
	CHECK(serv_channel.getAvailableWaitQueueRxTC(0) == MaxReceivedTxTcInWaitQueue - 1);

    CHECK(serv_channel.getAvailableWaitQueueRxTC(1) == MaxReceivedTxTcInWaitQueue);
    err = serv_channel.allFramesReceptionTCRequest();
    CHECK(err == ServiceChannelNotification::NO_SERVICE_EVENT);
    CHECK(serv_channel.getAvailableWaitQueueRxTC(1) == MaxReceivedTxTcInWaitQueue - 1);

    // VC reception
	CHECK(serv_channel.getAvailableRxInFramesAfterVCReception(0) == MaxReceivedRxTcInMasterBuffer);
	err = serv_channel.vcReceptionTC(0);
	CHECK(err == ServiceChannelNotification::NO_SERVICE_EVENT);
	CHECK(serv_channel.getAvailableWaitQueueRxTC(0) == MaxReceivedTxTcInWaitQueue);
	CHECK(serv_channel.getAvailableRxInFramesAfterVCReception(0) == MaxReceivedRxTcInVirtualChannelBuffer);
	CHECK(serv_channel.getAvailableRxInFramesAfterVCReception(0, 0) == MaxReceivedRxTcInMAPBuffer - 1);

    CHECK(serv_channel.getAvailableRxInFramesAfterVCReception(1) == MaxReceivedRxTcInMasterBuffer);
    err = serv_channel.vcReceptionTC(1);
    CHECK(err == ServiceChannelNotification::NO_SERVICE_EVENT);
    CHECK(serv_channel.getAvailableWaitQueueRxTC(1) == MaxReceivedTxTcInWaitQueue);
    CHECK(serv_channel.getAvailableRxInFramesAfterVCReception(1) == MaxReceivedRxTcInVirtualChannelBuffer - 1);

	// Packet extraction
	err = serv_channel.packetExtractionTC(0, 0, out_buffer);
	CHECK(err == ServiceChannelNotification::NO_SERVICE_EVENT);
	CHECK(out_buffer[0] == 0x00);
    CHECK(out_buffer[1] == 0x00);
    CHECK(out_buffer[2] == 0x1C);

	/**
	 * the next commented lines are duplicated (line 93) . I don't know why. It won't work if uncommented
	 */
	//    // All Frames Generation Service
	//    CHECK(serv_channel.tx_out_processed_packet().second == nullptr);
	//    err = serv_channel.all_frames_generation_request();
	//    CHECK(err == ServiceChannelNotif::NO_SERVICE_EVENT);
	//
	//    //TransferFrameTC packet = *serv_channel.get_tx_processed_packet();
	//
	//    CHECK(serv_channel.tx_out_processed_packet().second == packet_a);

	// TM store function

	/*
	uint8_t pckt_TM[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0xA2, 0xB3, 0x21, 0xA1};

	serv_channel.storeTM(pckt_TM, 9, 0, 0);

	const TransferFrameTM *packet_TM = serv_channel.txOutPacketTM().second;
	CHECK(packet_TM->getPacketLength() == 9);
	CHECK(packet_a->getTransferFrameVersionNumber() == 0);
	CHECK(packet_TM->getMasterChannelFrameCount() == 0);
	CHECK(packet_TM->getVirtualChannelFrameCount() == 2);
	CHECK(packet_TM->getTransferFrameDataFieldStatus() ==
	      (static_cast<uint16_t>(0x04) << 8U | (static_cast<uint16_t>(0xA2))));

	// All Frames Generation Service TM
	CHECK(serv_channel.txOutProcessedPacketTM().second == nullptr);
	*/

	uint8_t valid_pckt_TM[] = {0x00, 0x01, 0x00, 0x03, 0x04, 0xA2, 0xB3, 0x1F, 0xD6, 0xA2, 0xB3, 0x1F, 0x7B, 0x7C};
	uint8_t invalid_vcid_TM[] = {0x00, 0x0F, 0x00, 0x03, 0x04, 0xA2, 0xB3, 0x1F, 0xD6, 0xA2, 0xB3, 0x1F, 0x7B, 0x7C};
	uint8_t invalid_crc_TM[] = {0x00, 0x01, 0x00, 0x03, 0x04, 0xA2, 0xB3, 0x5B, 0x54, 0xA2, 0xB3, 0x1F, 0xD6, 0x01};

	// TM Reception

	// TODO: This should take the output of TM RX called previously. Proper functional tests should include two service
	//  channels to simulate communication between GS and SC

	CHECK(serv_channel.rxInAvailableTM(0) == MaxReceivedRxTmInVirtBuffer);
	CHECK(serv_channel.availableSpaceBufferTxTM() == MaxTxInMasterChannel - 0);

	err = serv_channel.allFramesReceptionTMRequest(valid_pckt_TM, 14);
	CHECK(serv_channel.rxInAvailableTM(0) == MaxReceivedRxTmInVirtBuffer - 1);
	CHECK(serv_channel.availableSpaceBufferRxTM() == MaxTxInMasterChannel - 1);

	err = serv_channel.allFramesReceptionTMRequest(invalid_vcid_TM, 14);
	CHECK(err == ServiceChannelNotification::INVALID_VC_ID);
	CHECK(serv_channel.rxInAvailableTM(0) == MaxReceivedRxTmInVirtBuffer - 1);
	CHECK(serv_channel.availableSpaceBufferRxTM() == MaxRxInMasterChannel - 1);

	err = serv_channel.allFramesReceptionTMRequest(invalid_crc_TM, 14);
	CHECK(err == ServiceChannelNotification::RX_INVALID_CRC);
	CHECK(serv_channel.rxInAvailableTM(0) == MaxReceivedRxTmInVirtBuffer - 1);
	CHECK(serv_channel.availableSpaceBufferRxTM() == MaxRxInMasterChannel - 1);

	uint8_t resulting_tm_packet[14] = {0};

	err = serv_channel.packetExtractionTM(0, resulting_tm_packet);
	// Valid packet passes to lower procedures
	CHECK(err == ServiceChannelNotification::NO_SERVICE_EVENT);
	CHECK(serv_channel.rxInAvailableTM(0) == MaxReceivedRxTmInVirtBuffer - 0);
	CHECK(serv_channel.availableSpaceBufferRxTM() == MaxTxInMasterChannel - 0);

	// TM Transmission
	CHECK(serv_channel.getFrameCountTM(0) == 0);
	uint8_t pck_tm_data[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x02, 0x03, 0x04, 0xA2, 0xB3, 0x5B, 0x55};
	err = serv_channel.storeTM(pck_tm_data, 15, 0);
	CHECK(err == ServiceChannelNotification::NO_SERVICE_EVENT);
	CHECK(serv_channel.availableMcTxTM() == MaxReceivedUnprocessedTxTmInVirtBuffer - 1);

	const TransferFrameTM* packet_tm_mc = serv_channel.packetMasterChannel();
	CHECK(serv_channel.getFrameCountTM(0) == 1);

	CHECK(packet_tm_mc->packetData()[0] == 0x06);
	CHECK(packet_tm_mc->packetData()[1] == 0x71);
	CHECK(packet_tm_mc->packetData()[2] == 0x00);
	CHECK(packet_tm_mc->packetData()[3] == 0x00);
}
