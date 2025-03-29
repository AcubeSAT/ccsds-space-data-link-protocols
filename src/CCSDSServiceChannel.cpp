#include "etl/optional.h"
#include "etl/iterator.h"
#include "CCSDSServiceChannel.hpp"
#include "CCSDSChannelsInterface.hpp"
#include "TransferFrameTM.hpp"
#include "CLCW.hpp"
#include "TransferFrameTC.hpp"
#include "CCSDSLoggerImpl.h"

namespace CCSDSDataLinkLayer {
	// Helper services
    void BaseServiceChannel::printTransferFrameTM(const TransferFrameTM &TransferFrameTM,
                                              const bool ocfPresent,
                                              const bool eccPresent,
                                              const bool verbosePrimaryHeader,
                                              const bool verboseOCF,
                                              uint16_t transferFrameDataFieldLength) {

        static etl::string<DefsAndUtils::TmHelperFuncMaxMessageSize> debugOutput;
        debugOutput.clear();
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
            debugOutput.append(std::to_string(dataPtr[DefsAndUtils::TmPrimaryHeaderSize + i]).c_str());
            debugOutput.append(" | ");
        }
        debugOutput.append(std::to_string(dataPtr[DefsAndUtils::TmPrimaryHeaderSize + transferFrameDataFieldLength]).c_str());
        debugOutput.append(" | ");

        // Operational Control field (It is assumed that the ocf field carries a "CLCW", as defined in the TC Data Link
        // Protocol)
        debugOutput.append("\n- Operational Control Field -");
        if (ocfPresent) {
            const uint16_t offset = DefsAndUtils::TmPrimaryHeaderSize + transferFrameDataFieldLength;
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
            uint16_t offset = DefsAndUtils::TmPrimaryHeaderSize + transferFrameDataFieldLength
                              + ocfPresent * DefsAndUtils::TmOperationalControlFieldSize;
            debugOutput.append(std::to_string((static_cast<uint16_t >(dataPtr[offset]) << 8) |
                                              static_cast<uint16_t>(dataPtr[offset + 1])).c_str());
        }
        debugOutput.append("\n");

        LOG_DEBUG << debugOutput.c_str();
    }

    void BaseServiceChannel::printTransferFrameTC(const TransferFrameTC &TransferFrameTC,
                                                  const SecurityAssociation& securityAssociation,
                                                  const bool segHeaderPresent,
                                                  const bool eccFieldPresent,
                                                  const bool verbosePrimaryHeader,
                                                  const uint16_t transferFrameDataFieldLength) {
        // TODO also print the security header and trailer

        static etl::string<DefsAndUtils::TcHelperFuncMaxMessageSize> debugOutput;
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
            const bool firstFlag = (dataPtr[DefsAndUtils::TcPrimaryHeaderSize] & 0x80) >> 7;
            const bool secondFlag = (dataPtr[DefsAndUtils::TcPrimaryHeaderSize] & 0x40) >> 6;
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
        // Technically speaking, the segment header is the first byte of the dataField, if it exists
        for (uint16_t i = segHeaderPresent * DefsAndUtils::TcSegmentHeaderSize + securityAssociation.getSecurityHeaderLength();
             i < transferFrameDataFieldLength - 1; i++) {
            debugOutput.append(std::to_string(dataPtr[DefsAndUtils::TcPrimaryHeaderSize + i]).c_str());
            debugOutput.append(" | ");
        }
        debugOutput.append(std::to_string(dataPtr[DefsAndUtils::TcPrimaryHeaderSize + transferFrameDataFieldLength]).c_str());
        debugOutput.append(" | ");

        // Error Control Field
        debugOutput.append("\n- Error Control Field -\n");
        if (eccFieldPresent) {
            debugOutput.append(std::to_string((static_cast<uint16_t >(dataPtr[DefsAndUtils::TcPrimaryHeaderSize +
                                                                              transferFrameDataFieldLength +
                                                                              securityAssociation.getSecurityHeaderLength() +
                                                                              securityAssociation.getSecurityTrailerLength()]) << 8) |
                                              static_cast<uint16_t>(dataPtr[DefsAndUtils::TcPrimaryHeaderSize +
                                                                            transferFrameDataFieldLength +
                                                                            securityAssociation.getSecurityHeaderLength() +
                                                                            securityAssociation.getSecurityTrailerLength() +
                                                                            1])).c_str());
        }
        debugOutput.append("\n");

        LOG_DEBUG << debugOutput.c_str();
    }

