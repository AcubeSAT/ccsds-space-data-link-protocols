#include <string>
#include "etl/string.h"
#include "FramePrintingFunctions.hpp"
#include "VirtualChannel.hpp"
#include "ChannelObjects.hpp"
#include "Logger.hpp"

namespace CCSDSDataLinkLayer {
#if defined(INCLUDE_FRAME_PRINTING_FUNCTIONS)
    etl::string<FramePrintingFunctions::TmPrintingFuncMaxMessageSize>
    FramePrintingFunctions::tmDebugOutput;

    etl::string<FramePrintingFunctions::TcPrintingFuncMaxMessageSize>
        FramePrintingFunctions::tcDebugOutput;

#if defined(INCLUDE_SPACE_SEGMENT_CODE)
    etl::expected<void, ServiceChannelNotification> FramePrintingFunctions::printTransferFrameTM(
        Objects::VirtualChannelTmName vcChanName,
        const TransferFrameTM &transferFrameTM,
        bool verbosePrimaryHeader,
        bool verboseOcfField,
        void (*ocfAppendFunc)(bool, const uint8_t*)) {

        // gather information from the channel
        const Defs::VcidScidKey key = static_cast<Defs::VcidScidKey>(vcChanName);
        const auto it = Objects::virtualChannelSsTmMap.find(key);
        if (it == Objects::virtualChannelSsTmMap.end()) {
            return etl::unexpected(ServiceChannelNotification::INVALID_CHANNEL_NAME);
        }
        VirtualChannelSsTm &vcChan = it->second;
        MasterChannelSsTm& mcChan = Objects::masterChannelSsTmMap.at(vcChan.getParentScid());
        const PhysicalChannel& phyChan = Objects::physicalChannelMap.at(mcChan.getParentPcid());

        const bool ocfPresent = vcChan.getOperationalControlFieldPresent();
        const bool eccPresent = phyChan.getFrameErrorControlFieldPresent();
        const uint16_t secHeaderLength = vcChan.getSecondaryHeaderLength();
        const uint16_t transferFrameLength = phyChan.getTMFrameLength();

        const uint16_t transferFrameDataFieldLength = transferFrameLength -
                                                      Defs::TmPrimaryHeaderSize - secHeaderLength - ocfPresent *
                                                      Defs::TmOperationalControlFieldSize -
                                                      eccPresent * Defs::ErrorControlFieldSize;
        const uint8_t *dataPtr = transferFrameTM.getFrameData();
        tmDebugOutput.clear();

        // Primary Header fields
        tmDebugOutput.append("\nTM FRAME\n- Primary Header -");
        if (verbosePrimaryHeader) {
            tmDebugOutput.append("\nTFVN: ");
            tmDebugOutput.append(std::to_string((dataPtr[0] & 0xC0) >> 6).c_str());
            tmDebugOutput.append("\nSCID: ");
            tmDebugOutput.append(std::to_string((static_cast<uint16_t>(dataPtr[0] & 0x3F) << 4U) |
                                              (static_cast<uint16_t>(dataPtr[1] & 0xF0) >> 4U)).c_str());
            tmDebugOutput.append("\nVCID: ");
            tmDebugOutput.append(std::to_string(((dataPtr[1] & 0x0E)) >> 1U).c_str());
            tmDebugOutput.append("\nOCF flag: ");
            tmDebugOutput.append(std::to_string((dataPtr[1]) & 0x01).c_str());
            tmDebugOutput.append("\nMC Frame Count: ");
            tmDebugOutput.append(std::to_string(dataPtr[2]).c_str());
            tmDebugOutput.append("\nVC Frame Count: ");
            tmDebugOutput.append(std::to_string(dataPtr[3]).c_str());
            tmDebugOutput.append("\nSecondary Header Flag: ");
            tmDebugOutput.append(std::to_string((dataPtr[4] & 0x80) >> 7U).c_str());
            tmDebugOutput.append("\nSync Flag: ");
            tmDebugOutput.append(std::to_string((dataPtr[4] & 0x40) >> 6U).c_str());
            tmDebugOutput.append("\nPacket Order Flag: ");
            tmDebugOutput.append(std::to_string((dataPtr[4] & 0x20) >> 5U).c_str());
            tmDebugOutput.append("\nSegment Length Id: ");
            tmDebugOutput.append(std::to_string((dataPtr[4] >> 3) & 0x3).c_str());
            tmDebugOutput.append("\nFirst Header Pointer: ");
            tmDebugOutput.append(std::to_string(((static_cast<uint16_t>(((dataPtr[4]) & 0x07)) << 8U) |
                                               (static_cast<uint16_t>((dataPtr[5]))))).c_str());
        } else {
            tmDebugOutput.append("\n| ");
            tmDebugOutput.append(std::to_string((dataPtr[0] & 0xC0) >> 6).c_str());
            tmDebugOutput.append(" | ");
            tmDebugOutput.append(std::to_string((static_cast<uint16_t>(dataPtr[0] & 0x3F) << 4U) |
                                              (static_cast<uint16_t>(dataPtr[1] & 0xF0) >> 4U)).c_str());
            tmDebugOutput.append(" | ");
            tmDebugOutput.append(std::to_string(((dataPtr[1] & 0x0E)) >> 1U).c_str());
            tmDebugOutput.append(" | ");
            tmDebugOutput.append(std::to_string((dataPtr[1]) & 0x01).c_str());
            tmDebugOutput.append(" | ");
            tmDebugOutput.append(std::to_string(dataPtr[2]).c_str());
            tmDebugOutput.append(" | ");
            tmDebugOutput.append(std::to_string(dataPtr[3]).c_str());
            tmDebugOutput.append(" | ");
            tmDebugOutput.append(std::to_string((dataPtr[4] & 0x80) >> 7U).c_str());
            tmDebugOutput.append(" | ");
            tmDebugOutput.append(std::to_string((dataPtr[4] & 0x40) >> 6U).c_str());
            tmDebugOutput.append(" | ");
            tmDebugOutput.append(std::to_string((dataPtr[4] & 0x20) >> 5U).c_str());
            tmDebugOutput.append(" | ");
            tmDebugOutput.append(std::to_string((dataPtr[4] >> 3) & 0x3).c_str());
            tmDebugOutput.append(" | ");
            tmDebugOutput.append(std::to_string(((static_cast<uint16_t>(((dataPtr[4]) & 0x07)) << 8U) |
                                               (static_cast<uint16_t>((dataPtr[5]))))).c_str());
            tmDebugOutput.append(" | ");
        }

        // Secondary Header (currently unimplemented)
        // @TODO Modify service accordingly if the secondary header is implemented

        // Data Field
        tmDebugOutput.append("\n- Data Field -\n| ");
        for (uint16_t i = 0; i < transferFrameDataFieldLength - 1; i++) {
            tmDebugOutput.append(std::to_string(dataPtr[Defs::TmPrimaryHeaderSize + i]).c_str());
            tmDebugOutput.append(" | ");
        }
        tmDebugOutput.append(std::to_string(dataPtr[Defs::TmPrimaryHeaderSize + transferFrameDataFieldLength]).c_str());
        tmDebugOutput.append(" | ");

        // Operational Control field (It is assumed that the ocf field carries a "CLCW", as defined in the TC Data Link
        // Protocol)
        tmDebugOutput.append("\n- Operational Control Field -");
        if (ocfPresent) {
            const uint8_t* ocfFieldSrc = dataPtr + Defs::TmPrimaryHeaderSize + transferFrameDataFieldLength;
            ocfAppendFunc(verboseOcfField, ocfFieldSrc);
        }

        // Error Control Field
        tmDebugOutput.append("\n- Error Control Field -\n");
        if (eccPresent) {
            uint16_t offset = Defs::TmPrimaryHeaderSize + transferFrameDataFieldLength
                              + ocfPresent * Defs::TmOperationalControlFieldSize;
            tmDebugOutput.append(std::to_string((static_cast<uint16_t >(dataPtr[offset]) << 8) |
                                              static_cast<uint16_t>(dataPtr[offset + 1])).c_str());
        }
        tmDebugOutput.append("\n");

        LOG_DEBUG << tmDebugOutput.c_str();
    }
#endif

