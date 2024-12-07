#include <catch2/catch_test_macros.hpp>
#include <CCSDSChannel.hpp>
#include <TransferFrameTC.hpp>
#include <TransferFrameTM.hpp>
#include <CCSDSServiceChannel.hpp>
#include <CCSDSSecurityAssociation.hpp>
#include <etl/array.h>

TEST_CASE("Security Association (40 bit HMAC)") {
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

    ServiceChannel serv_channel = ServiceChannel(master_channel, phy_channel_fop);

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


    SDLSVerificationStatusCode verCode;
    SECTION("Normal Operation") {
        uint8_t frameData[] = {0x00, 0xAC, 0x00, 0x0A, 0x00,     // primary header (type ad frame)
                               0x00,                                             // segment header
                               0xFF, 0xFF,                                   // spi
                               0x00, 0x00, 0x00, 0x00,             // sequence number
                               0x00, 0x00, 0x1C, 0x0E, 0xFD, // payload data
                               0x00, 0x00, 0x00, 0x00, 0x00  // mac
        };
        uint16_t transferFrameDataFieldLength = 6; // segment header + payload data
        TransferFrameTC frameTc = TransferFrameTC(frameData, 22, 0, true);

        verCode = senderSA.applySecurityTC(frameTc, transferFrameDataFieldLength, 0, 0);
        CHECK(verCode == NO_FAILURE);
        // spi
        CHECK(((static_cast<uint16_t>(frameData[6]) << 8) | static_cast<uint16_t>(frameData[7])) == SecurityParameterIndex);
        // sequence number
        CHECK(frameData[11] == 0x01);

        verCode = receiverSA.processSecurityTC(frameTc, transferFrameDataFieldLength, 0, 0);
        CHECK(verCode == NO_FAILURE);
    }
}