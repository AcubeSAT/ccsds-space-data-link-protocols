#include "SpaceSegmentTcDataHandlingFunctions.hpp"
#include "AddressingAndParsingUtilities.hpp"

#ifdef INCLUDE_SPACE_SEGMENT_CODE
namespace CCSDSDataLinkLayer {
	void SpaceSegmentTcDataHandling::resetMapChannel(MAPChannelSs& mapChan) {
		MasterChannelSsTc& mcChan = Objects::masterChannelSsTcMap.at(mapChan.getParentScid());

		TransferFrameTC* frameTcPtr;
		while (!mapChan.framesAfterProcessSDLSSecurityTypeAD.isEmpty()) {
			frameTcPtr = mapChan.framesAfterProcessSDLSSecurityTypeAD.getFront();
			mapChan.framesAfterProcessSDLSSecurityTypeAD.pop();
			Objects::frameOctetPool.deleteBlock(frameTcPtr->getFrameData());
			mcChan.frameMasterCopies.erase(frameTcPtr);
		}

		while (!mapChan.framesAfterProcessSDLSSecurityTypeBD.isEmpty()) {
			frameTcPtr = mapChan.framesAfterProcessSDLSSecurityTypeBD.getFront();
			mapChan.framesAfterProcessSDLSSecurityTypeBD.pop();
			Objects::frameOctetPool.deleteBlock(frameTcPtr->getFrameData());
			mcChan.frameMasterCopies.erase(frameTcPtr);
		}

		mapChan.segmentedPacketConstructor.resetPacket();
		mapChan.segmentedPacketConstructor.previousFrameSeqFlag = Defs::SequenceFlag::NO_SEGMENTATION;
		mapChan.segmentedPacketConstructor.segmentedPacketRejectionMode = false;
	}

	void SpaceSegmentTcDataHandling::resetVirtualChannel(VirtualChannelSsTc& vcChan) {
		MasterChannelSsTc& mcChan = Objects::masterChannelSsTcMap.at(vcChan.getParentScid());

		TransferFrameTC* frameTcPtr;
		while (!vcChan.framesAfterAllFramesReception.isEmpty()) {
			frameTcPtr = vcChan.framesAfterAllFramesReception.getFront();
			vcChan.framesAfterAllFramesReception.pop();
			Objects::frameOctetPool.deleteBlock(frameTcPtr->getFrameData());
			mcChan.frameMasterCopies.erase(frameTcPtr);
		}

		while (!vcChan.framesAfterVcReceptionTypeAD.isEmpty()) {
			frameTcPtr = vcChan.framesAfterVcReceptionTypeAD.getFront();
			vcChan.framesAfterVcReceptionTypeAD.pop();
			Objects::frameOctetPool.deleteBlock(frameTcPtr->getFrameData());
			mcChan.frameMasterCopies.erase(frameTcPtr);
		}

		while (!vcChan.framesAfterVcReceptionTypeBD.isEmpty()) {
			frameTcPtr = vcChan.framesAfterVcReceptionTypeBD.getFront();
			vcChan.framesAfterVcReceptionTypeBD.pop();
			Objects::frameOctetPool.deleteBlock(frameTcPtr->getFrameData());
			mcChan.frameMasterCopies.erase(frameTcPtr);
		}

		while (!vcChan.framesAfterProcessSDLSSecurityTypeAD.isEmpty()) {
			frameTcPtr = vcChan.framesAfterProcessSDLSSecurityTypeAD.getFront();
			vcChan.framesAfterProcessSDLSSecurityTypeAD.pop();
			Objects::frameOctetPool.deleteBlock(frameTcPtr->getFrameData());
			mcChan.frameMasterCopies.erase(frameTcPtr);
		}

		while (!vcChan.framesAfterProcessSDLSSecurityTypeBD.isEmpty()) {
			frameTcPtr = vcChan.framesAfterProcessSDLSSecurityTypeBD.getFront();
			vcChan.framesAfterProcessSDLSSecurityTypeBD.pop();
			Objects::frameOctetPool.deleteBlock(frameTcPtr->getFrameData());
			mcChan.frameMasterCopies.erase(frameTcPtr);
		}

		vcChan.segmentedPacketConstructor.resetPacket();
		vcChan.segmentedPacketConstructor.previousFrameSeqFlag = Defs::SequenceFlag::NO_SEGMENTATION;
		vcChan.segmentedPacketConstructor.segmentedPacketRejectionMode = false;

		vcChan.setClcwStatusField(0);

		if (vcChan.getCopInEffect()) {
			FrameAcceptanceReporting& farm = Objects::farmMap.at(constructVcidScidKey(vcChan.getVcid(), vcChan.getParentScid()));
			farm.resetFARM();
		}
	}

	void SpaceSegmentTcDataHandling::resetMasterChannel(MasterChannelSsTc& mcChan) {
		mcChan.setNoRfAvailable(false);
		mcChan.setNoBitLock(false);
	}

