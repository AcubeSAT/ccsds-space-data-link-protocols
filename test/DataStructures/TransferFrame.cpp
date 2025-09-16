#include "TransferFrameTC.hpp"
#include "TransferFrameTM.hpp"
#include "catch2/catch_all.hpp"

using namespace CCSDSDataLinkLayer;

TEST_CASE("TransferFrameTC", "[Data Structures]") {
    const uint16_t maxFrameLength = 50;
    uint8_t frameData[maxFrameLength];

    const Defs::ServiceType serviceType = Defs::ServiceType::TYPE_BC; // -> bypass and ctrl&cmd flags: 1
    const Defs::Vcid vcid = 10;
    const Defs::Scid scid = 456;
    const bool segHdrPresent = true;
    Defs::SequenceFlag sequenceFlag = Defs::SequenceFlag::SEGMENTATION_END;
    const Defs::Mapid mapId = 5;

    auto frameTc =
        TransferFrameTC(frameData, serviceType, vcid, scid, maxFrameLength, segHdrPresent, sequenceFlag, mapId);

    SECTION("Ensuring all fields are placed in the correct positions") {
        // Note: The frame sequence number is not set from the constructor, thus not checked here
        CHECK((frameData[0] >> 6U) == static_cast<uint8_t>(Defs::TransferFrameVersionNumber::TM_TC_SYNCHRONOUS_TRANSFER_FRAME_V1));
        CHECK(((frameData[0] >> 5U) & 0x1) == 1); // bypass flag
        CHECK(((frameData[0] >> 5U) & 0x1) == 1); // ctrl&cmd flag
        CHECK(((static_cast<uint16_t>(frameData[0] & 0x03) << 8U) | static_cast<uint16_t>(frameData[1])) == scid);
        CHECK(((frameData[2] >> 2U) & 0x3F) == vcid);
        CHECK(((static_cast<uint16_t>(frameData[2] & 0x03) << 8U) | static_cast<uint16_t>(frameData[3])) == maxFrameLength);
    }

    SECTION("Setters and Getters") {
        uint16_t newLength = 25;
        uint8_t sequenceNumber = 35;
        bool newSegHdrPresent = ~segHdrPresent;

        frameTc.setFrameLength(newLength);
        frameTc.setTransferFrameSequenceNumber(sequenceNumber);
        frameTc.setSegmentationHeaderPresentFlag(newSegHdrPresent);

        CHECK(frameTc.getTransferFrameVersionNumber() == Defs::TransferFrameVersionNumber::TM_TC_SYNCHRONOUS_TRANSFER_FRAME_V1);
        CHECK(frameTc.getBypassFlag() == true);
        CHECK(frameTc.getCtrlAndCmdFlag() == true);
        CHECK(frameTc.getServiceType() == serviceType);
        CHECK(frameTc.getSpacecraftId() == scid);
        CHECK(frameTc.getVirtualChannelId() == vcid);
        CHECK(frameTc.getFrameLength() == newLength);
        CHECK(frameTc.getTransferFrameSequenceNumber() == sequenceNumber);
        CHECK(frameTc.getMapId().value() == mapId);
        CHECK(frameTc.getSequenceFlag() == sequenceFlag);
    }
}

TEST_CASE("TransferFrameTM", "[Data Structures]") {
    const uint16_t frameLength = 50;
    uint8_t frameData[frameLength];

    const Defs::Vcid vcid = 7;
    const Defs::Scid scid = 456;
    const bool operationalControlFieldPresent = true;
    const uint8_t virtualChannelFrameCount = 35;
    const bool transferFrameSecondaryHeaderPresent = true;
    const uint8_t transferFrameSecondaryHeaderLength = 10;
    const Defs::SynchronizationFlag syncFlag = Defs::SynchronizationFlag::OCTET_SYNCHRONIZED_FORWARD_ORDERED;
    const bool packetOrder = 1;
    const uint8_t segmentLengthIdentifier = Defs::SegmentLengthIdentifierLegacy;
    const uint16_t firstHeaderPointer = 567;
    const bool eccFieldPresent = true;

    auto frameTm = TransferFrameTM(frameData, frameLength, vcid, scid, operationalControlFieldPresent,
        virtualChannelFrameCount, transferFrameSecondaryHeaderPresent, transferFrameSecondaryHeaderLength,
        syncFlag, packetOrder, segmentLengthIdentifier, firstHeaderPointer, eccFieldPresent);

    SECTION("Ensuring all fields are placed in the correct positions") {
        // Note: The master channel frame count is not set from the constructor, thus not checked here
        CHECK((frameData[0] >> 6U) == static_cast<uint8_t>(Defs::TransferFrameVersionNumber::TM_TC_SYNCHRONOUS_TRANSFER_FRAME_V1));
        CHECK(((static_cast<uint16_t>(frameData[0] & 0x3F) << 4U) | (static_cast<uint16_t>(frameData[1] & 0xF0) >> 4U)) == scid);
        CHECK(((frameData[1] & 0x0E) >> 1U) == vcid);
        CHECK((frameData[1] & 0x01) == operationalControlFieldPresent);
        CHECK(frameData[3] == virtualChannelFrameCount);
        CHECK((frameData[4] >> 7U) == transferFrameSecondaryHeaderPresent);
        CHECK(((frameData[4] >> 6U) & 0x01) == static_cast<uint8_t>(syncFlag));
        CHECK(((frameData[4] >> 5U) & 0x01) == packetOrder);
        CHECK(((frameData[4] >> 3U) & 0x03) == segmentLengthIdentifier);
        CHECK(((static_cast<uint16_t>(frameData[4] & 0x07) << 8U) | static_cast<uint16_t>(frameData[5] & 0xFF)) == firstHeaderPointer);
        CHECK((frameData[6] >> 6) == static_cast<uint8_t>(Defs::SecondaryHeaderVersionNumber::VERSION_1));
        CHECK((frameData[6] & 0x3F) == transferFrameSecondaryHeaderLength - 1);
    }

    SECTION("Setters and Getters") {
        bool newOperationalControlFieldPresent = ~operationalControlFieldPresent;
        uint8_t masterChannelFrameCount = 35;
        frameTm.setMasterChannelFrameCount(masterChannelFrameCount);
        frameTm.setOperationalControlField(newOperationalControlFieldPresent);

        CHECK(frameTm.getTransferFrameVersionNumber() == Defs::TransferFrameVersionNumber::TM_TC_SYNCHRONOUS_TRANSFER_FRAME_V1);
        CHECK(frameTm.getSpacecraftId() == scid);
        CHECK(frameTm.getVirtualChannelId() == vcid);
        CHECK(frameTm.getOperationalControlFieldFlag() == newOperationalControlFieldPresent);
        CHECK(frameTm.getMasterChannelFrameCount() == masterChannelFrameCount);
        CHECK(frameTm.getVirtualChannelFrameCount() == virtualChannelFrameCount);
        CHECK(frameTm.getTransferFrameSecondaryHeaderFlag() == transferFrameSecondaryHeaderPresent);
        CHECK(frameTm.getSynchronizationFlag() == syncFlag);
        CHECK(frameTm.getPacketOrderFlag() == packetOrder);
        CHECK(frameTm.getSegmentLengthId() == segmentLengthIdentifier);
        CHECK(frameTm.getFirstHeaderPointer() == firstHeaderPointer);
        CHECK(frameTm.getSecondaryHeaderVersionNumber() == Defs::SecondaryHeaderVersionNumber::VERSION_1);
        CHECK(frameTm.getSecondaryHeaderLength() == transferFrameSecondaryHeaderLength);
    }
}