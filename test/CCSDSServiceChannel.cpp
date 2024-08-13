//#include <catch2/catch.hpp>
#include <catch2/catch_test_macros.hpp>
#include <CCSDSChannel.hpp>
#include <TransferFrameTC.hpp>
#include <TransferFrameTM.hpp>
#include <CCSDSServiceChannel.hpp>
#include <iostream>

TEST_CASE("Service Channel") {
	ServiceChannelNotification err;

	// Set up Service Channel
	PhysicalChannel phy_channel_fop = PhysicalChannel(1024, 12, 1024, 220000, 20);

	etl::flat_map<uint8_t, MAPChannel, MaxMapChannels> map_channels = {
	    {0, MAPChannel(0, true, true)},
	    {1, MAPChannel(1, false, false)},
	    {2, MAPChannel(2, true, false)},
	};

	MasterChannel master_channel = MasterChannel();
	master_channel.addVC(0, 128, true, 2, 2, true, true, true, 8, SynchronizationFlag::FORWARD_ORDERED, 255, 10, 10, 3,
	                     map_channels);

	master_channel.addVC(1, 128, false, 2, 2, true, true, true, true, SynchronizationFlag::FORWARD_ORDERED, 20, 3, 3, 3);

	master_channel.addVC(2, 128, false, 2, 2, false, true, true, true, SynchronizationFlag::FORWARD_ORDERED, 20, 3, 3, 3,
	                     map_channels);

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

	err = serv_channel.storePacketTxTC(pckt_type_a, 9, 5, 0, ServiceType::TYPE_AD);
	CHECK(err == ServiceChannelNotification::INVALID_VC_ID);

    serv_channel.storePacketTxTC(pckt_type_a, 9, 0, 0, ServiceType::TYPE_AD);
    serv_channel.storePacketTxTC(pckt_type_b, 10, 0, 0, ServiceType::TYPE_BD);

    serv_channel.storePacketTxTC(pckt_type_a2, 3, 0, 0, ServiceType::TYPE_AD);

	CHECK(serv_channel.availableUnprocessedFramesVcCopyTxTC(0) == MaxReceivedUnprocessedTxTcInVirtBuffer);
	err = serv_channel.mappRequestTxTC(0, 0, 9, ServiceType::TYPE_AD);
    const TransferFrameTC* frame_a = serv_channel.backUnprocessedFrameMcCopyTxTC().second;

    CHECK(err == ServiceChannelNotification::NO_SERVICE_EVENT);
	CHECK(serv_channel.availableUnprocessedFramesVcCopyTxTC(0) == MaxReceivedUnprocessedTxTcInVirtBuffer - 1);

	err = serv_channel.mappRequestTxTC(0, 0, 10, ServiceType::TYPE_BD);
    const TransferFrameTC* frame_c = serv_channel.backUnprocessedFrameMcCopyTxTC().second;
    CHECK(frame_c->getFrameLength() == 17);
    CHECK(frame_c->getServiceType() == ServiceType::TYPE_BD);
    CHECK(err == ServiceChannelNotification::NO_SERVICE_EVENT);
    CHECK(serv_channel.frontUnprocessedFrameVcCopyTxTC(0).second == frame_a);

    // VC Generation Service
	CHECK(serv_channel.availableUnprocessedFramesVcCopyTxTC(0) == MaxReceivedUnprocessedTxTcInVirtBuffer - 2U);

	// Process first type-A transfer frame
	err = serv_channel.vcGenerationRequestTxTC(0);
    CHECK(serv_channel.frontUnprocessedFrameVcCopyTxTC(0).second == frame_c);
	CHECK(err == ServiceChannelNotification::NO_SERVICE_EVENT);
	CHECK(serv_channel.availableUnprocessedFramesVcCopyTxTC(0) == MaxReceivedUnprocessedTxTcInVirtBuffer - 1U);
	err = serv_channel.pushSentQueue(0);

	// All Frames Generation Service
	err = serv_channel.allFramesGenerationRequestTxTC();
	CHECK(err == ServiceChannelNotification::NO_SERVICE_EVENT);

	TransferFrameTC transferFrameTc = *serv_channel.frontFrameBeforeAllFramesGenerationTxTC();

	CHECK(serv_channel.frontFrameAfterAllFramesGenerationTxTC().second == frame_a);

	CHECK(frame_a->acknowledged() == false);
	CHECK(frame_a->transferFrameSequenceNumber() == 0);
	serv_channel.acknowledgeFrame(0, 0);

	CHECK(err == ServiceChannelNotification::NO_SERVICE_EVENT);

	// Process first type-B transfer frame
	err = serv_channel.vcGenerationRequestTxTC(0);

	// Try to process extra type-A transfer frame
	err = serv_channel.vcGenerationRequestTxTC(0);
	CHECK(serv_channel.availableUnprocessedFramesVcCopyTxTC(0) == MaxReceivedUnprocessedTxTcInVirtBuffer);
	CHECK(err == ServiceChannelNotification::NO_TX_PACKETS_TO_PROCESS);

	// Rx side
	// new transferFrameData
	uint8_t frame1[] = {0x10, 0xB1, 0x00, 0x0A, 0x00, 0x00, 0x00, 0x1C, 0xD3, 0x8C};
	uint8_t frame2[] = {0x10, 0xB4, 0x04, 0x0A, 0x00, 0xAE, 0x3B, 0xC8, 0x58, 0x81};
	uint8_t out_buffer[10] = {0};

    serv_channel.storeFrameRxTC(frame1, 10);
    serv_channel.storeFrameRxTC(frame2, 10);

	// All frames reception
	CHECK(serv_channel.getAvailableWaitQueueRxTC(0) == MaxReceivedTxTcInWaitQueue);
	err = serv_channel.allFramesReceptionRequestRxTC();
	CHECK(err == ServiceChannelNotification::NO_SERVICE_EVENT);
	CHECK(serv_channel.getAvailableWaitQueueRxTC(0) == MaxReceivedTxTcInWaitQueue - 1);

	CHECK(serv_channel.getAvailableWaitQueueRxTC(1) == MaxReceivedTxTcInWaitQueue);
	err = serv_channel.allFramesReceptionRequestRxTC();
	CHECK(err == ServiceChannelNotification::NO_SERVICE_EVENT);
	CHECK(serv_channel.getAvailableWaitQueueRxTC(1) == MaxReceivedTxTcInWaitQueue - 1);

	// VC reception
	CHECK(serv_channel.getAvailableInFramesAfterVCReceptionRxTC(0) == MaxReceivedRxTcInMasterBuffer);
	err = serv_channel.vcReceptionRxTC(0);
	CHECK(err == ServiceChannelNotification::NO_SERVICE_EVENT);
	CHECK(serv_channel.getAvailableWaitQueueRxTC(0) == MaxReceivedTxTcInWaitQueue);
	CHECK(serv_channel.getAvailableInFramesAfterVCReceptionRxTC(0) == MaxReceivedRxTcInVirtualChannelBuffer);
	CHECK(serv_channel.getAvailableInFramesAfterVCReceptionRxTC(0, 0) == MaxReceivedRxTcInMAPBuffer - 1);

	CHECK(serv_channel.getAvailableInFramesAfterVCReceptionRxTC(1) == MaxReceivedRxTcInMasterBuffer);
	err = serv_channel.vcReceptionRxTC(1);
	CHECK(err == ServiceChannelNotification::NO_SERVICE_EVENT);
	CHECK(serv_channel.getAvailableWaitQueueRxTC(1) == MaxReceivedTxTcInWaitQueue);
	CHECK(serv_channel.getAvailableInFramesAfterVCReceptionRxTC(1) == MaxReceivedRxTcInVirtualChannelBuffer - 1);

	// Packet extraction
	err = serv_channel.packetExtractionTC(0, 0, out_buffer);
	CHECK(err == ServiceChannelNotification::NO_SERVICE_EVENT);

	CHECK(out_buffer[0] == 0x00);
	CHECK(out_buffer[1] == 0x1C);

	err = serv_channel.packetExtractionTC(1, 0, out_buffer);
	CHECK(err == ServiceChannelNotification::INVALID_SERVICE_CALL);

	err = serv_channel.packetExtractionTC(1, out_buffer);
	CHECK(err == ServiceChannelNotification::NO_SERVICE_EVENT);

	CHECK(out_buffer[0] == 0xAE);
	CHECK(out_buffer[1] == 0x3B);
	CHECK(out_buffer[2] == 0xC8);

	/**
	 * the next commented lines are duplicated (line 93) . I don't know why. It won't work if uncommented
	 */
	//    // All Frames Generation Service
	//    CHECK(serv_channel.tx_out_processed_packet().second == nullptr);
	//    err = serv_channel.all_frames_generation_request();
	//    CHECK(err == ServiceChannelNotif::NO_SERVICE_EVENT);
	//
	//    //TransferFrameTC transferFrameData = *serv_channel.get_tx_processed_packet();
	//
	//    CHECK(serv_channel.tx_out_processed_packet().second == frame_a);

	// TM store function

	/*
	uint8_t pckt_TM[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0xA2, 0xB3, 0x21, 0xA1};

	serv_channel.storeTM(pckt_TM, 9, 0, 0);

	const TransferFrameTM *packet_TM = serv_channel.backFrameAfterVcGenerationTxTM().second;
	CHECK(packet_TM->getFrameLength() == 9);
	CHECK(frame_a->getTransferFrameVersionNumber() == 0);
	CHECK(packet_TM->getMasterChannelFrameCount() == 0);
	CHECK(packet_TM->getVirtualChannelFrameCount() == 2);
	CHECK(packet_TM->getTransferFrameDataFieldStatus() ==
	      (static_cast<uint16_t>(0x04) << 8U | (static_cast<uint16_t>(0xA2))));

	// All Frames Generation Service TM
	CHECK(serv_channel.frontFrameAfterAllFramesGenerationTxTM().second == nullptr);
	*/

	uint8_t vaild_frame_TM[128] = {0x00, 0x01, 0x00, 0x03, 0x04, 0xA2, 0xB3, 0x1F, 0xD6, 0xA2, 0xB3, 0x1F, 0x7B, 0x7C};
	uint8_t len = 14;
	for (uint16_t i = 0; i < TmTransferFrameSize-len-2; i++) {
		vaild_frame_TM[i+14]= idle_data[i];
	}
	uint16_t crc = 36061;
	vaild_frame_TM[TmTransferFrameSize-2] = (crc >> 8) & 0xFF;
	vaild_frame_TM[TmTransferFrameSize - 1] = crc & 0xFF;

	uint8_t invalid_vcid_TM[TmTransferFrameSize] = {0x00, 0x0F, 0x00, 0x03, 0x04, 0xA2, 0xB3, 0x1F, 0xD6, 0xA2, 0xB3, 0x1F, 0x7B, 0x7C};
	uint8_t invalid_crc_TM[TmTransferFrameSize] = {0x00, 0x01, 0x00, 0x03, 0x04, 0xA2, 0xB3, 0x5B, 0x54, 0xA2, 0xB3, 0x1F, 0xD6, 0x01};

	// Packets that carry a CLCW
	uint8_t valid_no_crc_frame_TM[TmTransferFrameSize] = {0x00, 0x05, 0x00, 0x03, 0x04, 0xA2, 0xB3, 0x5B, 0x54, 0xA2, 0xC7, 0x0};
	uint8_t valid_no_crc_frame_TM2[TmTransferFrameSize] = {0x00, 0x05, 0x00, 0x03, 0x04, 0xA2, 0xB3, 0x5B, 0x54, 0xA2, 0xD0, 0x0};
	uint8_t valid_no_crc_frame_TM3[TmTransferFrameSize] = {0x00, 0x05, 0x00, 0x03, 0x04, 0xA2, 0xB3, 0x5B, 0x54, 0xA2, 0xD8, 0x0};
	uint8_t valid_no_crc_frame_TM4[TmTransferFrameSize] = {0x00, 0x05, 0x00, 0x03, 0x04, 0xA2, 0xB3, 0x5B, 0x54, 0xA2, 0xC7, 0x01};
	uint8_t valid_no_crc_frame_TM5[TmTransferFrameSize] = {0x00, 0x05, 0x00, 0x03, 0x04, 0xA2, 0xB3, 0x5B, 0x54, 0xA2, 0xD0, 0x01};
	uint8_t valid_no_crc_frame_TM6[TmTransferFrameSize] = {0x00, 0x05, 0x00, 0x03, 0x04, 0xA2, 0xB3, 0x5B, 0x54, 0xA2, 0xD8, 0x01};
	uint8_t valid_no_crc_frame_TM7[TmTransferFrameSize] = {0x00, 0x05, 0x00, 0x03, 0x04, 0xA2, 0xB3, 0x5B, 0x54, 0xA2, 0xC8, 0x01};
	uint8_t valid_no_crc_frame_TM8[TmTransferFrameSize] = {0x00, 0x05, 0x00, 0x03, 0x04, 0xA2, 0xB3, 0x5B, 0x54, 0xA2, 0xC7, 0x04};
	uint8_t valid_no_crc_frame_TM9[TmTransferFrameSize] = {0x00, 0x05, 0x00, 0x03, 0x04, 0xA2, 0xB3, 0x5B, 0x54, 0xA2, 0xC7, 0x05};
	uint8_t valid_no_crc_frame_TM10[TmTransferFrameSize] = {0x00, 0x05, 0x00, 0x03, 0x04, 0xA2, 0xB3, 0x5B, 0x54, 0xA2, 0xC8, 0x05};
	uint8_t valid_no_crc_frame_TM11[TmTransferFrameSize] = {0x00, 0x05, 0x00, 0x03, 0x04, 0xA2, 0xB3, 0x5B, 0x54, 0xA2, 0xD8, 0x05};

	// TM Reception

	// TODO: This should take the output of TM RX called previously. Proper functional tests should include two service
	//  channels to simulate communication between GS and SC

	CHECK(serv_channel.availableFramesVcCopyRxTM(0) == MaxReceivedRxTmInVirtBuffer);
	CHECK(serv_channel.availableFramesAfterVcGenerationTxTM() == MaxTxInMasterChannel - 0);

	err = serv_channel.allFramesReceptionRequestRxTM(vaild_frame_TM, TmTransferFrameSize);
	CHECK(serv_channel.availableFramesVcCopyRxTM(0) == MaxReceivedRxTmInVirtBuffer - 1);
	CHECK(serv_channel.availableFramesMcCopyRxTM() == MaxTxInMasterChannel - 1);

	err = serv_channel.allFramesReceptionRequestRxTM(invalid_vcid_TM, TmTransferFrameSize);
	CHECK(err == ServiceChannelNotification::INVALID_VC_ID);
	CHECK(serv_channel.availableFramesVcCopyRxTM(0) == MaxReceivedRxTmInVirtBuffer - 1);
	CHECK(serv_channel.availableFramesMcCopyRxTM() == MaxRxInMasterChannel - 1);

	err = serv_channel.allFramesReceptionRequestRxTM(invalid_crc_TM, TmTransferFrameSize);
	CHECK(err == ServiceChannelNotification::RX_INVALID_CRC);
	CHECK(serv_channel.availableFramesVcCopyRxTM(0) == MaxReceivedRxTmInVirtBuffer - 1);
	CHECK(serv_channel.availableFramesMcCopyRxTM() == MaxRxInMasterChannel - 1);

	uint8_t resulting_tm_packet[14] = {0};

	err = serv_channel.packetExtractionRxTM(0, resulting_tm_packet);
	// Valid transfer frame passes to lower procedures
	CHECK(err == ServiceChannelNotification::NO_SERVICE_EVENT);
	CHECK(serv_channel.availableFramesVcCopyRxTM(0) == MaxReceivedRxTmInVirtBuffer - 0);
	CHECK(serv_channel.availableFramesMcCopyRxTM() == MaxTxInMasterChannel - 0);

	// E1 change of state
	err = serv_channel.allFramesReceptionRequestRxTM(valid_no_crc_frame_TM, 12);
	CHECK(serv_channel.fopState(2) == INITIAL);

	// E3 change of state
	err = serv_channel.allFramesReceptionRequestRxTM(valid_no_crc_frame_TM2, 12);
	CHECK(serv_channel.fopState(2) == INITIAL);

	// E4 change of state
	err = serv_channel.allFramesReceptionRequestRxTM(valid_no_crc_frame_TM3, 12);
	CHECK(serv_channel.fopState(2) == INITIAL);

	// E5 change of state
	err = serv_channel.allFramesReceptionRequestRxTM(valid_no_crc_frame_TM4, 12);
	CHECK(serv_channel.fopState(2) == INITIAL);

	// E7 change of state
	err = serv_channel.allFramesReceptionRequestRxTM(valid_no_crc_frame_TM5, 12);
	CHECK(serv_channel.fopState(2) == INITIAL);

	// E11 change of state
	err = serv_channel.allFramesReceptionRequestRxTM(valid_no_crc_frame_TM6, 12);
	CHECK(serv_channel.fopState(2) == INITIAL);

	err = serv_channel.allFramesReceptionRequestRxTM(valid_no_crc_frame_TM7, 12);
	CHECK(serv_channel.fopState(2) == INITIAL);

	CHECK(serv_channel.txAvailableTC(2, 0) == MaxReceivedTcInMapChannel);
    serv_channel.storePacketTxTC(pckt_type_a, 9, 2, 0, ServiceType::TYPE_AD);
	err = serv_channel.mappRequestTxTC(2, 0, 9, ServiceType::TYPE_AD);
	CHECK(err == ServiceChannelNotification::NO_SERVICE_EVENT);
	//    CHECK(serv_channel.availableUnprocessedFramesVcCopyTxTC(2, 0) == MaxReceivedTcInMapChannel);
	CHECK(serv_channel.availableUnprocessedFramesVcCopyTxTC(2) == MaxReceivedUnprocessedTxTcInVirtBuffer - 1);
	serv_channel.initiateAdNoClcw(2);
	// Process first type-A transfer frame
	err = serv_channel.vcGenerationRequestTxTC(2);
	CHECK(err == ServiceChannelNotification::NO_SERVICE_EVENT);
	CHECK(serv_channel.availableUnprocessedFramesVcCopyTxTC(2) == MaxReceivedUnprocessedTxTcInVirtBuffer);
	err = serv_channel.pushSentQueue(2);
	CHECK(frame_a->transferFrameSequenceNumber() == 0);
	serv_channel.acknowledgeFrame(2, 0);
	// E13 change of state
	err = serv_channel.allFramesReceptionRequestRxTM(valid_no_crc_frame_TM, 12);
	CHECK(serv_channel.fopState(2) == INITIAL);
	// E2 change of state
	err = serv_channel.allFramesReceptionRequestRxTM(valid_no_crc_frame_TM8, 12);
	CHECK(serv_channel.fopState(2) == INITIAL);
	// E6 change of state
	err = serv_channel.allFramesReceptionRequestRxTM(valid_no_crc_frame_TM9, 12);
	CHECK(serv_channel.fopState(2) == INITIAL);
	// E8 change of state
	err = serv_channel.allFramesReceptionRequestRxTM(valid_no_crc_frame_TM10, 12);
	CHECK(serv_channel.fopState(2) == INITIAL);
	// E9 change of state
	err = serv_channel.allFramesReceptionRequestRxTM(valid_no_crc_frame_TM11, 12);
	CHECK(serv_channel.fopState(2) == INITIAL);
}