    etl::expected<void, ServiceChannelNotification> SpaceSegmentTcDataHandling::allFramesReception(
        const PhysicalChannel& phyChan,
        etl::span<uint8_t> frameSource) {

		// Extract the frame's length
    	if (frameSource.size() < 4) {
    		return etl::unexpected(ServiceChannelNotification::INVALID_LENGTH);
    	}
    	const uint16_t frameLength = (static_cast<uint16_t>(frameSource.data()[2] & 0x03) << 8U) |
				   (static_cast<uint16_t>(frameSource.data()[3]));

    	if (frameLength > frameSource.size()) {
    		return etl::unexpected(ServiceChannelNotification::INVALID_LENGTH);
    	}

	    // Create a new TC frame object. The frameData pointer will be used temporarily, to avoid needless memory pool
	    // allocation/deallocation in case the frame needs to be rejected
	    auto frameTc = TransferFrameTC(frameSource.data(), frameLength);

    	// validate crc if the field is present
    	if (phyChan.getFrameErrorControlFieldPresent()) {
    		const uint16_t len = frameLength - 2;
    		const uint16_t crc = calculateCRC16CCITT(etl::span{frameSource.data(), len});

    		const uint16_t packet_crc = (static_cast<uint16_t>(frameTc.getFrameData()[len]) << 8) |
										frameTc.getFrameData()[len + 1];
    		if (crc != packet_crc) {
    			return etl::unexpected(ServiceChannelNotification::INVALID_CRC);
    		}
    	}

	    // Check for valid TFVN
	    if (frameTc.getTransferFrameVersionNumber() != static_cast<uint8_t>(phyChan.getTFVN())) {
		    return etl::unexpected(ServiceChannelNotification::INVALID_TFVN);
	    }

	    // Check for valid SCID
    	const auto mcIt = Objects::masterChannelSsTcMap.find(frameTc.getSpacecraftId());
    	if (mcIt == Objects::masterChannelSsTcMap.end()) {
    		return etl::unexpected(ServiceChannelNotification::INVALID_SCID);
    	}
    	MasterChannelSsTc& mcChan = mcIt->second;

	    // check if frame's vcid is valid
    	const Defs::VcidScidKey key = constructVcidScidKey(frameTc.getVirtualChannelId(), frameTc.getSpacecraftId());
    	const auto vcIt = Objects::virtualChannelSsTcMap.find(key);
	    if (vcIt == Objects::virtualChannelSsTcMap.end()) {
		    return etl::unexpected(ServiceChannelNotification::INVALID_VCID);
	    }
    	VirtualChannelSsTc& vcChan = vcIt->second;

    	// if this virtual channel states that a segmentation header is present, check that the map channel is correct
    	if (vcChan.getSegmentHeaderPresent()) {
    		frameTc.setSegmentationHeaderPresentFlag(true);
    		const Defs::MapidVcidScidKey key = constructMapidVcidScidKey(frameTc.getMapId().value(), frameTc.getSpacecraftId(), frameTc.getSpacecraftId());
    		if (!Objects::mapChannelSsMap.contains(key)) {
    			return etl::unexpected(ServiceChannelNotification::INVALID_MAPID);
    		}
    	}

    	// If this virtual channel does not have cop-1 active, then type BC frames should not arrive. Also ensure
    	// that the service type is not "Type_Reserved"
    	if (frameTc.getServiceType() == Defs::ServiceType::TYPE_RESERVED ||
    		(!vcChan.getCopInEffect() && frameTc.getServiceType() == Defs::ServiceType::TYPE_RESERVED)) {
			return etl::unexpected(ServiceChannelNotification::INVALID_FRAME_SERVICE_TYPE);
    	}

	    // All checks passed. Ensure there is space to accept the frame
    	if (vcChan.channelMutex.tryLockFor(Defs::MutexDelayMs)) {
    		return etl::unexpected(ServiceChannelNotification::FAILED_TO_LOCK_MUTEX);
    	}

    	if (mcChan.channelMutex.tryLockFor(Defs::MutexDelayMs)) {
    		vcChan.channelMutex.unlock();
    		return etl::unexpected(ServiceChannelNotification::FAILED_TO_LOCK_MUTEX);
    	}

    	if (Objects::frameOctetPool.poolMutex.tryLockFor(Defs::MutexDelayMs)) {
    		mcChan.channelMutex.unlock();
    		vcChan.channelMutex.unlock();
    		return etl::unexpected(ServiceChannelNotification::FAILED_TO_LOCK_MUTEX);
    	}

    	if (vcChan.framesAfterAllFramesReception.isFull()) {
    		Objects::frameOctetPool.poolMutex.unlock();
		    mcChan.channelMutex.unlock();
		    vcChan.channelMutex.unlock();
		    return etl::unexpected(ServiceChannelNotification::FRAME_QUEUE_FULL);
    	}

    	if (mcChan.frameMasterCopies.isFull() ||
    		Objects::frameOctetPool.findFit(frameLength).second != MasterChannelAlert::NO_MC_ALERT) {
    		Objects::frameOctetPool.poolMutex.unlock();
    		mcChan.channelMutex.unlock();
		    vcChan.channelMutex.unlock();
		    return etl::unexpected(ServiceChannelNotification::NOT_ENOUGH_SPACE_IN_MASTER_COPY_OR_MEMORY_POOL);
    	}

	    // Allocate transfer frame octets to memory pool and assign new pointer to the TC frame object
		uint8_t* frameData = Objects::frameOctetPool.allocateBlock(frameLength, frameSource.data());
    	frameTc.setNewFrameDataPointer(frameData);

    	TransferFrameTC* frameTcPtr = mcChan.frameMasterCopies.push(frameTc);
    	vcChan.framesAfterAllFramesReception.push(frameTcPtr);

	    return {};
    }

