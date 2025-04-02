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

	    if (!physicalChannel.isValidScid(scid)) {
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
	    if (physicalChannel.getFrameErrorControlFieldPresent()) {
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
	    uint8_t *transferFrameData = ChannelsInterface::allocateBlockFromMemPoolMasterChannelSpaceSegment(
		    mcChanVariant, DefsAndUtils::FrameType::TC,
		    frameLength, frameData).value();
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
		PhysicalChannel &physicalChannel,
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
		                   ServiceChannelNotification::REQUESTED_FRAME_TYPE_NOT_FOUND);
		    return etl::unexpected(ServiceChannelNotification::REQUESTED_FRAME_TYPE_NOT_FOUND);
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

    	// Pass the frame to the next process if this virtual channel (as well as the corresponding map channel
    	// if a segmentation header exists) is not associated with this SA
    	const bool vChanAssociated = baseVchanPtr->getAssociatedSdlsSPI() == securityAssociation.getSecurityParameterIndex();
    	bool mapChanIsAssociated = false;
    	if (segHeaderExists) {
    		mapChanIsAssociated =
    			ChannelsInterface::upcastToBase(*mapChanVariantPtr)->getAssociatedSdlsSPI() == securityAssociation.getSecurityParameterIndex();
    	}

    	if (!vChanAssociated || (segHeaderExists && !mapChanIsAssociated)) {
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
										physicalChannel.getFrameErrorControlFieldPresent() * DefsAndUtils::ErrorControlFieldSize;

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
    	if (packetLength < (DefsAndUtils::SpacePacketPrimaryHeaderLength + 1) || packetLength > DefsAndUtils::MaxSpacePacketSize) {
    		ccsdsLogNotice(TxRx::Tx, NotificationType::TypeServiceChannelNotif, ServiceChannelNotification::INVALID_LENGTH);
    		return etl::unexpected(ServiceChannelNotification::INVALID_LENGTH);
    	}

	    auto opResult = ChannelsInterface::pushTmPacketVirtualChannelSpaceSegment(vcChanVariant, packetSource, packetLength);

    	if (!opResult.has_value()) {
    		ccsdsLogNotice(TxRx::Tx, NotificationType::TypeServiceChannelNotif, ServiceChannelNotification::PACKET_QUEUE_FULL);
    		return etl::unexpected(ServiceChannelNotification::PACKET_QUEUE_FULL);
    	}

    	return {};
    }

    // Virtual Channel Generation
    etl::expected<void, ServiceChannelNotification>
    ServiceChannelSpaceSegment::blockingTM(
	    const PhysicalChannel &physicalChannel,
	    MasterChannelSpaceSegmentVariant &mcChanVariant,
	    VirtualChannelSpaceSegmentVariant &vcChanVariant,
	    bool& finishedOperationsFlag,
	    etl::optional<etl::pair<TransferFrameTM*, uint16_t>>& segmentationData
    ) {
    	const BaseMasterChannel* baseMchanPtr = ChannelsInterface::upcastToBase(mcChanVariant);
    	const BaseVirtualChannel* baseVchanPtr = ChannelsInterface::upcastToBase(vcChanVariant);

    	// get existing frame under processing
    	auto expectedFrame = ChannelsInterface::getFrameVirtualChannelSpaceSegment(vcChanVariant,
    		ChannelsInterface::VchanBuffType::UNDER_PROCESSING_TM,
    		DefsAndUtils::TmFrameProcessingStage::UNDER_PROCESSING_BY_VC_GENERATION);

    	TransferFrameTM* frameTmPtr;
	    if (expectedFrame.has_value()) {
	    	// set pointer to already existing frame that needs processing
		    frameTmPtr = etl::get<TransferFrameTM*>(expectedFrame.value());
	    } else {
			// a new frame needs to be created
	    	const uint16_t frameLength = physicalChannel.getTMFrameLength();
	    	// check if there is enough space for a new frame
			if (!ChannelsInterface::hasCapacityForFrameDataMasterChannelSpaceSegment(
				mcChanVariant, DefsAndUtils::FrameType::TM, 1, frameLength)) {
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

	    	// allocate frame data and create the frame object
			uint8_t *frameData = ChannelsInterface::allocateBlockFromMemPoolMasterChannelSpaceSegment(
				mcChanVariant, DefsAndUtils::FrameType::TM, frameLength).value();
			frameTmPtr = etl::get<TransferFrameTM *>(
				ChannelsInterface::addFrameObjectToMasterCopyBufferMasterChannelSpaceSegment(
					mcChanVariant,
					DefsAndUtils::FrameType::TM,
					TransferFrameTM(frameData,
					                frameLength,
					                DefsAndUtils::extractVcidFromGvcid(baseVchanPtr->getGvcid()),
					                DefsAndUtils::extractScidFromMcid(baseMchanPtr->getMscid()),
					                baseVchanPtr->getOperationalControlFieldTMPresent(),
					                ChannelsInterface::readAndUpdateTmFrameCountVirtualChannelSpaceSegment(
						                vcChanVariant),
					                baseVchanPtr->getSecondaryHeaderTMPresent(),
					                baseVchanPtr->getSynchronization(),
					                DefsAndUtils::PacketOrderFlag,
					                DefsAndUtils::SegmentLengthIdentifierLegacy,
					                0,
					                physicalChannel.getFrameErrorControlFieldPresent())).value());

			ChannelsInterface::pushFrameVirtualChannelSpaceSegment(mcChanVariant, vcChanVariant, frameTmPtr,
			                                                       ChannelsInterface::VchanBuffType::UNDER_PROCESSING_TM);
	    	frameTmPtr->setFirstDataFieldEmptyOctet(0);
	    }

	    const uint16_t transferFrameDataFieldLength =
			    physicalChannel.getTMFrameLength()
    	        - DefsAndUtils::TmPrimaryHeaderSize
			    - baseVchanPtr->getOperationalControlFieldTMPresent() * DefsAndUtils::TmOperationalControlFieldSize
			    - physicalChannel.getFrameErrorControlFieldPresent() * DefsAndUtils::ErrorControlFieldSize;

    	// Helper function for popping packet lengths. Returns false when there are no more packets to pop.
    	auto updatePacketLength = [](VirtualChannelSpaceSegmentVariant &vcChanVar, uint16_t &packetLen) -> bool {
    		if (auto expectedPacketLength =
						ChannelsInterface::popTmPacketLengthVirtualChannelSpaceSegment(vcChanVar);
				expectedPacketLength.has_value()) {
    			packetLen = expectedPacketLength.value();
    			return true;
				}
    		return false;
    	};

    	// Pop packets from queue and append them to the data field.
    	uint16_t packetLength;
    	while (updatePacketLength(vcChanVariant, packetLength)) {
			if (frameTmPtr->getFirstDataFieldEmptyOctet() + packetLength > transferFrameDataFieldLength) {
				// Next packet does not fit. Give a request to the segmentation method.
				frameTmPtr->updateProcessingStage(DefsAndUtils::TmFrameProcessingStage::UNDER_PROCESSING_BY_VC_GENERATION);
				segmentationData = etl::make_pair(frameTmPtr, packetLength);
				return {};
			}

    		ChannelsInterface::popTmPacketSegmentVirtualChannelSpaceSegment(vcChanVariant,
					packetLength, frameTmPtr->getFrameData() + DefsAndUtils::TmPrimaryHeaderSize + frameTmPtr->getFirstDataFieldEmptyOctet());
    		frameTmPtr->setFirstDataFieldEmptyOctet(frameTmPtr->getFirstDataFieldEmptyOctet() + packetLength);
    	}

    	// Ran out of packets and frame is filled. Mark it as processed, end operations.
    	if (frameTmPtr->getFirstDataFieldEmptyOctet() == transferFrameDataFieldLength) {
			frameTmPtr->updateProcessingStage(DefsAndUtils::TmFrameProcessingStage::PROCESSED_BY_MC_GENERATION);
    		finishedOperationsFlag = true;
    		return {};
    	}

    	// Ran out of packets but frame is not filled. Push idle space packet to queue.
    	const uint16_t remainingSpace = transferFrameDataFieldLength - frameTmPtr->getFirstDataFieldEmptyOctet();
    	generateIdleSpacePacket(vcChanVariant, remainingSpace);

    	updatePacketLength(vcChanVariant, packetLength);
    	if (remainingSpace >= packetLength) {
    		// Generated idle packet fits perfectly. Append it, mark the frame as processed, end operations.
    		ChannelsInterface::popTmPacketSegmentVirtualChannelSpaceSegment(vcChanVariant,
		    packetLength, frameTmPtr->getFrameData() + DefsAndUtils::TmPrimaryHeaderSize + frameTmPtr->getFirstDataFieldEmptyOctet());
    		frameTmPtr->setFirstDataFieldEmptyOctet(frameTmPtr->getFirstDataFieldEmptyOctet() + packetLength);
    		frameTmPtr->updateProcessingStage(DefsAndUtils::TmFrameProcessingStage::PROCESSED_BY_VC_GENERATION);
    		finishedOperationsFlag = true;
    		return {};
    	} else {
    		// Generated idle packet does not fit. Give a request to the segmentation method.
    		frameTmPtr->updateProcessingStage(DefsAndUtils::TmFrameProcessingStage::UNDER_PROCESSING_BY_VC_GENERATION);
    		segmentationData = etl::make_pair(frameTmPtr, packetLength);
    		return {};
    	}
    }

	etl::expected<void, ServiceChannelNotification>
	ServiceChannelSpaceSegment::segmentationTM(
	const PhysicalChannel& physicalChannel,
	MasterChannelSpaceSegmentVariant &mcChanVariant,
	VirtualChannelSpaceSegmentVariant &vcChanVariant,
	TransferFrameTM* frameTm,
	const uint16_t packetLength
	) {
    	const BaseMasterChannel* baseMchanPtr = ChannelsInterface::upcastToBase(mcChanVariant);
    	const BaseVirtualChannel* baseVchanPtr = ChannelsInterface::upcastToBase(vcChanVariant);

    	const uint16_t transferFrameDataFieldLength =
			   physicalChannel.getTMFrameLength()
			   - DefsAndUtils::TmPrimaryHeaderSize
			   - baseVchanPtr->getOperationalControlFieldTMPresent() * DefsAndUtils::TmOperationalControlFieldSize
			   - physicalChannel.getFrameErrorControlFieldPresent() * DefsAndUtils::ErrorControlFieldSize;

    	// calculate amount of new transfer frames and octets needed to fit the whole packet
    	const uint16_t prevFrameCapacity = transferFrameDataFieldLength - frameTm->getFirstDataFieldEmptyOctet();
    	const uint8_t numberOfNewTransferFrames = (packetLength - prevFrameCapacity) / transferFrameDataFieldLength +
										((packetLength - prevFrameCapacity) % transferFrameDataFieldLength ? 1 : 0);
    	const uint16_t numberOfNewOctets = numberOfNewTransferFrames * (
									   DefsAndUtils::TmPrimaryHeaderSize + transferFrameDataFieldLength +
									   baseVchanPtr->getOperationalControlFieldTMPresent() * DefsAndUtils::TmOperationalControlFieldSize +
									   physicalChannel.getFrameErrorControlFieldPresent() * DefsAndUtils::ErrorControlFieldSize);

	    // Ensure there is enough space for the new frames. If not, then the operation should be halted, and
    	// the packet's length must be returned to the front of the packet length queue (since blockingTM popped it).
    	etl::optional<ServiceChannelNotification> notif;
	    if (!ChannelsInterface::hasCapacityForFrameDataMasterChannelSpaceSegment(
		    mcChanVariant, DefsAndUtils::FrameType::TM,
		    numberOfNewTransferFrames, numberOfNewOctets)) {
	    	notif = ServiceChannelNotification::NOT_ENOUGH_SPACE_IN_MASTER_COPY_OR_MEMORY_POOL;
	    }

	    if (ChannelsInterface::frameListAvailableVirtualChannelSpaceSegment(
		        vcChanVariant, ChannelsInterface::VchanBuffType::UNDER_PROCESSING_TM) < numberOfNewTransferFrames) {
	    	notif = ServiceChannelNotification::FRAME_LIST_FULL;
	    }

    	if (notif.has_value()) {
    		// return the packet length back to the queue
    		etl::visit([&](auto& vcChan) {
    			vcChan.packetLengthBufferTM.push_front(packetLength);
    		}, vcChanVariant);

    		ccsdsLogNotice(TxRx::Tx, NotificationType::TypeServiceChannelNotif, notif.value());
    		return etl::unexpected(notif.value());
    	}

	    // fill half-full frame
	    ChannelsInterface::popTmPacketSegmentVirtualChannelSpaceSegment(vcChanVariant, prevFrameCapacity,
	                                                                    frameTm->getFrameData() +
	                                                                    DefsAndUtils::TmPrimaryHeaderSize + frameTm->
	                                                                    getFirstDataFieldEmptyOctet());
    	frameTm->updateProcessingStage(DefsAndUtils::TmFrameProcessingStage::PROCESSED_BY_VC_GENERATION);

    	// create the new frames
    	uint16_t remainingPacketSegmentLength = packetLength - prevFrameCapacity;
    	const uint16_t frameLength = physicalChannel.getTMFrameLength();
	    for (uint8_t i = 0; i < numberOfNewTransferFrames; ++i) {
		    uint8_t *frameData = ChannelsInterface::allocateBlockFromMemPoolMasterChannelSpaceSegment(
			    mcChanVariant, DefsAndUtils::FrameType::TM, frameLength).value();

	    	ChannelsInterface::popTmPacketSegmentVirtualChannelSpaceSegment(vcChanVariant, remainingPacketSegmentLength,
	    		frameData + DefsAndUtils::TmOperationalControlFieldSize);

		    uint16_t firstHeaderPointer;
	    	if (i == numberOfNewTransferFrames - 1) {
	    		// last frame
	    		if (remainingPacketSegmentLength == transferFrameDataFieldLength) {
	    			firstHeaderPointer = DefsAndUtils::TmNoPacketStartFirstHeaderPointerVal;
	    		} else {
	    			firstHeaderPointer = remainingPacketSegmentLength;
	    		}
	    	} else {
	    		// intermediate frame
	    		firstHeaderPointer = DefsAndUtils::TmNoPacketStartFirstHeaderPointerVal;
	    	}

		    auto frameTmPtr = etl::get<TransferFrameTM*>(
	    	ChannelsInterface::addFrameObjectToMasterCopyBufferMasterChannelSpaceSegment(
			    mcChanVariant, DefsAndUtils::FrameType::TM,
			    TransferFrameTM(frameData,
			                    frameLength,
			                    DefsAndUtils::extractVcidFromGvcid(baseVchanPtr->getGvcid()),
			                    DefsAndUtils::extractScidFromMcid(baseMchanPtr->getMscid()),
			                    baseVchanPtr->getOperationalControlFieldTMPresent(),
			                    ChannelsInterface::readAndUpdateTmFrameCountVirtualChannelSpaceSegment(vcChanVariant),
			                    baseVchanPtr->getSecondaryHeaderTMPresent(),
			                    baseVchanPtr->getSynchronization(),
			                    DefsAndUtils::PacketOrderFlag,
			                    DefsAndUtils::SegmentLengthIdentifierLegacy,
			                    firstHeaderPointer,
			                    physicalChannel.getFrameErrorControlFieldPresent(),
			                    (i == numberOfNewTransferFrames - 1) ? firstHeaderPointer : frameLength)).value());

	    	if (i == numberOfNewTransferFrames - 1) {
	    		// last frame
	    		if (remainingPacketSegmentLength == transferFrameDataFieldLength) {
	    			frameTmPtr->updateProcessingStage(DefsAndUtils::TmFrameProcessingStage::PROCESSED_BY_VC_GENERATION);
	    		} else {
	    			frameTmPtr->updateProcessingStage(DefsAndUtils::TmFrameProcessingStage::UNDER_PROCESSING_BY_VC_GENERATION);
	    		}
	    	} else {
	    		// intermediate frame
	    		frameTmPtr->updateProcessingStage(DefsAndUtils::TmFrameProcessingStage::PROCESSED_BY_VC_GENERATION);
	    	}

	    	ChannelsInterface::pushFrameVirtualChannelSpaceSegment(mcChanVariant, vcChanVariant, frameTmPtr,
													   ChannelsInterface::VchanBuffType::UNDER_PROCESSING_TM);
    		remainingPacketSegmentLength -= transferFrameDataFieldLength;
    	}

    	return {};
    }

    void ServiceChannelSpaceSegment::generateIdleSpacePacket(VirtualChannelSpaceSegmentVariant &vChanVariant,
                                                        const uint16_t remainingDataFieldSpace) {
	    uint16_t idlePacketDataFieldLength;
	    if (remainingDataFieldSpace >= DefsAndUtils::SpacePacketPrimaryHeaderLength + 1) {
		    idlePacketDataFieldLength = remainingDataFieldSpace - DefsAndUtils::SpacePacketPrimaryHeaderLength;
	    } else {
		    idlePacketDataFieldLength = 1;
	    }

    	uint8_t tmpData[DefsAndUtils::SpacePacketPrimaryHeaderLength + idlePacketDataFieldLength];

    	// Static primary header fields
	    for (uint8_t i = 0; i < DefsAndUtils::SpacePacketPrimaryHeaderLength - 2; ++i) {
		    tmpData[i] = DefsAndUtils::IdlePacketPrimaryHeader[i];
	    }

    	// Data length field
    	// The idle packet data length needs to be reduced by 1  (see p. 4.1.3.5 of space packet protocol)
	    tmpData[DefsAndUtils::SpacePacketPrimaryHeaderLength - 2] = static_cast<uint8_t>((idlePacketDataFieldLength - 1)  >> 8);
	    tmpData[DefsAndUtils::SpacePacketPrimaryHeaderLength - 1] = static_cast<uint8_t>(idlePacketDataFieldLength - 1);

    	// Data field (idle data)
	    for (uint16_t i = 0; i < idlePacketDataFieldLength; i++) {
		    tmpData[i + DefsAndUtils::SpacePacketPrimaryHeaderLength] = DefsAndUtils::idle_data[i];
	    }

    	ChannelsInterface::pushTmPacketVirtualChannelSpaceSegment(vChanVariant,
    		tmpData, DefsAndUtils::SpacePacketPrimaryHeaderLength + idlePacketDataFieldLength);
    }

    etl::expected<void, ServiceChannelNotification>
    ServiceChannelSpaceSegment::vcGenerationServiceTM(
	    const PhysicalChannel &physicalChannel,
	    MasterChannelSpaceSegmentVariant &mcChanVariant,
	    VirtualChannelSpaceSegmentVariant &vcChanVariant
    ) {
    	const BaseMasterChannel* baseMchanPtr = ChannelsInterface::upcastToBase(mcChanVariant);
    	const BaseVirtualChannel* baseVchanPtr = ChannelsInterface::upcastToBase(vcChanVariant);

	    if ((DefsAndUtils::extractScidFromMcid(baseMchanPtr->getMscid()) !=
	         DefsAndUtils::extractScidFromGvcid(baseVchanPtr->getGvcid())) ||
	        !physicalChannel.isValidScid(DefsAndUtils::extractScidFromMcid(baseMchanPtr->getMscid()))
	    ) {
		    ccsdsLogNotice(TxRx::Tx, NotificationType::TypeServiceChannelNotif,
		                   ServiceChannelNotification::INVALID_CHANNELS_COMBINATION);
		    return etl::unexpected(ServiceChannelNotification::INVALID_CHANNELS_COMBINATION);
	    }

    	// Return immediately if there are no packets available
    	if (!ChannelsInterface::packetAvailableVirtualChannelSpaceSegment(vcChanVariant)) {
    		return etl::unexpected(ServiceChannelNotification::PACKET_QUEUE_EMPTY);
    	}

		bool finishedOperationsFlag = false;
    	etl::expected<void, ServiceChannelNotification> opResult;
    	etl::optional<etl::pair<TransferFrameTM *, uint16_t>> segmentationData;
    	while (true) {
    		// Block packets until queue empties or a packet that does not fit is encountered.
		    opResult = blockingTM(physicalChannel, mcChanVariant, vcChanVariant, finishedOperationsFlag,
		                               segmentationData);
    		if (!opResult.has_value()) {
    			// Not enough space for new frames.
    			return etl::unexpected(opResult.error());
    		}

    		if (finishedOperationsFlag) {
    			return {};
    		}

		    if (segmentationData.has_value()) {
		    	// Got request by blockingTM for segmentation.
			    opResult = segmentationTM(physicalChannel, mcChanVariant, vcChanVariant, segmentationData.value().first,
			                   segmentationData.value().second);
		    	if (!opResult.has_value()) {
		    		return  etl::unexpected(opResult.error());
		    	}
		    	segmentationData = etl::nullopt;
		    }
    	}
    }

	// Virtual channel multiplexing
	etl::expected<void, ServiceChannelNotification> ServiceChannelSpaceSegment::generateOidFrame(
	const PhysicalChannel &physicalChannel,
	MasterChannelSpaceSegmentVariant &mcChanVariant,
	VirtualChannelSpaceSegmentVariant &vChanVariant) {
    	// ensure there is enough space for an OID frame
    	const uint16_t frameLength = physicalChannel.getTMFrameLength();
    	if (!ChannelsInterface::hasCapacityForFrameDataMasterChannelSpaceSegment(
			mcChanVariant, DefsAndUtils::FrameType::TM,
			1, frameLength)) {
    		ccsdsLogNotice(TxRx::Tx, NotificationType::TypeServiceChannelNotif,
						   ServiceChannelNotification::NOT_ENOUGH_SPACE_IN_MASTER_COPY_OR_MEMORY_POOL);
    		return etl::unexpected(ServiceChannelNotification::NOT_ENOUGH_SPACE_IN_MASTER_COPY_OR_MEMORY_POOL);
		}

    	if (ChannelsInterface::frameListAvailableMasterChannelSpaceSegment(mcChanVariant) == 0) {
    		ccsdsLogNotice(TxRx::Tx, NotificationType::TypeServiceChannelNotif,
						   ServiceChannelNotification::FRAME_LIST_FULL);
    		return etl::unexpected(ServiceChannelNotification::FRAME_LIST_FULL);
		}

    	const BaseVirtualChannel* baseVchanPtr = ChannelsInterface::upcastToBase(vChanVariant);
		const BaseMasterChannel* baseMchanPtr = ChannelsInterface::upcastToBase(mcChanVariant);

    	const uint16_t transferFrameDataFieldLength =
			   physicalChannel.getTMFrameLength()
			   - DefsAndUtils::TmPrimaryHeaderSize
			   - baseVchanPtr->getOperationalControlFieldTMPresent() * DefsAndUtils::TmOperationalControlFieldSize
			   - physicalChannel.getFrameErrorControlFieldPresent() * DefsAndUtils::ErrorControlFieldSize;

	    uint8_t *frameData = ChannelsInterface::allocateBlockFromMemPoolMasterChannelSpaceSegment(
		    mcChanVariant, DefsAndUtils::FrameType::TM, frameLength).value();

    	for (uint16_t i = 0; i < transferFrameDataFieldLength; i++) {
    		frameData[i + DefsAndUtils::TmPrimaryHeaderSize] = DefsAndUtils::idle_data[i];
    	}

    	auto oidFramePtr = etl::get<TransferFrameTM*>(
			ChannelsInterface::addFrameObjectToMasterCopyBufferMasterChannelSpaceSegment(
				mcChanVariant, DefsAndUtils::FrameType::TM,
				TransferFrameTM(frameData,
								frameLength,
								DefsAndUtils::extractVcidFromGvcid(baseVchanPtr->getGvcid()),
								DefsAndUtils::extractScidFromMcid(baseMchanPtr->getMscid()),
								baseVchanPtr->getOperationalControlFieldTMPresent(),
								ChannelsInterface::readAndUpdateTmFrameCountVirtualChannelSpaceSegment(vChanVariant),
								baseVchanPtr->getSecondaryHeaderTMPresent(),
								baseVchanPtr->getSynchronization(),
								DefsAndUtils::PacketOrderFlag,
								DefsAndUtils::SegmentLengthIdentifierLegacy,
								DefsAndUtils::TmOIDFrameFirstHeaderPointer,
								physicalChannel.getFrameErrorControlFieldPresent(),
								transferFrameDataFieldLength)).value());

    	oidFramePtr->updateProcessingStage(DefsAndUtils::TmFrameProcessingStage::PROCESSED_BY_VC_MULTIPLEXER);
    	ChannelsInterface::pushTmFrameMasterChannelSpaceSegment(mcChanVariant, oidFramePtr);
    	return {};
    }

	etl::expected<void, ServiceChannelNotification> ServiceChannelSpaceSegment::virtualChannelMultiplexerTM(
		PhysicalChannel &physicalChannel,
		MasterChannelSpaceSegmentVariant mcChanVariant,
		etl::span<VirtualChannelSpaceSegmentVariant &> &vcChanVariants,
		VirtualChannelSpaceSegmentVariant &vcChanVariantForOidGeneration) {
		const BaseMasterChannel *baseMcChanPtr = ChannelsInterface::upcastToBase(mcChanVariant);
		const uint16_t scid = DefsAndUtils::extractScidFromMcid(baseMcChanPtr->getMscid());

		if (!physicalChannel.isValidScid(scid)) {
			ccsdsLogNotice(TxRx::Tx, NotificationType::TypeServiceChannelNotif,
			               ServiceChannelNotification::INVALID_CHANNELS_COMBINATION);
			return etl::unexpected(ServiceChannelNotification::INVALID_CHANNELS_COMBINATION);
		}

		// multiplex virtual channel frames
		bool allChannelsEmpty = true;
		for (auto& vcChanVariant: vcChanVariants) {
			if (BaseVirtualChannel *baseVcChanPtr = ChannelsInterface::upcastToBase(vcChanVariant);
				DefsAndUtils::extractScidFromGvcid(baseVcChanPtr->getGvcid()) != scid) {
				continue;
			}

				auto expectedFrame = ChannelsInterface::getFrameVirtualChannelSpaceSegment(
					vcChanVariant, ChannelsInterface::VchanBuffType::UNDER_PROCESSING_TM,
					DefsAndUtils::TmFrameProcessingStage::PROCESSED_BY_VC_GENERATION);

				if (expectedFrame.has_value()) {
					allChannelsEmpty = false;
					TransferFrameTM *frameTmPtr = etl::get<TransferFrameTM *>(expectedFrame.value());

					auto opResult = ChannelsInterface::pushTmFrameMasterChannelSpaceSegment(mcChanVariant, frameTmPtr);
					if (!opResult.has_value()) {
						// master channel buffer full
						ccsdsLogNotice(TxRx::Tx, NotificationType::TypeServiceChannelNotif,
						               ServiceChannelNotification::FRAME_LIST_FULL);
						return etl::unexpected(ServiceChannelNotification::FRAME_LIST_FULL);
					} else {
						frameTmPtr->updateProcessingStage(DefsAndUtils::TmFrameProcessingStage::PROCESSED_BY_VC_MULTIPLEXER);
						ChannelsInterface::popFrameVirtualChannelSpaceSegment(vcChanVariant, DefsAndUtils::FrameType::TM, frameTmPtr);
					}
				}
		}

		// oid frame generation
		if (allChannelsEmpty) {
			return generateOidFrame(physicalChannel, mcChanVariant, vcChanVariantForOidGeneration);
		}
		return {};
	}

    //  Master Channel Generation
	etl::expected<void, ServiceChannelNotification> ServiceChannelSpaceSegment::mcGenerationRequestTM(
	MasterChannelSpaceSegmentVariant &mcChanVariant,
	etl::span<etl::optional<CLCW>>& clcwContainers) {
		// give priority to frames waiting for a clcw
    	auto expectedTmFrame = ChannelsInterface::getTmFrameMasterChannelSpaceSegment(mcChanVariant, DefsAndUtils::TmFrameProcessingStage::UNDER_PROCESSING_MC_GENERATION);
    	TransferFrameTM* frameTmPtr;
    	if (expectedTmFrame.has_value()) {
    		frameTmPtr = expectedTmFrame.value();
    		for (auto& clcwContainer : clcwContainers) {
    			if (clcwContainer.has_value()) {
    				frameTmPtr->setOperationalControlField(clcwContainer.value().getRawBytes());
    				frameTmPtr->setMasterChannelFrameCount(ChannelsInterface::readAndUpdateTmFrameCountMasterChannelSpaceSegment(mcChanVariant));
    				frameTmPtr->updateProcessingStage(DefsAndUtils::TmFrameProcessingStage::PROCESSED_BY_MC_GENERATION);
    				clcwContainer.reset();
    				break;
    			}
    		}
    	}

    	// process a new frame
    	expectedTmFrame = ChannelsInterface::getTmFrameMasterChannelSpaceSegment(mcChanVariant, DefsAndUtils::TmFrameProcessingStage::PROCESSED_BY_VC_MULTIPLEXER);
    	if (!expectedTmFrame.has_value()) {
    		ccsdsLogNotice(TxRx::Tx, NotificationType::TypeServiceChannelNotif, ServiceChannelNotification::REQUESTED_FRAME_TYPE_NOT_FOUND);
    		return etl::unexpected(ServiceChannelNotification::REQUESTED_FRAME_TYPE_NOT_FOUND);
    	}

    	frameTmPtr = expectedTmFrame.value();
    	// TODO: Process secondary headers here (if implemented)

    	if (frameTmPtr->getOperationalControlFieldFlag()) {
    		bool foundClcw = false;
    		for (auto& clcwContainer : clcwContainers) {
    			if (clcwContainer.has_value()) {
    				frameTmPtr->setOperationalControlField(clcwContainer.value().getRawBytes());
    				clcwContainer.reset();
    				foundClcw = true;
    				break;
    			}
    		}

    		if (!foundClcw) {
    			// frame must wait for a clcw
    			frameTmPtr->updateProcessingStage(DefsAndUtils::TmFrameProcessingStage::UNDER_PROCESSING_MC_GENERATION);
    		} else {
    			frameTmPtr->setMasterChannelFrameCount(ChannelsInterface::readAndUpdateTmFrameCountMasterChannelSpaceSegment(mcChanVariant));
    			frameTmPtr->updateProcessingStage(DefsAndUtils::TmFrameProcessingStage::PROCESSED_BY_MC_GENERATION);
    		}
    	} else {
    		frameTmPtr->setMasterChannelFrameCount(ChannelsInterface::readAndUpdateTmFrameCountMasterChannelSpaceSegment(mcChanVariant));
    		frameTmPtr->updateProcessingStage(DefsAndUtils::TmFrameProcessingStage::PROCESSED_BY_MC_GENERATION);
    	}
    	return {};
    }

    // All Frames Generation
    etl::expected<void, ServiceChannelNotification> allFramesGenerationRequestTM(
	    PhysicalChannel &physicalChannel,
	    MasterChannelSpaceSegmentVariant &mcChanVariant,
	    uint8_t *frameDestination) {
		const BaseMasterChannel* baseMcChanPtr = ChannelsInterface::upcastToBase(mcChanVariant);

    	if (!physicalChannel.isValidScid(DefsAndUtils::extractScidFromMcid(baseMcChanPtr->getMscid()))) {
    		ccsdsLogNotice(TxRx::Tx, NotificationType::TypeServiceChannelNotif,
						   ServiceChannelNotification::INVALID_CHANNELS_COMBINATION);
    		return etl::unexpected(ServiceChannelNotification::INVALID_CHANNELS_COMBINATION);
    	}

    	auto expectedTmFrame = ChannelsInterface::getTmFrameMasterChannelSpaceSegment(mcChanVariant, DefsAndUtils::TmFrameProcessingStage::PROCESSED_BY_MC_GENERATION);
    	if (!expectedTmFrame.has_value()) {
    		ccsdsLogNotice(TxRx::Tx, NotificationType::TypeServiceChannelNotif,
						  ServiceChannelNotification::REQUESTED_FRAME_TYPE_NOT_FOUND);
    		return etl::unexpected(ServiceChannelNotification::REQUESTED_FRAME_TYPE_NOT_FOUND);
    	}

    	TransferFrameTM* frameTmPtr = expectedTmFrame.value();

    	if (physicalChannel.getFrameErrorControlFieldPresent()) {
    		frameTmPtr->appendCRC();
    	}

    	memcpy(frameDestination, frameTmPtr->getFrameData(), frameTmPtr->getFrameLength());

    	ChannelsInterface::popTmFrameMasterChannelSpaceSegment(mcChanVariant, frameTmPtr);
    	ChannelsInterface::removeFrameDataMasterChannelSpaceSegment(mcChanVariant, frameTmPtr);
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