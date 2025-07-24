#include "FramePrintingFunctions.hpp"
#include "etl/string.h"
#include "VirtualChannel.hpp"
#include "StructureGeneration.hpp"
#include "Logger.hpp"

namespace CCSDSDataLinkLayer {
    static void printTransferFrameTM(Defs::VcidScidKey key,
                                     const TransferFrameTM &TransferFrameTM,
                                     const bool verbosePrimaryHeader,
                                     const bool verboseOCF) {
        // get channel parameters
#if defined(INCLUDE_SPACE_SEGMENT_CODE)
        VirtualChannelSsTm& vcChan = Objects::virtualChannelSsTmMap.at(key);
        const bool ocfPresent = vcChan.getOperationalControlFieldPresent();
        const uint8_t pcid = Objects::masterChannelSsTmMap.at(vcChan.getParentScid()).getParentPcid();
        const bool eccPresent = Objects::physicalChannelMap.at(pcid).getFrameErrorControlFieldPresent();
        const uint16_t secHeaderLength = vcChan.getSecondaryHeaderLength();
        const uint16_t transferFrameLength = Objects::physicalChannelMap.at(pcid).getTMFrameLength();
#elif defined(INCLUDE_GROUND_SEGMENT_CODE)
        return; // gs tm is unimplimented
#else
        return;
#endif

        const uint16_t transferFrameDataFieldLength = transferFrameLength -
                                                      Defs::TmPrimaryHeaderSize - secHeaderLength - ocfPresent *
                                                      Defs::TmOperationalControlFieldSize -
                                                      eccPresent * Defs::ErrorControlFieldSize;

        static etl::string<Defs::TmHelperFuncMaxMessageSize> debugOutput;
        uint8_t *dataPtr = TransferFrameTM.getFrameData();

        // Primary Header fields
        debugOutput.append("\nTM FRAME\n- Primary Header -");
        if (verbosePrimaryHeader) {
            debugOutput.append("\nTFVN: ");
            debugOutput.append(std::to_string((dataPtr[0] & 0xC0) >> 6).c_str());
            debugOutput.append("\nSCID: ");
            debugOutput.append(std::to_string((static_cast<uint16_t>(dataPtr[0] & 0x3F) << 4U) |
                                              (static_cast<uint16_t>(dataPtr[1] & 0xF0) >> 4U)).c_str());
            debugOutput.append("\nVCID: ");
            debugOutput.append(std::to_string(((dataPtr[1] & 0x0E)) >> 1U).c_str());
            debugOutput.append("\nOCF flag: ");
            debugOutput.append(std::to_string((dataPtr[1]) & 0x01).c_str());
            debugOutput.append("\nMC Frame Count: ");
            debugOutput.append(std::to_string(dataPtr[2]).c_str());
            debugOutput.append("\nVC Frame Count: ");
            debugOutput.append(std::to_string(dataPtr[3]).c_str());
            debugOutput.append("\nSecondary Header Flag: ");
            debugOutput.append(std::to_string((dataPtr[4] & 0x80) >> 7U).c_str());
            debugOutput.append("\nSync Flag: ");
            debugOutput.append(std::to_string((dataPtr[4] & 0x40) >> 6U).c_str());
            debugOutput.append("\nPacket Order Flag: ");
            debugOutput.append(std::to_string((dataPtr[4] & 0x20) >> 5U).c_str());
            debugOutput.append("\nSegment Length Id: ");
            debugOutput.append(std::to_string((dataPtr[4] >> 3) & 0x3).c_str());
            debugOutput.append("\nFirst Header Pointer: ");
            debugOutput.append(std::to_string(((static_cast<uint16_t>(((dataPtr[4]) & 0x07)) << 8U) |
                                               (static_cast<uint16_t>((dataPtr[5]))))).c_str());
        } else {
            debugOutput.append("\n| ");
            debugOutput.append(std::to_string((dataPtr[0] & 0xC0) >> 6).c_str());
            debugOutput.append(" | ");
            debugOutput.append(std::to_string((static_cast<uint16_t>(dataPtr[0] & 0x3F) << 4U) |
                                              (static_cast<uint16_t>(dataPtr[1] & 0xF0) >> 4U)).c_str());
            debugOutput.append(" | ");
            debugOutput.append(std::to_string(((dataPtr[1] & 0x0E)) >> 1U).c_str());
            debugOutput.append(" | ");
            debugOutput.append(std::to_string((dataPtr[1]) & 0x01).c_str());
            debugOutput.append(" | ");
            debugOutput.append(std::to_string(dataPtr[2]).c_str());
            debugOutput.append(" | ");
            debugOutput.append(std::to_string(dataPtr[3]).c_str());
            debugOutput.append(" | ");
            debugOutput.append(std::to_string((dataPtr[4] & 0x80) >> 7U).c_str());
            debugOutput.append(" | ");
            debugOutput.append(std::to_string((dataPtr[4] & 0x40) >> 6U).c_str());
            debugOutput.append(" | ");
            debugOutput.append(std::to_string((dataPtr[4] & 0x20) >> 5U).c_str());
            debugOutput.append(" | ");
            debugOutput.append(std::to_string((dataPtr[4] >> 3) & 0x3).c_str());
            debugOutput.append(" | ");
            debugOutput.append(std::to_string(((static_cast<uint16_t>(((dataPtr[4]) & 0x07)) << 8U) |
                                               (static_cast<uint16_t>((dataPtr[5]))))).c_str());
            debugOutput.append(" | ");
        }

        // Secondary Header (currently unimplemented)
        // @TODO Modify service accordingly if the secondary header is implemented

        // Data Field
        debugOutput.append("\n- Data Field -\n| ");
        for (uint16_t i = 0; i < transferFrameDataFieldLength - 1; i++) {
            debugOutput.append(std::to_string(dataPtr[Defs::TmPrimaryHeaderSize + i]).c_str());
            debugOutput.append(" | ");
        }
        debugOutput.append(std::to_string(dataPtr[Defs::TmPrimaryHeaderSize + transferFrameDataFieldLength]).c_str());
        debugOutput.append(" | ");

        // Operational Control field (It is assumed that the ocf field carries a "CLCW", as defined in the TC Data Link
        // Protocol)
        debugOutput.append("\n- Operational Control Field -");
        if (ocfPresent) {
            const uint16_t offset = Defs::TmPrimaryHeaderSize + transferFrameDataFieldLength;
            if (verboseOCF) {
                debugOutput.append("\nControl Word Type: ");
                debugOutput.append(std::to_string((dataPtr[offset] & 0x80) >> 7).c_str());
                debugOutput.append("\nCLCW Version Number: ");
                debugOutput.append(std::to_string((dataPtr[offset] & 0x60) >> 5).c_str());
                debugOutput.append("\nStatus Field: ");
                debugOutput.append(std::to_string((dataPtr[offset] & 0x1C) >> 2).c_str());
                debugOutput.append("\nCOP In Effect: ");
                debugOutput.append(std::to_string(dataPtr[offset] & 0x04).c_str());
                debugOutput.append("\nVCID: ");
                debugOutput.append(std::to_string((dataPtr[offset + 1] & 0xFC) >> 2).c_str());
                debugOutput.append("\nReserved Spare: ");
                debugOutput.append(std::to_string(dataPtr[offset + 1] & 0x04).c_str());
                debugOutput.append("\nNo RF Avail: ");
                debugOutput.append(std::to_string((dataPtr[offset + 2] & 0x80) >> 7).c_str());
                debugOutput.append("\nNo Bit Lock: ");
                debugOutput.append(std::to_string((dataPtr[offset + 2] & 0x40) >> 6).c_str());
                debugOutput.append("\nLockout: ");
                debugOutput.append(std::to_string((dataPtr[offset + 2] & 0x20) >> 5).c_str());
                debugOutput.append("\nWait: ");
                debugOutput.append(std::to_string((dataPtr[offset + 2] & 0x10) >> 4).c_str());
                debugOutput.append("\nRetransmit: ");
                debugOutput.append(std::to_string((dataPtr[offset + 2] & 0x08) >> 3).c_str());
                debugOutput.append("\nFarm-B Counter: ");
                debugOutput.append(std::to_string((dataPtr[offset + 2] & 0x02) >> 1).c_str());
                debugOutput.append("\nReserved Spare:");
                debugOutput.append(std::to_string(dataPtr[offset + 2] & 0x01).c_str());
                debugOutput.append("\nReport Value: ");
                debugOutput.append(std::to_string(dataPtr[offset + 3]).c_str());
            } else {
                debugOutput.append("\n| ");
                debugOutput.append(std::to_string((dataPtr[offset] & 0x80) >> 7).c_str());
                debugOutput.append(" | ");
                debugOutput.append(std::to_string((dataPtr[offset] & 0x60) >> 5).c_str());
                debugOutput.append(" | ");
                debugOutput.append(std::to_string((dataPtr[offset] & 0x1C) >> 2).c_str());
                debugOutput.append(" | ");
                debugOutput.append(std::to_string(dataPtr[offset] & 0x04).c_str());
                debugOutput.append(" | ");
                debugOutput.append(std::to_string((dataPtr[offset + 1] & 0xFC) >> 2).c_str());
                debugOutput.append(" | ");
                debugOutput.append(std::to_string(dataPtr[offset + 1] & 0x04).c_str());
                debugOutput.append(" | ");
                debugOutput.append(std::to_string((dataPtr[offset + 2] & 0x80) >> 7).c_str());
                debugOutput.append(" | ");
                debugOutput.append(std::to_string((dataPtr[offset + 2] & 0x40) >> 6).c_str());
                debugOutput.append(" | ");
                debugOutput.append(std::to_string((dataPtr[offset + 2] & 0x20) >> 5).c_str());
                debugOutput.append(" | ");
                debugOutput.append(std::to_string((dataPtr[offset + 2] & 0x10) >> 4).c_str());
                debugOutput.append(" | ");
                debugOutput.append(std::to_string((dataPtr[offset + 2] & 0x08) >> 3).c_str());
                debugOutput.append(" | ");
                debugOutput.append(std::to_string((dataPtr[offset + 2] & 0x02) >> 1).c_str());
                debugOutput.append(" | ");
                debugOutput.append(std::to_string(dataPtr[offset + 2] & 0x01).c_str());
                debugOutput.append(" | ");
                debugOutput.append(std::to_string(dataPtr[offset + 3]).c_str());
                debugOutput.append(" | ");
            }
        }

        // Error Control Field
        debugOutput.append("\n- Error Control Field -\n");
        if (eccPresent) {
            uint16_t offset = Defs::TmPrimaryHeaderSize + transferFrameDataFieldLength
                              + ocfPresent * Defs::TmOperationalControlFieldSize;
            debugOutput.append(std::to_string((static_cast<uint16_t >(dataPtr[offset]) << 8) |
                                              static_cast<uint16_t>(dataPtr[offset + 1])).c_str());
        }
        debugOutput.append("\n");

        LOG_DEBUG << debugOutput.c_str();
    }