	etl::pair<ServiceChannelNotification, uint8_t> SpaceSegmentTcDataHandling::virtualChannelReception(
			VirtualChannelSsTc& vcChan) {
    	FrameAcceptanceReporting& farm = Objects::farmMap.at(constructVcidScidKey(vcChan.getVcid(), vcChan.getParentScid()));

    	if (vcChan.getCopInEffect()) {
    		const etl::pair<FARMNotification, uint8_t> farmOutput = farm.applyFarmStateTable();
    		if (farmOutput.first != FARMNotification::NO_FARM_EVENT) {
    			return etl::make_pair(ServiceChannelNotification::FARM_ERROR, farmOutput.second);
    		}
    		return etl::make_pair(ServiceChannelNotification::NO_SERVICE_EVENT, farmOutput.second);
    	} else {
    		// just push to the next queue
    		if (!vcChan.channelMutex.tryLockFor(Defs::MutexDelayMs)) {
    			return etl::make_pair(ServiceChannelNotification::FAILED_TO_LOCK_MUTEX, 0);
    		}

    		if (vcChan.framesAfterAllFramesReception.isEmpty()) {
    			vcChan.channelMutex.unlock();
    			return etl::make_pair(ServiceChannelNotification::FRAME_QUEUE_EMPTY, 0);
    		}

    		TransferFrameTC* frameTC = vcChan.framesAfterAllFramesReception.getFront();

    		if (frameTC->getServiceType() == Defs::ServiceType::TYPE_AD) {
    			if (vcChan.framesAfterAllFramesReception.isFull()) {
    				vcChan.channelMutex.unlock();
    				return etl::make_pair(ServiceChannelNotification::FRAME_QUEUE_FULL, 0);
    			}

    			vcChan.framesAfterAllFramesReception.pop();
    			vcChan.framesAfterVcReceptionTypeAD.push(frameTC);
    			vcChan.channelMutex.unlock();
    			return etl::make_pair(ServiceChannelNotification::NO_SERVICE_EVENT, 0);
    		} else { // Type-BD
    			if (vcChan.framesAfterVcReceptionTypeBD.isFull()) {
    				// we need to discard the oldest frame in the circular buffer
    				MasterChannelSsTc& mcChan = Objects::masterChannelSsTcMap.at(vcChan.getParentScid());
    				if (mcChan.channelMutex.tryLockFor(Defs::MutexDelayMs)) {
    					vcChan.channelMutex.unlock();
    					return etl::make_pair(ServiceChannelNotification::FAILED_TO_LOCK_MUTEX, 0);
    				}

    				TransferFrameTC* oldestFrame = vcChan.framesAfterVcReceptionTypeBD.getFront();
    				Objects::frameOctetPool.deleteBlock(oldestFrame->getFrameData());
    				mcChan.frameMasterCopies.erase(oldestFrame);
    				mcChan.channelMutex.unlock();
    			}
    			vcChan.framesAfterVcReceptionTypeBD.push(frameTC);
    		}
    		vcChan.channelMutex.unlock();
    		return etl::make_pair(ServiceChannelNotification::NO_SERVICE_EVENT, 0);
    	}
	}

