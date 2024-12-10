#include <catch2/catch_test_macros.hpp>
#include <CCSDSChannel.hpp>
#include <TransferFrameTC.hpp>
#include <TransferFrameTM.hpp>
#include <CCSDSServiceChannel.hpp>
#include <CCSDSSecurityAssociation.hpp>
#include <etl/array.h>

TEST_CASE("Security Association (40 bit HMAC authentication)") {
    // Set up Service Channel
    PhysicalChannel phy_channel_fop = PhysicalChannel(1024, 12, 1024, 220000, 20);

    etl::flat_map<uint8_t, MAPChannel, MaxMapChannels> map_channels = {
            {0, MAPChannel(0, true, true)},
            {1, MAPChannel(1, false, false)},
            {2, MAPChannel(2, true, false)},
    };

    MasterChannel master_channel = MasterChannel();
    master_channel.addVC(0, true, 128, true, true, true,  2, 2, true, true, 8, true, SynchronizationFlag::OCTET_SYNCHRONIZED_FORWARD_ORDERED, 255, 10, 10, 3,
                         map_channels);

    master_channel.addVC(1, false, 128, false, false, false,  2, 2, true, true, true, true, SynchronizationFlag::OCTET_SYNCHRONIZED_FORWARD_ORDERED, 20, 3, 3, 3);

    master_channel.addVC(2, false, 128, false, false, false, 2, 2, false, true, true, true, SynchronizationFlag::OCTET_SYNCHRONIZED_FORWARD_ORDERED, 20, 3, 3, 3);


    // Set up security association. Associate vc 0 (with only map channels 0,1) and vc 1
    etl::array<uint8_t, MaxMapChannels> vc0MapsChannels = {0, 1, 2};
    etl::array<uint8_t, MaxMapChannels> vc1MapsChannels = {0, 1, };
    etl::flat_map<uint8_t, etl::array<uint8_t, MaxMapChannels>, MaxVirtualChannels> associatedChannels = {
            {0 , vc0MapsChannels},
            {1, vc1MapsChannels}
    };
    SecurityAssociation senderSA = SecurityAssociation(SecurityParameterIndex,
                                                       associatedChannels, HMAC_40_BIT, SENDER);
    SecurityAssociation receiverSA = SecurityAssociation(SecurityParameterIndex,
                                                       associatedChannels, HMAC_40_BIT, RECEIVER);

    ServiceChannel serv_channel = ServiceChannel(master_channel, phy_channel_fop, senderSA, receiverSA);

    // test frames
    uint8_t frameData1[] = {0x00, 0xAC, 0x00, 0x0A, 0x00,    // primary header (type ad frame)
                           0x00,                                             // segment header
                           0xFF, 0xFF,                                   // spi
                           0x00, 0x00, 0x00, 0x00,             // sequence number
                           0x00, 0x00, 0x1C, 0x0E, 0xFD, // payload data
                           0x00, 0x00, 0x00, 0x00, 0x00  // mac
    };
    uint8_t frameData2[] = {0x00, 0xAC, 0x00, 0x0A, 0x00,    // primary header (type ad frame)
                            0x00,                                             // segment header
                            0xFF, 0xFF,                                   // spi
                            0x00, 0x00, 0x00, 0x00,             // sequence number
                            0x45, 0xAF, 0x1C, 0x03, 0xFD, // payload data
                            0x00, 0x00, 0x00, 0x00, 0x00  // mac
    };
    uint16_t transferFrameDataFieldLength = 6; // segment header + payload data

    SDLSVerificationStatusCode verCode;
    SECTION("Normal Operation") {
        senderSA.resetSequenceNumber();
        receiverSA.resetSequenceNumber();
        uint8_t* frameData = frameData1;

        TransferFrameTC frameTc = TransferFrameTC(frameData, 22, 0, true);

        verCode = senderSA.applySecurityTC(&frameTc, transferFrameDataFieldLength, 0, 0);
        CHECK(verCode == NO_FAILURE);
        // spi
        CHECK(((static_cast<uint16_t>(frameData[6]) << 8) | static_cast<uint16_t>(frameData[7])) == SecurityParameterIndex);
        // sequence number
        CHECK(frameData[11] == 0x01);
        // print MAC
        etl::string<20> macString;
        for (uint8_t i = 0; i < 5; i++) {
            macString.append(std::to_string(frameData[i + 17]).c_str());
            macString.append(" ");
        }
        LOG_DEBUG << "Mac value: " << macString.c_str();

        verCode = receiverSA.processSecurityTC(&frameTc, transferFrameDataFieldLength, 0, 0);
        CHECK(verCode == NO_FAILURE);
    }

    SECTION("Multiple frames") {
        senderSA.resetSequenceNumber();
        receiverSA.resetSequenceNumber();

        TransferFrameTC frameTc1 = TransferFrameTC(frameData1, 22, 0, true);
        TransferFrameTC frameTc2 = TransferFrameTC(frameData2, 22, 0, true);

        // process first frame
        verCode = senderSA.applySecurityTC(&frameTc1, transferFrameDataFieldLength, 0, 0);
        CHECK(verCode == NO_FAILURE);
        // spi
        CHECK(((static_cast<uint16_t>(frameData1[6]) << 8) | static_cast<uint16_t>(frameData1[7])) == SecurityParameterIndex);
        // sequence number
        CHECK(frameData1[11] == 0x01);

        // process second frame
        verCode = senderSA.applySecurityTC(&frameTc2, transferFrameDataFieldLength, 0, 0);
        CHECK(verCode == NO_FAILURE);
        // spi
        CHECK(((static_cast<uint16_t>(frameData2[6]) << 8) | static_cast<uint16_t>(frameData2[7])) == SecurityParameterIndex);
        // sequence number
        CHECK(frameData2[11] == 0x02);

        // send first frame
        verCode = receiverSA.processSecurityTC(&frameTc1, transferFrameDataFieldLength, 0, 0);
        CHECK(verCode == NO_FAILURE);

        // try to send frame again (replay attack)
        verCode = receiverSA.processSecurityTC(&frameTc1, transferFrameDataFieldLength, 0, 0);
        CHECK(verCode == ANTI_REPLAY_SEQUENCE_NUMBER_FAILURE);

        // send second frame
        verCode = receiverSA.processSecurityTC(&frameTc2, transferFrameDataFieldLength, 0, 0);
        CHECK(verCode == NO_FAILURE);
    }

    SECTION("Wrong MAC") {
        senderSA.resetSequenceNumber();
        receiverSA.resetSequenceNumber();

        TransferFrameTC frameTc1 = TransferFrameTC(frameData1, 22, 0, true);
        TransferFrameTC frameTc2 = TransferFrameTC(frameData2, 22, 0, true);

        // make first frame
        verCode = senderSA.applySecurityTC(&frameTc1, transferFrameDataFieldLength, 0, 0);
        CHECK(verCode == NO_FAILURE);
        // spi
        CHECK(((static_cast<uint16_t>(frameData1[6]) << 8) | static_cast<uint16_t>(frameData1[7])) == SecurityParameterIndex);
        // sequence number
        CHECK(frameData1[11] == 0x01);
        // corrupt mac
        frameData1[18] = 0x9;

        // process second frame
        verCode = senderSA.applySecurityTC(&frameTc2, transferFrameDataFieldLength, 0, 0);
        CHECK(verCode == NO_FAILURE);
        // spi
        CHECK(((static_cast<uint16_t>(frameData2[6]) << 8) | static_cast<uint16_t>(frameData2[7])) == SecurityParameterIndex);
        // sequence number
        CHECK(frameData2[11] == 0x02);
        // corrupt payload
        frameData2[14] = 0x00;

        // send first frame
        verCode = receiverSA.processSecurityTC(&frameTc1, transferFrameDataFieldLength, 0, 0);
        CHECK(verCode == MAC_VERIFICATION_FAILURE);

        // send second frame
        verCode = receiverSA.processSecurityTC(&frameTc2, transferFrameDataFieldLength, 0, 0);
        CHECK(verCode == MAC_VERIFICATION_FAILURE);
    }
    SECTION("Unassociated channels") {
        senderSA.resetSequenceNumber();
        receiverSA.resetSequenceNumber();

        // virtual channel with no map channels
        TransferFrameTC frameTc1 = TransferFrameTC(frameData1, 22, 0, false);
        verCode = senderSA.applySecurityTC(&frameTc1, transferFrameDataFieldLength, 5, 0);
        CHECK(verCode == UNASSOCIATED_CHANNEL);
        verCode = receiverSA.processSecurityTC(&frameTc1, transferFrameDataFieldLength, 5, 0);
        CHECK(verCode == UNASSOCIATED_CHANNEL);

        // virtual channel with map channels
        TransferFrameTC frameTc2 = TransferFrameTC(frameData2, 22, 0, true);
        verCode = senderSA.applySecurityTC(&frameTc2, transferFrameDataFieldLength, 0, 5);
        CHECK(verCode == UNASSOCIATED_CHANNEL);
        verCode = receiverSA.processSecurityTC(&frameTc2, transferFrameDataFieldLength, 0, 5);
        CHECK(verCode == UNASSOCIATED_CHANNEL);
    }
    SECTION("Data Link Integration (TxTC side) ") {
        ServiceChannelNotification err;
        serv_channel.resetSequenceCountersSA();
        uint8_t packet1[] = {0x9A, 0x06, 0x00, 0x3, 0xF0};
        uint8_t packet2[] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xAA};

        // normal operation (generate 3 frames, the first via blocking and the other 2 via sgementation)
        serv_channel.storePacketTxTC(packet1, sizeof(packet1), 0, 0, ServiceType::TYPE_AD);
        serv_channel.storePacketTxTC(packet2, sizeof(packet2), 0, 0, ServiceType::TYPE_AD);
        serv_channel.packetProcessingRequestTxTC(0, 0, transferFrameDataFieldLength, ServiceType::TYPE_AD);

        CHECK(serv_channel.availableFramesBeforeSDLSProcessing(0) == MaxReceivedUnprocessedTxTcInVirtBuffer - 3);
        uint8_t *frameData3 = serv_channel.frontFrameBeforeSDLSProcessing(0).second->getFrameData();
        err = serv_channel.applySDLSSecurityTxTC(0, 0);
        uint8_t *frameData4 = serv_channel.frontFrameBeforeSDLSProcessing(0).second->getFrameData();
        err = serv_channel.applySDLSSecurityTxTC(0, 0);
        uint8_t *frameData5 = serv_channel.frontFrameBeforeSDLSProcessing(0).second->getFrameData();
        err = serv_channel.applySDLSSecurityTxTC(0, 0);
        CHECK(err == NO_SERVICE_EVENT);
        CHECK(serv_channel.availableFramesBeforeSDLSProcessing(0) == MaxReceivedUnprocessedTxTcInVirtBuffer);


        CHECK(frameData3[TcPrimaryHeaderSize + TcSegmentHeaderSize] ==
              static_cast<uint8_t>(SecurityParameterIndex >> 8));
        CHECK(frameData3[TcPrimaryHeaderSize + TcSegmentHeaderSize + 1] ==
              static_cast<uint8_t>(SecurityParameterIndex));
        CHECK(frameData3[TcPrimaryHeaderSize + TcSegmentHeaderSize + 5] == 0x01); // seq counter
        CHECK(frameData3[TcPrimaryHeaderSize + TcSegmentHeaderSize + senderSA.getSecurityHeaderLength()] ==
              0x9A); // payload start
        CHECK(frameData3[TcPrimaryHeaderSize + TcSegmentHeaderSize + senderSA.getSecurityHeaderLength() +
                         sizeof(packet1) - 1] == 0xF0); // payload end

        CHECK(frameData4[TcPrimaryHeaderSize + TcSegmentHeaderSize] ==
              static_cast<uint8_t>(SecurityParameterIndex >> 8));
        CHECK(frameData4[TcPrimaryHeaderSize + TcSegmentHeaderSize + 1] ==
              static_cast<uint8_t>(SecurityParameterIndex));
        CHECK(frameData4[TcPrimaryHeaderSize + TcSegmentHeaderSize + 5] == 0x02); // seq counter
        CHECK(frameData4[TcPrimaryHeaderSize + TcSegmentHeaderSize + senderSA.getSecurityHeaderLength()] ==
              0x11); // payload start
        CHECK(frameData4[TcPrimaryHeaderSize + TcSegmentHeaderSize + senderSA.getSecurityHeaderLength() + 5 - 1] ==
              0x55); // payload end

        CHECK(frameData5[TcPrimaryHeaderSize + TcSegmentHeaderSize] ==
              static_cast<uint8_t>(SecurityParameterIndex >> 8));
        CHECK(frameData5[TcPrimaryHeaderSize + TcSegmentHeaderSize + 1] ==
              static_cast<uint8_t>(SecurityParameterIndex));
        CHECK(frameData5[TcPrimaryHeaderSize + TcSegmentHeaderSize + 5] == 0x03); // seq counter
        CHECK(frameData5[TcPrimaryHeaderSize + TcSegmentHeaderSize + senderSA.getSecurityHeaderLength()] ==
              0x66); // payload start
        CHECK(frameData5[TcPrimaryHeaderSize + TcSegmentHeaderSize + senderSA.getSecurityHeaderLength() + 5 - 1] ==
              0xAA); // payload end

        // send a type-BC frame (security services not applicable)
        serv_channel.storePacketTxTC(packet1, sizeof(packet1), 0, 0, ServiceType::TYPE_BC);
        serv_channel.packetProcessingRequestTxTC(0, 0, transferFrameDataFieldLength, ServiceType::TYPE_BC);

        CHECK(serv_channel.availableFramesBeforeSDLSProcessing(0) == MaxReceivedUnprocessedTxTcInVirtBuffer - 1);
        uint8_t *frameData6 = serv_channel.frontFrameBeforeSDLSProcessing(0).second->getFrameData();

        err = serv_channel.applySDLSSecurityTxTC(0, 0);
        CHECK(err == NO_SERVICE_EVENT);
        CHECK(serv_channel.availableFramesBeforeSDLSProcessing(0) == MaxReceivedUnprocessedTxTcInVirtBuffer);

        CHECK(frameData6[TcPrimaryHeaderSize] == 0x9A); // payload start
        CHECK(frameData6[TcPrimaryHeaderSize + sizeof(packet1) - 1] == 0xF0); // payload end
    }
    SECTION("Data Link Integration (RxTC side) ") {
        serv_channel.resetSequenceCountersSA();


    }

}