    void printTransferFrameTC(Defs::VcidScidKey key,
                              const TransferFrameTC &TransferFrameTC,
                              bool verbosePrimaryHeader) {
        // TODO also print the security header and trailer

        // get channel parameters
        SecurityAssociation* sa;
#if defined(INCLUDE_SPACE_SEGMENT_CODE)
        VirtualChannelSsTc& vcChan = Objects::virtualChannelSsTcMap.at(key);
        const uint8_t pcid = Objects::masterChannelSsTcMap.at(vcChan.getParentScid()).getParentPcid();
        if (vcChan.getAssociatedSdlsSPI().has_value()) {
            sa = &Objects::saSpaceSegmentMap.at(key);
        }
#elif defined(INCLUDE_GROUND_SEGMENT_CODE)
        VirtualChannelGsTc vcChan = Objects::virtualChannelGsTcMap.at(key);
        const uint8_t pcid = Objects::masterChannelGsTcMap.at(vcChan.getParentScid()).getParentPcid();
        if (vcChan.getAssociatedSdlsSPI().has_value()) {
            sa = &Objects::saGroundSegmentMap.at(key);
        }
#else
        return;
#endif
        const bool eccPresent = Objects::physicalChannelMap.at(pcid).getFrameErrorControlFieldPresent();
        const bool segHeaderPresent = vcChan.getSegmentHeaderPresent();
        uint16_t securityHeaderLength = 0;
        uint16_t securityTrailerLength = 0;
        if (vcChan.getAssociatedSdlsSPI().has_value()) {
            securityHeaderLength = sa->getSecurityHeaderLength();
            securityTrailerLength = sa->getSecurityTrailerLength();
        }

        const uint16_t transferFrameDataFieldLength =
                TransferFrameTC.getTransferFrameLength() - Defs::TcPrimaryHeaderSize
                - securityHeaderLength - securityTrailerLength - eccPresent * Defs::ErrorControlFieldSize;

        static etl::string<Defs::TcHelperFuncMaxMessageSize> debugOutput;
        debugOutput.clear();
        const uint8_t *dataPtr = TransferFrameTC.getFrameData();

        // Primary Header fields
        debugOutput.append("\nTC FRAME\n- Primary Header -");
        if (verbosePrimaryHeader) {
            debugOutput.append("\nTFVN: ");
            debugOutput.append(std::to_string((dataPtr[0] & 0xC0) >> 6).c_str());

            const bool byPassFlag = (dataPtr[0] >> 5) & 0x01;
            const bool ctrlCommandFlag = (dataPtr[0] >> 4) & 0x01;
            debugOutput.append("\nBypass Flag: ");
            debugOutput.append(std::to_string(byPassFlag).c_str());
            debugOutput.append("\nCtrl and Command Flag: ");
            debugOutput.append(std::to_string(ctrlCommandFlag).c_str());
            if (!byPassFlag && !ctrlCommandFlag) {
                debugOutput.append(" (Type-AD)");
            } else if (!byPassFlag && ctrlCommandFlag) {
                debugOutput.append(" (Reserved Type)");
            } else if (byPassFlag && !ctrlCommandFlag) {
                debugOutput.append(" (Type-BD)");
            } else {
                debugOutput.append(" (Type-BC)");
            }

            debugOutput.append("\nReserved Spare: ");
            debugOutput.append(std::to_string((dataPtr[0] & 0x0C) >> 2).c_str());
            debugOutput.append("\nSCID: ");
            debugOutput.append(std::to_string((static_cast<uint16_t>(dataPtr[0] & 0x03) << 8U) |
                                              (static_cast<uint16_t>(dataPtr[1]))).c_str());
            debugOutput.append("\nVCID: ");
            debugOutput.append(std::to_string((dataPtr[2] >> 2U) & 0x3F).c_str());
            debugOutput.append("\nFrame Length: ");
            debugOutput.append(std::to_string(
                    (static_cast<uint16_t>(dataPtr[2] & 0x03) << 8U) | (static_cast<uint16_t>(dataPtr[3]))).c_str());
            debugOutput.append("\nFrame sequence number: ");
            debugOutput.append(std::to_string(dataPtr[4]).c_str());
        } else {
            debugOutput.append("\n| ");
            debugOutput.append(std::to_string((dataPtr[0] & 0xC0) >> 6).c_str());
            debugOutput.append(" | ");
            debugOutput.append(std::to_string((dataPtr[0] >> 5U) & 0x01).c_str());
            debugOutput.append(" | ");
            debugOutput.append(std::to_string((dataPtr[0] >> 4U) & 0x01).c_str());
            debugOutput.append(" | ");
            debugOutput.append(std::to_string((dataPtr[0] & 0x0C) >> 2).c_str());
            debugOutput.append(" | ");
            debugOutput.append(std::to_string((static_cast<uint16_t>(dataPtr[0] & 0x03) << 8U) |
                                              (static_cast<uint16_t>(dataPtr[1]))).c_str());
            debugOutput.append(" | ");
            debugOutput.append(std::to_string((dataPtr[2] >> 2U) & 0x3F).c_str());
            debugOutput.append(" | ");
            debugOutput.append(std::to_string(
                    (static_cast<uint16_t>(dataPtr[2] & 0x03) << 8U) | (static_cast<uint16_t>(dataPtr[3]))).c_str());
            debugOutput.append(" | ");
            debugOutput.append(std::to_string(dataPtr[4]).c_str());
            debugOutput.append(" | ");
        }

        // Segment Header
        debugOutput.append("\n- Segment Header -");
        if (segHeaderPresent) {
            const bool firstFlag = (dataPtr[Defs::TcPrimaryHeaderSize] & 0x80) >> 7;
            const bool secondFlag = (dataPtr[Defs::TcPrimaryHeaderSize] & 0x40) >> 6;
            debugOutput.append("\nSequence Flags: ");
            debugOutput.append(std::to_string(firstFlag).c_str());
            debugOutput.append(" ");
            debugOutput.append(std::to_string(secondFlag).c_str());
            if (!firstFlag && secondFlag) {
                debugOutput.append(" (first portion)");
            } else if (!firstFlag && !secondFlag) {
                debugOutput.append(" (continuing portion)");
            } else if (firstFlag && !secondFlag) {
                debugOutput.append(" (last portion)");
            } else {
                debugOutput.append(" (no segmentation)");
            }

            debugOutput.append("\nMAP ID: ");
            debugOutput.append(std::to_string(dataPtr[5] & 0x3F).c_str());
        }

        // Data Field
        debugOutput.append("\n- Data Field -\n| ");
        // The segment header is the conuted as the first byter of the dataField, if it exists
        for (uint16_t i = segHeaderPresent * Defs::TcSegmentHeaderSize + securityHeaderLength;
             i < transferFrameDataFieldLength - 1; i++) {
            debugOutput.append(std::to_string(dataPtr[Defs::TcPrimaryHeaderSize + i]).c_str());
            debugOutput.append(" | ");
        }
        debugOutput.append(std::to_string(dataPtr[Defs::TcPrimaryHeaderSize + transferFrameDataFieldLength]).c_str());
        debugOutput.append(" | ");

        // Error Control Field
        debugOutput.append("\n- Error Control Field -\n");
        if (eccPresent) {
            debugOutput.append(std::to_string((static_cast<uint16_t >(dataPtr[Defs::TcPrimaryHeaderSize +
                                                                              transferFrameDataFieldLength +
                                                                              securityHeaderLength +
                                                                              securityTrailerLength]) << 8) |
                                              static_cast<uint16_t>(dataPtr[Defs::TcPrimaryHeaderSize +
                                                                            transferFrameDataFieldLength +
                                                                            securityHeaderLength +
                                                                            securityTrailerLength +
                                                                            1])).c_str());
        }
        debugOutput.append("\n");

        LOG_DEBUG << debugOutput.c_str();
    }
} // CCSDSDataLinkLayer