	etl::expected<void, ServiceChannelNotification> SpaceSegmentTcDataHandling::processSdlsSecurity(
			PhysicalChannel& phyChan,
			VirtualChannelSsTc& vcChan,
			Defs::ServiceType serviceType) {

    	// Fetch next frame to process
    	TransferFrameTC* frameTcPtr;
    	etl::optional<uint16_t> associatedSlsSpi;

    	if (!vcChan.channelMutex.tryLockFor(Defs::MutexDelayMs)) {
    		return etl::unexpected(ServiceChannelNotification::FAILED_TO_LOCK_MUTEX);
    	}

    	if (serviceType == Defs::ServiceType::TYPE_AD) {
    		if (vcChan.framesAfterVcReceptionTypeAD.isEmpty()) {
    			vcChan.channelMutex.unlock();
    			return etl::unexpected(ServiceChannelNotification::FRAME_QUEUE_EMPTY);
    		}
    		frameTcPtr = vcChan.framesAfterVcReceptionTypeAD.getFront();
    	} else { // Type-BD
    		if (vcChan.framesAfterVcReceptionTypeBD.isEmpty()) {
    			vcChan.channelMutex.unlock();
    			return etl::unexpected(ServiceChannelNotification::FRAME_QUEUE_EMPTY);
    		}
    		frameTcPtr = vcChan.framesAfterVcReceptionTypeBD.getFront();
    	}
    	vcChan.channelMutex.unlock();

    	// Check if the frame is associated with a security association
    	// If a segmentation header exists in this virtual channel, also find the respective map channel
    	MAPChannelSs* mapChan;
    	if (vcChan.getSegmentHeaderPresent()) {
    		mapChan = &Objects::mapChannelSsMap.at(
					constructMapidVcidScidKey(frameTcPtr->getMapId().value(), vcChan.getVcid(), vcChan.getParentScid()));
    		associatedSlsSpi = mapChan->getAssociatedSdlsSPI().value();
    	} else {
    		associatedSlsSpi = vcChan.getAssociatedSdlsSPI().value();
    	}

    	// Lock channel mutexes and check if there is enough space in the output buffer
    	if (vcChan.getSegmentHeaderPresent()) {
    		if (!mapChan->channelMutex.tryLockFor(Defs::MutexDelayMs)) {
    			return etl::unexpected(ServiceChannelNotification::FRAME_QUEUE_EMPTY);
    		}

    		if (!vcChan.channelMutex.tryLockFor(Defs::MutexDelayMs)) {
    			mapChan->channelMutex.unlock();
    			return etl::unexpected(ServiceChannelNotification::FRAME_QUEUE_FULL);
    		}

    		if (serviceType == Defs::ServiceType::TYPE_AD && mapChan->framesAfterProcessSDLSSecurityTypeAD.isFull()) {
			    vcChan.channelMutex.unlock();
			    mapChan->channelMutex.unlock();
			    return etl::unexpected(ServiceChannelNotification::FRAME_QUEUE_FULL);
    		}

    		if (serviceType == Defs::ServiceType::TYPE_BD && mapChan->framesAfterProcessSDLSSecurityTypeBD.isFull()) {
			    vcChan.channelMutex.unlock();
			    mapChan->channelMutex.unlock();
			    return etl::unexpected(ServiceChannelNotification::FRAME_QUEUE_FULL);
    		}
    	} else {
    		if (!vcChan.channelMutex.tryLockFor(Defs::MutexDelayMs)) {
    			return etl::unexpected(ServiceChannelNotification::FRAME_QUEUE_FULL);
    		}

    		if (serviceType == Defs::ServiceType::TYPE_AD && vcChan.framesAfterProcessSDLSSecurityTypeAD.isFull()) {
    			vcChan.channelMutex.unlock();
    			return etl::unexpected(ServiceChannelNotification::FRAME_QUEUE_FULL);
    		}

    		if (serviceType == Defs::ServiceType::TYPE_BD && vcChan.framesAfterProcessSDLSSecurityTypeBD.isFull()) {
    			vcChan.channelMutex.unlock();
    			return etl::unexpected(ServiceChannelNotification::FRAME_QUEUE_FULL);
    		}
    	}

    	if (associatedSlsSpi.has_value()) {
    		// security processing is required for this frame
    		SecurityAssociation& sa = Objects::saSpaceSegmentMap.at(associatedSlsSpi.value());

    		if (!sa.saMutex.tryLockFor(Defs::MutexDelayMs)) {
    			vcChan.channelMutex.unlock();
    			if (vcChan.getSegmentHeaderPresent()) {
    				mapChan->channelMutex.unlock();
    			}
    			return etl::unexpected(ServiceChannelNotification::FAILED_TO_LOCK_MUTEX);
    		}

		    const uint16_t transferFrameDataFieldLength = frameTcPtr->getFrameLength() -
		                                            Defs::TcPrimaryHeaderSize -
		                                            sa.getSecurityHeaderLength() -
		                                            sa.getSecurityTrailerLength() -
		                                            phyChan.getFrameErrorControlFieldPresent() *
		                                            Defs::ErrorControlFieldSize;
    		if (sa.getSecurityAssociationStatus() == Defs::SecurityAssociationStatus::RUNNING) {
    			if (const auto status = sa.processSecurity(frameTcPtr, transferFrameDataFieldLength);
					!status.has_value()) {
    				// Because of the previous checks, the only possible errors that can occur are:
    				// INVALID_SPI
    				// MAC_VERIFICATION_FAILURE
    				// ANTI_REPLAY_SEQUENCE_NUMBER_FAILURE
    				// MAC_CALCULATION_ERROR
    				// In case of the first 3 issues, the frame must be discarded
    				if (status.error() == SDLSVerificationError::MAC_CALCULATION_ERROR) {
    					sa.saMutex.unlock();
    					vcChan.channelMutex.unlock();
    					if (vcChan.getSegmentHeaderPresent()) {
    						mapChan->channelMutex.unlock();
    					}
    					return etl::unexpected(ServiceChannelNotification::SLDS_CALCULATION_ERROR);
    				} else {
    					MasterChannelSsTc& mcChan = Objects::masterChannelSsTcMap.at(vcChan.getParentScid());
    					if (!mcChan.channelMutex.tryLockFor(Defs::MutexDelayMs)) {
    						sa.saMutex.unlock();
    						vcChan.channelMutex.unlock();
    						if (vcChan.getSegmentHeaderPresent()) {
    							mapChan->channelMutex.unlock();
    						}
    						return etl::unexpected(ServiceChannelNotification::FAILED_TO_LOCK_MUTEX);
    					}

    					if (!Objects::frameOctetPool.poolMutex.tryLockFor(Defs::MutexDelayMs)) {
    						sa.saMutex.unlock();
    						mcChan.channelMutex.unlock();
    						vcChan.channelMutex.unlock();
    						if (vcChan.getSegmentHeaderPresent()) {
    							mapChan->channelMutex.unlock();
    						}
    						return etl::unexpected(ServiceChannelNotification::FAILED_TO_LOCK_MUTEX);
    					}

    					Objects::frameOctetPool.deleteBlock(frameTcPtr->getFrameData());

    					Objects::frameOctetPool.poolMutex.unlock();
    					sa.saMutex.unlock();
    					mcChan.frameMasterCopies.erase(frameTcPtr);
    					vcChan.channelMutex.unlock();
    					if (vcChan.getSegmentHeaderPresent()) {
    						mapChan->channelMutex.unlock();
    					}
    					return etl::unexpected(ServiceChannelNotification::UNAUTHORIZED_SENDER);
    				}
				}
    		}
    		sa.saMutex.unlock();
    	}

    	// push to next stage
    	if (serviceType == Defs::ServiceType::TYPE_AD) {
		    vcChan.framesAfterVcReceptionTypeAD.pop();
    		if (vcChan.getSegmentHeaderPresent()) {
    			mapChan->framesAfterProcessSDLSSecurityTypeAD.push(frameTcPtr);
    		} else {
    			vcChan.framesAfterProcessSDLSSecurityTypeAD.push(frameTcPtr);
    		}
	    } else { // Type-BD
		    vcChan.framesAfterVcReceptionTypeBD.pop();
	    	if (vcChan.getSegmentHeaderPresent()) {
	    		mapChan->framesAfterProcessSDLSSecurityTypeBD.push(frameTcPtr);
	    	} else {
	    		vcChan.framesAfterProcessSDLSSecurityTypeBD.push(frameTcPtr);
	    	}
	    }

    	vcChan.channelMutex.unlock();
    	if (vcChan.getSegmentHeaderPresent()) {
    		mapChan->channelMutex.unlock();
    	}
    	return {};
    }