TEST_CASE("VC Generation Service") {
	PhysicalChannel phy_channel_fop = PhysicalChannel(1024, 12, 1024, 220000, 20);

	etl::flat_map<uint8_t, MAPChannel, MaxMapChannels> map_channels = {
	    {0, MAPChannel(0, true, true)},
	    {1, MAPChannel(1, false, false)},
	    {2, MAPChannel(2, true, false)},
	};

	MasterChannel master_channel = MasterChannel();
	master_channel.addVC(0, 128, true, 2, 2, true, true, true, 8, SynchronizationFlag::FORWARD_ORDERED, 255, 10, 10, 3,
	                     map_channels);

	ServiceChannel serv_channel = ServiceChannel(master_channel, phy_channel_fop);
	ServiceChannelNotification err;

	SECTION("Blocking") {
		uint8_t packet1[] = {1, 54, 32, 49, 12, 23};
		uint8_t packet2[] = {47, 31, 65, 81, 25, 44, 76, 99, 13};
		uint8_t packet3[] = {41, 91, 68, 10};
		uint8_t vid = 0 & 0x3F;

		err = serv_channel.storePacketTxTM(packet1, 6, 0);
		CHECK(err == NO_SERVICE_EVENT);
		err = serv_channel.storePacketTxTM(packet2, 9, 0);
		CHECK(err == NO_SERVICE_EVENT);
		err = serv_channel.storePacketTxTM(packet3, 4, 0);
		CHECK(err == NO_SERVICE_EVENT);
		CHECK(serv_channel.availablePacketLengthBufferTxTM(0) == PacketBufferTmSize - 3);
		CHECK(serv_channel.availablePacketBufferTxTM(0) == PacketBufferTmSize - 19);
		uint16_t maxTransferFrameFieldLength = 15;
		err = serv_channel.vcGenerationServiceTxTM(maxTransferFrameFieldLength, 0);
		CHECK(err == NO_SERVICE_EVENT);
		CHECK(serv_channel.availablePacketLengthBufferTxTM(0) == PacketBufferTmSize);
		CHECK(serv_channel.availablePacketBufferTxTM(0) == PacketBufferTmSize);

		const TransferFrameTM* transferFrame = serv_channel.frontFrameAfterVcGenerationTxTM();

		CHECK(transferFrame->getSegmentLengthId() == 3);
		for (uint8_t i = TmPrimaryHeaderSize; i < maxTransferFrameFieldLength + TmPrimaryHeaderSize; i++) {
			if (i < TmPrimaryHeaderSize + sizeof(packet1)) {
				CHECK(transferFrame->getframeData()[i] == packet1[i - TmPrimaryHeaderSize]);
			} else if (i < TmPrimaryHeaderSize + sizeof(packet1) + sizeof(packet2)) {
				CHECK(transferFrame->getframeData()[i] == packet2[i - TmPrimaryHeaderSize - sizeof(packet1)]);
			} else {
				CHECK(transferFrame->getframeData()[i] ==
                      packet3[i - TmPrimaryHeaderSize - sizeof(packet1) - sizeof(packet2)]);
			}
		}
	}
	SECTION("Segmentation") {
		uint8_t packet5[] = {47, 31, 65, 81, 25, 44, 76, 99, 13, 43, 78};
        serv_channel.storePacketTxTM(packet5, 11, 0);
		CHECK(serv_channel.availablePacketLengthBufferTxTM(0) == PacketBufferTmSize - 1);
		CHECK(serv_channel.availablePacketBufferTxTM(0) == PacketBufferTmSize - 11);

		err = serv_channel.vcGenerationServiceTxTM(5, 0);
		CHECK(err == NO_SERVICE_EVENT);
		CHECK(serv_channel.availablePacketLengthBufferTxTM(0) == PacketBufferTmSize);
		CHECK(serv_channel.availablePacketBufferTxTM(0) == PacketBufferTmSize);
		const TransferFrameTM* transferFrame = serv_channel.frontFrameAfterVcGenerationTxTM();
		CHECK(transferFrame->getframeData()[6] == 47);
		CHECK(transferFrame->getframeData()[7] == 31);
		CHECK(transferFrame->getframeData()[8] == 65);
		CHECK(transferFrame->getframeData()[9] == 81);
		CHECK(transferFrame->getframeData()[10] == 25);
		CHECK(transferFrame->getSegmentLengthId() == 1);

        serv_channel.mcGenerationRequestTxTM();
		transferFrame = serv_channel.frontFrameAfterVcGenerationTxTM();

		CHECK(transferFrame->getframeData()[6] == 44);
		CHECK(transferFrame->getframeData()[7] == 76);
		CHECK(transferFrame->getframeData()[8] == 99);
		CHECK(transferFrame->getframeData()[9] == 13);
		CHECK(transferFrame->getframeData()[10] == 43);
		CHECK(transferFrame->getSegmentLengthId() == 0);

        serv_channel.mcGenerationRequestTxTM();
		transferFrame = serv_channel.frontFrameAfterVcGenerationTxTM();

		CHECK(transferFrame->getframeData()[6] == 78);
		CHECK(transferFrame->getSegmentLengthId() == 2);
	}
    SECTION("Concurrent segmentation and blocking") {
        uint8_t packet6[5] = {87, 0, 39, 90, 43};
        uint8_t packet7[7] = {12, 49, 20, 38, 30, 49, 70};
        uint8_t packet8[20] = {69, 75, 76, 78, 89, 28, 29, 39, 42, 45, 8,
                               30, 41, 56, 98, 79, 82, 90, 92, 99};
        uint8_t packet9[4] = {8, 36, 39, 47};
        uint8_t packet10[4] = {5, 17, 38, 46};
        uint8_t concat_packet[40] = {87, 0, 39, 90, 43, 12, 49, 20, 38, 30,
                                     49, 70, 69, 75, 76, 78, 89, 28, 29, 39,
                                     42, 45, 8,30, 41, 56, 98, 79, 82, 90,
                                     92, 99, 8, 36, 39, 47, 5, 17, 38, 46};
        uint16_t transferFrameDataFieldLength = 10;

        err = serv_channel.storePacketTxTM(packet6, sizeof(packet6), 0);
        CHECK(err == NO_SERVICE_EVENT);
        err = serv_channel.storePacketTxTM(packet7, sizeof(packet7), 0);
        CHECK(err == NO_SERVICE_EVENT);
        err = serv_channel.storePacketTxTM(packet8, sizeof(packet8), 0);
        CHECK(err == NO_SERVICE_EVENT);
        err = serv_channel.storePacketTxTM(packet9, sizeof(packet9), 0);
        CHECK(err == NO_SERVICE_EVENT);
        err = serv_channel.storePacketTxTM(packet10, sizeof(packet10), 0);
        CHECK(err == NO_SERVICE_EVENT);

        serv_channel.vcGenerationServiceTxTM(transferFrameDataFieldLength, 0);

        CHECK(serv_channel.availablePacketBufferTxTM(0) == PacketBufferTmSize);
        CHECK(serv_channel.availablePacketLengthBufferTxTM(0) == PacketBufferTmSize);

//        CHECK(master_channel.virtualChannels.at(0).frameCountTM == 4);

        const TransferFrameTM* frame1 = serv_channel.frontFrameAfterVcGenerationTxTM();
        serv_channel.mcGenerationRequestTxTM();
        const TransferFrameTM* frame2 = serv_channel.frontFrameAfterVcGenerationTxTM();
        serv_channel.mcGenerationRequestTxTM();
        const TransferFrameTM* frame3 = serv_channel.frontFrameAfterVcGenerationTxTM();
        serv_channel.mcGenerationRequestTxTM();
        const TransferFrameTM* frame4 = serv_channel.frontFrameAfterVcGenerationTxTM();

        CHECK(frame1->getSegmentLengthId() == 1);
        CHECK(frame1->getFirstHeaderPointer() == 0);
        CHECK(frame2->getSegmentLengthId() == 1);
        CHECK(frame2->getFirstHeaderPointer() == 2);
        CHECK(frame3->getSegmentLengthId() == 0);
        CHECK(frame3->getFirstHeaderPointer() == 2047);
        CHECK(frame4->getSegmentLengthId() == 2);
        CHECK(frame4->getFirstHeaderPointer() == 2);

        for (uint8_t i = 0; i < 40; i++) {
            if (i <= 9) {
                CHECK(concat_packet[i] == frame1->getframeData()[i + TmPrimaryHeaderSize]);
            }
            else if (i <= 19 ) {
                CHECK(concat_packet[i] == frame2->getframeData()[i%10 + TmPrimaryHeaderSize]);
            }
            else if (i <= 29) {
                CHECK(concat_packet[i] == frame3->getframeData()[i%10 + TmPrimaryHeaderSize]);
            }
            else {
                CHECK(concat_packet[i] == frame4->getframeData()[i%10 + TmPrimaryHeaderSize]);
            }
        }
    }
}