    etl::expected<void, ServiceChannelNotification> FramePrintingFunctions::printTransferFrameTC(Objects::VirtualChannelTcName vcChanName,
                                     const TransferFrameTC &transferFrameTC,
                                     bool verbosePrimaryHeader) {
        // TODO also print the security header and trailer

        bool eccPresent;
        bool segHeaderPresent;
        uint16_t securityHeaderLength = 0;
        uint16_t securityTrailerLength = 0;

        // gather information from a channel
#if defined(INCLUDE_SPACE_SEGMENT_CODE)
        const Defs::VcidScidKey key = static_cast<Defs::VcidScidKey>(vcChanName);
        const auto vcIt = Objects::virtualChannelSsTcMap.find(key);
        if (vcIt == Objects::virtualChannelSsTcMap.end()) {
            return etl::unexpected(ServiceChannelNotification::INVALID_CHANNEL_NAME);
        }
        const VirtualChannelSsTc& vcChan = vcIt->second;
        MasterChannelSsTc& mcChan = Objects::masterChannelSsTcMap.at(vcChan.getParentScid());
        const PhysicalChannel& phyChan = Objects::physicalChannelMap.at(mcChan.getParentPcid());

        eccPresent = phyChan.getFrameErrorControlFieldPresent();
        segHeaderPresent = vcChan.getSegmentHeaderPresent();
        if (vcChan.getAssociatedSdlsSPI().has_value()) {
            SecurityAssociation &sa = Objects::saSpaceSegmentMap.at(key);
            securityHeaderLength = sa.getSecurityHeaderLength();
            securityTrailerLength = sa.getSecurityTrailerLength();
        }
#elif defined(INCLUDE_GROUND_SEGMENT_CODE)
        const Defs::VcidScidKey key = static_cast<Defs::VcidScidKey>(vcChanName);
        const auto vcIt = Objects::virtualChannelGsTcMap.find(key);
        if (vcIt == Objects::virtualChannelGsTcMap.end()) {
            return etl::unexpected(ServiceChannelNotification::INVALID_CHANNEL_NAME);
        }
        const VirtualChannelGsTc& vcChan = vcIt->second;
        MasterChannelGsTc& mcChan = Objects::masterChannelGsTcMap.at(vcChan.getParentScid());
        const PhysicalChannel& phyChan = Objects::physicalChannelMap.at(mcChan.getParentPcid());

        eccPresent = phyChan.getFrameErrorControlFieldPresent();
        segHeaderPresent = vcChan.getSegmentHeaderPresent();
        if (vcChan.getAssociatedSdlsSPI().has_value()) {
            SecurityAssociation &sa = Objects::saGroundSegmentMap.at(key);
            securityHeaderLength = sa.getSecurityHeaderLength();
            securityTrailerLength = sa.getSecurityTrailerLength();
        }
#else
#error "Neither INCLUDE_SPACE_SEGMENT_CODE nor INCLUDE_GROUND_SEGMENT_CODE macros are defined"
#endif

        const uint16_t transferFrameDataFieldLength = transferFrameTC.getTransferFrameLength() -
                                                      Defs::TcPrimaryHeaderSize -
                                                      securityHeaderLength -
                                                      securityTrailerLength -
                                                      eccPresent * Defs::ErrorControlFieldSize;

        const uint8_t *dataPtr = transferFrameTC.getFrameData();
        tcDebugOutput.clear();

        // Primary Header fields
        tcDebugOutput.append("\nTC FRAME\n- Primary Header -");
        if (verbosePrimaryHeader) {
            tcDebugOutput.append("\nTFVN: ");
            tcDebugOutput.append(std::to_string((dataPtr[0] & 0xC0) >> 6).c_str());

            const bool byPassFlag = (dataPtr[0] >> 5) & 0x01;
            const bool ctrlCommandFlag = (dataPtr[0] >> 4) & 0x01;
            tcDebugOutput.append("\nBypass Flag: ");
            tcDebugOutput.append(std::to_string(byPassFlag).c_str());
            tcDebugOutput.append("\nCtrl and Command Flag: ");
            tcDebugOutput.append(std::to_string(ctrlCommandFlag).c_str());
            if (!byPassFlag && !ctrlCommandFlag) {
                tcDebugOutput.append(" (Type-AD)");
            } else if (!byPassFlag && ctrlCommandFlag) {
                tcDebugOutput.append(" (Reserved Type)");
            } else if (byPassFlag && !ctrlCommandFlag) {
                tcDebugOutput.append(" (Type-BD)");
            } else {
                tcDebugOutput.append(" (Type-BC)");
            }

            tcDebugOutput.append("\nReserved Spare: ");
            tcDebugOutput.append(std::to_string((dataPtr[0] & 0x0C) >> 2).c_str());
            tcDebugOutput.append("\nSCID: ");
            tcDebugOutput.append(std::to_string((static_cast<uint16_t>(dataPtr[0] & 0x03) << 8U) |
                                              (static_cast<uint16_t>(dataPtr[1]))).c_str());
            tcDebugOutput.append("\nVCID: ");
            tcDebugOutput.append(std::to_string((dataPtr[2] >> 2U) & 0x3F).c_str());
            tcDebugOutput.append("\nFrame Length: ");
            tcDebugOutput.append(std::to_string(
                    (static_cast<uint16_t>(dataPtr[2] & 0x03) << 8U) | (static_cast<uint16_t>(dataPtr[3]))).c_str());
            tcDebugOutput.append("\nFrame sequence number: ");
            tcDebugOutput.append(std::to_string(dataPtr[4]).c_str());
        } else {
            tcDebugOutput.append("\n| ");
            tcDebugOutput.append(std::to_string((dataPtr[0] & 0xC0) >> 6).c_str());
            tcDebugOutput.append(" | ");
            tcDebugOutput.append(std::to_string((dataPtr[0] >> 5U) & 0x01).c_str());
            tcDebugOutput.append(" | ");
            tcDebugOutput.append(std::to_string((dataPtr[0] >> 4U) & 0x01).c_str());
            tcDebugOutput.append(" | ");
            tcDebugOutput.append(std::to_string((dataPtr[0] & 0x0C) >> 2).c_str());
            tcDebugOutput.append(" | ");
            tcDebugOutput.append(std::to_string((static_cast<uint16_t>(dataPtr[0] & 0x03) << 8U) |
                                              (static_cast<uint16_t>(dataPtr[1]))).c_str());
            tcDebugOutput.append(" | ");
            tcDebugOutput.append(std::to_string((dataPtr[2] >> 2U) & 0x3F).c_str());
            tcDebugOutput.append(" | ");
            tcDebugOutput.append(std::to_string(
                    (static_cast<uint16_t>(dataPtr[2] & 0x03) << 8U) | (static_cast<uint16_t>(dataPtr[3]))).c_str());
            tcDebugOutput.append(" | ");
            tcDebugOutput.append(std::to_string(dataPtr[4]).c_str());
            tcDebugOutput.append(" | ");
        }

        // Segment Header
        tcDebugOutput.append("\n- Segment Header -");
        if (segHeaderPresent) {
            const bool firstFlag = (dataPtr[Defs::TcPrimaryHeaderSize] & 0x80) >> 7;
            const bool secondFlag = (dataPtr[Defs::TcPrimaryHeaderSize] & 0x40) >> 6;
            tcDebugOutput.append("\nSequence Flags: ");
            tcDebugOutput.append(std::to_string(firstFlag).c_str());
            tcDebugOutput.append(" ");
            tcDebugOutput.append(std::to_string(secondFlag).c_str());
            if (!firstFlag && secondFlag) {
                tcDebugOutput.append(" (first portion)");
            } else if (!firstFlag && !secondFlag) {
                tcDebugOutput.append(" (continuing portion)");
            } else if (firstFlag && !secondFlag) {
                tcDebugOutput.append(" (last portion)");
            } else {
                tcDebugOutput.append(" (no segmentation)");
            }

            tcDebugOutput.append("\nMAP ID: ");
            tcDebugOutput.append(std::to_string(dataPtr[5] & 0x3F).c_str());
        }

        // Data Field
        tcDebugOutput.append("\n- Data Field -\n| ");
        // The segment header is the conuted as the first byter of the dataField, if it exists
        for (uint16_t i = segHeaderPresent * Defs::TcSegmentHeaderSize + securityHeaderLength;
             i < transferFrameDataFieldLength - 1; i++) {
            tcDebugOutput.append(std::to_string(dataPtr[Defs::TcPrimaryHeaderSize + i]).c_str());
            tcDebugOutput.append(" | ");
        }
        tcDebugOutput.append(std::to_string(dataPtr[Defs::TcPrimaryHeaderSize + transferFrameDataFieldLength]).c_str());
        tcDebugOutput.append(" | ");

        // Error Control Field
        tcDebugOutput.append("\n- Error Control Field -\n");
        if (eccPresent) {
            tcDebugOutput.append(std::to_string((static_cast<uint16_t >(dataPtr[Defs::TcPrimaryHeaderSize +
                                                                              transferFrameDataFieldLength +
                                                                              securityHeaderLength +
                                                                              securityTrailerLength]) << 8) |
                                              static_cast<uint16_t>(dataPtr[Defs::TcPrimaryHeaderSize +
                                                                            transferFrameDataFieldLength +
                                                                            securityHeaderLength +
                                                                            securityTrailerLength +
                                                                            1])).c_str());
        }
        tcDebugOutput.append("\n");

        LOG_DEBUG << tcDebugOutput.c_str();
    }