	void SpaceSegmentTcDataHandling::eraseFrame(
		MasterChannelSsTc *mcChan,
		Queue<TransferFrameTC*> *framesAfterSdlsProcessing,
		etl::optional<Defs::SegmentedPacketConstructorTc*> segmentedPacketConstructorTc) {
    	TransferFrameTC *frameTcPtr = framesAfterSdlsProcessing->getFront();

    	if (segmentedPacketConstructorTc.has_value()) {
    		segmentedPacketConstructorTc.value()->previousFrameSeqFlag = frameTcPtr->getSequenceFlag();
    	}

		framesAfterSdlsProcessing->pop();
		Objects::frameOctetPool.deleteBlock(frameTcPtr->getFrameData());
		mcChan->frameMasterCopies.erase(frameTcPtr);
	}

    etl::expected<uint16_t, ServiceChannelNotification>
	SpaceSegmentTcDataHandling::validateNextPacket(
		TransferFrameTC *frameTcPtr,
		uint16_t preDataFieldLength) {
	    uint8_t *packetPtr = frameTcPtr->getFrameData() + preDataFieldLength + frameTcPtr->getNextPacketIndex();
	    etl::optional<Defs::PacketVersionNumber> pvn = getPacketVersionNumber(
		    packetPtr[0]);
	    if (!pvn.has_value()) {
		    return etl::unexpected(ServiceChannelNotification::INVALID_PACKET_PVN);
	    }

	    if (pvn.value() == Defs::PacketVersionNumber::SPACE_PACKET) {
		    return getSpacePacketLength(etl::span{packetPtr, Defs::SpacePacketPrimaryHeaderLength});
	    } else {
		    // ENCAPSULATION_PACKET
		    return getEncapsulationPacketLength(etl::span{packetPtr, Defs::EpTotalOffet + Defs:: MaximumEppPacketLengthFieldSize});
	    }
    }