#ifdef SPACE_SEGMENT
	/** ==========================================
     *   TC TransferFrame - Receiving End (TC Rx)
     *  ==========================================
     */

    // All Frames Reception
    etl::expected<void, ServiceChannelNotification> ServiceChannelSpaceSegment::allFramesReceptionRequestTC(
	    const PhysicalChannel &physicalChannel,
	    MasterChannelSpaceSegmentVariant &mcChanVariant,
	    const VirtualChannelSearchFunctionType vChanSearchFunction,
	    const MapChannelSearchFunctionType mapChanSearchFunction,
	    uint8_t *frameData, const uint16_t frameLength) {
	    const BaseMasterChannel *baseMcChanPtr = ChannelsInterface::upcastToBase(mcChanVariant);
	    const uint16_t scid = DefsAndUtils::extractScidFromMcid(baseMcChanPtr->getMscid());

	    if (!physicalChannel.validScid(scid)) {
		    ccsdsLogNotice(TxRx::Rx, NotificationType::TypeServiceChannelNotif,
		                   ServiceChannelNotification::INVALID_CHANNELS_COMBINATION);
		    return etl::unexpected(ServiceChannelNotification::INVALID_CHANNELS_COMBINATION);
	    }

	    if (!ChannelsInterface::hasCapacityForFrameDataMasterChannelSpaceSegment(
		    mcChanVariant, DefsAndUtils::FrameType::TC,
		    1, frameLength)) {
		    ccsdsLogNotice(TxRx::Rx, NotificationType::TypeServiceChannelNotif,
		                   ServiceChannelNotification::NOT_ENOUGH_SPACE_IN_MASTER_COPY_OR_MEMORY_POOL);
		    return etl::unexpected(ServiceChannelNotification::NOT_ENOUGH_SPACE_IN_MASTER_COPY_OR_MEMORY_POOL);
	    }

	    // Create a new TC frame object. The frameData pointer will be used temporarily, to avoid needless memory pool
	    // allocation/deallocation in case the frame needs to be rejected
	    auto frameTc = TransferFrameTC(frameData, frameLength);

	    // Check for valid TFVN
	    if (frameTc.getTransferFrameVersionNumber() != static_cast<uint8_t>(physicalChannel.getTFVN())) {
		    ccsdsLogNotice(TxRx::Rx, NotificationType::TypeServiceChannelNotif,
		                   ServiceChannelNotification::INVALID_TFVN);
		    return etl::unexpected(ServiceChannelNotification::INVALID_TFVN);
	    }

	    // Check for valid SCID
	    if (frameTc.getSpacecraftId() != scid) {
		    ccsdsLogNotice(TxRx::Rx, NotificationType::TypeServiceChannelNotif,
		                   ServiceChannelNotification::INVALID_SCID);
		    return etl::unexpected(ServiceChannelNotification::INVALID_SCID);
	    }

	    // Check for valid length field
	    if (frameTc.getFrameLength() != frameLength) {
		    ccsdsLogNotice(TxRx::Rx, NotificationType::TypeServiceChannelNotif,
		                   ServiceChannelNotification::INVALID_LENGTH);
		    return etl::unexpected(ServiceChannelNotification::INVALID_LENGTH);
	    }

	    // check if frame's vcid is valid
	    const uint8_t vcid = frameTc.getVirtualChannelId();
	    auto vchanVariantOpt = vChanSearchFunction(vcid);

	    if (!vchanVariantOpt.has_value()) {
		    // if it isn't, abort operation
		    ccsdsLogNotice(TxRx::Rx, NotificationType::TypeServiceChannelNotif,
		                   ServiceChannelNotification::INVALID_VCID);
		    return etl::unexpected(ServiceChannelNotification::INVALID_VCID);
	    }

	    auto vchanVariant = vchanVariantOpt.value();
	    const auto baseVcChanPtr = ChannelsInterface::upcastToBase(vchanVariant);

	    if (ChannelsInterface::frameListAvailableVirtualChannelSpaceSegment(
		    vchanVariant, ChannelsInterface::VchanBuffType::AFTER_ALL_FRAMES_RECEPTION_TC) == 0) {
		    ccsdsLogNotice(TxRx::Rx, NotificationType::TypeServiceChannelNotif,
		                   ServiceChannelNotification::FRAME_LIST_FULL);
		    return etl::unexpected(ServiceChannelNotification::FRAME_LIST_FULL);
	    }

	    // if a segment header exists, check if the frame's mapid is valid.
	    if (baseVcChanPtr->getSegmentHeaderTCPresent()) {
		    frameTc.setSegmentationHeaderPresentFlag(true);

		    if (const auto mapChanVariantOpt = mapChanSearchFunction(vcid, frameTc.getMapId().value()); !mapChanVariantOpt.
			    has_value()) {
			    ccsdsLogNotice(TxRx::Rx, NotificationType::TypeServiceChannelNotif,
			                   ServiceChannelNotification::INVALID_MAPID);
			    return etl::unexpected(ServiceChannelNotification::INVALID_MAPID);
		    }
	    }

	    // check crc field if present
	    if (baseVcChanPtr->getFrameErrorControlFieldPresent()) {
		    const uint16_t len = frameTc.getFrameLength() - 2;
		    const uint16_t crc = TransferFrame::calculateCRC(frameData, len);

		    const uint16_t packet_crc = (static_cast<uint16_t>(frameTc.getFrameData()[len]) << 8) |
		                                frameTc.getFrameData()[len + 1];
		    if (crc != packet_crc) {
			    ccsdsLogNotice(TxRx::Rx, NotificationType::TypeServiceChannelNotif,
			                   ServiceChannelNotification::INVALID_CRC);
			    return etl::unexpected(ServiceChannelNotification::INVALID_CRC);
		    }
	    }

	    // All checks passed. Allocate transfer frame octets to memory pool and assign new pointer to the TC frame object
	    uint8_t *transferFrameData = ChannelsInterface::addFrameOctetsToMemPoolMasterChannelSpaceSegment(
		    mcChanVariant, DefsAndUtils::FrameType::TC,
		    frameData, frameLength).value();
	    frameTc.setNewFrameDataPointer(transferFrameData);
	    frameTc.updateProcessingStage(DefsAndUtils::TcFrameProcessingStage::PROCESSED_BY_ALL_FRAMES_RECEPTION);

	    auto frameTcPtr = etl::get<TransferFrameTC *>(
		    ChannelsInterface::addFrameObjectToMasterCopyBufferMasterChannelSpaceSegment(mcChanVariant,
			    DefsAndUtils::FrameType::TC, frameTc).value());

	    ChannelsInterface::pushFrameVirtualChannelSpaceSegment(mcChanVariant, vchanVariant, frameTcPtr,
	                                                           ChannelsInterface::VchanBuffType::AFTER_ALL_FRAMES_RECEPTION_TC);

	    return {};
    }

    // Virtual Channel Reception
    etl::pair<ServiceChannelNotification, uint8_t> ServiceChannelSpaceSegment::vcReceptionTC(
    	FrameAcceptanceReporting& farm,
    	MasterChannelSpaceSegmentVariant& mcChanVariant,
    	VirtualChannelSpaceSegmentVariant& vcChanVariant) {

    	const BaseMasterChannel* baseMchanPtr = ChannelsInterface::upcastToBase(mcChanVariant);
    	const BaseVirtualChannel* baseVchanPtr = ChannelsInterface::upcastToBase(vcChanVariant);

    	if (DefsAndUtils::extractScidFromMcid(baseMchanPtr->getMscid()) !=
    		DefsAndUtils::extractScidFromGvcid(baseVchanPtr->getGvcid())) {
    		ccsdsLogNotice(TxRx::Rx, NotificationType::TypeServiceChannelNotif, ServiceChannelNotification::INVALID_CHANNELS_COMBINATION);
    		return etl::make_pair(ServiceChannelNotification::INVALID_CHANNELS_COMBINATION, 0);
    	}

    	const etl::pair<FARMNotification, uint8_t> farmOutput = farm.applyFarmStateTable(mcChanVariant, vcChanVariant);

	    if (farmOutput.first != FARMNotification::NO_FARM_EVENT) {
		    ccsdsLogNotice(TxRx::Rx, NotificationType::TypeServiceChannelNotif, ServiceChannelNotification::FARM_ERROR);
		    return etl::make_pair(ServiceChannelNotification::FARM_ERROR, farmOutput.second);
	    } else {
		    return etl::make_pair(ServiceChannelNotification::NO_SERVICE_EVENT, farmOutput.second);
	    }
    }

    // SDLS Processing
	etl::expected<void, ServiceChannelNotification> ServiceChannelSpaceSegment::processSDLSSecurityTC(
		SecurityAssociation &securityAssociation,
		MasterChannelSpaceSegmentVariant &mcChanVariant,
		VirtualChannelSpaceSegmentVariant &vcChanVariant,
		const MapChannelSearchFunctionType mapChanFunction,
		const DefsAndUtils::ServiceType serviceType) {

	    if ((serviceType != DefsAndUtils::ServiceType::TYPE_AD) && (serviceType != DefsAndUtils::ServiceType::TYPE_BD)) {
		    ccsdsLogNotice(TxRx::Rx, NotificationType::TypeServiceChannelNotif, ServiceChannelNotification::INVALID_SERVICE_TYPE);
		    return etl::unexpected(ServiceChannelNotification::INVALID_SERVICE_TYPE);
	    }

	    const BaseMasterChannel* baseMchanPtr = ChannelsInterface::upcastToBase(mcChanVariant);
	    const BaseVirtualChannel* baseVchanPtr = ChannelsInterface::upcastToBase(vcChanVariant);

	    if (DefsAndUtils::extractScidFromMcid(baseMchanPtr->getMscid()) !=
	        DefsAndUtils::extractScidFromGvcid(baseVchanPtr->getGvcid())) {
		    ccsdsLogNotice(TxRx::Rx, NotificationType::TypeServiceChannelNotif,
		                   ServiceChannelNotification::INVALID_CHANNELS_COMBINATION);
		    return etl::unexpected(ServiceChannelNotification::INVALID_CHANNELS_COMBINATION);
	    }

	    // get next frame to process
	    const ChannelsInterface::VchanBuffType buffInType = (serviceType == DefsAndUtils::ServiceType::TYPE_AD)
		                                                      ? ChannelsInterface::VchanBuffType::AFTER_VC_GENERATION_TC_TYPE_AD
		                                                      : ChannelsInterface::VchanBuffType::AFTER_VC_GENERATION_TC_TYPE_BD;
	    auto expectedFrameTC = ChannelsInterface::getFrameVirtualChannelSpaceSegment(
		    vcChanVariant, buffInType, DefsAndUtils::TcFrameProcessingStage::PROCESSED_BY_FARM);

	    if (!expectedFrameTC.has_value()) {
		    ccsdsLogNotice(TxRx::Rx, NotificationType::TypeServiceChannelNotif,
		                   ServiceChannelNotification::FRAME_LIST_EMPTY);
		    return etl::unexpected(ServiceChannelNotification::FRAME_LIST_EMPTY);
	    }

    	auto *frameTc = etl::get<TransferFrameTC *>(expectedFrameTC.value());

    	// find the map channel, if a segmentation header exists
	    MAPChannelSpaceSegmentVariant* mapChanVariantPtr = nullptr;
    	const bool segHeaderExists = baseVchanPtr->getSegmentHeaderTCPresent();
    	if (segHeaderExists) {
		    etl::optional<MAPChannelSpaceSegmentVariant> optMapChanVariant = mapChanFunction(
			    DefsAndUtils::extractVcidFromGvcid(baseVchanPtr->getGvcid()), frameTc->getMapId().value());

    		if (!optMapChanVariant.has_value()) {
    			ccsdsLogNotice(TxRx::Rx, NotificationType::TypeServiceChannelNotif, ServiceChannelNotification::INVALID_MAPID);
    			return etl::unexpected(ServiceChannelNotification::INVALID_MAPID);
    		}

    		mapChanVariantPtr = &optMapChanVariant.value();
    	}

    	// ensure there is enough space in the map channel output buffer
    	if (segHeaderExists) {
		    if (ChannelsInterface::frameListAvailableMapChannelSpaceSegment(*mapChanVariantPtr) == 0) {
    			ccsdsLogNotice(TxRx::Rx, NotificationType::TypeServiceChannelNotif, ServiceChannelNotification::FRAME_LIST_FULL);
    			return etl::unexpected(ServiceChannelNotification::FRAME_LIST_FULL);
    		}
    	}

    	// Pass the frame to the next process if this virtual channel is not associated with this SA
    	if (baseVchanPtr->getAssociatedSdlsSPI() != securityAssociation.getSecurityParameterIndex()) {
    		if (segHeaderExists) {
    			// frames with a segmentation header need to be moved to the corresponding map channel list
    			ChannelsInterface::pushFrameMapChannelSpaceSegment(*mapChanVariantPtr, frameTc);
    		}

    		frameTc->updateProcessingStage(DefsAndUtils::TcFrameProcessingStage::PROCESSED_BY_SECURITY_RX);
    		return {};
    	}

    	// encryption and authentication procedures
    	const uint16_t transferFrameDataFieldLength = frameTc->getFrameLength() - DefsAndUtils::TcPrimaryHeaderSize
										- securityAssociation.getSecurityHeaderLength() -
										securityAssociation.getSecurityTrailerLength() -
										baseVchanPtr->getFrameErrorControlFieldPresent() * DefsAndUtils::ErrorControlFieldSize;

    	etl::expected<void, SDLSVerificationError> opResult = securityAssociation.processSecurityTC(frameTc, transferFrameDataFieldLength);

    	if (!opResult.has_value()) {
    		ServiceChannelNotification notif;
    		if (opResult.error() == SDLSVerificationError::INVALID_SPI) {
				notif = ServiceChannelNotification::INVALID_SECURITY_PARAMETER_INDEX;
    		} else if (opResult.error() == SDLSVerificationError::MAC_VERIFICATION_FAILURE) {
				notif = ServiceChannelNotification::INVALID_MAC;
    		} else if (opResult.error() == SDLSVerificationError::ANTI_REPLAY_SEQUENCE_NUMBER_FAILURE) {
    			notif = ServiceChannelNotification::FRAME_REPLAY_ATTEMPT;
    		} else {
    			// errors that do not indicate an attack attempt
    			notif = ServiceChannelNotification::SLDS_ERROR;
    		}

    		// discard frame
    		ChannelsInterface::popFrameVirtualChannelSpaceSegment(vcChanVariant, frameTc, buffInType);
    		ChannelsInterface::removeFrameDataMasterChannelSpaceSegment(mcChanVariant, frameTc);

    		ccsdsLogNotice(TxRx::Rx, NotificationType::TypeServiceChannelNotif, notif);
    		return etl::unexpected(notif);
    	}

    	// pass to next processing stage
    	if (segHeaderExists) {
    		// frames with a segmentation header need to be moved to the corresponding map channel list
    		ChannelsInterface::pushFrameMapChannelSpaceSegment(*mapChanVariantPtr, frameTc);
    	}
    	frameTc->updateProcessingStage(DefsAndUtils::TcFrameProcessingStage::PROCESSED_BY_SECURITY_RX);
    	return {};
    }

    // Virtual Channel Extraction
    etl::expected<uint16_t, ServiceChannelNotification>
    ServiceChannelSpaceSegment::packetExtractionRxTC(uint8_t vid, uint8_t mapid, ServiceType serviceType, uint8_t *packetDest) {
	    if ((serviceType != ServiceType::TYPE_AD) && (serviceType != ServiceType::TYPE_BD)) {
		    ccsdsLogNotice(TxRx::Rx, NotificationType::TypeServiceChannelNotif, ServiceChannelNotification::INVALID_SERVICE_TYPE);
		    return etl::unexpected(ServiceChannelNotification::INVALID_SERVICE_TYPE);
	    }

	    if (masterChannel.virtualChannels.find(vid) == masterChannel.virtualChannels.end()) {
		    ccsdsLogNotice(TxRx::Rx, NotificationType::TypeServiceChannelNotif, ServiceChannelNotification::INVALID_VC_ID);
		    return etl::unexpected(ServiceChannelNotification::INVALID_VC_ID);
	    }

	    VirtualChannelSpaceSegment* vchan = &(masterChannel.virtualChannels.at(vid));

	    MAPChannelSpaceSegment* mapChannel;
	    if (vchan->segmentHeaderTCPresent) {
		    if (vchan->mapChannels.find(mapid) == vchan->mapChannels.end()) {
			    ccsdsLogNotice(TxRx::Rx, NotificationType::TypeServiceChannelNotif, ServiceChannelNotification::INVALID_MAP_ID);
			    return etl::unexpected(ServiceChannelNotification::INVALID_MAP_ID);
		    }
		    mapChannel = &(vchan->mapChannels.at(mapid));
	    }

	    etl::list<TransferFrameTC *, MaxReceivedUnprocessedTxTcInVirtBuffer> *inFrameBuf;
	    etl::optional<TransferFrameTC *> *blockingFrame;
	    uint8_t *nextPacketPosition;
	    etl::queue<TransferFrameTC *, MaxFramesWithSegmentedPackets> *segmentationFramesBuf;
	    SequenceFlag*previousFrameSequenceFlag;
	    if (vchan->segmentHeaderTCPresent) {
		    if (serviceType == ServiceType::TYPE_AD) {
			    inFrameBuf = &mapChannel->framesAfterSDLSProcessingTypeADRxTC;
			    blockingFrame = &mapChannel->frameWithMultiplePacketsTypeADRxTC;
			    nextPacketPosition = &mapChannel->nextPacketPositionTypeAD;
			    segmentationFramesBuf = &mapChannel->framesWithSegmentedPacketsTypeADRxTC;
			    previousFrameSequenceFlag = &mapChannel->previousFrameSequenceFlagTypeAD;
		    } else { // TYPE_BD
			    inFrameBuf = &mapChannel->framesAfterSDLSProcessingTypeBDRxTC;
			    blockingFrame = &mapChannel->frameWithMultiplePacketsTypeBDRxTC;
			    nextPacketPosition = &mapChannel->nextPacketPositionTypeBD;
			    segmentationFramesBuf = &mapChannel->framesWithSegmentedPacketsTypeBDRxTC;
			    previousFrameSequenceFlag = &mapChannel->previousFrameSequenceFlagTypeBD;
		    }
	    } else {
		    if (serviceType == ServiceType::TYPE_AD) {
			    inFrameBuf = &vchan->framesAfterSDLSProcessingTypeADRxTC;
			    blockingFrame = &vchan->frameWithMultiplePacketsTypeADRxTC;
			    nextPacketPosition = &vchan->nextPacketPositionTypeAD;
		    } else { // TYPE_BD
			    inFrameBuf = &vchan->framesAfterSDLSProcessingTypeBDRxTC;
			    blockingFrame = &vchan->frameWithMultiplePacketsTypeBDRxTC;
			    nextPacketPosition = &vchan->nextPacketPositionTypeBD;
		    }
	    }

	    uint8_t segmentHeaderLength = (vchan->segmentHeaderTCPresent) ? TcSegmentHeaderSize : 0;

	    bool saAssociated = vchan->segmentHeaderTCPresent ? securityAssociation.isAssociated(vid, mapid)
	                                                      : securityAssociation.isAssociated(vid);
	    uint8_t securityHeaderLength = saAssociated ? securityAssociation.getSecurityHeaderLength() : 0;
	    uint8_t securityTrailerLength = saAssociated ? securityAssociation.getSecurityTrailerLength() : 0;

	    uint8_t prePayloadSegmentLength = TcPrimaryHeaderSize + segmentHeaderLength + securityHeaderLength;
	    uint8_t afterPayloadSegmentLength =
	        securityTrailerLength + ErrorControlFieldSize * vchan->frameErrorControlFieldPresent;

	    // Return next packet of waiting frame with multiple packets (blocking frame)
	    if (blockingFrame->has_value()) {
		    uint8_t *frameData = blockingFrame->value()->getFrameData();
		    uint16_t packetLen = getSpacePacketLength(frameData + *nextPacketPosition);
		    uint16_t frameLen = blockingFrame->value()->getFrameLength();

		    if ((packetLen < PacketPrimaryHeaderLength + 1) ||
		        (*nextPacketPosition + packetLen > frameLen - afterPayloadSegmentLength)) {
			    // packet has an invalid length -> discard the frame
			    masterChannel.masterChannelPoolRxTC.deletePacket(blockingFrame->value()->getFrameData(), frameLen);
			    masterChannel.removeMasterRxTC(blockingFrame->value());
			    blockingFrame = etl::nullopt;
			    ccsdsLogNotice(TxRx::Rx, NotificationType::TypeServiceChannelNotif, ServiceChannelNotification::RX_INVALID_LENGTH);
			    return etl::unexpected(ServiceChannelNotification::RX_INVALID_LENGTH);
		    } else {
			    std::memcpy(packetDest, frameData + *nextPacketPosition,
			                packetLen <= MaxPacketSize ? packetLen : MaxPacketSize);

			    if (*nextPacketPosition + packetLen == frameLen - afterPayloadSegmentLength) {
				    // reached last packet -> discard the frame
				    masterChannel.masterChannelPoolRxTC.deletePacket(blockingFrame->value()->getFrameData(), frameLen);
				    masterChannel.removeMasterRxTC(blockingFrame->value());
				    blockingFrame = etl::nullopt;
			    } else {
				    // update the next packet position
				    *nextPacketPosition += packetLen;
			    }

			    return packetLen;
		    }
	    }

	    // No "blocking frame" is stored. Get a new frame from the higher layer buffer.
	    TransferFrameTC *frameTc;
	    if (inFrameBuf->empty()) {
		    ccsdsLogNotice(TxRx::Rx, NotificationType::TypeServiceChannelNotif, ServiceChannelNotification::NO_RX_PACKETS_TO_PROCESS);
		    return etl::unexpected(ServiceChannelNotification::NO_RX_PACKETS_TO_PROCESS);
	    } else {
		    frameTc = inFrameBuf->front();
		    inFrameBuf->pop_front();
	    }

	    bool blockingAllowed = vchan->segmentHeaderTCPresent ? mapChannel->blockingTC : vchan->blockingTC;
	    if ((!vchan->segmentHeaderTCPresent) ||
	        (vchan->segmentHeaderTCPresent && segmentationFramesBuf->empty())) {
		    // Blocking scenario. Arrival of 2 possible frames:
		    // - virtual channel frame, where only blocking could occur
		    // - map channel frame, where the segment header does not exist and hence only blocking could occur
		    uint8_t *frameData = frameTc->getFrameData();
		    uint16_t frameLen = frameTc->getFrameLength();
		    uint16_t dataFieldLen = frameLen - prePayloadSegmentLength -
		                            afterPayloadSegmentLength;  // defined as the length of the space captured by packets
		    *nextPacketPosition = prePayloadSegmentLength;
		    uint16_t firstPacketLen = getSpacePacketLength(frameData + *nextPacketPosition);

		    if ((firstPacketLen < PacketPrimaryHeaderLength + 1) || (firstPacketLen > dataFieldLen)) {
			    // packet has an invalid length -> discard the frame
			    masterChannel.masterChannelPoolRxTC.deletePacket(frameData, frameLen);
			    masterChannel.removeMasterRxTC(frameTc);
			    ccsdsLogNotice(TxRx::Rx, NotificationType::TypeServiceChannelNotif, ServiceChannelNotification::RX_INVALID_LENGTH);
			    return etl::unexpected(ServiceChannelNotification::RX_INVALID_LENGTH);
		    } else {
			    // copy the first packet
			    std::memcpy(packetDest, frameData + *nextPacketPosition,
			                firstPacketLen <= MaxPacketSize ? firstPacketLen : MaxPacketSize);

			    if (firstPacketLen == dataFieldLen) {
				    // The entire data field is a single packet. Delete the frame
				    masterChannel.masterChannelPoolRxTC.deletePacket(frameData, frameLen);
				    masterChannel.removeMasterRxTC(frameTc);
			    } else {
				    // More packets remaining
				    blockingFrame->emplace(frameTc);
				    *nextPacketPosition += firstPacketLen;
			    }
			    return firstPacketLen;
		    }
	    } else {
		    // TODO rework this...
		    // Segmentation scenario. Arrival of map channel frame which could contain a partial packet.
		    while (true) {
			    SequenceFlag sequenceFlag = static_cast<SequenceFlag>(static_cast<uint8_t>(frameTc->getSequenceFlag().value())
			                                                            >> 6U); // valid only if the segmentation header is present
			    if (segmentationFramesBuf->empty() && (sequenceFlag == SequenceFlag::SegmentationStart)) {
				    // arrival of first part of segmented packet
				    segmentationFramesBuf->push(frameTc);
				    *previousFrameSequenceFlag = SequenceFlag::SegmentationStart;
				    ccsdsLogNotice(TxRx::Rx, NotificationType::TypeServiceChannelNotif, ServiceChannelNotification::PROCESSING_SEGMENTED_PACKET);
				    return etl::unexpected(ServiceChannelNotification::PROCESSING_SEGMENTED_PACKET);
			    } else if (!segmentationFramesBuf->empty() && !segmentationFramesBuf->full() &&
			               ((*previousFrameSequenceFlag == SequenceFlag::SegmentationStart) ||
			                (*previousFrameSequenceFlag == SequenceFlag::SegmentationMiddle)) &&
			               sequenceFlag == SequenceFlag::SegmentationMiddle) {
				    // arrival of middle part of segmented packet
				    segmentationFramesBuf->push(frameTc);
				    *previousFrameSequenceFlag = SequenceFlag::SegmentationMiddle;
				    ccsdsLogNotice(TxRx::Rx, NotificationType::TypeServiceChannelNotif, ServiceChannelNotification::PROCESSING_SEGMENTED_PACKET);
				    return etl::unexpected(ServiceChannelNotification::PROCESSING_SEGMENTED_PACKET);
			    } else if ((!segmentationFramesBuf->empty()) && !segmentationFramesBuf->full() &&
			               (*previousFrameSequenceFlag == SequenceFlag::SegmentationMiddle) &&
			               sequenceFlag == SequenceFlag::SegmentationEnd) {
				    // arrival of last part of segmented packet
				    segmentationFramesBuf->push(frameTc);

				    // copy the complete packet to the destination and delete stored frames
				    uint8_t nextCopyPosition = 0;  // position 0 corresponds to packetDest
				    while (!segmentationFramesBuf->empty()) {
					    TransferFrameTC *toBeRemovedFrame = segmentationFramesBuf->front();
					    uint16_t numOctets = toBeRemovedFrame->getFrameLength()
					                         - prePayloadSegmentLength - afterPayloadSegmentLength;

					    // copying will not be complete if the maximum packet length is reached (it is assumed that the user's buffer
					    // is MaxPacketSize long)
					    if (nextCopyPosition + numOctets <= MaxPacketSize - 1) {
						    std::memcpy(toBeRemovedFrame->getFrameData() + nextCopyPosition,
						                toBeRemovedFrame->getFrameData(), numOctets);
					    }
					    nextCopyPosition += numOctets;

					    masterChannel.masterChannelPoolRxTC.deletePacket(toBeRemovedFrame->getFrameData(),
					                                                     toBeRemovedFrame->getFrameLength());
					    masterChannel.removeMasterRxTC(toBeRemovedFrame);
					    segmentationFramesBuf->pop();
				    }

				    return nextCopyPosition;
			    } else {
				    // Unexpected sequence flag or segmented frames buffer full. Discard current and all previous frames
				    masterChannel.masterChannelPoolRxTC.deletePacket(frameTc->getFrameData(),
				                                                     frameTc->getFrameLength());
				    masterChannel.removeMasterRxTC(frameTc);

				    while (!segmentationFramesBuf->empty()) {
					    TransferFrameTC *toBeRemovedFrame = segmentationFramesBuf->front();
					    masterChannel.masterChannelPoolRxTC.deletePacket(toBeRemovedFrame->getFrameData(),
					                                                     toBeRemovedFrame->getFrameLength());
					    masterChannel.removeMasterRxTC(toBeRemovedFrame);
					    segmentationFramesBuf->pop();
				    }

				    ccsdsLogNotice(TxRx::Rx, NotificationType::TypeServiceChannelNotif, ServiceChannelNotification::INVALID_SEQUENCE_FLAG);
				    return etl::unexpected(ServiceChannelNotification::INVALID_SEQUENCE_FLAG);
			    }
		    }
	    }
    }

	/** ========================================
     *   TM TransferFrame - Sending End (TM Tx)
     *  ========================================
     */
    // Packet Processing
    etl::expected<void, ServiceChannelNotification>
        ServiceChannelSpaceSegment::storePacketTM(
        	VirtualChannelSpaceSegmentVariant &vcChanVariant,
        	const uint8_t *packetSource, const uint16_t packetLength) {
	    auto opResult = ChannelsInterface::pushTmPacketVirtualChannelSpaceSegment(vcChanVariant, packetSource, packetLength);

    	if (!opResult.has_value()) {
    		ccsdsLogNotice(TxRx::Rx, NotificationType::TypeServiceChannelNotif, ServiceChannelNotification::PACKET_QUEUE_FULL);
    		return etl::unexpected(ServiceChannelNotification::PACKET_QUEUE_EMPTY);
    	}

    	return {};
    }

    // Virtual Channel Generation
    etl::expected<void, ServiceChannelNotification>
    ServiceChannelSpaceSegment::segmentationTM(
    	MasterChannelSpaceSegmentVariant &mcChanVariant,
    	VirtualChannelSpaceSegmentVariant &vcChanVariant,
    	TransferFrameTM *prevFrame,
    	uint16_t transferFrameDataFieldLength,
    	uint16_t packetLength) {

    	const BaseMasterChannel* baseMchanPtr = ChannelsInterface::upcastToBase(mcChanVariant);
	    const BaseVirtualChannel* baseVchanPtr = ChannelsInterface::upcastToBase(vcChanVariant);

	    if (DefsAndUtils::extractScidFromMcid(baseMchanPtr->getMscid()) !=
	        DefsAndUtils::extractScidFromGvcid(baseVchanPtr->getGvcid())) {
		    ccsdsLogNotice(TxRx::Rx, NotificationType::TypeServiceChannelNotif,
		                   ServiceChannelNotification::INVALID_CHANNELS_COMBINATION);
		    return etl::unexpected(ServiceChannelNotification::INVALID_CHANNELS_COMBINATION);
	    }

	    uint16_t numberOfNewTransferFrames = 0;
	    const uint16_t prevFrameCapacity = (prevFrame ==
	                                         nullptr) ? 0 : transferFrameDataFieldLength -
	                                           prevFrame->getFirstDataFieldEmptyOctet();

	    // Calculate amount of new transfer frames needed
	    if (prevFrame == nullptr) {
		    numberOfNewTransferFrames = packetLength / transferFrameDataFieldLength +
		                                ((packetLength % transferFrameDataFieldLength) ? 1 : 0);
	    } else {
		    numberOfNewTransferFrames = (packetLength - prevFrameCapacity) / transferFrameDataFieldLength +
		                                ((packetLength - prevFrameCapacity) % transferFrameDataFieldLength ? 1 : 0);
	    }

	    const uint16_t numberOfNewOctets = numberOfNewTransferFrames * (
		                                       DefsAndUtils::TmPrimaryHeaderSize + transferFrameDataFieldLength +
		                                       baseVchanPtr->getOperationalControlFieldTMPresent() * DefsAndUtils::TmOperationalControlFieldSize +
		                                       baseVchanPtr->getFrameErrorControlFieldPresent() * DefsAndUtils::ErrorControlFieldSize);

    	const uint8_t trailerSize = baseVchanPtr->getFrameErrorControlFieldPresent() * DefsAndUtils::ErrorControlFieldSize +
					  baseVchanPtr->getOperationalControlFieldTMPresent() * DefsAndUtils::TmOperationalControlFieldSize;

	    // ensure there is enough space for the new frames
	    if (!ChannelsInterface::hasCapacityForFrameDataMasterChannelSpaceSegment(
		    mcChanVariant, DefsAndUtils::FrameType::TM,
		    numberOfNewTransferFrames, numberOfNewOctets)) {
		    ccsdsLogNotice(TxRx::Tx, NotificationType::TypeServiceChannelNotif,
		                   ServiceChannelNotification::NOT_ENOUGH_SPACE_IN_MASTER_COPY_OR_MEMORY_POOL);
		    return etl::unexpected(ServiceChannelNotification::NOT_ENOUGH_SPACE_IN_MASTER_COPY_OR_MEMORY_POOL);
	    }

	    if (ChannelsInterface::frameListAvailableVirtualChannelSpaceSegment(
		        vcChanVariant, ChannelsInterface::VchanBuffType::UNDER_PROCESSING_TM) < numberOfNewTransferFrames) {
		    ccsdsLogNotice(TxRx::Tx, NotificationType::TypeServiceChannelNotif,
		                   ServiceChannelNotification::FRAME_LIST_FULL);
		    return etl::unexpected(ServiceChannelNotification::FRAME_LIST_FULL);
	    }

	    // fill previous frame
	    if (prevFrame != nullptr) {
		    ChannelsInterface::popTmPacketSegmentVirtualChannelSpaceSegment(
			    vcChanVariant,
			    prevFrame->getFrameData() + DefsAndUtils::TmPrimaryHeaderSize + transferFrameDataFieldLength -
			    prevFrameCapacity,
			    prevFrameCapacity);
		    prevFrame->setFirstDataFieldEmptyOctet(transferFrameDataFieldLength);
		    packetLength -= prevFrameCapacity;
	    }

	    // new frames creation
    	static uint8_t tmpData[DefsAndUtils::MaxTmTransferFrameLength] = {0};
	    uint16_t currentTransferFrameDataFieldLength = 0;
	    for (uint16_t i = 0; i < numberOfNewTransferFrames; i++) {
		    currentTransferFrameDataFieldLength =
		        packetLength > transferFrameDataFieldLength ? transferFrameDataFieldLength : packetLength;

		    ChannelsInterface::popTmPacketSegmentVirtualChannelSpaceSegment(
			    vcChanVariant,
			    tmpData + DefsAndUtils::TmPrimaryHeaderSize,
			    currentTransferFrameDataFieldLength);

	    	// calculate first header pointer value
		    uint16_t firstHeaderPointer = 0;
		    if (prevFrame != nullptr && i == numberOfNewTransferFrames - 1) {
			    firstHeaderPointer = (packetLength == transferFrameDataFieldLength) ? DefsAndUtils::TmNoPacketStartFirstHeaderPointerVal
			                                                                        : packetLength;
		    } else if (prevFrame == nullptr) {
			    if (i == 0) {
				    firstHeaderPointer = 0;
			    } else if (i == numberOfNewTransferFrames - 1) {
				    firstHeaderPointer = (packetLength == transferFrameDataFieldLength)
				                             ? DefsAndUtils::TmNoPacketStartFirstHeaderPointerVal : packetLength;
			    }
		    } else {
			    firstHeaderPointer = DefsAndUtils::TmNoPacketStartFirstHeaderPointerVal;
		    }


	    	uint8_t* transferFrameData = ChannelsInterface::addFrameOctetsToMemPoolMasterChannelSpaceSegment(mcChanVariant,
	    		DefsAndUtils::FrameType::TM,
	    		tmpData,
	    		DefsAndUtils::TmPrimaryHeaderSize + transferFrameDataFieldLength + trailerSize
	    		).value();

		    auto transferFrameTm =
		        TransferFrameTM(transferFrameData,
		                        transferFrameDataFieldLength + DefsAndUtils::TmPrimaryHeaderSize + trailerSize,
		                        DefsAndUtils::extractVcidFromGvcid(baseVchanPtr->getGvcid()),
		                        DefsAndUtils::extractScidFromMcid(baseMchanPtr->getMscid()),
		                        baseVchanPtr->getOperationalControlFieldTMPresent(),
		                        ChannelsInterface::readAndUpdateTmFrameCountVirtualChannelSpaceSegment(vcChanVariant),
		                        baseVchanPtr->getSecondaryHeaderTMPresent(),
		                        baseVchanPtr->getSynchronization(),
		                        DefsAndUtils::PacketOrderFlag,
		                        DefsAndUtils::SegmentLengthIdentifierLegacy,
		                        firstHeaderPointer,
		                        baseVchanPtr->getFrameErrorControlFieldPresent(),
		                        currentTransferFrameDataFieldLength);

			if (currentTransferFrameDataFieldLength == transferFrameDataFieldLength) {
				// This frame is filled up and processing is finished
				transferFrameTm.updateProcessingStage(DefsAndUtils::TmFrameProcessingStage::PROCESSED_BY_VC_GENERATION);
			}

		    auto framePtr = etl::get<TransferFrameTM *>(
			    ChannelsInterface::addFrameObjectToMasterCopyBufferMasterChannelSpaceSegment(
				    mcChanVariant,
				    DefsAndUtils::FrameType::TM,
				    transferFrameTm).value());

	    	ChannelsInterface::pushFrameVirtualChannelSpaceSegment(mcChanVariant, vcChanVariant, framePtr, ChannelsInterface::VchanBuffType::UNDER_PROCESSING_TM);

		    packetLength -= transferFrameDataFieldLength;
	    }
	    return {};
    }

    etl::expected<void, ServiceChannelNotification>
    ServiceChannelSpaceSegment::blockingTM(
	    MasterChannelSpaceSegmentVariant &mcChanVariant,
	    VirtualChannelSpaceSegmentVariant &vcChanVariant,
	    TransferFrameTM *prevFrame,
	    uint16_t transferFrameDataFieldLength,
	    uint16_t packetLength
	    ) {
    	const BaseMasterChannel* baseMchanPtr = ChannelsInterface::upcastToBase(mcChanVariant);
    	const BaseVirtualChannel* baseVchanPtr = ChannelsInterface::upcastToBase(vcChanVariant);

    	if (DefsAndUtils::extractScidFromMcid(baseMchanPtr->getMscid()) !=
			DefsAndUtils::extractScidFromGvcid(baseVchanPtr->getGvcid())) {
    		ccsdsLogNotice(TxRx::Rx, NotificationType::TypeServiceChannelNotif,
						   ServiceChannelNotification::INVALID_CHANNELS_COMBINATION);
    		return etl::unexpected(ServiceChannelNotification::INVALID_CHANNELS_COMBINATION);
		}

	    // ensure there is enough space for a new frame, if needed
	    const uint16_t frameLength = DefsAndUtils::TmPrimaryHeaderSize +
	                           transferFrameDataFieldLength +
	                           baseVchanPtr->getOperationalControlFieldTMPresent() *
	                           DefsAndUtils::TmOperationalControlFieldSize +
	                           baseVchanPtr->getFrameErrorControlFieldPresent() * DefsAndUtils::ErrorControlFieldSize;

    	if (prevFrame == nullptr) {
		    if (!ChannelsInterface::hasCapacityForFrameDataMasterChannelSpaceSegment(
			    mcChanVariant, DefsAndUtils::FrameType::TM,
			    1, frameLength)) {
			    ccsdsLogNotice(TxRx::Tx, NotificationType::TypeServiceChannelNotif,
			                   ServiceChannelNotification::NOT_ENOUGH_SPACE_IN_MASTER_COPY_OR_MEMORY_POOL);
			    return etl::unexpected(ServiceChannelNotification::NOT_ENOUGH_SPACE_IN_MASTER_COPY_OR_MEMORY_POOL);
		    }

		    if (ChannelsInterface::frameListAvailableVirtualChannelSpaceSegment(
			        vcChanVariant, ChannelsInterface::VchanBuffType::UNDER_PROCESSING_TM) == 0) {
			    ccsdsLogNotice(TxRx::Tx, NotificationType::TypeServiceChannelNotif,
			                   ServiceChannelNotification::FRAME_LIST_FULL);
			    return etl::unexpected(ServiceChannelNotification::FRAME_LIST_FULL);
		    }
	    }

	    uint16_t currentTransferFrameDataFieldLength = (prevFrame == nullptr) ? 0
	                                                                          : prevFrame->getFirstDataFieldEmptyOctet();
	    for (uint8_t & octet : tmpData) {  // clear up array between consecutive calls
		    octet = 0;
	    }

	    while (currentTransferFrameDataFieldLength + packetLength <= transferFrameDataFieldLength &&
	           !vchan.packetLengthBufferTxTM.empty()) {
		    for (uint16_t i = 0; i < packetLength; i++) {
			    tmpData[i + TmPrimaryHeaderSize + currentTransferFrameDataFieldLength] = vchan.packetBufferTxTM.front();
			    vchan.packetBufferTxTM.pop_front();
		    }
		    currentTransferFrameDataFieldLength += packetLength;
		    vchan.packetLengthBufferTxTM.pop_front();
		    packetLength = vchan.packetLengthBufferTxTM.front();
		    // If blocking is disabled, stop the operation on the first packet
		    if (!vchan.blockingTM) {
			    break;
		    }
	    }

    	static uint8_t tmpData[DefsAndUtils::MaxTmTransferFrameLength] = {0};
	    const uint8_t trailerSize = baseVchanPtr->getOperationalControlFieldTMPresent() * DefsAndUtils::TmOperationalControlFieldSize +
	                          baseVchanPtr->getFrameErrorControlFieldPresent() * DefsAndUtils::ErrorControlFieldSize;
	    if (prevFrame == nullptr) {
		    uint8_t *transferFrameData = masterChannel.masterChannelPoolTxTM.allocatePacket(
		        tmpData, TmPrimaryHeaderSize + transferFrameDataFieldLength + trailerSize);
		    if (transferFrameData == nullptr) {
			    ccsdsLogNotice(TxRx::Tx, NotificationType::TypeServiceChannelNotif, ServiceChannelNotification::MEMORY_POOL_FULL);
			    return etl::unexpected(ServiceChannelNotification::MEMORY_POOL_FULL);
		    }


		    const uint16_t firstEmptyOctet = currentTransferFrameDataFieldLength;
		    TransferFrameTM transferFrameTm =
		        TransferFrameTM(transferFrameData,
		                        transferFrameDataFieldLength + DefsAndUtils::TmPrimaryHeaderSize + trailerSize,
		                        DefsAndUtils::extractVcidFromGvcid(baseVchanPtr->getGvcid()),
		                        DefsAndUtils::extractScidFromMcid(baseMchanPtr->getMscid()),
		                        baseVchanPtr->getOperationalControlFieldTMPresent(),
		                        ChannelsInterface::readAndUpdateTmFrameCountVirtualChannelSpaceSegment(vcChanVariant),
		                        baseVchanPtr->getSecondaryHeaderTMPresent(),
		                        baseVchanPtr->getSynchronization(),
		                        DefsAndUtils::PacketOrderFlag,
		                        DefsAndUtils::SegmentLengthIdentifierLegacy,
		                        0,
		                        baseVchanPtr->getFrameErrorControlFieldPresent(),
		                        firstEmptyOctet);

		    masterChannel.masterCopyTxTM.push_back(transferFrameTm);
		    masterChannel.framesAfterVcGenerationServiceTxTM.push_back(&(masterChannel.masterCopyTxTM.back()));
	    } else {
		    for (uint16_t i = 0; i < TmPrimaryHeaderSize + prevFrame->getFirstDataFieldEmptyOctet(); i++) {
			    tmpData[i] = prevFrame->getFrameData()[i];
		    }
		    prevFrame->setFirstDataFieldEmptyOctet(currentTransferFrameDataFieldLength);
		    prevFrame->setNewFrameData(tmpData, TmPrimaryHeaderSize + prevFrame->getFirstDataFieldEmptyOctet());
	    }
	    return {};
    }

    etl::expected<bool, ServiceChannelNotification>
    ServiceChannelSpaceSegment::generateIdleSpacePacket(uint8_t vid, TransferFrameTM *lastProcessedFrame,
                                            uint16_t transferFrameDataFieldLength, bool lastPacketPlacedIdle) {

	    if (masterChannel.virtualChannels.find(vid) == masterChannel.virtualChannels.end()) {
		    ccsdsLogNotice(TxRx::Tx, NotificationType::TypeServiceChannelNotif, ServiceChannelNotification::INVALID_VC_ID);
		    return etl::unexpected(ServiceChannelNotification::INVALID_VC_ID);
	    }

	    VirtualChannelSpaceSegment& vchan = masterChannel.virtualChannels.at(vid);

	    if (lastProcessedFrame == nullptr) {
		    return false;
	    }

	    uint16_t firstDataFieldEmptyOctet = lastProcessedFrame->getFirstDataFieldEmptyOctet();
	    if (firstDataFieldEmptyOctet == transferFrameDataFieldLength) {
		    return false;
	    }

	    // The idle packet data length field, reduced by 1 (see p. 4.1.3.5 of space packet protocol)
	    uint16_t idlePacketDataLength;
	    uint16_t remainingSpace = transferFrameDataFieldLength - firstDataFieldEmptyOctet;
	    if (vchan.packetLengthBufferTxTM.empty()) {
		    if (remainingSpace >= PacketPrimaryHeaderLength + 1) {
			    // The idle packet can fit in the transfer frame (PacketPrimaryHeaderLength + 1 is the minimum size).
			    idlePacketDataLength = remainingSpace - PacketPrimaryHeaderLength - 1;
		    } else {
			    // The idle packet cannot fit (will be segmented).
			    // It must be large enough to fully cover the second frame (since packet buffer is empty).
			    idlePacketDataLength = (remainingSpace + transferFrameDataFieldLength) - PacketPrimaryHeaderLength - 1;
		    }

		    for (uint8_t i = 0; i < PacketPrimaryHeaderLength - 2; i++) {
			    vchan.packetBufferTxTM.push_back(PacketPrimaryHeader[i]);

		    }

		    vchan.packetBufferTxTM.push_back(static_cast<uint8_t>(idlePacketDataLength >> 8));
		    vchan.packetBufferTxTM.push_back(static_cast<uint8_t>(idlePacketDataLength));

		    for (uint16_t i = 0; i < idlePacketDataLength + 1; i++) {
			    vchan.packetBufferTxTM.push_back(idle_data[i]);
		    }

		    vchan.packetLengthBufferTxTM.push_back(PacketPrimaryHeaderLength + idlePacketDataLength + 1);
	    } else {
		    // Packets do exist, but due to blocking/segmentation permissions they may not be able to be placed.In this case, an idle packet
		    // needs to be pushed to the front of the queue, in order to be processed first.

		    bool nonIdleCanFit = vchan.packetLengthBufferTxTM.front() <= remainingSpace;
		    if ((nonIdleCanFit && lastPacketPlacedIdle) ||
		        (nonIdleCanFit && !lastPacketPlacedIdle && vchan.blockingTM) ||
		        (!nonIdleCanFit && vchan.segmentationTM)) {
			    return false;
		    }

		    if (remainingSpace >= PacketPrimaryHeaderLength + 1) {
			    // The idle packet can fit in the transfer frame (PacketPrimaryHeaderLength + 1 is the minimum size).
			    idlePacketDataLength = remainingSpace - PacketPrimaryHeaderLength - 1;
		    } else {
			    // The idle packet cannot fit (will be segmented).
			    // It must have the smallest possible length, in order to not waste data field space from the second frame.
			    idlePacketDataLength = 0;
		    }


		    for (uint16_t i = 0; i < idlePacketDataLength + 1; i++) {
			    vchan.packetBufferTxTM.push_front(idle_data[i]);
		    }

		    vchan.packetBufferTxTM.push_front((static_cast<uint8_t>(idlePacketDataLength)));
		    vchan.packetBufferTxTM.push_front((static_cast<uint8_t>(idlePacketDataLength) >> 8));


		    for (int8_t i = PacketPrimaryHeaderLength - 3; i >= 0; i--) {
			    vchan.packetBufferTxTM.push_front(PacketPrimaryHeader[i]);
		    }

		    vchan.packetLengthBufferTxTM.push_front(PacketPrimaryHeaderLength + idlePacketDataLength + 1);
	    }

	    return true;
    }

    etl::expected<void, ServiceChannelNotification>
    ServiceChannelSpaceSegment::vcGenerationServiceTxTM(uint16_t transferFrameDataFieldLength, uint8_t vid) {
	    if (masterChannel.virtualChannels.find(vid) == masterChannel.virtualChannels.end()) {
		    ccsdsLogNotice(TxRx::Tx, NotificationType::TypeServiceChannelNotif, ServiceChannelNotification::INVALID_VC_ID);
		    return etl::unexpected(ServiceChannelNotification::INVALID_VC_ID);
	    }

	    VirtualChannelSpaceSegment& vchan = masterChannel.virtualChannels.at(vid);

	    // generate OID frame
	    if (vchan.packetLengthBufferTxTM.empty()) {
		    if (masterChannel.masterCopyTxTM.full()) {
			    ccsdsLogNotice(TxRx::Tx, NotificationType::TypeServiceChannelNotif, ServiceChannelNotification::MASTER_CHANNEL_FRAME_BUFFER_FULL);
			    return etl::unexpected(ServiceChannelNotification::MASTER_CHANNEL_FRAME_BUFFER_FULL);
		    }
		    if (masterChannel.masterChannelPoolTxTM.findFit(TmPrimaryHeaderSize + transferFrameDataFieldLength +
		                                                    vchan.operationalControlFieldTMPresent *
		                                                        TmOperationalControlFieldSize +
		                                                    vchan.frameErrorControlFieldPresent *
		                                                        ErrorControlFieldSize).second != MasterChannelAlert::NO_MC_ALERT) {
			    ccsdsLogNotice(TxRx::Tx, NotificationType::TypeServiceChannelNotif, ServiceChannelNotification::MEMORY_POOL_FULL);
			    return etl::unexpected(ServiceChannelNotification::MEMORY_POOL_FULL);
		    }

		    static uint8_t tmpData[TmTransferFrameSize] = {0};

		    for (uint8_t & octet : tmpData) {
			    octet = 0;
		    }
		    std::memcpy(tmpData + TmPrimaryHeaderSize, idle_data, transferFrameDataFieldLength);

		    uint8_t *transferFrameData = masterChannel.masterChannelPoolTxTM.allocatePacket(
		        tmpData,
		        TmPrimaryHeaderSize + transferFrameDataFieldLength +
		            TmOperationalControlFieldSize * vchan.operationalControlFieldTMPresent +
		            ErrorControlFieldSize * vchan.frameErrorControlFieldPresent);
		    vchan.frameCountTM = (vchan.frameCountTM == 255) ? (0) : (vchan.frameCountTM + 1);
		    TransferFrameTM frameOID =
		        TransferFrameTM(transferFrameData,
		                        TmPrimaryHeaderSize + transferFrameDataFieldLength +
		                            TmOperationalControlFieldSize * vchan.operationalControlFieldTMPresent +
		                            ErrorControlFieldSize * vchan.frameErrorControlFieldPresent,
		                        vid,
		                        vchan.operationalControlFieldTMPresent,
		                        vchan.frameCountTM,
		                        vchan.secondaryHeaderTMPresent,
		                        vchan.synchronizationTM,
		                        PacketOrderFlag,
		                        SegmentLengthIdentifierLegacy,
		                        TmOIDFrameFirstHeaderPointer,
		                        vchan.frameErrorControlFieldPresent,
		                        transferFrameDataFieldLength);

		    masterChannel.masterCopyTxTM.push_back(frameOID);
		    masterChannel.framesAfterVcGenerationServiceTxTM.push_back(&(masterChannel.masterCopyTxTM.back()));
		    ccsdsLogNotice(TxRx::Tx, NotificationType::TypeServiceChannelNotif, ServiceChannelNotification::PACKET_BUFFER_EMPTY);
		    return etl::unexpected(ServiceChannelNotification::PACKET_BUFFER_EMPTY);
	    }

	    etl::expected<void, ServiceChannelNotification> notif;
	    etl::expected<bool, ServiceChannelNotification> idleSpacePacketFuncNotif;
	    bool idlePacketQueued = false; // Flag indicating if the first packet in the queue is an idle space packet
	    bool lastPacketPlacedIdle = false; // Flag indicating if the last placed packet in a frame is an idle space packet
	    while (!vchan.packetLengthBufferTxTM.empty()) {
		    uint16_t packetLength = vchan.packetLengthBufferTxTM.front();
		    TransferFrameTM *prevFrame = nullptr;
		    etl::list<TransferFrameTM *, MaxReceivedUnprocessedTxTmInVirtBuffer> *TmBuffer = &(masterChannel.framesAfterVcGenerationServiceTxTM);

		    if (!TmBuffer->empty()) {
			    for (auto rit = TmBuffer->crbegin(); rit != TmBuffer->crend(); ++rit) {
				    if ((*rit)->getFirstHeaderPointer() != TmOIDFrameFirstHeaderPointer) {
					    prevFrame = *rit;
					    break;
				    }
			    }
		    }

		    if (prevFrame == nullptr || (prevFrame->getFirstDataFieldEmptyOctet() == transferFrameDataFieldLength)) {
			    if (packetLength <= transferFrameDataFieldLength) {
				    uint8_t available = vchan.packetLengthBufferTxTM.available();
				    notif = blockingTM(nullptr, transferFrameDataFieldLength, packetLength, vid);
				    lastPacketPlacedIdle =
				        idlePacketQueued && (available - vchan.packetLengthBufferTxTM.available() == 1);
			    } else if (vchan.segmentationTM || idlePacketQueued) {
				    notif = segmentationTM(nullptr, transferFrameDataFieldLength, packetLength, vid);
				    lastPacketPlacedIdle = idlePacketQueued;
			    } else {
				    // packet too large to fit in a frame and segmentation is not allowed -> discard
				    for (uint16_t i = 0; i < packetLength; i++) {
					    vchan.packetBufferTxTM.pop_front();
				    }
				    vchan.packetLengthBufferTxTM.pop_front();
				    notif = etl::unexpected(ServiceChannelNotification::PACKET_EXCEEDS_MAX_SIZE);
			    }
		    } else {
			    if (packetLength <= transferFrameDataFieldLength - prevFrame->getFirstDataFieldEmptyOctet()) {
				    uint8_t available = vchan.packetLengthBufferTxTM.available();
				    notif = blockingTM(prevFrame, transferFrameDataFieldLength, packetLength, vid);
				    lastPacketPlacedIdle =
				        idlePacketQueued && (available - vchan.packetLengthBufferTxTM.available() == 1);
			    } else {
				    notif = segmentationTM(prevFrame, transferFrameDataFieldLength, packetLength, vid);
				    lastPacketPlacedIdle = idlePacketQueued;
			    }
		    }

		    if (!notif.has_value()) {
			    break;
		    }

		    // generate idle space packets if needed
		    idleSpacePacketFuncNotif = generateIdleSpacePacket(vid, masterChannel.framesAfterVcGenerationServiceTxTM.back(),
		                                                       transferFrameDataFieldLength, lastPacketPlacedIdle);
		    if (!idleSpacePacketFuncNotif.has_value()) {
			    notif = etl::unexpected(ServiceChannelNotification::INVALID_INPUT);
			    break;
		    }
		    idlePacketQueued = idleSpacePacketFuncNotif.value();

	    }
	    return notif;
    }

    //  Master Channel Generation
    etl::expected<void, ServiceChannelNotification> ServiceChannelSpaceSegment::mcGenerationRequestTxTM() {
	    if (masterChannel.framesAfterVcGenerationServiceTxTM.empty()) {
		    ccsdsLogNotice(TxRx::Tx, NotificationType::TypeServiceChannelNotif, ServiceChannelNotification::NO_RX_PACKETS_TO_PROCESS);
		    return etl::unexpected(ServiceChannelNotification::NO_RX_PACKETS_TO_PROCESS);
	    }
	    if (masterChannel.toBeTransmittedFramesAfterMCGenerationListTxTM.full()) {
		    ccsdsLogNotice(TxRx::Tx, NotificationType::TypeServiceChannelNotif, ServiceChannelNotification::TX_MC_FRAME_BUFFER_FULL);
		    return etl::unexpected(ServiceChannelNotification::TX_MC_FRAME_BUFFER_FULL);
	    }
	    TransferFrameTM *frame = masterChannel.framesAfterVcGenerationServiceTxTM.front();

	    // TODO: Process secondary headers here (if implemented)

	    // If the frame has an ocf field, ensure there is a clcw available
	    if (frame->getOperationalControlFieldFlag()) {
		    etl::flat_map<uint8_t, etl::queue<CLCW, 1>, MaxVirtualChannels>::iterator it = masterChannel.virtualChannelClcwQueues.begin();
		    while ((it != masterChannel.virtualChannelClcwQueues.end()) && (it->second.empty())) {
			    ++it;
		    }

		    if (it == masterChannel.virtualChannelClcwQueues.end()) {
			    ccsdsLogNotice(TxRx::Tx, NotificationType::TypeServiceChannelNotif, ServiceChannelNotification::CLCW_BUFFER_EMPTY);
			    return etl::unexpected(ServiceChannelNotification::CLCW_BUFFER_EMPTY);
		    } else {
			    frame->setOperationalControlField(it->second.front().clcw);
			    it->second.pop();
		    }
	    }

	    // set master channel frame counter
	    masterChannel.masterChannelFrameCountTM = (masterChannel.masterChannelFrameCountTM == 255) ? 0 : (masterChannel.masterChannelFrameCountTM +
	                                                                                                      1);
	    frame->setMasterChannelFrameCount(masterChannel.masterChannelFrameCountTM);

	    masterChannel.toBeTransmittedFramesAfterMCGenerationListTxTM.push_back(frame);
	    masterChannel.framesAfterVcGenerationServiceTxTM.pop_front();

	    return {};
    }


    // All Frames Generation
    etl::expected<void, ServiceChannelNotification> ServiceChannelSpaceSegment::allFramesGenerationRequestTxTM(uint8_t *frameDataTarget) {
	    if (masterChannel.toBeTransmittedFramesAfterMCGenerationListTxTM.empty()) {
		    ccsdsLogNotice(TxRx::Tx, NotificationType::TypeServiceChannelNotif, ServiceChannelNotification::NO_RX_PACKETS_TO_PROCESS);
		    return etl::unexpected(ServiceChannelNotification::NO_TX_PACKETS_TO_PROCESS);
	    }

	    TransferFrameTM *frame = masterChannel.toBeTransmittedFramesAfterMCGenerationListTxTM.front();

	    uint8_t vid = frame->getVirtualChannelId();
	    BaseVirtualChannel&vchan = masterChannel.virtualChannels.at(vid);

	    if (vchan.frameErrorControlFieldPresent) {
		    frame->appendCRC();
	    }

	    memcpy(frameDataTarget, frame->getFrameData(), frame->getFrameLength());

	    masterChannel.toBeTransmittedFramesAfterMCGenerationListTxTM.pop_front();
	    // Finally, remove octets from memory pool and master copy
	    masterChannel.masterChannelPoolTxTM.deletePacket(frame->getFrameData(), frame->getFrameLength());
	    masterChannel.removeMasterTxTM(frame);

	    return {};
    }
