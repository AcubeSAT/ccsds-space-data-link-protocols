#include <catch2/catch.hpp>
#include <CCSDSTransferFrameTM.hpp>
#include <CCSDSTransferFrameTC.hpp>
#include <Packet.hpp>

TEST_CASE("CCSDS TC Transfer Frame") {
    CCSDSTransferFrameTC transferFrame = CCSDSTransferFrameTC(33, 1022);

    String<TC_PRIMARY_HEADER_SIZE> primHeader = transferFrame.getPrimaryHeader();

    String<TC_MAX_TRANSFER_FRAME_SIZE> frame = transferFrame.transferFrame();

    CHECK(primHeader.size() == TC_PRIMARY_HEADER_SIZE); // Check the size of the primary header

    CHECK(primHeader.at(0) == 0x02U);
    CHECK(primHeader.at(1) == 0x37U);
    // CHECK(primHeader.at(2) == 0x87U);
    // CHECK(primHeader.at(3) == 0xFEU);
    CHECK(primHeader.at(4) == 0x00U);

    transferFrame.setFrameSequenceCount(0xA1U);
    transferFrame.incrementFrameSequenceCount();

    primHeader = transferFrame.getPrimaryHeader();
    //CHECK(primHeader.at(4) == 0xA2U);

    CHECK(frame.at(0) == 0x02U);
    CHECK(frame.at(1) == 0x37U);
    // CHECK(frame.at(2) == 0x87U);
    // CHECK(frame.at(3) == 0xFEU);
    CHECK(frame.at(4) == 0x00U);
}

TEST_CASE("CCSDS TM Transfer Frame") {
    CCSDSTransferFrameTM transferFrame = CCSDSTransferFrameTM(4);

    String<TM_PRIMARY_HEADER_SIZE> primHeader = transferFrame.getPrimaryHeader();
    transferFrame.setFirstHeaderPointer(0x07FFU);

    String<TM_TRANSFER_FRAME_SIZE> frame = transferFrame.transferFrame();

    CHECK(primHeader.size() == TM_PRIMARY_HEADER_SIZE); // Check the size of the primary header

    // Validate the contents of the primary header String. Created from constructor
    CHECK(primHeader.at(0) == 0x23U);
    CHECK(primHeader.at(1) == 0x78U);
    CHECK(primHeader.at(2) == 0x00U);
    CHECK(primHeader.at(3) == 0x00U);
    CHECK(primHeader.at(4) == 0x18U);
    CHECK(primHeader.at(5) == 0x00U);

    // Primary header from frame
    CHECK(frame.at(0) == 0x23U);
    CHECK(frame.at(1) == 0x78U);
    CHECK(frame.at(2) == 0x00U);
    CHECK(frame.at(3) == 0x00U);
    CHECK(frame.at(4) == 0x1FU);
    CHECK(static_cast<uint8_t>(frame.at(5)) == 0xFFU);

    // Check individual attributes
    CHECK(not transferFrame.secondaryHeaderExists());
    CHECK(transferFrame.masterChannelID() == (SPACECRAFT_IDENTIFIER & 0x03FFU));
    CHECK(transferFrame.masterChannelFrameCount() == 0); // Master channel frame count
    CHECK(transferFrame.virtualChannelFrameCount() == 0);
    CHECK(transferFrame.virtualChannelID() == 4);
    CHECK(not transferFrame.ocfFlag());
    CHECK(transferFrame.getFirstHeaderPointer() == 0x07FFU);
}