    bool SpaceSegmentTcDataHandling::validateSequenceFlag(
    	Defs::SegmentedPacketConstructorTc *segmentedPacketConstructor,
        TransferFrameTC *frameTcPtr) {

	    const Defs::SequenceFlag prevSeqFlag = segmentedPacketConstructor->previousFrameSeqFlag;
	    const Defs::SequenceFlag newSeqFlag = frameTcPtr->getSequenceFlag();

	    if (newSeqFlag == Defs::SequenceFlag::NO_SEGMENTATION &&
	        (prevSeqFlag == Defs::SequenceFlag::NO_SEGMENTATION ||
	         prevSeqFlag == Defs::SequenceFlag::SEGMENTATION_END)) {
		    // new unsegmented packet
		    return true;
	    }

	    if (newSeqFlag == Defs::SequenceFlag::SEGMENTATION_START &&
	        (prevSeqFlag == Defs::SequenceFlag::NO_SEGMENTATION ||
	         prevSeqFlag == Defs::SequenceFlag::SEGMENTATION_END)) {
		    // new segmented packet
		    return true;
	    }

	    if (newSeqFlag == Defs::SequenceFlag::SEGMENTATION_MIDDLE ||
	        (prevSeqFlag == Defs::SequenceFlag::SEGMENTATION_START ||
	         prevSeqFlag == Defs::SequenceFlag::SEGMENTATION_MIDDLE)) {
		    // middle of segmented packet
		    return true;
	    }

	    if (newSeqFlag == Defs::SequenceFlag::SEGMENTATION_END &&
	        (prevSeqFlag == Defs::SequenceFlag::SEGMENTATION_MIDDLE ||
	         prevSeqFlag == Defs::SequenceFlag::SEGMENTATION_START)) {
		    // end of segmented packet
		    return true;
	    }

	    // every other scenario is invalid
	    return false;
    }

	SpaceSegmentTcDataHandling::PacketExtractionType SpaceSegmentTcDataHandling::detectExtractionScenario(
		bool segmentationHeaderPresent,
		Defs::SegmentedPacketConstructorTc* segmentedPacketConstructor,
		TransferFrameTC *frameTcPtr,
		bool blockingAllowed,
		bool segmentationAllowed) {

		if (segmentationHeaderPresent && segmentedPacketConstructor->segmentedPacketRejectionMode) {
			if (auto seqFlag = frameTcPtr->getSequenceFlag();
				seqFlag == Defs::SequenceFlag::NO_SEGMENTATION ||
				seqFlag == Defs::SequenceFlag::SEGMENTATION_START) {
				// A frame with NO_SEGMENTATION or SEGMENTATION_START has arrived, exit rejection mode
				// and proceed with processing
				segmentedPacketConstructor->segmentedPacketRejectionMode = false;
				segmentedPacketConstructor->resetPacket();
			} else {
				return PacketExtractionType::REJECTION_MODE;
			}
		}

		if (segmentationHeaderPresent) {
			if (blockingAllowed) {
				if (segmentationAllowed) {
					if (validateSequenceFlag(segmentedPacketConstructor, frameTcPtr)) {
						return frameTcPtr->getSequenceFlag() == Defs::SequenceFlag::NO_SEGMENTATION ?
							PacketExtractionType::SINGLE_OR_BLOCKED_PACKETS : PacketExtractionType::SEGMENTED_PACKET;
					}
					return PacketExtractionType::INVALID_SCENARIO;
				} else {
					if (frameTcPtr->getSequenceFlag() == Defs::SequenceFlag::NO_SEGMENTATION) {
						return PacketExtractionType::SINGLE_OR_BLOCKED_PACKETS;
					}
					return PacketExtractionType::INVALID_SCENARIO;
				}
			} else {
				if (segmentationAllowed) {
					if (validateSequenceFlag(segmentedPacketConstructor, frameTcPtr)) {
						return frameTcPtr->getSequenceFlag() == Defs::SequenceFlag::NO_SEGMENTATION ?
							PacketExtractionType::SINGLE_OR_BLOCKED_PACKETS : PacketExtractionType::SEGMENTED_PACKET;
					}
					return PacketExtractionType::INVALID_SCENARIO;
				} else {
					if (frameTcPtr->getSequenceFlag() == Defs::SequenceFlag::NO_SEGMENTATION) {
						return PacketExtractionType::SINGLE_OR_BLOCKED_PACKETS;
					}
					return PacketExtractionType::INVALID_SCENARIO;
				}
			}
		}

    	return PacketExtractionType::SINGLE_OR_BLOCKED_PACKETS;
    }