#endif // SPACE_SEGMENT

#ifdef GROUND_SEGMENT
	/** ========================================
	 *   TC TransferFrame - Sending End (TC Tx)
	 *  ========================================
     */

    // Packet Processing
    etl::expected<void, ServiceChannelNotification>
    ServiceChannelGroundSegment::segmentationTC(uint16_t maxTransferFrameDataFieldLength, uint16_t packetLength,
                                   uint8_t vid, uint8_t mapid, ServiceType serviceType) {

	    if (masterChannel.virtualChannels.find(vid) == masterChannel.virtualChannels.end()) {
		    return etl::unexpected(ServiceChannelNotification::INVALID_VC_ID);
	    }

	    VirtualChannelGroundSegment* vchan = &(masterChannel.virtualChannels.at(vid));

	    MAPChannelGroundSegment* mapChannel;
	    if (vchan->segmentHeaderTCPresent) {
		    if (vchan->mapChannels.find(mapid) == vchan->mapChannels.end()) {
			    return etl::unexpected(ServiceChannelNotification::INVALID_MAP_ID);
		    }
		    mapChannel = &(vchan->mapChannels.at(mapid));
	    }

	    etl::queue<uint16_t, PacketBufferTcSize> *packetLengthBufferTcTx;
	    etl::queue<uint8_t, PacketBufferTcSize> *packetBufferTcTx;

	    if (vchan->segmentHeaderTCPresent) {
		    if (serviceType == ServiceType::TYPE_AD) {
			    packetBufferTcTx = &mapChannel->packetBufferTxTcTypeAD;
			    packetLengthBufferTcTx = &mapChannel->packetLengthBufferTxTcTypeAD;
		    } else { // TYPE_BD
			    packetBufferTcTx = &mapChannel->packetBufferTxTcTypeBD;
			    packetLengthBufferTcTx = &mapChannel->packetLengthBufferTxTcTypeBD;
		    }
	    } else if (!vchan->segmentHeaderTCPresent) {
		    if (serviceType == ServiceType::TYPE_AD) {
			    packetBufferTcTx = &vchan->packetBufferTxTcTypeAD;
			    packetLengthBufferTcTx = &vchan->packetLengthBufferTxTcTypeAD;
		    } else { // TYPE_BD
			    packetBufferTcTx = &vchan->packetBufferTxTcTypeBD;
			    packetLengthBufferTcTx = &vchan->packetLengthBufferTxTcTypeBD;
		    }
	    } else {
		    // TYPE_BC or TYPE_RESERVED are invalid
		    return etl::unexpected(ServiceChannelNotification::INVALID_INPUT);
	    }

	    uint8_t securityHeaderLength;
	    uint8_t securityTrailerLength;
	    bool saAssociated = vchan->segmentHeaderTCPresent ? securityAssociation.isAssociated(vid, mapid)
	                                                      : securityAssociation.isAssociated(vid);
	    securityHeaderLength = saAssociated ? securityAssociation.getSecurityHeaderLength() : 0;
	    securityTrailerLength = saAssociated ? securityAssociation.getSecurityTrailerLength() : 0;

	    static uint8_t tmpData[MaxTcTransferFrameSize] = {0};
	    uint16_t numberOfNewTransferFrames = 0;
	    uint8_t trailerSize = vchan->frameErrorControlFieldPresent * ErrorControlFieldSize + securityTrailerLength;
	    uint8_t segmentHeaderLength = vchan->segmentHeaderTCPresent ? TcSegmentHeaderSize : 0;

	    // Calculate amount of new transfer frames needed
	    numberOfNewTransferFrames = packetLength / (maxTransferFrameDataFieldLength - segmentHeaderLength)
	                                +
	                                ((packetLength % (maxTransferFrameDataFieldLength - segmentHeaderLength)) ? 1 : 0);


	    // ensure there is enough space for the new frames
	    if (masterChannel.masterCopyTxTC.available() < numberOfNewTransferFrames) {
		    return etl::unexpected(ServiceChannelNotification::MASTER_CHANNEL_FRAME_BUFFER_FULL);
	    }
	    if (vchan->framesBeforeSDLSProcessingTxTC.available() < numberOfNewTransferFrames) {
		    return etl::unexpected(ServiceChannelNotification::VC_MC_FRAME_BUFFER_FULL);
	    }
	    if (masterChannel.masterChannelPoolTxTC.findFit(numberOfNewTransferFrames *
	                                                        (TcPrimaryHeaderSize + segmentHeaderLength +
	                                                         securityHeaderLength + trailerSize) + packetLength).second !=
	        MasterChannelAlert::NO_MC_ALERT) {
		    return etl::unexpected(ServiceChannelNotification::MEMORY_POOL_FULL);
	    }

	    // new frames creation
	    uint16_t currentTransferFrameDataFieldLength;
	    for (uint16_t i = 0; i < numberOfNewTransferFrames; i++) {
		    for (uint8_t & octet : tmpData) {  // clear up array between consecutive calls
			    octet = 0;
		    }

		    currentTransferFrameDataFieldLength =
		        packetLength > (maxTransferFrameDataFieldLength - segmentHeaderLength)
		            ? maxTransferFrameDataFieldLength : packetLength + segmentHeaderLength;

		    for (uint16_t j = 0; j < currentTransferFrameDataFieldLength - segmentHeaderLength; j++) {
			    tmpData[j + TcPrimaryHeaderSize + segmentHeaderLength +
			            securityHeaderLength] = packetBufferTcTx->front();
			    packetBufferTcTx->pop();
		    }

		    SequenceFlag segmentLengthId = SequenceFlag::SegmentationMiddle;

		    if (i == numberOfNewTransferFrames - 1) {
			    segmentLengthId = SequenceFlag::SegmentationEnd;
		    } else if (i == 0) {
			    segmentLengthId = SequenceFlag::SegmentationStart;
		    }

		    uint8_t *transferFrameData = masterChannel.masterChannelPoolTxTC.allocatePacket(
		        tmpData,
		        TcPrimaryHeaderSize + securityHeaderLength + currentTransferFrameDataFieldLength + trailerSize);

		    if (transferFrameData == nullptr) {
			    return etl::unexpected(ServiceChannelNotification::MEMORY_POOL_FULL);
		    }

		    TransferFrameTC transferFrameTc =
		        TransferFrameTC(transferFrameData,
		                        serviceType,
		                        vid,
		                        currentTransferFrameDataFieldLength + TcPrimaryHeaderSize + securityHeaderLength +
		                            trailerSize,
		                        vchan->segmentHeaderTCPresent,
		                        segmentLengthId,
		                        mapid,
		                        currentTransferFrameDataFieldLength);
		    masterChannel.masterCopyTxTC.push_back(transferFrameTc);
		    vchan->framesBeforeSDLSProcessingTxTC.push_back(&(masterChannel.masterCopyTxTC.back()));
		    packetLength -= maxTransferFrameDataFieldLength - segmentHeaderLength;
	    }
	    packetLengthBufferTcTx->pop();
	    return {};
    }

    etl::expected<void, ServiceChannelNotification>
    ServiceChannelGroundSegment::blockingTC(uint16_t maxTransferFrameDataFieldLength, uint16_t packetLength,
                               uint8_t vid, uint8_t mapid, ServiceType serviceType) {

	    if (masterChannel.virtualChannels.find(vid) == masterChannel.virtualChannels.end()) {
		    ccsdsLogNotice(TxRx::Tx, NotificationType::TypeServiceChannelNotif, ServiceChannelNotification::INVALID_VC_ID);
		    return etl::unexpected(ServiceChannelNotification::INVALID_VC_ID);
	    }

	    VirtualChannelGroundSegment* vchan = &(masterChannel.virtualChannels.at(vid));

	    MAPChannelGroundSegment* mapChannel;
	    if (vchan->segmentHeaderTCPresent) {
		    if (vchan->mapChannels.find(mapid) == vchan->mapChannels.end()) {
			    ccsdsLogNotice(TxRx::Tx, NotificationType::TypeServiceChannelNotif, ServiceChannelNotification::INVALID_MAP_ID);
			    return etl::unexpected(ServiceChannelNotification::INVALID_MAP_ID);
		    }
		    mapChannel = &(vchan->mapChannels.at(mapid));
	    }

	    etl::queue<uint16_t, PacketBufferTcSize> *packetLengthBufferTcTx;
	    etl::queue<uint8_t, PacketBufferTcSize> *packetBufferTcTx;

	    if (vchan->segmentHeaderTCPresent) {
		    if (serviceType == ServiceType::TYPE_AD) {
			    packetBufferTcTx = &mapChannel->packetBufferTxTcTypeAD;
			    packetLengthBufferTcTx = &mapChannel->packetLengthBufferTxTcTypeAD;
		    } else { // TYPE_BD
			    packetBufferTcTx = &mapChannel->packetBufferTxTcTypeBD;
			    packetLengthBufferTcTx = &mapChannel->packetLengthBufferTxTcTypeBD;
		    }
	    } else if (!vchan->segmentHeaderTCPresent) {
		    if (serviceType == ServiceType::TYPE_AD) {
			    packetBufferTcTx = &vchan->packetBufferTxTcTypeAD;
			    packetLengthBufferTcTx = &vchan->packetLengthBufferTxTcTypeAD;
		    } else { // TYPE_BD
			    packetBufferTcTx = &vchan->packetBufferTxTcTypeBD;
			    packetLengthBufferTcTx = &vchan->packetLengthBufferTxTcTypeBD;
		    }
	    } else {
		    // TYPE_BC or TYPE_RESERVED are invalid
		    ccsdsLogNotice(TxRx::Tx, NotificationType::TypeServiceChannelNotif, ServiceChannelNotification::INVALID_INPUT);
		    return etl::unexpected(ServiceChannelNotification::INVALID_INPUT);
	    }

	    uint8_t securityHeaderLength;
	    uint8_t securityTrailerLength;
	    bool saAssociated = vchan->segmentHeaderTCPresent ? securityAssociation.isAssociated(vid, mapid)
	                                                      : securityAssociation.isAssociated(vid);
	    securityHeaderLength = saAssociated ? securityAssociation.getSecurityHeaderLength() : 0;
	    securityTrailerLength = saAssociated ? securityAssociation.getSecurityTrailerLength() : 0;


	    uint8_t trailerSize = vchan->frameErrorControlFieldPresent * ErrorControlFieldSize + securityTrailerLength;
	    uint8_t segmentHeaderLength = vchan->segmentHeaderTCPresent ? TcSegmentHeaderSize : 0;

	    // ensure there is enough space for a new frame
	    if (masterChannel.masterCopyTxTC.available() == 0) {
		    return etl::unexpected(ServiceChannelNotification::MASTER_CHANNEL_FRAME_BUFFER_FULL);
	    }
	    if (vchan->framesBeforeSDLSProcessingTxTC.available() == 0) {
		    return etl::unexpected(ServiceChannelNotification::VC_MC_FRAME_BUFFER_FULL);
	    }
	    if (masterChannel.masterChannelPoolTxTC.findFit(
	                                               TcPrimaryHeaderSize + segmentHeaderLength + securityHeaderLength + packetLength + trailerSize).second !=
	        MasterChannelAlert::NO_MC_ALERT) {
		    return etl::unexpected(ServiceChannelNotification::MEMORY_POOL_FULL);
	    }


	    uint16_t currentTransferFrameDataFieldLength = segmentHeaderLength;
	    // @TODO change MaxTCtransfersize to vchan.maxFrameLengthTC
	    static uint8_t tmpData[MaxTcTransferFrameSize] = {0};
	    for (uint8_t & octet : tmpData) {  // clear up array between consecutive calls
		    octet = 0;
	    }

	    bool blockingAllowed = vchan->segmentHeaderTCPresent ? mapChannel->blockingTC : vchan->blockingTC;
	    while (currentTransferFrameDataFieldLength + packetLength <= maxTransferFrameDataFieldLength &&
	           !packetLengthBufferTcTx->empty()) {
		    for (uint16_t i = 0; i < packetLength; i++) {
			    tmpData[i + TcPrimaryHeaderSize + securityHeaderLength +
			            currentTransferFrameDataFieldLength] = packetBufferTcTx->front();
			    packetBufferTcTx->pop();
		    }
		    currentTransferFrameDataFieldLength += packetLength;
		    packetLengthBufferTcTx->pop();
		    packetLength = packetLengthBufferTcTx->front();
		    // If blocking is disabled, stop the operation on the first packet
		    if (blockingAllowed) {
			    break;
		    }
	    }

	    uint8_t *transferFrameData = masterChannel.masterChannelPoolTxTC.allocatePacket(
	        tmpData,
	        TcPrimaryHeaderSize + securityHeaderLength + currentTransferFrameDataFieldLength + trailerSize);

	    if (transferFrameData == nullptr) {
		    return etl::unexpected(ServiceChannelNotification::MEMORY_POOL_FULL);
	    }

	    SequenceFlag segmentLengthId = SequenceFlag::NoSegmentation;
	    uint16_t firstEmptyOctet = currentTransferFrameDataFieldLength;

	    TransferFrameTC transferFrameTc =
	        TransferFrameTC(transferFrameData,
	                        serviceType,
	                        vid,
	                        currentTransferFrameDataFieldLength + TcPrimaryHeaderSize + securityHeaderLength +
	                            trailerSize,
	                        vchan->segmentHeaderTCPresent,
	                        segmentLengthId,
	                        mapid,
	                        firstEmptyOctet);

	    masterChannel.masterCopyTxTC.push_back(transferFrameTc);
	    vchan->framesBeforeSDLSProcessingTxTC.push_back(&(masterChannel.masterCopyTxTC.back()));

	    return {};
    }

    etl::expected<void, ServiceChannelNotification>
    ServiceChannelGroundSegment::storePacketTxTC(uint8_t *packet, uint16_t packetLength, uint8_t vid,
                                    ServiceType serviceType, etl::optional<uint8_t> mapid) {
	    if (masterChannel.virtualChannels.find(vid) == masterChannel.virtualChannels.end()) {
		    ccsdsLogNotice(TxRx::Tx, NotificationType::TypeServiceChannelNotif, ServiceChannelNotification::INVALID_VC_ID);
		    return etl::unexpected(ServiceChannelNotification::INVALID_VC_ID);
	    }

	    VirtualChannelGroundSegment* vchan = &(masterChannel.virtualChannels.at(vid));

	    MAPChannelGroundSegment* mapChannel;
	    if (vchan->segmentHeaderTCPresent) {
		    if (!mapid.has_value()) {
			    ccsdsLogNotice(TxRx::Tx, NotificationType::TypeServiceChannelNotif, ServiceChannelNotification::INVALID_INPUT);
			    return etl::unexpected(ServiceChannelNotification::INVALID_INPUT);
		    }

		    if (vchan->mapChannels.find(mapid.value()) == vchan->mapChannels.end()) {
			    ccsdsLogNotice(TxRx::Tx, NotificationType::TypeServiceChannelNotif, ServiceChannelNotification::INVALID_MAP_ID);
			    return etl::unexpected(ServiceChannelNotification::INVALID_MAP_ID);
		    }
		    mapChannel = &(vchan->mapChannels.at(mapid.value()));
	    }

	    etl::queue<uint16_t, PacketBufferTcSize> *packetLengthBufferTcTx;
	    etl::queue<uint8_t, PacketBufferTcSize> *packetBufferTcTx;

	    if (vchan->segmentHeaderTCPresent) {
		    if (serviceType == ServiceType::TYPE_AD) {
			    packetBufferTcTx = &(mapChannel->packetBufferTxTcTypeAD);
			    packetLengthBufferTcTx = &(mapChannel->packetLengthBufferTxTcTypeAD);
		    } else { // TYPE_BD
			    packetBufferTcTx = &(mapChannel->packetBufferTxTcTypeBD);
			    packetLengthBufferTcTx = &(mapChannel->packetLengthBufferTxTcTypeBD);
		    }
	    } else if (!vchan->segmentHeaderTCPresent) {
		    if (serviceType == ServiceType::TYPE_AD) {
			    packetBufferTcTx = &(vchan->packetBufferTxTcTypeAD);
			    packetLengthBufferTcTx = &(vchan->packetLengthBufferTxTcTypeAD);
		    } else { // TYPE_BD
			    packetBufferTcTx = &(vchan->packetBufferTxTcTypeBD);
			    packetLengthBufferTcTx = &(vchan->packetLengthBufferTxTcTypeBD);
		    }
	    } else {
		    // TYPE_BC or TYPE_RESERVED are invalid
		    ccsdsLogNotice(TxRx::Tx, NotificationType::TypeServiceChannelNotif, ServiceChannelNotification::INVALID_SERVICE_TYPE);
		    return etl::unexpected(ServiceChannelNotification::INVALID_SERVICE_TYPE);
	    }

	    if (packetLength <= packetBufferTcTx->available()) {
		    packetLengthBufferTcTx->push(packetLength);
		    for (uint16_t i = 0; i < packetLength; i++) {
			    packetBufferTcTx->push(packet[i]);
		    }
		    return {};
	    }

	    return etl::unexpected(ServiceChannelNotification::VC_MC_FRAME_BUFFER_FULL);
    }

    etl::expected<void, ServiceChannelNotification>
    ServiceChannelGroundSegment::packetProcessingRequestTxTC(uint8_t vid, uint8_t mapid, uint8_t maxTransferFrameDataFieldLength,
                                                ServiceType serviceType) {

	    if (serviceType == ServiceType::TYPE_BC || serviceType == ServiceType::TYPE_RESERVED) {
		    // TYPE_BC or TYPE_RESERVED are invalid
		    ccsdsLogNotice(TxRx::Tx, NotificationType::TypeServiceChannelNotif, ServiceChannelNotification::INVALID_SERVICE_TYPE);
		    return etl::unexpected(ServiceChannelNotification::INVALID_SERVICE_TYPE);
	    }

	    if (masterChannel.virtualChannels.find(vid) == masterChannel.virtualChannels.end()) {
		    ccsdsLogNotice(TxRx::Tx, NotificationType::TypeServiceChannelNotif, ServiceChannelNotification::INVALID_VC_ID);
		    return etl::unexpected(ServiceChannelNotification::INVALID_VC_ID);
	    }

	    VirtualChannelGroundSegment* vchan = &(masterChannel.virtualChannels.at(vid));

	    MAPChannelGroundSegment* mapChannel;
	    bool segmentationAllowed = false;
	    if (vchan->segmentHeaderTCPresent) {
		    if (vchan->mapChannels.find(mapid) == vchan->mapChannels.end()) {
			    ccsdsLogNotice(TxRx::Tx, NotificationType::TypeServiceChannelNotif, ServiceChannelNotification::INVALID_MAP_ID);
			    return etl::unexpected(ServiceChannelNotification::INVALID_MAP_ID);
		    }
		    mapChannel = &(vchan->mapChannels.at(mapid));
		    segmentationAllowed = mapChannel->segmentationTC;
	    }

	    uint8_t securityHeaderLength;
	    uint8_t securityTrailerLength;

	    bool saAssociated = vchan->segmentHeaderTCPresent ? securityAssociation.isAssociated(vid, mapid)
	                                                      : securityAssociation.isAssociated(vid);
	    securityHeaderLength = saAssociated ? securityAssociation.getSecurityHeaderLength() : 0;
	    securityTrailerLength = saAssociated ? securityAssociation.getSecurityTrailerLength() : 0;

	    if (maxTransferFrameDataFieldLength >
	        MaxTcTransferFrameSize - TcPrimaryHeaderSize - securityHeaderLength - securityTrailerLength -
	            vchan->frameErrorControlFieldPresent * ErrorControlFieldSize) {
		    return etl::unexpected(ServiceChannelNotification::INVALID_INPUT);
	    }

	    etl::queue<uint16_t, PacketBufferTcSize> *packetLengthBufferTcTx;
	    etl::queue<uint8_t, PacketBufferTcSize> *packetBufferTcTx;

	    if (vchan->segmentHeaderTCPresent) {
		    if (serviceType == ServiceType::TYPE_AD) {
			    packetBufferTcTx = &mapChannel->packetBufferTxTcTypeAD;
			    packetLengthBufferTcTx = &mapChannel->packetLengthBufferTxTcTypeAD;
		    } else { // TYPE_BD
			    packetBufferTcTx = &mapChannel->packetBufferTxTcTypeBD;
			    packetLengthBufferTcTx = &mapChannel->packetLengthBufferTxTcTypeBD;
		    }
	    } else {
		    if (serviceType == ServiceType::TYPE_AD) {
			    packetBufferTcTx = &vchan->packetBufferTxTcTypeAD;
			    packetLengthBufferTcTx = &vchan->packetLengthBufferTxTcTypeAD;
		    } else { // TYPE_BD
			    packetBufferTcTx = &vchan->packetBufferTxTcTypeBD;
			    packetLengthBufferTcTx = &vchan->packetLengthBufferTxTcTypeBD;
		    }
	    }

	    if (packetLengthBufferTcTx->empty()) {
		    return etl::unexpected(ServiceChannelNotification::PACKET_BUFFER_EMPTY);
	    }

	    etl::expected<void, ServiceChannelNotification> notif;

	    while (!packetLengthBufferTcTx->empty()) {
		    uint16_t packetLength = packetLengthBufferTcTx->front();

		    if (packetLength <= maxTransferFrameDataFieldLength - 1 * vchan->segmentHeaderTCPresent) {
			    notif = blockingTC(maxTransferFrameDataFieldLength, packetLength, vid, mapid, serviceType);
		    } else if (segmentationAllowed) {
			    notif = segmentationTC(maxTransferFrameDataFieldLength, packetLength, vid, mapid, serviceType);
		    } else {
			    // packet too large to fit in a frame and segmentation is not allowed -> discard and notify
			    for (uint16_t i = 0; i < packetLength; i++) {
				    packetBufferTcTx->pop();
			    }
			    packetLengthBufferTcTx->pop();
			    notif = etl::unexpected(ServiceChannelNotification::PACKET_EXCEEDS_MAX_SIZE);
		    }

		    if (!notif.has_value()) {
			    break;
		    }
	    }
	    return notif;
    }

    // SDLS Processing
    etl::expected<void, ServiceChannelNotification> ServiceChannelGroundSegment::applySDLSSecurityTxTC(uint8_t vid, uint8_t mapid) {
	    if (masterChannel.virtualChannels.find(vid) == masterChannel.virtualChannels.end()) {
		    ccsdsLogNotice(TxRx::Tx, NotificationType::TypeServiceChannelNotif, ServiceChannelNotification::INVALID_VC_ID);
		    return etl::unexpected(ServiceChannelNotification::INVALID_VC_ID);
	    }

	    VirtualChannelGroundSegment* vchan = &(masterChannel.virtualChannels.at(vid));

	    if (vchan->segmentHeaderTCPresent) {
		    if (vchan->mapChannels.find(mapid) == vchan->mapChannels.end()) {
			    ccsdsLogNotice(TxRx::Tx, NotificationType::TypeServiceChannelNotif, ServiceChannelNotification::INVALID_MAP_ID);
			    return etl::unexpected(ServiceChannelNotification::INVALID_MAP_ID);
		    }
	    }

	    TransferFrameTC *frameTc;
	    if (vchan->framesBeforeSDLSProcessingTxTC.empty()) {
		    return etl::unexpected(ServiceChannelNotification::NO_TX_PACKETS_TO_PROCESS);
	    } else {
		    frameTc = vchan->framesBeforeSDLSProcessingTxTC.front();
	    }

	    if (vchan->unprocessedFrameListBufferTxTC.full()) {
		    return etl::unexpected(ServiceChannelNotification::VC_MC_FRAME_BUFFER_FULL);
	    }


	    uint16_t transferFrameDataFieldLength = frameTc->getFrameLength() - TcPrimaryHeaderSize
	                                            - securityAssociation.getSecurityHeaderLength() -
	                                            securityAssociation.getSecurityTrailerLength() -
	                                            vchan->frameErrorControlFieldPresent * ErrorControlFieldSize;
	    SDLSVerificationError sdlsNotification = securityAssociation.applySecurityTC(frameTc, transferFrameDataFieldLength,
	                                                                           vid, mapid);

	    vchan->framesBeforeSDLSProcessingTxTC.pop_front();
	    if (sdlsNotification == SDLSVerificationError::MAC_CALCULATION_ERROR || sdlsNotification == SDLSVerificationError::INVALID_FRAME_TYPE) {
		    ccsdsLogNotice(TxRx::Tx, NotificationType::TypeSDLSVerificationStatusCode, sdlsNotification);

		    masterChannel.masterChannelPoolTxTC.deletePacket(frameTc->getFrameData(), frameTc->getFrameLength());
		    masterChannel.masterCopyTxTC.remove(*frameTc);
		    return etl::unexpected(ServiceChannelNotification::SDLS_ERROR);
	    } else {
		    vchan->unprocessedFrameListBufferTxTC.push_back(frameTc);
	    }

	    return {};
    }

    // Virtual Channel Generation
    etl::pair<ServiceChannelNotification, FopSignals> ServiceChannelGroundSegment::vcGenerationRequestTxTC(uint8_t vid) {
	    if (masterChannel.virtualChannels.find(vid) == masterChannel.virtualChannels.end()) {
		    ccsdsLogNotice(TxRx::Tx, NotificationType::TypeServiceChannelNotif, ServiceChannelNotification::INVALID_VC_ID);
		    return etl::make_pair(ServiceChannelNotification::INVALID_VC_ID, FopSignals(0));
	    }

	    VirtualChannelGroundSegment* vchan = &(masterChannel.virtualChannels.at(vid));

	    /**
         * Execute state machine. A notification will be returned to indicate if there was unexpected input
         * or event-state combination. The detected event code is also returned (0 for no event). Useful for
         * debugging.
	     */
	    std::pair<FOPNotification, uint8_t> event = vchan->fop.applyFopStateTable();

	    if (event.first != FOPNotification::NO_FOP_EVENT) {
		    ccsdsLogNotice(TxRx::Tx, NotificationType::TypeServiceChannelNotif, ServiceChannelNotification::FOP_ERROR);
		    return etl::make_pair(ServiceChannelNotification::FOP_ERROR, FopSignals(event.second));
	    }

	    // create FopSignals return struct
	    FopSignals fopSignals = FopSignals(event.second);

	    /** transfer notification signal handling
         * - ACCEPT_RESPONSE_TO_TRANSFER_FDU: AD/BD Frame accepted by FOP. Delete pointer from higher layer buffer.
         * - REJECT_RESPONSE_TO_TRANSFER_FDU: AD/BD Frame was not accepted by FOP. Do nothing (a new push will attempted for that fdu).
         * - POSITIVE_CONFIRM_TO_TRANSFER_FDU: AD Frame was received by FARM. Delete it's master copy.
         * - NEGATIVE_CONFIRM_TO_TRANSFER_FDU: AD Frame was not received by FARM, or an error has occurred (usually
         *   accompanied by an alert signal as well). Delete master copy. Delete the lower layer buffer pointer, if it exists.
         *  TODO perhaps the POSITIVE_CONFIRM_TO_TRANSFER_FDU can somehow be translated to which packets went through (and
         *       inform the user)
	     */
	    TransferNotificationSignal *transferNotificationSignal;
	    etl::ilist<TransferFrameTC *>::iterator high_layer_buffer_it = vchan->unprocessedFrameListBufferTxTC.begin();
	    etl::ilist<TransferFrameTC *>::iterator low_layer_buffer_it = masterChannel.outFramesBeforeAllFramesGenerationListTxTC.begin();
	    etl::ilist<TransferFrameTC>::iterator master_copy_buffer_it;
	    while (!vchan->fop.transferNotificationSignalQueue.empty()) {
		    transferNotificationSignal = &vchan->fop.transferNotificationSignalQueue.front();

		    if (!transferNotificationSignal->frame) {
			    ccsdsLogNotice(TxRx::Tx, NotificationType::TypeServiceChannelNotif, ServiceChannelNotification::FOP_ERROR);
			    return etl::make_pair(ServiceChannelNotification::UNEXPECTED_FOP_RETURN_SIGNAL, FopSignals(event.second));
		    }

		    switch (transferNotificationSignal->transferNotificationType) {
			    case TransferNotificationType::ACCEPT_RESPONSE_TO_TRANSFER_FDU:
				    while (high_layer_buffer_it != vchan->unprocessedFrameListBufferTxTC.end()) {
					    if (*high_layer_buffer_it == transferNotificationSignal->frame.value()) {
						    vchan->unprocessedFrameListBufferTxTC.erase(high_layer_buffer_it);
						    break;
					    }
					    ++high_layer_buffer_it;
				    }
				    break;
			    case TransferNotificationType::NEGATIVE_CONFIRM_RESPONSE_TO_TRANSFER_FDU:
				    while (low_layer_buffer_it != masterChannel.outFramesBeforeAllFramesGenerationListTxTC.end()) {
					    if (*low_layer_buffer_it == transferNotificationSignal->frame.value()) {
						    masterChannel.outFramesBeforeAllFramesGenerationListTxTC.erase(low_layer_buffer_it);
						    break;
					    }
					    ++low_layer_buffer_it;
				    }
				    [[fallthrough]];
			    case TransferNotificationType::POSITIVE_CONFIRM_RESPONSE_TO_TRANSFER_FDU:
				    masterChannel.masterChannelPoolTxTC.deletePacket(
				        transferNotificationSignal->frame.value()->getFrameData(),
				        transferNotificationSignal->frame.value()->getFrameLength());

				    master_copy_buffer_it = masterChannel.masterCopyTxTC.begin();
				    while (master_copy_buffer_it != masterChannel.masterCopyTxTC.end()) {
					    if (&(*master_copy_buffer_it) == transferNotificationSignal->frame.value()) {
						    masterChannel.masterCopyTxTC.erase(master_copy_buffer_it);
						    break;
					    }
					    ++master_copy_buffer_it;
				    }
				    break;
			    case TransferNotificationType::REJECT_RESPONSE_TO_TRANSFER_FDU:
				    break;
		    }
		    vchan->fop.transferNotificationSignalQueue.pop();
	    }


	    /** lower layer request signal handling and lower layer response signal pushing
         * - LOW_LAYER_TRANSMIT: AD/BD/BC request for transmitting frame. If there is space, pass the pointer to the lower layer buffer
         *   and send *_ACCEPT to FOP. Otherwise, send *_REJECT.
         * - LOW_LAYER_ABORT: FOP asks to stop ongoing type AD/BC transmission. Delete pointers from lower layer buffer.
         *  TODO (optional): The protocol optionally suggests to stop transmissions to layers lower than the data link.
         *                   Maybe a flag could be raised to notify the user about this.
	     */
	    FopToLowerLayerRequestSignal* fopToLowerLayerRequestSignal;
	    while (!vchan->fop.fopToLowerLayerRequestSignalQueue.empty()) {
		    fopToLowerLayerRequestSignal = &vchan->fop.fopToLowerLayerRequestSignalQueue.front();
		    if (fopToLowerLayerRequestSignal->lowerLayerRequestType == LowerLayerRequestType::LOW_LAYER_TRANSMIT) {

			    if (!fopToLowerLayerRequestSignal->frame) {
				    ccsdsLogNotice(TxRx::Tx, NotificationType::TypeServiceChannelNotif,
				                   ServiceChannelNotification::FOP_ERROR);
				    return etl::make_pair(ServiceChannelNotification::UNEXPECTED_FOP_RETURN_SIGNAL,
				                          FopSignals(event.second));
			    }

			    if (masterChannel.outFramesBeforeAllFramesGenerationListTxTC.full()) {
				    switch (fopToLowerLayerRequestSignal->serviceType) {
					    case ServiceType::TYPE_AD:
						    vchan->fop.pushLowerLayerResponseSignal(
						        LowerLayerResponseSignal(LowerLayerResponseSignal::AD_REJECT));
						    break;
					    case ServiceType::TYPE_BC:
						    vchan->fop.pushLowerLayerResponseSignal(
						        LowerLayerResponseSignal(LowerLayerResponseSignal::BC_REJECT));
						    break;
					    case ServiceType::TYPE_BD:
						    vchan->fop.pushLowerLayerResponseSignal(
						        LowerLayerResponseSignal(LowerLayerResponseSignal::BD_REJECT));
						    break;
					    case ServiceType::TYPE_RESERVED:
						    ccsdsLogNotice(TxRx::Tx, NotificationType::TypeServiceChannelNotif,
						                   ServiceChannelNotification::FOP_ERROR);
						    return etl::make_pair(ServiceChannelNotification::UNEXPECTED_FOP_RETURN_SIGNAL,
						                          FopSignals(event.second));
				    }
			    } else {
				    switch (fopToLowerLayerRequestSignal->serviceType) {
					    case ServiceType::TYPE_AD:
						    vchan->fop.pushLowerLayerResponseSignal(
						        LowerLayerResponseSignal(LowerLayerResponseSignal::AD_ACCEPT));
						    break;
					    case ServiceType::TYPE_BC:
						    vchan->fop.pushLowerLayerResponseSignal(
						        LowerLayerResponseSignal(LowerLayerResponseSignal::BC_ACCEPT));
						    break;
					    case ServiceType::TYPE_BD:
						    vchan->fop.pushLowerLayerResponseSignal(
						        LowerLayerResponseSignal(LowerLayerResponseSignal::BD_ACCEPT));
						    break;
					    case ServiceType::TYPE_RESERVED:
						    ccsdsLogNotice(TxRx::Tx, NotificationType::TypeServiceChannelNotif,
						                   ServiceChannelNotification::FOP_ERROR);
						    return etl::make_pair(ServiceChannelNotification::UNEXPECTED_FOP_RETURN_SIGNAL,
						                          FopSignals(event.second));
				    }

				    masterChannel.outFramesBeforeAllFramesGenerationListTxTC.push_back(
				        fopToLowerLayerRequestSignal->frame.value());
			    }
		    } else {
			    masterChannel.outFramesBeforeAllFramesGenerationListTxTC.clear();
		    }
		    vchan->fop.fopToLowerLayerRequestSignalQueue.pop();
	    }

	    /** directive notification signal handling (processing of one signal only)
         * - ACCEPT_RESPONSE_TO_DIRECTIVE: FOP accepted the request. Initiate directives with set V(R) or unlock will also
         *   generate a type BC frame.
         * - REJECT_RESPONSE_TO_DIRECTIVE: FOP rejected the request. No other action needs to be taken.
         * - POSITIVE_CONFIRM_RESPONSE_TO_DIRECTIVE: Can be received for 3 possible directives:
         *   -- Initiate directives with set V(R) or unlock successfully received by farm. Delete type BC frame master copy.
         *   -- Initiate with clcw check. No other action needs to be taken.
         * - NEGATIVE_CONFIRM_RESPONSE_TO_DIRECTIVE: Can be received for 3 possible directives:
         *   -- Initiate directives with set V(R) or unlock were not received by farm or
         *      an error occurred. Delete type BC frame master copy. Delete the lower layer buffer pointer, if it exists.
         *   -- Initiate with clcw check. No other action needs to be taken.
         *
         *   In any case, the user must also be informed and receive the corresponding request ID.
	     */
	    DirectiveNotificationSignal *directiveNotificationSignal;
	    if (!vchan->fop.directiveNotificationSignalQueue.empty()) {
		    directiveNotificationSignal = &vchan->fop.directiveNotificationSignalQueue.front();

		    // received a response for an initiate directive with set V(R) or unlock
		    if (directiveNotificationSignal->frame.has_value()) {
			    switch (directiveNotificationSignal->directiveNotificationType) {
				    case DirectiveNotificationType::NEGATIVE_CONFIRM_RESPONSE_TO_DIRECTIVE:
					    while (low_layer_buffer_it != masterChannel.outFramesBeforeAllFramesGenerationListTxTC.end()) {
						    if (*low_layer_buffer_it == directiveNotificationSignal->frame.value()) {
							    masterChannel.outFramesBeforeAllFramesGenerationListTxTC.erase(low_layer_buffer_it);
							    break;
						    }
						    ++low_layer_buffer_it;
					    }
					    [[fallthrough]];
				    case DirectiveNotificationType::POSITIVE_CONFIRM_RESPONSE_TO_DIRECTIVE:
					    masterChannel.masterChannelPoolTxTC.deletePacket(
					        directiveNotificationSignal->frame.value()->getFrameData(),
					        directiveNotificationSignal->frame.value()->getFrameLength());

					    master_copy_buffer_it = masterChannel.masterCopyTxTC.begin();
					    while (master_copy_buffer_it != masterChannel.masterCopyTxTC.end()) {
						    if (&(*master_copy_buffer_it) == directiveNotificationSignal->frame.value()) {
							    masterChannel.masterCopyTxTC.erase(master_copy_buffer_it);
							    break;
						    }
						    ++master_copy_buffer_it;
					    }
					    break;
				    case DirectiveNotificationType::ACCEPT_RESPONSE_TO_DIRECTIVE:
					    [[fallthrough]];
				    case DirectiveNotificationType::REJECT_RESPONSE_TO_DIRECTIVE:
					    break;
			    }
		    }

		    fopSignals.directiveNotificationSignal.emplace(*directiveNotificationSignal);
		    vchan->fop.directiveNotificationSignalQueue.pop();
	    }

	    /**
         *  alert signal handling (processing of one signal only)
         *  Inform the user. In every alert, FOP purges it's queues and sends NEGATIVE_CONFIRM_RESPONSE_TO_TRANSFER_FDU
         *  and NEGATIVE_CONFIRM_RESPONSE_TO_DIRECTIVE. Therefore frame deletion is handled via transfer notification and
         *  lower request signal handling.
	     */
	    if (!vchan->fop.asynchronousNotificationSignalQueue.empty()) {
		    fopSignals.asynchronousNotificationSignal.emplace(vchan->fop.asynchronousNotificationSignalQueue.front());
		    vchan->fop.asynchronousNotificationSignalQueue.pop();
	    }

	    /** push transfer fdu signals
         * The higher layer buffer pointer will not be deleted, since it is not yet known if fop can accept the frame.
	     */
	    TransferFrameTC *tcFrame;
	    high_layer_buffer_it = vchan->unprocessedFrameListBufferTxTC.begin();
	    while (high_layer_buffer_it != vchan->unprocessedFrameListBufferTxTC.end() &&
	           !vchan->fop.transferFduSignalQueue.full()) {
		    vchan->fop.transferFduSignalQueue.push(
		        FduTransferSignal((*high_layer_buffer_it)->getServiceType(), *high_layer_buffer_it));
		    ++high_layer_buffer_it;
	    }

	    return etl::make_pair(ServiceChannelNotification::NO_SERVICE_EVENT,fopSignals);
    }


    //         - FOP Directives
    etl::expected<void, ServiceChannelNotification>
    ServiceChannelGroundSegment::pushDirectiveRequestSignal(uint8_t vid, const DirectiveRequestSignal &directiveRequestSignal) {
	    if (masterChannel.virtualChannels.find(vid) == masterChannel.virtualChannels.end()) {
		    ccsdsLogNotice(TxRx::Tx, NotificationType::TypeServiceChannelNotif, ServiceChannelNotification::INVALID_VC_ID);
		    return etl::unexpected(ServiceChannelNotification::INVALID_VC_ID);
	    }

	    VirtualChannelGroundSegment* vchan = &(masterChannel.virtualChannels.at(vid));

	    FOPNotification fopNotification = vchan->fop.pushDirectiveRequestSignal(directiveRequestSignal);
	    if (fopNotification == FOPNotification::SIGNAL_QUEUE_FULL) {
		    ccsdsLogNotice(TxRx::Tx, NotificationType::TypeServiceChannelNotif, ServiceChannelNotification::FOP_BUFFER_FULL);
		    return etl::unexpected(ServiceChannelNotification::FOP_BUFFER_FULL);
	    }

	    return {};
    }

    etl::expected<void, ServiceChannelNotification> ServiceChannelGroundSegment::pushClcwToFop(uint8_t vid, CLCW clcw) {
	    if (masterChannel.virtualChannels.find(vid) == masterChannel.virtualChannels.end()) {
		    ccsdsLogNotice(TxRx::Tx, NotificationType::TypeServiceChannelNotif, ServiceChannelNotification::INVALID_VC_ID);
		    return etl::unexpected(ServiceChannelNotification::INVALID_VC_ID);
	    }

	    VirtualChannelGroundSegment* vchan = &(masterChannel.virtualChannels.at(vid));

	    vchan->fop.pushClcw(clcw);
	    return {};
    }

    FOPState ServiceChannelGroundSegment::getFopState(uint8_t vid) const {
	    return masterChannel.virtualChannels.at(vid).fop.state;
    }

    uint16_t ServiceChannelGroundSegment::getT1Timer(uint8_t vid) const {
	    return masterChannel.virtualChannels.at(vid).fop.tiInitial;
    }

    uint8_t ServiceChannelGroundSegment::getFopSlidingWindowWidth(uint8_t vid) const {
	    return masterChannel.virtualChannels.at(vid).fop.fopSlidingWindowWidth;
    }

    bool ServiceChannelGroundSegment::getTimeoutType(uint8_t vid) const {
	    return masterChannel.virtualChannels.at(vid).fop.timeoutType;
    }

    uint8_t ServiceChannelGroundSegment::getTransmitterFrameSeqNumber(uint8_t vid) const {
	    return masterChannel.virtualChannels.at(vid).fop.transmitterFrameSeqNumber;
    }

    uint8_t ServiceChannelGroundSegment::getExpectedFrameSeqNumber(uint8_t vid) const {
	    return masterChannel.virtualChannels.at(vid).fop.expectedAcknowledgementSeqNumber;
    }

    // All frames generation
    etl::expected<uint16_t, ServiceChannelNotification>
    ServiceChannelGroundSegment::allFramesGenerationRequestTxTC(uint8_t *frameTarget) {
	    if (masterChannel.outFramesBeforeAllFramesGenerationListTxTC.empty()) {
		    ccsdsLogNotice(TxRx::Tx, NotificationType::TypeServiceChannelNotif, ServiceChannelNotification::NO_TX_PACKETS_TO_PROCESS);
		    return etl::unexpected(ServiceChannelNotification::NO_TX_PACKETS_TO_PROCESS);
	    }

	    TransferFrameTC *frame = masterChannel.outFramesBeforeAllFramesGenerationListTxTC.front();
	    uint16_t frameLength = frame->getFrameLength();

	    uint8_t vid = frame->getVirtualChannelId();
	    BaseVirtualChannel&vchan = masterChannel.virtualChannels.at(vid);

	    if (vchan.frameErrorControlFieldPresent) {
		    frame->appendCRC();
	    }

	    std::memcpy(frameTarget, frame->getFrameData(), frameLength);

	    // NOTE: Type AD and BC frame deletion occurs in vcGeneration, once the right signal is given from fop.
	    //       Therefore, only type BD frames are deleted here
	    masterChannel.outFramesBeforeAllFramesGenerationListTxTC.pop_front();
	    if (frame->getServiceType() == ServiceType::TYPE_BD) {
		    masterChannel.masterChannelPoolTxTC.deletePacket(frame->getFrameData(), frameLength);
		    masterChannel.removeMasterTxTC(frame);
	    }

	    return frameLength;
    }

    // TM TransferFrame - Receiving End (TM Rx)
    //
    ////     - All Frames Reception
    //    ServiceChannelNotification ServiceChannelGroundSegment::allFramesReceptionRequestRxTM(uint8_t *frameData, uint16_t frameLength) {
    //        if (masterChannel.masterCopyRxTM.full()) {
    //            ccsdsLogNotice(TxRx::Rx, NotificationType::TypeServiceChannelNotif, ServiceChannelNotification::RX_IN_MC_FULL);
    //            return ServiceChannelNotification::RX_IN_MC_FULL;
    //        }
    //
    //        uint8_t vid = (frameData[1] >> 1) & 0x7;
    //        // Check if Virtual channel Id does not exist in the relevant Virtual Channels map
    //        if (masterChannel.virtualChannels.find(vid) == masterChannel.virtualChannels.end()) {
    //            // If it doesn't, abort operation
    //            ccsdsLogNotice(TxRx::Rx, NotificationType::TypeServiceChannelNotif, ServiceChannelNotification::INVALID_VC_ID);
    //            return ServiceChannelNotification::INVALID_VC_ID;
    //        }
    //
    //        VirtualChannel *virtualChannel = &(masterChannel.virtualChannels.at(vid));
    //        uint8_t trailerSize = virtualChannel->operationalControlFieldTMPresent * TmOperationalControlFieldSize +
    //                              virtualChannel->frameErrorControlFieldPresent * ErrorControlFieldSize;
    //        TransferFrameTM frame = TransferFrameTM(frameData, frameLength, virtualChannel->frameErrorControlFieldPresent,
    //                                                frameLength - TmPrimaryHeaderSize - trailerSize);
    //        bool eccFieldExists = virtualChannel->frameErrorControlFieldPresent;
    //
    //        if (virtualChannel->framesAfterMcReceptionRxTM.full()) {
    //            ccsdsLogNotice(TxRx::Rx, NotificationType::TypeServiceChannelNotif, ServiceChannelNotification::RX_IN_BUFFER_FULL);
    //            return ServiceChannelNotification::RX_IN_BUFFER_FULL;
    //        }
    //
    //        if (eccFieldExists) {
    //            uint16_t len = frame.getFrameLength() - 2;
    //            uint16_t crc = TransferFrameTM::calculateCRC(frame.getFrameData(), len);
    //
    //            uint16_t packet_crc =
    //                    ((static_cast<uint16_t>(frame.getFrameData()[len]) << 8) & 0xFF00) | frame.getFrameData()[len + 1];
    //            if (crc != packet_crc) {
    //                ccsdsLogNotice(TxRx::Rx, NotificationType::TypeServiceChannelNotif, ServiceChannelNotification::RX_INVALID_CRC);
    //                // Invalid transfer frame is discarded and service aborted
    //                return ServiceChannelNotification::RX_INVALID_CRC;
    //            }
    //        }
    //        // Master Channel Reception
    //        uint8_t mc_lost_frames = frame.getMasterChannelFrameCount();
    //
    //        // Check if master channel frames have been lost
    //        uint8_t mc_counter_diff = (mc_lost_frames - masterChannel.masterChannelFrameCountTM) % 0xFF;
    //
    //        if (mc_counter_diff > 1) {
    //            // Log error that frames have been lost, but don't abort processing
    //            ccsdsLogNotice(TxRx::Rx, NotificationType::TypeServiceChannelNotif, ServiceChannelNotification::MC_RX_INVALID_COUNT, mc_counter_diff);
    //        }
    //
    //        // TODO: There should be a TC Tx service that takes as input the clcw stream from yacms and places them in receivedClcwBuffer
    //        // CLCW extraction
    //        etl::optional<uint32_t> operationalControlField = frame.getOperationalControlField();
    //        if (operationalControlField.has_value() && operationalControlField.value() >> 31 == 0) {
    //            CLCW clcw = CLCW(operationalControlField.value());
    //            virtualChannel->receivedClcwBuffer.push_back(CLCW(clcw.getClcw()));
    ////        virtualChannel->fop.validClcwArrival();
    ////        virtualChannel->fop.acknowledgePreviousFrames(clcw.getReportValue());
    //        }
    //        // TODO: Will we use secondary headers? If so they need to be processed here and forward to the respective service
    //        masterChannel.masterCopyRxTM.push_back(frame);
    //
    //        TransferFrameTM *masterFrame = &(masterChannel.masterCopyRxTM.back());
    //        virtualChannel->framesAfterMcReceptionRxTM.push_back(masterFrame);
    //
    //        return ServiceChannelNotification::NO_SERVICE_EVENT;
    //    }
    //
    //
    ////     - Packet Extraction
    //    ServiceChannelNotification ServiceChannelGroundSegment::packetExtractionRxTM(uint8_t vid, uint8_t *packetTarget) {
    //        VirtualChannel *virtualChannel = &(masterChannel.virtualChannels.at(vid));
    //
    //        if (virtualChannel->framesAfterMcReceptionRxTM.full()) {
    //            ccsdsLogNotice(TxRx::Rx, NotificationType::TypeServiceChannelNotif, ServiceChannelNotification::RX_IN_BUFFER_FULL);
    //            return ServiceChannelNotification::RX_IN_BUFFER_FULL;
    //        }
    //
    //        if (virtualChannel->framesAfterMcReceptionRxTM.empty()) {
    //            ccsdsLogNotice(TxRx::Rx, NotificationType::TypeServiceChannelNotif, ServiceChannelNotification::NO_RX_PACKETS_TO_PROCESS);
    //            return ServiceChannelNotification::NO_RX_PACKETS_TO_PROCESS;
    //        }
    //        TransferFrameTM *transferFrameTm = virtualChannel->framesAfterMcReceptionRxTM.front();
    //
    //        uint16_t frameSize = transferFrameTm->getFrameLength();
    //        uint8_t headerSize = 5 + virtualChannel->secondaryHeaderTMLength;
    //        uint8_t trailerSize =
    //                4 * transferFrameTm->getOperationalControlFieldFlag() +
    //                2 * virtualChannel->frameErrorControlFieldPresent;
    //        memcpy(packetTarget, transferFrameTm->getFrameData() + headerSize + 1, frameSize - headerSize - trailerSize);
    //
    //        virtualChannel->framesAfterMcReceptionRxTM.pop_front();
    //        masterChannel.removeMasterRxTM(transferFrameTm);
    //
    //        return ServiceChannelNotification::NO_SERVICE_EVENT;
    //    }
#endif // GROUND_SEGMENT
} // namespace CCSDSDataLinkLayer