TEST_CASE("CLCW construction at VC Reception") {
	PhysicalChannel phy_channel_fop = PhysicalChannel(1024, 12, 1024, 220000, 20);

	etl::flat_map<uint8_t, MAPChannel, MaxMapChannels> map_channels = {
	    {0, MAPChannel(0, true, true)},
	    {1, MAPChannel(1, false, false)},
	    {2, MAPChannel(2, true, false)},
	};

	MasterChannel master_channel = MasterChannel();
	master_channel.addVC(0, 128, true, 2, 2, false, false, 0, 8, SynchronizationFlag::FORWARD_ORDERED, 255, 10, 10, 3,
	                     map_channels);

	ServiceChannel serv_channel = ServiceChannel(master_channel, phy_channel_fop);
	VirtualChannel virtualChannel = master_channel.virtualChannels.at(0);

	ServiceChannelNotification err;
	uint8_t packet1[] = {0x10, 0xB1, 0x00, 0x0A, 0x00, 0x00, 0x00, 0x1C, 0xD3, 0x8C};
	uint8_t packet2[] = {0x10, 0xB1, 0x00, 0x0A, 0x03, 0x00, 0x00, 0x1C, 0xD3, 0x8C};
	uint8_t packet3[] = {0x10, 0xB1, 0x00, 0x0A, 0x12, 0x00, 0x00, 0x1C, 0xD3, 0x8C};
    serv_channel.storeFrameRxTC(packet1, 10);
    serv_channel.storeFrameRxTC(packet2, 10);
    serv_channel.storeFrameRxTC(packet3, 10);
    serv_channel.allFramesReceptionRequestRxTC();
    serv_channel.allFramesReceptionRequestRxTC();
    serv_channel.allFramesReceptionRequestRxTC();
	err = serv_channel.vcReceptionRxTC(0);
	// Checks if frame sequence number is the same as expected
	CHECK(serv_channel.getClcwInBuffer().getWait() == false);
	CHECK(serv_channel.getClcwInBuffer().getRetransmit() == false);
	CHECK(serv_channel.getClcwInBuffer().getLockout() == false);
	CLCW clcw =
	    CLCW(serv_channel.getClcwTransferFrameDataBuffer()[TmTransferFrameSize - 4 -
	                                                       2 * virtualChannel.frameErrorControlFieldPresent]
	             << 24 |
	         serv_channel.getClcwTransferFrameDataBuffer()[TmTransferFrameSize - 4 -
	                                                       2 * virtualChannel.frameErrorControlFieldPresent + 1]
	             << 16 |
	         serv_channel.getClcwTransferFrameDataBuffer()[TmTransferFrameSize - 4 -
	                                                       2 * virtualChannel.frameErrorControlFieldPresent + 2]
	             << 8 |
	         serv_channel.getClcwTransferFrameDataBuffer()[TmTransferFrameSize - 4 -
	                                                       2 * virtualChannel.frameErrorControlFieldPresent + 3]);
	CHECK(clcw.getWait() == false);
	CHECK(clcw.getRetransmit() == false);
	CHECK(clcw.getLockout() == false);
	CHECK(err == NO_SERVICE_EVENT);

	err = serv_channel.vcReceptionRxTC(0);
	// Checks if frame sequence number is bigger than expected but smaller that positive window
	CHECK(serv_channel.getClcwInBuffer().getWait() == false);
	CHECK(serv_channel.getClcwInBuffer().getRetransmit() == true);
	CHECK(serv_channel.getClcwInBuffer().getLockout() == false);
	CLCW clcw2 =
	    CLCW(serv_channel.getClcwTransferFrameDataBuffer()[TmTransferFrameSize - 4 -
	                                                       2 * virtualChannel.frameErrorControlFieldPresent]
	             << 24 |
	         serv_channel.getClcwTransferFrameDataBuffer()[TmTransferFrameSize - 4 -
	                                                       2 * virtualChannel.frameErrorControlFieldPresent + 1]
	             << 16 |
	         serv_channel.getClcwTransferFrameDataBuffer()[TmTransferFrameSize - 4 -
	                                                       2 * virtualChannel.frameErrorControlFieldPresent + 2]
	             << 8 |
	         serv_channel.getClcwTransferFrameDataBuffer()[TmTransferFrameSize - 4 -
	                                                       2 * virtualChannel.frameErrorControlFieldPresent + 3]);
	CHECK(clcw2.getWait() == false);
	CHECK(clcw2.getRetransmit() == true);
	CHECK(clcw2.getLockout() == false);
	CHECK(err == NO_SERVICE_EVENT);
	CHECK(err == NO_SERVICE_EVENT);

	err = serv_channel.vcReceptionRxTC(0);
	// Checks if frame sequence number is bigger than expected and bigger that positive window
	CHECK(serv_channel.getClcwInBuffer().getWait() == false);
	CHECK(serv_channel.getClcwInBuffer().getRetransmit() == true);
	CHECK(serv_channel.getClcwInBuffer().getLockout() == true);
	CLCW clcw3 =
	    CLCW(serv_channel.getClcwTransferFrameDataBuffer()[TmTransferFrameSize - 4 -
	                                                       2 * virtualChannel.frameErrorControlFieldPresent]
	             << 24 |
	         serv_channel.getClcwTransferFrameDataBuffer()[TmTransferFrameSize - 4 -
	                                                       2 * virtualChannel.frameErrorControlFieldPresent + 1]
	             << 16 |
	         serv_channel.getClcwTransferFrameDataBuffer()[TmTransferFrameSize - 4 -
	                                                       2 * virtualChannel.frameErrorControlFieldPresent + 2]
	             << 8 |
	         serv_channel.getClcwTransferFrameDataBuffer()[TmTransferFrameSize - 4 -
	                                                       2 * virtualChannel.frameErrorControlFieldPresent + 3]);
	CHECK(clcw3.getWait() == false);
	CHECK(clcw3.getRetransmit() == true);
	CHECK(clcw3.getLockout() == true);
	CHECK(err == NO_SERVICE_EVENT);
	CHECK(err == NO_SERVICE_EVENT);
}