	etl::expected<void, ServiceChannelNotification>
	SpaceSegmentTcDataHandling::packetExtraction(
		PhysicalChannel& phyChan,
		etl::variant<etl::reference_wrapper<MAPChannelSs>, etl::reference_wrapper<VirtualChannelSsTc>> channel,
		Defs::ServiceType serviceType,
		uint8_t* packetDestination) {

		MasterChannelSsTc* mcChan;
    	Mutex* channelMutex;
    	Queue<TransferFrameTC*>* framesAfterSdlsProcessing;
    	etl::optional<uint16_t> associatedSdlsSpi;
    	Defs::DataFieldContent dataFieldContent;
    	Defs::SegmentedPacketConstructorTc* segmentedPacketConstructor;
    	bool blockingAllowed = false;
    	bool segmentationAllowed = false;

    	// Gather information about the channel
    	if (etl::holds_alternative<etl::reference_wrapper<MAPChannelSs>>(channel)) {
    		MAPChannelSs& mapChan = etl::get<etl::reference_wrapper<MAPChannelSs>>(channel).get();
    		mcChan = &Objects::masterChannelSsTcMap.at(mapChan.getParentScid());
    		channelMutex = &mapChan.channelMutex;
			associatedSdlsSpi = mapChan.getAssociatedSdlsSPI();
    		dataFieldContent = mapChan.getDataFieldContent();
    		segmentedPacketConstructor = &mapChan.segmentedPacketConstructor;

    		if (dataFieldContent == Defs::DataFieldContent::PACKET) {
    			blockingAllowed = mapChan.getBlocking();
    			segmentationAllowed = mapChan.getSegmentation();
    		}

    		if (serviceType == Defs::ServiceType::TYPE_AD) {
    			framesAfterSdlsProcessing = &mapChan.framesAfterProcessSDLSSecurityTypeAD;
    		} else { // Type-BD
    			framesAfterSdlsProcessing = &mapChan.framesAfterProcessSDLSSecurityTypeBD;
    		}
    	} else {
    		VirtualChannelSsTc& vcChan = etl::get<etl::reference_wrapper<VirtualChannelSsTc>>(channel).get();
    		mcChan = &Objects::masterChannelSsTcMap.at(vcChan.getParentScid());
    		channelMutex = &vcChan.channelMutex;
    		associatedSdlsSpi = vcChan.getAssociatedSdlsSPI();
    		dataFieldContent = vcChan.getDataFieldContent();
    		segmentedPacketConstructor = &vcChan.segmentedPacketConstructor;

    		if (dataFieldContent == Defs::DataFieldContent::PACKET) {
    			blockingAllowed = vcChan.getBlocking();
    		}

    		if (serviceType == Defs::ServiceType::TYPE_AD) {
    			framesAfterSdlsProcessing = &vcChan.framesAfterProcessSDLSSecurityTypeAD;
    		} else { // Type-BD
    			framesAfterSdlsProcessing = &vcChan.framesAfterProcessSDLSSecurityTypeBD;
    		}
    	}

    	// Calculate length parameters
    	uint16_t securityHeaderLength;
    	uint16_t securityTrailerLength;
    	if (associatedSdlsSpi.has_value()) {
    		SecurityAssociation& sa = Objects::saSpaceSegmentMap.at(associatedSdlsSpi.value());
    		securityHeaderLength = sa.getSecurityHeaderLength();
    		securityTrailerLength = sa.getSecurityTrailerLength();
    	} else {
    		securityHeaderLength = 0;
    		securityTrailerLength = 0;
    	}

    	const uint16_t preDataFieldLength = (Defs::TcPrimaryHeaderSize + securityHeaderLength +
			etl::holds_alternative<etl::reference_wrapper<MAPChannelSs>>(channel)) ? Defs::TcSegmentHeaderSize : 0;

    	// Fetch next frame
    	if (!channelMutex->tryLockFor(Defs::MutexDelayMs)) {
    		return etl::unexpected(ServiceChannelNotification::FAILED_TO_LOCK_MUTEX);
    	}

    	if (!mcChan->channelMutex.tryLockFor(Defs::MutexDelayMs)) {
    		channelMutex->unlock();
    		return etl::unexpected(ServiceChannelNotification::FAILED_TO_LOCK_MUTEX);
    	}

    	if (Objects::frameOctetPool.poolMutex.tryLockFor(Defs::MutexDelayMs)) {
    		mcChan->channelMutex.unlock();
    		channelMutex->unlock();
    		return etl::unexpected(ServiceChannelNotification::FAILED_TO_LOCK_MUTEX);
    	}

    	if (framesAfterSdlsProcessing->isEmpty()) {
    		Objects::frameOctetPool.poolMutex.unlock();
    		mcChan->channelMutex.unlock();
    		channelMutex->unlock();
    		return etl::unexpected(ServiceChannelNotification::FRAME_QUEUE_EMPTY);
    	}

    	TransferFrameTC* frameTcPtr = framesAfterSdlsProcessing->getFront();
    	const uint16_t transferFrameDataFieldLength = frameTcPtr->getFrameLength() -
										preDataFieldLength -
										securityTrailerLength -
										phyChan.getFrameErrorControlFieldPresent() *
										Defs::ErrorControlFieldSize;

		auto unlockMutexes = [&mcChan, &channelMutex]() {
			Objects::frameOctetPool.poolMutex.unlock();
			mcChan->channelMutex.unlock();
			channelMutex->unlock();
		};

    	// Packet/Vca sdu processing
    	if (dataFieldContent == Defs::DataFieldContent::VCA_SDU) {
    		// Simplest case, just copy the sdu to the user provided buffer and delete the frame
    		memcpy(packetDestination, frameTcPtr->getFrameData() + preDataFieldLength, transferFrameDataFieldLength);
    		eraseFrame(mcChan, framesAfterSdlsProcessing, etl::nullopt);
    		unlockMutexes();
    		return {};
    	}

		// PACKET (Space packet or Encapsulation packet)
		switch (PacketExtractionType packetExtractionType = detectExtractionScenario(
		etl::holds_alternative<etl::reference_wrapper<MAPChannelSs>>(channel),
			segmentedPacketConstructor,
			frameTcPtr,
			blockingAllowed,
			segmentationAllowed)) {
			case PacketExtractionType::REJECTION_MODE:
				eraseFrame(mcChan, framesAfterSdlsProcessing, segmentedPacketConstructor);
				unlockMutexes();
				return  etl::unexpected(ServiceChannelNotification::CURRENTLY_REJECTING_ERRONEOUS_SEGMENTED_PACKET);
			case PacketExtractionType::SEGMENTED_PACKET:
				if (!segmentedPacketConstructor->addNextSegementedPacketPiece(
					// packet exceeded maximum buffer size
					etl::span{frameTcPtr->getFrameData() + preDataFieldLength, transferFrameDataFieldLength})) {
					eraseFrame(mcChan, framesAfterSdlsProcessing, segmentedPacketConstructor);
					segmentedPacketConstructor->resetPacket();
					segmentedPacketConstructor->segmentedPacketRejectionMode = true;
					unlockMutexes();
					return etl::unexpected(ServiceChannelNotification::PACKET_TOO_LONG);
				} else {
					const Defs::SequenceFlag seqFlag = frameTcPtr->getSequenceFlag();
					eraseFrame(mcChan, framesAfterSdlsProcessing, segmentedPacketConstructor);

					if (seqFlag == Defs::SequenceFlag::SEGMENTATION_END) {
						// finished with that packet, deliver to user
						segmentedPacketConstructor->extractPacket(packetDestination);
						unlockMutexes();
						return {};
					}
					// a full packet is not ready yet
					unlockMutexes();
					return etl::unexpected(ServiceChannelNotification::CURRENTLY_CONSTRUCTING_SEGMENTED_PACKET);
				}
			case PacketExtractionType::SINGLE_OR_BLOCKED_PACKETS: {
				const auto status = validateNextPacket(frameTcPtr, preDataFieldLength);

				// check if vpn is valid
				if (!status.has_value()) {
					eraseFrame(mcChan, framesAfterSdlsProcessing, segmentedPacketConstructor);
					unlockMutexes();
					return etl::unexpected(ServiceChannelNotification::INVALID_PACKET_PVN);
				}

				// length checks
				const uint16_t packetLength = status.value();
				if (packetLength > transferFrameDataFieldLength ||
					(!blockingAllowed && packetLength != transferFrameDataFieldLength)) {
					eraseFrame(mcChan, framesAfterSdlsProcessing, segmentedPacketConstructor);
					unlockMutexes();
					return etl::unexpected(ServiceChannelNotification::ERRONEOUS_PACKET);
					}

				memcpy(packetDestination,
					frameTcPtr->getFrameData() + preDataFieldLength + frameTcPtr->getNextPacketIndex(),
					packetLength);
				frameTcPtr->incrementNextPacketIndex(packetLength);

				if (frameTcPtr->getNextPacketIndex() == transferFrameDataFieldLength) {
					// finished extracting packets from that frame
					eraseFrame(mcChan, framesAfterSdlsProcessing, segmentedPacketConstructor);
				}

				unlockMutexes();
				return {};
			}
			case PacketExtractionType::INVALID_SCENARIO:
				eraseFrame(mcChan, framesAfterSdlsProcessing, segmentedPacketConstructor);
				if (etl::holds_alternative<etl::reference_wrapper<MAPChannelSs>>(channel)) {
					segmentedPacketConstructor->segmentedPacketRejectionMode = true;
				}
				unlockMutexes();
				return etl::unexpected(ServiceChannelNotification::ERRONEOUS_PACKET);
		}
	}

} // CCSDSDataLinkLayer
#endif // INCLUDE_SPACE_SEGMENT_CODE