    void FramePrintingFunctions::appendClcwField(bool verboseOutput, const uint8_t* ocfFieldSrc) {
            if (verboseOutput) {
                tmDebugOutput.append("\nControl Word Type: ");
                tmDebugOutput.append(std::to_string((ocfFieldSrc[0] & 0x80) >> 7).c_str());
                tmDebugOutput.append("\nCLCW Version Number: ");
                tmDebugOutput.append(std::to_string((ocfFieldSrc[0] & 0x60) >> 5).c_str());
                tmDebugOutput.append("\nStatus Field: ");
                tmDebugOutput.append(std::to_string((ocfFieldSrc[0] & 0x1C) >> 2).c_str());
                tmDebugOutput.append("\nCOP In Effect: ");
                tmDebugOutput.append(std::to_string(ocfFieldSrc[0] & 0x04).c_str());
                tmDebugOutput.append("\nVCID: ");
                tmDebugOutput.append(std::to_string((ocfFieldSrc[1] & 0xFC) >> 2).c_str());
                tmDebugOutput.append("\nReserved Spare: ");
                tmDebugOutput.append(std::to_string(ocfFieldSrc[1] & 0x04).c_str());
                tmDebugOutput.append("\nNo RF Avail: ");
                tmDebugOutput.append(std::to_string((ocfFieldSrc[2] & 0x80) >> 7).c_str());
                tmDebugOutput.append("\nNo Bit Lock: ");
                tmDebugOutput.append(std::to_string((ocfFieldSrc[2] & 0x40) >> 6).c_str());
                tmDebugOutput.append("\nLockout: ");
                tmDebugOutput.append(std::to_string((ocfFieldSrc[2] & 0x20) >> 5).c_str());
                tmDebugOutput.append("\nWait: ");
                tmDebugOutput.append(std::to_string((ocfFieldSrc[2] & 0x10) >> 4).c_str());
                tmDebugOutput.append("\nRetransmit: ");
                tmDebugOutput.append(std::to_string((ocfFieldSrc[2] & 0x08) >> 3).c_str());
                tmDebugOutput.append("\nFarm-B Counter: ");
                tmDebugOutput.append(std::to_string((ocfFieldSrc[2] & 0x02) >> 1).c_str());
                tmDebugOutput.append("\nReserved Spare:");
                tmDebugOutput.append(std::to_string(ocfFieldSrc[2] & 0x01).c_str());
                tmDebugOutput.append("\nReport Value: ");
                tmDebugOutput.append(std::to_string(ocfFieldSrc[3]).c_str());
            } else {
                tmDebugOutput.append("\n| ");
                tmDebugOutput.append(std::to_string((ocfFieldSrc[0] & 0x80) >> 7).c_str());
                tmDebugOutput.append(" | ");
                tmDebugOutput.append(std::to_string((ocfFieldSrc[0] & 0x60) >> 5).c_str());
                tmDebugOutput.append(" | ");
                tmDebugOutput.append(std::to_string((ocfFieldSrc[0] & 0x1C) >> 2).c_str());
                tmDebugOutput.append(" | ");
                tmDebugOutput.append(std::to_string(ocfFieldSrc[0] & 0x04).c_str());
                tmDebugOutput.append(" | ");
                tmDebugOutput.append(std::to_string((ocfFieldSrc[1] & 0xFC) >> 2).c_str());
                tmDebugOutput.append(" | ");
                tmDebugOutput.append(std::to_string(ocfFieldSrc[1] & 0x04).c_str());
                tmDebugOutput.append(" | ");
                tmDebugOutput.append(std::to_string((ocfFieldSrc[2] & 0x80) >> 7).c_str());
                tmDebugOutput.append(" | ");
                tmDebugOutput.append(std::to_string((ocfFieldSrc[2] & 0x40) >> 6).c_str());
                tmDebugOutput.append(" | ");
                tmDebugOutput.append(std::to_string((ocfFieldSrc[2] & 0x20) >> 5).c_str());
                tmDebugOutput.append(" | ");
                tmDebugOutput.append(std::to_string((ocfFieldSrc[2] & 0x10) >> 4).c_str());
                tmDebugOutput.append(" | ");
                tmDebugOutput.append(std::to_string((ocfFieldSrc[2] & 0x08) >> 3).c_str());
                tmDebugOutput.append(" | ");
                tmDebugOutput.append(std::to_string((ocfFieldSrc[2] & 0x02) >> 1).c_str());
                tmDebugOutput.append(" | ");
                tmDebugOutput.append(std::to_string(ocfFieldSrc[2] & 0x01).c_str());
                tmDebugOutput.append(" | ");
                tmDebugOutput.append(std::to_string(ocfFieldSrc[3]).c_str());
                tmDebugOutput.append(" | ");
            }
    }
#endif // INCLUDE_FRAME_PRINTING_FUNCTIONS
} // CCSDSDataLinkLayer