TEST_CASE("Frame Acknowledgement") {
    ServiceChannelNotification err;

	PhysicalChannel phy_channel_fop = PhysicalChannel(1024, 12, 1024, 220000, 20);

	etl::flat_map<uint8_t, MAPChannel, MaxMapChannels> map_channels = {
	    {0, MAPChannel(0, true, true)},
	    {1, MAPChannel(1, false, false)},
	    {2, MAPChannel(2, true, false)},
	};

	MasterChannel master_channel = MasterChannel();
	master_channel.addVC(0, 128, true, 2, 2, false, false, 0, 8, SynchronizationFlag::FORWARD_ORDERED, 255, 10, 10, 3,
	                     map_channels);
	master_channel.addVC(1, 128, true, 2, 2, false, false, 0, 8, SynchronizationFlag::FORWARD_ORDERED, 255, 10, 10, 3,
	                     map_channels);

	ServiceChannel serv_channel = ServiceChannel(master_channel, phy_channel_fop);
	VirtualChannel virtualChannel = master_channel.virtualChannels.at(0);

	uint8_t packet[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0xA2, 0xB3, 0x21, 0xA1};
	uint8_t packet1[] = {0x10, 0xB1, 0x00, 0x0A, 0x00, 0x00, 0x00, 0x1C, 0xD3, 0x8C};
	uint8_t packet2[] = {0x10, 0xB1, 0x00, 0x0A, 0x01, 0x00, 0x00, 0x1C, 0xD3, 0x8C};
	uint8_t packet3[] = {0x10, 0xB1, 0x00, 0x0A, 0x12, 0x00, 0x00, 0x1C, 0xD3, 0x8C};

	// Initate  AD Service
	serv_channel.initiateAdClcw(0);
	// Send a TC Frame with the right frame sequence number
	err = serv_channel.storePacketTxTC(packet1, 10, 0, 0, ServiceType::TYPE_AD);
	CHECK(err == ServiceChannelNotification::NO_SERVICE_EVENT);

	err = serv_channel.mappRequestTxTC(0, 0, 10, ServiceType::TYPE_AD);
    CHECK(err == ServiceChannelNotification::NO_SERVICE_EVENT);

	err = serv_channel.vcGenerationRequestTxTC(0);
    CHECK(err == ServiceChannelNotification::NO_SERVICE_EVENT);

	err = serv_channel.allFramesGenerationRequestTxTC();
    CHECK(err == ServiceChannelNotification::NO_SERVICE_EVENT);
	CHECK(serv_channel.getLastMasterCopyTcFrame().getProcessedByFOP() == true);

	TransferFrameTC transferFrame = serv_channel.getLastMasterCopyTcFrame();
	// Receive the same frame
	err = serv_channel.storeFrameRxTC(transferFrame.frameData(), transferFrame.getFrameLength());
    CHECK(err == ServiceChannelNotification::NO_SERVICE_EVENT);

	err = serv_channel.allFramesReceptionRequestRxTC();
    CHECK(err == ServiceChannelNotification::NO_SERVICE_EVENT);

	err = serv_channel.vcReceptionRxTC(0);
    CHECK(err == ServiceChannelNotification::NO_SERVICE_EVENT);

	// Check the clcw that was created in vcReceptionRxTC
	err = serv_channel.allFramesReceptionRequestRxTM(serv_channel.getClcwTransferFrameDataBuffer(),
                                                     MaxTcTransferFrameSize);
    CHECK(err == ServiceChannelNotification::NO_SERVICE_EVENT);
	CHECK(serv_channel.getLastMasterCopyTcFrame().acknowledged() == true);

	// Repeat the process with the next frame
	err = serv_channel.storePacketTxTC(packet2, 9, 0, 0, ServiceType::TYPE_AD);
    CHECK(err == ServiceChannelNotification::NO_SERVICE_EVENT);

	err = serv_channel.mappRequestTxTC(0, 0, 9, ServiceType::TYPE_AD);
    CHECK(err == ServiceChannelNotification::NO_SERVICE_EVENT);

	err = serv_channel.vcGenerationRequestTxTC(0);
    CHECK(err == ServiceChannelNotification::NO_SERVICE_EVENT);
	CHECK(serv_channel.getLastMasterCopyTcFrame().transferFrameSequenceNumber() == 1);
	CHECK(serv_channel.getLastMasterCopyTcFrame().getProcessedByFOP() == true);
	TransferFrameTC transferFrame2 = serv_channel.getLastMasterCopyTcFrame();

	err = serv_channel.storeFrameRxTC(transferFrame2.frameData(), transferFrame.getFrameLength());
    CHECK(err == ServiceChannelNotification::NO_SERVICE_EVENT);

	err = serv_channel.allFramesReceptionRequestRxTC();
    CHECK(err == ServiceChannelNotification::NO_SERVICE_EVENT);

	err = serv_channel.vcReceptionRxTC(0);
    CHECK(err == ServiceChannelNotification::NO_SERVICE_EVENT);

	err = serv_channel.allFramesReceptionRequestRxTM(serv_channel.getClcwTransferFrameDataBuffer(),
                                                     MaxTcTransferFrameSize);
    CHECK(err == ServiceChannelNotification::NO_SERVICE_EVENT);
	CHECK(serv_channel.getLastMasterCopyTcFrame().acknowledged() == true);

	serv_channel.setVs(1, 10);
	serv_channel.initiateAdClcw(1);
	// Set the transmitter frame sequence number outside the FOP window
	err = serv_channel.storePacketTxTC(packet1, 9, 1, 0, ServiceType::TYPE_AD);
    CHECK(err == ServiceChannelNotification::NO_SERVICE_EVENT);

	err = serv_channel.mappRequestTxTC(1, 0, 9, ServiceType::TYPE_AD);
    CHECK(err == ServiceChannelNotification::NO_SERVICE_EVENT);

	err = serv_channel.vcGenerationRequestTxTC(1);
    CHECK(err == ServiceChannelNotification::NO_SERVICE_EVENT);

	err = serv_channel.allFramesGenerationRequestTxTC();
    CHECK(err == ServiceChannelNotification::NO_SERVICE_EVENT);
	CHECK(serv_channel.getLastMasterCopyTcFrame().getProcessedByFOP() == true);
	CHECK(serv_channel.getLastMasterCopyTcFrame().transferFrameSequenceNumber() == 10);

	TransferFrameTC transferFrame3 = serv_channel.getLastMasterCopyTcFrame();
	// Receive the same frame
	err = serv_channel.storeFrameRxTC(transferFrame3.frameData(), transferFrame.getFrameLength());
    CHECK(err == ServiceChannelNotification::NO_SERVICE_EVENT);

	err = serv_channel.allFramesReceptionRequestRxTC();
    CHECK(err == ServiceChannelNotification::NO_SERVICE_EVENT);

	err = serv_channel.vcReceptionRxTC(1);
    CHECK(err == ServiceChannelNotification::NO_SERVICE_EVENT);

	err = serv_channel.allFramesReceptionRequestRxTM(serv_channel.getClcwTransferFrameDataBuffer(), TmTransferFrameSize);
    CHECK(err == ServiceChannelNotification::NO_SERVICE_EVENT);
	CHECK(serv_channel.getLastMasterCopyTcFrame().acknowledged() == false);
}