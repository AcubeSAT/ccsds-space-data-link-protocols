//#include <catch2/catch.hpp>
#include <catch2/catch_test_macros.hpp>
#include <TransferFrameTC.hpp>
#include <TransferFrameTM.hpp>

TEST_CASE("TC Frame Generation - Raw Data Constructor") {
    uint8_t data[] = {200, 155, 32, 40, 66};
    TransferFrameTC frame = TransferFrameTC(data, 5, 0);
    TransferFrameHeaderTC hdr = frame.getTransferFrameHeader();
    CHECK(hdr.getTransferFrameVersionNumber() == 0x3);
    CHECK(hdr.getBypassFlag() == 0x0);
    CHECK(hdr.getCtrlAndCmdFlag() == 0x0);
    CHECK(hdr.getSpacecraftId(TC) == 0x9B);
    CHECK(hdr.getVirtualChannelId(TC) == 0x08);
    CHECK(hdr.getTransferFrameLength() == 0x28);
    CHECK(hdr.getTransferFrameSequenceNumber() == 0x42);
}

TEST_CASE("TC Frame Generation - Field Constructor") {
    uint8_t data[MaxTcTransferFrameSize] = {0};
    TransferFrameTC frame = TransferFrameTC(data, ServiceType::TYPE_AD, 0x08, 10, true, 0x3, 4, 0, TC);
    TransferFrameHeaderTC hdr = frame.getTransferFrameHeader();
    CHECK(hdr.getTransferFrameVersionNumber() == 0x0);
    CHECK(hdr.getBypassFlag() == 0x0);
    CHECK(hdr.getCtrlAndCmdFlag() == 0x0);
    CHECK(frame.getServiceType() == ServiceType::TYPE_AD);
    CHECK(hdr.getSpacecraftId(TC) == SpacecraftIdentifier);
    CHECK(hdr.getVirtualChannelId(TC) == 0x08);
    CHECK(hdr.getTransferFrameLength() == 0x0A);
    CHECK(hdr.getTransferFrameSequenceNumber() == 0x0);
    CHECK(frame.getMapId() == 4);

    // Test setters
    frame.setServiceType(ServiceType::TYPE_BC);
    CHECK(hdr.getBypassFlag() == 0x1);
    CHECK(hdr.getCtrlAndCmdFlag() == 0x1);
    CHECK(frame.getServiceType() == ServiceType::TYPE_BC);

    frame.setFrameLength(9);
    CHECK(hdr.getTransferFrameLength() == 9);

    frame.setTransferFrameSequenceNumber(10);
    CHECK(hdr.getTransferFrameSequenceNumber());

    frame.setSequenceFlags(NoSegmentation);
    CHECK(((frame.getFrameData()[5] & 0xC0) >> 6) == NoSegmentation);
}

TEST_CASE("TM Frame Generation - Raw Data Constructor") {
    uint8_t data[] = {200, 184, 99, 23, 40, 190};
    TransferFrameTM frame = TransferFrameTM(data, 6, true, 0);
    TransferFrameHeaderTM hdr = frame.getTransferFrameHeader();
    CHECK(hdr.getSpacecraftId(TM) == 0x8B);
    CHECK(hdr.getOperationalControlFieldFlag() == 0x0);
    CHECK(hdr.getVirtualChannelId(TM) == 0x04);
    CHECK(hdr.getMasterChannelFrameCount() == 0x63);
    CHECK(hdr.getVirtualChannelFrameCount() == 0x17);
    CHECK(hdr.getTransferFrameSecondaryHeaderFlag() == 0x0);
    CHECK(hdr.getSynchronizationFlag() == 0x0);
    CHECK(hdr.getPacketOrderFlag() == 0x01);
    CHECK(hdr.getFirstHeaderPointer() == 0xBE);
}

TEST_CASE("TM Frame Generation - Field Constructor") {
    uint8_t data[TmTransferFrameSize] = {0};
    TransferFrameTM frame = TransferFrameTM(data, 10, 4, false, 0x17, false,
                                            OCTET_SYNCHRONIZED_FORWARD_ORDERED, PacketOrder, SegmentLengthIdentifier,
                                            0xBE, true, 0, TM);
    TransferFrameHeaderTM hdr = frame.getTransferFrameHeader();

    CHECK(hdr.getTransferFrameVersionNumber() == 0);
    CHECK(hdr.getSpacecraftId(TM) == SpacecraftIdentifier);
    CHECK(hdr.getVirtualChannelId(TM) == 0x04);
    CHECK(hdr.getOperationalControlFieldFlag() == false);
    CHECK(hdr.getMasterChannelFrameCount() == 0x0);
    CHECK(hdr.getVirtualChannelFrameCount() == 0x17);
    CHECK(hdr.getTransferFrameSecondaryHeaderFlag() == false);
    CHECK(hdr.getSynchronizationFlag() == OCTET_SYNCHRONIZED_FORWARD_ORDERED);
    CHECK(hdr.getPacketOrderFlag() == PacketOrder);
    CHECK(hdr.getSegmentLengthId() == SegmentLengthIdentifier);
    CHECK(hdr.getFirstHeaderPointer() == 0xBE);
}

