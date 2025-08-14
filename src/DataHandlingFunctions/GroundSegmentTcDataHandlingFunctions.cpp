#include "GroundSegmentTcDataHandlingFunctions.hpp"
#include "AddressingAndParsingUtilities.hpp"
#include "SecurityAssociation.hpp"

namespace CCSDSDataLinkLayer {
#ifdef INCLUDE_GROUND_SEGMENT_CODE

	void GroundSegmentTcDataHandling::resetMapChannel(MAPChannelGs& mapChan) {
		mapChan.packetLengthsTypeAD.reset();
		mapChan.packetOctetsTypeAD.reset();
		mapChan.packetLengthsTypeBD.reset();
		mapChan.packetOctetsTypeBD.reset();
	}

	void GroundSegmentTcDataHandling::resetVirtualChannel(VirtualChannelGsTc& vcChan) {
		vcChan.packetLengthsTypeAD.reset();
		vcChan.packetOctetsTypeAD.reset();
		vcChan.packetLengthsTypeBD.reset();
		vcChan.packetOctetsTypeBD.reset();

		MasterChannelGsTc& mcChan = Objects::masterChannelGsTcMap.at(vcChan.getParentScid());

		TransferFrameTC* frameTcPtr;
		while (!vcChan.framesAfterPacketProcessing.isEmpty()) {
			frameTcPtr = vcChan.framesAfterPacketProcessing.getFront();
			vcChan.framesAfterPacketProcessing.pop();
			Objects::frameOctetPool.deleteBlock(frameTcPtr->getFrameData(), frameTcPtr->getFrameLength());
			mcChan.frameMasterCopies.erase(frameTcPtr);
		}

		while (!vcChan.framesAfterApplySDLSSecurity.isEmpty()) {
			frameTcPtr = vcChan.framesAfterApplySDLSSecurity.getFront();
			vcChan.framesAfterApplySDLSSecurity.popFront();
			Objects::frameOctetPool.deleteBlock(frameTcPtr->getFrameData(), frameTcPtr->getFrameLength());
			mcChan.frameMasterCopies.erase(frameTcPtr);
		}

		if (vcChan.getCopInEffect()) {
			FrameOperationProcedure& fop = Objects::fopMap.at(constructVcidScidKey(vcChan.getVcid(), vcChan.getParentScid()));
			fop.resetFOP();
		}
	}

	void GroundSegmentTcDataHandling::resetMasterChannel(MasterChannelGsTc& mcChan) {
		TransferFrameTC* frameTcPtr;
		while (!mcChan.framesAfterVcGeneration.isEmpty()) {
			frameTcPtr = mcChan.framesAfterVcGeneration.getFront();
			mcChan.framesAfterVcGeneration.pop();
			Objects::frameOctetPool.deleteBlock(frameTcPtr->getFrameData(), frameTcPtr->getFrameLength());
			mcChan.frameMasterCopies.erase(frameTcPtr);
		}
	}

    etl::expected<void, ServiceChannelNotification> GroundSegmentTcDataHandling::storePacket(
    	const PhysicalChannel& phyChan,
        etl::variant<etl::reference_wrapper<VirtualChannelGsTc>, etl::reference_wrapper<MAPChannelGs>> chanVariant,
        etl::span<uint8_t> packetSource,
        const Defs::ServiceType serviceType) {
        etl::optional<Defs::PacketVersionNumber> pvn = getPacketVersionNumber(packetSource[0]);

        // ensure the incoming packet is allowed by ccsds
        if (!pvn.has_value()) {
            return etl::unexpected(ServiceChannelNotification::INVALID_PACKET_PVN);
        }

        // get packet length from relevant field and length checks
        uint16_t length;
        if (pvn.value() == Defs::PacketVersionNumber::SPACE_PACKET) {
            if (packetSource.size() < Defs::SpacePacketPrimaryHeaderLength + 1) {
                return etl::unexpected(ServiceChannelNotification::INVALID_LENGTH);
            }

            length = getSpacePacketLength(packetSource);
        } else if (pvn.value() == Defs::PacketVersionNumber::ENCAPSULATION_PACKET) {
        	if (packetSource.size() < Defs::EpTotalOffet) {
        		return etl::unexpected(ServiceChannelNotification::INVALID_LENGTH);
        	}

        	length = getEncapsulationPacketLength(packetSource);
        }

    	if (packetSource.size() < length) {
    		return etl::unexpected(ServiceChannelNotification::INVALID_LENGTH);
    	}

    	if (length > Defs::MaxExpectedEncapsulationPacketSize) {
    		return etl::unexpected(ServiceChannelNotification::PACKET_TOO_LONG);
    	}

        Queue<uint8_t>* packetOctets;
        Queue<uint16_t>* packetLengths;
        Mutex* channelMutex;
    	bool segmentHeaderPresent;
    	bool segmentationAllowed;
    	uint16_t securityHeaderLength = 0;
    	uint16_t securityTrailerLength = 0;

        if (chanVariant.is_type<etl::reference_wrapper<VirtualChannelGsTc>>()) {
            VirtualChannelGsTc& vcChan = etl::get<etl::reference_wrapper<VirtualChannelGsTc>>(chanVariant).get();
            channelMutex = &vcChan.channelMutex;
        	segmentHeaderPresent = false; // frames can only be inserted directly to the virtual channel if no
        	                              // map channels exist (meaning the segment header is not present)
        	segmentationAllowed = false;
            if (serviceType == Defs::ServiceType::TYPE_AD) {
                packetOctets = &vcChan.packetOctetsTypeAD;
                packetLengths = &vcChan.packetLengthsTypeAD;
            } else if (serviceType == Defs::ServiceType::TYPE_BD) {
                packetOctets = &vcChan.packetOctetsTypeBD;
                packetLengths = &vcChan.packetLengthsTypeBD;
            } else {
				return etl::unexpected(ServiceChannelNotification::INVALID_FRAME_SERVICE_TYPE);
            }
        } else {
            MAPChannelGs& mapChan = etl::get<etl::reference_wrapper<MAPChannelGs>>(chanVariant).get();
            channelMutex = &mapChan.channelMutex;
        	segmentHeaderPresent = true;
        	segmentationAllowed = mapChan.getSegmentation();

            if (serviceType == Defs::ServiceType::TYPE_AD) {
                packetOctets = &mapChan.packetOctetsTypeAD;
                packetLengths = &mapChan.packetLengthsTypeAD;
            } else if (serviceType == Defs::ServiceType::TYPE_BD) {
                packetOctets = &mapChan.packetOctetsTypeBD;
                packetLengths = &mapChan.packetLengthsTypeBD;
            } else {
            	return etl::unexpected(ServiceChannelNotification::INVALID_FRAME_SERVICE_TYPE);
            }

        	if (mapChan.getAssociatedSdlsSPI().has_value()) {
        		SecurityAssociation& sa = Objects::saGroundSegmentMap.at(mapChan.getAssociatedSdlsSPI().value());
        		securityHeaderLength = sa.getSecurityHeaderLength();
        		securityTrailerLength = sa.getSecurityTrailerLength();
        	}
        }

    	const uint16_t maxTransferFrameDataFieldLength = phyChan.getMaxTcFrameLength() -
												 Defs::TcPrimaryHeaderSize -
												 Defs::TcSegmentHeaderSize * segmentHeaderPresent -
												 securityHeaderLength -
												 securityTrailerLength -
												 Defs::ErrorControlFieldSize * phyChan.getFrameErrorControlFieldPresent();


		if (!segmentationAllowed && (length > maxTransferFrameDataFieldLength)) {
			return etl::unexpected(ServiceChannelNotification::INVALID_LENGTH);
		}

        if (!channelMutex->tryLockFor(Defs::MutexDelayMs)) {
            return etl::unexpected(ServiceChannelNotification::FAILED_TO_LOCK_MUTEX);
        }

        if (packetLengths->isFull() || packetOctets->remainingCapacity() < packetSource.size()) {
            channelMutex->unlock();
            return etl::unexpected(ServiceChannelNotification::PACKET_QUEUE_FULL);
        }

        packetLengths->push(length);
        for (size_t i = 0; i < length; i++) {
            packetOctets->push(packetSource[i]);
        }

        channelMutex->unlock();
        return {};
    }

    etl::expected<void, ServiceChannelNotification> GroundSegmentTcDataHandling::storeVcaSdu(
    const PhysicalChannel& phyChan,
    etl::variant<etl::reference_wrapper<VirtualChannelGsTc>, etl::reference_wrapper<MAPChannelGs>> chanVariant,
    etl::span<uint8_t> vcaSduSource,
    const Defs::ServiceType serviceType) {

        Queue<uint8_t>* packetOctets;
        Queue<uint16_t>* packetLengths;
        Mutex* channelMutex;
        bool segmentHeaderPresent;
        uint16_t securityHeaderLength = 0;
        uint16_t securityTrailerLength = 0;

        if (chanVariant.is_type<etl::reference_wrapper<VirtualChannelGsTc>>()) {
            VirtualChannelGsTc& vcChan = etl::get<etl::reference_wrapper<VirtualChannelGsTc>>(chanVariant).get();
            channelMutex = &vcChan.channelMutex;
            segmentHeaderPresent = false; // frames can only be inserted directly to the virtual channel if no
                                          // map channels exist (meaning the segment header is not present)
            if (serviceType == Defs::ServiceType::TYPE_AD) {
                packetOctets = &vcChan.packetOctetsTypeAD;
                packetLengths = &vcChan.packetLengthsTypeAD;
            } else if (serviceType == Defs::ServiceType::TYPE_BD) {
                packetOctets = &vcChan.packetOctetsTypeBD;
                packetLengths = &vcChan.packetLengthsTypeBD;
            } else {
	            return etl::unexpected(ServiceChannelNotification::INVALID_FRAME_SERVICE_TYPE);
            }

            if (vcChan.getAssociatedSdlsSPI().has_value()) {
                SecurityAssociation& sa = Objects::saGroundSegmentMap.at(vcChan.getAssociatedSdlsSPI().value());
                securityHeaderLength = sa.getSecurityHeaderLength();
                securityTrailerLength = sa.getSecurityTrailerLength();
            }
        } else {
            MAPChannelGs& mapChan = etl::get<etl::reference_wrapper<MAPChannelGs>>(chanVariant).get();
            channelMutex = &mapChan.channelMutex;
            segmentHeaderPresent = true;

            if (serviceType == Defs::ServiceType::TYPE_AD) {
                packetOctets = &mapChan.packetOctetsTypeAD;
                packetLengths = &mapChan.packetLengthsTypeAD;
            } else if (serviceType == Defs::ServiceType::TYPE_BD) {
                packetOctets = &mapChan.packetOctetsTypeBD;
                packetLengths = &mapChan.packetLengthsTypeBD;
            } else {
            	return etl::unexpected(ServiceChannelNotification::INVALID_FRAME_SERVICE_TYPE);
            }

            if (mapChan.getAssociatedSdlsSPI().has_value()) {
                SecurityAssociation& sa = Objects::saGroundSegmentMap.at(mapChan.getAssociatedSdlsSPI().value());
                securityHeaderLength = sa.getSecurityHeaderLength();
                securityTrailerLength = sa.getSecurityTrailerLength();
            }
        }

        if (!channelMutex->tryLockFor(Defs::MutexDelayMs)) {
            return etl::unexpected(ServiceChannelNotification::FAILED_TO_LOCK_MUTEX);
        }

        if (packetLengths->isFull() || packetOctets->remainingCapacity() < vcaSduSource.size()) {
            channelMutex->unlock();
            return etl::unexpected(ServiceChannelNotification::PACKET_QUEUE_FULL);
        }

        const uint16_t maxTransferFrameDataFieldLength = phyChan.getMaxTcFrameLength() -
                                                 Defs::TcPrimaryHeaderSize -
                                                 Defs::TcSegmentHeaderSize * segmentHeaderPresent -
                                                 securityHeaderLength -
                                                 securityTrailerLength -
                                                 Defs::ErrorControlFieldSize * phyChan.getFrameErrorControlFieldPresent();

        if (vcaSduSource.size() > maxTransferFrameDataFieldLength) {
            channelMutex->unlock();
            return etl::unexpected(ServiceChannelNotification::INVALID_LENGTH);
        }

        packetLengths->push(vcaSduSource.size());
        for (size_t i = 0; i < vcaSduSource.size(); i++) {
            packetOctets->push(vcaSduSource[i]);
        }

        channelMutex->unlock();
        return {};
    }

	etl::expected<void, ServiceChannelNotification> GroundSegmentTcDataHandling::packetProcessing(
				const PhysicalChannel& phyChan,
				MasterChannelGsTc& mcChan,
				etl::variant<etl::reference_wrapper<VirtualChannelGsTc>, etl::reference_wrapper<MAPChannelGs>> chanVariant,
				const Defs::ServiceType serviceType) {

	    Queue<uint8_t>* packetOctets;
    	Queue<uint16_t>* packetLengths;
    	Queue<TransferFrameTC*>* framesAfterPacketProcessing;
    	Mutex* channelMutex;
    	bool segmentHeaderPresent;
    	bool blockingAllowed;
    	bool segmentationAllowed;
    	uint16_t securityHeaderLength = 0;
    	uint16_t securityTrailerLength = 0;
    	Defs::DataFieldContent dataFieldContent;
    	Defs::Vcid vcid;
    	Defs::Mapid mapid = 0;

    	// fetch all necessary parameters
    	if (chanVariant.is_type<etl::reference_wrapper<VirtualChannelGsTc>>()) {
    		VirtualChannelGsTc& vcChan = etl::get<etl::reference_wrapper<VirtualChannelGsTc>>(chanVariant).get();
    		channelMutex = &vcChan.channelMutex;
    		segmentHeaderPresent = false; // frames can only be inserted directly to the virtual channel if no
    							// map channels exist (meaning the segment header is not present)
    		blockingAllowed = vcChan.getBlocking();
    		segmentationAllowed = false;
    		framesAfterPacketProcessing = &vcChan.framesAfterPacketProcessing;
    		dataFieldContent = vcChan.getDataFieldContent();
			vcid = vcChan.getVcid();

    		if (serviceType == Defs::ServiceType::TYPE_AD) {
    			packetOctets = &vcChan.packetOctetsTypeAD;
    			packetLengths = &vcChan.packetLengthsTypeAD;
    		} else { // TYPE_BD
    			packetOctets = &vcChan.packetOctetsTypeBD;
    			packetLengths = &vcChan.packetLengthsTypeBD;
    		}

    		if (vcChan.getAssociatedSdlsSPI().has_value()) {
    			SecurityAssociation& sa = Objects::saGroundSegmentMap.at(vcChan.getAssociatedSdlsSPI().value());
    			securityHeaderLength = sa.getSecurityHeaderLength();
    			securityTrailerLength = sa.getSecurityTrailerLength();
    		}
    	} else {
    		MAPChannelGs& mapChan = etl::get<etl::reference_wrapper<MAPChannelGs>>(chanVariant).get();
    		channelMutex = &mapChan.channelMutex;
    		segmentHeaderPresent = true;
    		framesAfterPacketProcessing =
				&Objects::virtualChannelGsTcMap.at(constructVcidScidKey(mapChan.getParentVcid(), mcChan.getScid())).framesAfterPacketProcessing;
    		blockingAllowed = mapChan.getBlocking();
    		segmentationAllowed = mapChan.getSegmentation();
    		dataFieldContent = mapChan.getDataFieldContent();
			vcid = mapChan.getParentVcid();
			mapid = mapChan.getMapid();

    		if (serviceType == Defs::ServiceType::TYPE_AD) {
    			packetOctets = &mapChan.packetOctetsTypeAD;
    			packetLengths = &mapChan.packetLengthsTypeAD;
    		} else { // TYPE_BD
    			packetOctets = &mapChan.packetOctetsTypeBD;
    			packetLengths = &mapChan.packetLengthsTypeBD;
    		}

    		if (mapChan.getAssociatedSdlsSPI().has_value()) {
    			SecurityAssociation& sa = Objects::saGroundSegmentMap.at(mapChan.getAssociatedSdlsSPI().value());
    			securityHeaderLength = sa.getSecurityHeaderLength();
    			securityTrailerLength = sa.getSecurityTrailerLength();
    		}
    	}

    	if (!channelMutex->tryLockFor(Defs::MutexDelayMs)) {
    		return etl::unexpected(ServiceChannelNotification::FAILED_TO_LOCK_MUTEX);
    	}

    	if (!mcChan.channelMutex.tryLockFor(Defs::MutexDelayMs)) {
    		channelMutex->unlock();
    		return etl::unexpected(ServiceChannelNotification::FAILED_TO_LOCK_MUTEX);
    	}

    	if (!Objects::frameOctetPool.poolMutex.tryLockFor(Defs::MutexDelayMs)) {
    		mcChan.channelMutex.unlock();
    		channelMutex->unlock();
    		return etl::unexpected(ServiceChannelNotification::FAILED_TO_LOCK_MUTEX);
    	}

    	const uint16_t maxTransferFrameDataFieldLength = phyChan.getMaxTcFrameLength() -
												 Defs::TcPrimaryHeaderSize -
												 Defs::TcSegmentHeaderSize * segmentHeaderPresent -
												 securityHeaderLength -
												 securityTrailerLength -
												 Defs::ErrorControlFieldSize * phyChan.getFrameErrorControlFieldPresent();

	    while (!packetLengths->isEmpty()) {
			// fetch new packet
			const uint16_t packetLength = packetLengths->getFront();

			// check if previous frame can fit this packet (if blocking is allowed)
	    	if (!framesAfterPacketProcessing->isEmpty()) {
	    		TransferFrameTC* prevFrame = framesAfterPacketProcessing->getBack();
	    		if (dataFieldContent == Defs::DataFieldContent::PACKET &&
					blockingAllowed &&
					prevFrame->getSequenceFlag() == Defs::SequenceFlag::NO_SEGMENTATION &&
					packetLength < maxTransferFrameDataFieldLength - prevFrame->getFirstDataFieldEmptyOctet() + 1
					) {

	    			for (uint16_t i = 0; i < packetLength; i++) {
	    				const uint16_t index = Defs::TcPrimaryHeaderSize +
							segmentHeaderPresent * Defs::TcSegmentHeaderSize +
							securityHeaderLength +
							prevFrame->getFirstDataFieldEmptyOctet() + i;
	    				prevFrame->getFrameData()[index] = packetOctets->getFront();
	    				packetOctets->pop();
	    			}
	    			packetLengths->pop();
	    			prevFrame->setFirstDataFieldEmptyOctet(prevFrame->getFirstDataFieldEmptyOctet() + packetLength);
	    			prevFrame->setFrameLength(prevFrame->getFrameLength() + packetLength);
	    			continue; // finished with that packet, move to the next one
				}
	    	}

			// new frame creation
	    	if (packetLength > maxTransferFrameDataFieldLength &&
	    		segmentationAllowed &&
	    		dataFieldContent == Defs::DataFieldContent::PACKET) {
	    		// segmentation scenario -> calculate amount of new frames needed
	    		const uint16_t amountNewFrames = (packetLength / maxTransferFrameDataFieldLength) +
	    			                       (packetLength % maxTransferFrameDataFieldLength ? 1 : 0);
	    		const uint16_t amountAllocatedOctetsLastFrame = Defs::TcPrimaryHeaderSize +
	    			Defs::TcSegmentHeaderSize * segmentHeaderPresent +
	    			securityHeaderLength +
	    			(packetLength - (amountNewFrames - 1) * maxTransferFrameDataFieldLength) +
	    			securityTrailerLength +
	    			Defs::ErrorControlFieldSize * phyChan.getFrameErrorControlFieldPresent();

	    		const uint16_t amountAllocatedOctetsTotal = (amountNewFrames - 1) * phyChan.getMaxTcFrameLength() +
															amountAllocatedOctetsLastFrame;

	    		// ensure the necessary space exists
	    		if (framesAfterPacketProcessing->remainingCapacity() < amountNewFrames) {
					Objects::frameOctetPool.poolMutex.unlock();
	    			mcChan.channelMutex.unlock();
				    channelMutex->unlock();
				    return etl::unexpected(ServiceChannelNotification::FRAME_QUEUE_FULL);
	    		}

	    		if (mcChan.frameMasterCopies.remainingCapacity() < amountNewFrames ||
					Objects::frameOctetPool.findFit(amountAllocatedOctetsTotal).second != MasterChannelAlert::NO_MC_ALERT) {
	    			Objects::frameOctetPool.poolMutex.unlock();
	    			mcChan.channelMutex.unlock();
				    channelMutex->unlock();
				    return etl::unexpected(ServiceChannelNotification::NOT_ENOUGH_SPACE_IN_MASTER_COPY_OR_MEMORY_POOL);
	    		}

			    uint16_t remainingPacketOctets = packetLength;
			    for (uint16_t i = 0; i < amountNewFrames; i++) {
				    // determine sequence flag value
				    Defs::SequenceFlag seqFlag = Defs::SequenceFlag::SEGMENTATION_MIDDLE;
				    if (i == 0) {
					    seqFlag = Defs::SequenceFlag::SEGMENTATION_START;
				    } else if (i == amountNewFrames - 1) {
					    seqFlag = Defs::SequenceFlag::SEGMENTATION_END;
				    }

			    	// create frame object and allocate memory
			    	uint8_t* frameData = Objects::frameOctetPool.allocateBlock(
			    		(i < amountNewFrames - 1) ? phyChan.getMaxTcFrameLength() : amountAllocatedOctetsLastFrame, nullptr);
					for (uint16_t j = 0; j < (i < (amountNewFrames - 1) ? maxTransferFrameDataFieldLength : remainingPacketOctets); j++) {
						const uint16_t index = Defs::TcPrimaryHeaderSize +
						segmentHeaderPresent * Defs::TcSegmentHeaderSize +
						securityHeaderLength + j;
						frameData[index] = packetOctets->getFront();
						packetOctets->pop();
					}

			    	TransferFrameTC* frameTcPtr = mcChan.frameMasterCopies.push(
			    		TransferFrameTC(
								frameData,
								serviceType,
								vcid,
								mcChan.getScid(),
								(i < amountNewFrames - 1) ? phyChan.getMaxTcFrameLength() : amountAllocatedOctetsLastFrame,
								segmentHeaderPresent,
								seqFlag,
								mapid,
								(i < amountNewFrames - 1) ? maxTransferFrameDataFieldLength : remainingPacketOctets)
			    		);

			    	framesAfterPacketProcessing->push(frameTcPtr);
			    	remainingPacketOctets -= maxTransferFrameDataFieldLength; // underflow in last iteration, but we don't care
			    }
	    		packetLengths->pop();

	    	} else if (packetLength <= maxTransferFrameDataFieldLength) {
	    		// ensure the necessary space exists
	    		if (framesAfterPacketProcessing->remainingCapacity() == 0) {
	    			Objects::frameOctetPool.poolMutex.unlock();
	    			mcChan.channelMutex.unlock();
				    channelMutex->unlock();
				    return etl::unexpected(ServiceChannelNotification::FRAME_QUEUE_FULL);
	    		}

	    		// ensure the necessary space exists
	    		// if blocking is allowed, we need to allocate octets for a max length frame, since we might to insert
	    		// more packets
				uint16_t frameOctetsToAllocate = (blockingAllowed && (dataFieldContent == Defs::DataFieldContent::PACKET)) ?
					phyChan.getMaxTcFrameLength() :
	    			Defs::TcPrimaryHeaderSize +
					Defs::TcSegmentHeaderSize * segmentHeaderPresent +
					securityHeaderLength +
					packetLength +
					securityTrailerLength +
					Defs::ErrorControlFieldSize * phyChan.getFrameErrorControlFieldPresent();

	    		if (mcChan.frameMasterCopies.remainingCapacity() == 0 ||
					Objects::frameOctetPool.findFit(frameOctetsToAllocate).second != MasterChannelAlert::NO_MC_ALERT) {
	    			Objects::frameOctetPool.poolMutex.unlock();
	    			mcChan.channelMutex.unlock();
				    channelMutex->unlock();
				    return etl::unexpected(ServiceChannelNotification::NOT_ENOUGH_SPACE_IN_MASTER_COPY_OR_MEMORY_POOL);
				}

	    		// create frame object and allocate memory
	    		uint8_t* frameData = Objects::frameOctetPool.allocateBlock(frameOctetsToAllocate, nullptr);
	    		for (uint16_t j = 0; j < packetLength; j++) {
	    			const uint16_t index = Defs::TcPrimaryHeaderSize +
					segmentHeaderPresent * Defs::TcSegmentHeaderSize +
					securityHeaderLength + j;
	    			frameData[index] = packetOctets->getFront();
	    			packetOctets->pop();
	    		}
	    		packetLengths->pop();

	    		TransferFrameTC* frameTcPtr = mcChan.frameMasterCopies.push(
					TransferFrameTC(
							frameData,
							serviceType,
							vcid,
							mcChan.getScid(),
							frameOctetsToAllocate,
							segmentHeaderPresent,
							Defs::SequenceFlag::NO_SEGMENTATION,
							mapid,
							packetLength)
					);

	    		framesAfterPacketProcessing->push(frameTcPtr);
	    	} else {
	    		// invalid scenario (if storePacket, storeVcaSdu works as intended, it should not occur)
	    		Objects::frameOctetPool.poolMutex.unlock();
	    		mcChan.channelMutex.unlock();
			    channelMutex->unlock();
			    return etl::unexpected(ServiceChannelNotification::INVALID_LENGTH);
	    	}

		}

    	Objects::frameOctetPool.poolMutex.unlock();
	    mcChan.channelMutex.unlock();
	    channelMutex->unlock();
	    return {};
    }

	etl::expected<void, ServiceChannelNotification> GroundSegmentTcDataHandling::applySDLSSecurity(
		const PhysicalChannel& phyChan,
		VirtualChannelGsTc& vcChan) {

		if (!vcChan.channelMutex.tryLockFor(Defs::MutexDelayMs)) {
			return etl::unexpected(ServiceChannelNotification::FAILED_TO_LOCK_MUTEX);
		}

		if (vcChan.framesAfterPacketProcessing.isEmpty()) {
			vcChan.channelMutex.unlock();
			return etl::unexpected(ServiceChannelNotification::FRAME_QUEUE_EMPTY);
		}

    	if (vcChan.framesAfterApplySDLSSecurity.isFull()) {
    		vcChan.channelMutex.unlock();
    		return etl::unexpected(ServiceChannelNotification::FRAME_QUEUE_FULL);
    	}

	    TransferFrameTC* frameTcPtr = vcChan.framesAfterPacketProcessing.getFront();

    	etl::optional<uint16_t> associatedSdlsSpi;
    	if (vcChan.getSegmentHeaderPresent()) {
    		// check for association with the relevant map channel
    		const Defs::Mapid mapid = frameTcPtr->getMapId().value();
    		const Defs::MapidVcidScidKey key = constructMapidVcidScidKey(mapid, vcChan.getVcid(), vcChan.getParentScid());
    		MAPChannelGs& mapChan = Objects::mapChannelGsMap.at(key);

    		if (mapChan.getAssociatedSdlsSPI().has_value()) {
    			associatedSdlsSpi = mapChan.getAssociatedSdlsSPI().value();
    		}
    	} else {
    		// check for association with the virtual channel
    		if (vcChan.getAssociatedSdlsSPI().has_value()) {
    			associatedSdlsSpi = vcChan.getAssociatedSdlsSPI().value();
    		}
    	}

    	if (associatedSdlsSpi.has_value()) {
    		// security processing is required for this frame
    		SecurityAssociation& sa = Objects::saGroundSegmentMap.at(associatedSdlsSpi.value());

		    const uint16_t transferFrameDataFieldLength = frameTcPtr->getFrameLength() -
		                                            Defs::TcPrimaryHeaderSize -
		                                            sa.getSecurityHeaderLength() -
		                                            sa.getSecurityTrailerLength() -
		                                            phyChan.getFrameErrorControlFieldPresent() *
		                                            Defs::ErrorControlFieldSize;
    		if (const auto status = sa.processSecurityTC(frameTcPtr, transferFrameDataFieldLength);
    			!status.has_value()) {
    			// because of the previous checks, MAC_CALCULATION_ERROR is the only possible error
    			vcChan.channelMutex.unlock();
    			return etl::unexpected(ServiceChannelNotification::SLDS_CALCULATION_ERROR);
    		}
    	}

    	// push to next stage
    	vcChan.framesAfterPacketProcessing.pop();
    	vcChan.framesAfterApplySDLSSecurity.pushBack(frameTcPtr);

    	vcChan.channelMutex.unlock();
    	return {};
    }


	etl::expected<void, ServiceChannelNotification> GroundSegmentTcDataHandling::virtualChannelGeneration(
		MasterChannelGsTc& mcChan,
		VirtualChannelGsTc& vcChan) {

    	const Defs::VcidScidKey key = constructVcidScidKey(vcChan.getVcid(), mcChan.getScid());
    	FrameOperationProcedure& fop = Objects::fopMap.at(key);

    	bool encounteredFopSignal = false; // let the user know if no fop signals were encountered in this function call
    	                                   // (or fop is not present in this channel)
    	if (vcChan.getCopInEffect()) {
    		if (!fop.signalQueueMutex.tryLockFor(Defs::MutexDelayMs)) {
    			return etl::unexpected(ServiceChannelNotification::FAILED_TO_LOCK_MUTEX);
    		}

    		/** Handle a transfer notification signal
			 * - ACCEPT_RESPONSE_TO_TRANSFER_FDU: AD/BD Frame accepted by FOP. No action to be taken.
			 * - REJECT_RESPONSE_TO_TRANSFER_FDU: AD/BD Frame was not accepted by FOP. Push to the front of the higher
			 *                                    layer queue.
			 * - POSITIVE_CONFIRM_TO_TRANSFER_FDU: AD Frame was received by FARM. Delete it's master copy.
			 * - NEGATIVE_CONFIRM_TO_TRANSFER_FDU: AD Frame was not received by FARM, or an error has occurred (usually
			 *   accompanied by an alert signal as well). Delete master copy.
			 */
    		if (!fop.transferNotificationSignalQueue.empty()) {
    			encounteredFopSignal = true;
    			TransferNotificationSignal& signal = fop.transferNotificationSignalQueue.front();
    			switch (signal.transferNotificationType) {
    				case TransferNotificationType::ACCEPT_RESPONSE_TO_TRANSFER_FDU:
    					break;
    				case TransferNotificationType::REJECT_RESPONSE_TO_TRANSFER_FDU:
    					if (!vcChan.channelMutex.tryLockFor(Defs::MutexDelayMs)) {
    						fop.signalQueueMutex.unlock();
    						return etl::unexpected(ServiceChannelNotification::FAILED_TO_LOCK_MUTEX);
    					}

    				if (!vcChan.framesAfterApplySDLSSecurity.isFull()) {
    					vcChan.framesAfterApplySDLSSecurity.pushFront(signal.frame.value());
    					fop.transferNotificationSignalQueue.pop();
    				}

    				vcChan.channelMutex.unlock();
    				break;
    				case TransferNotificationType::POSITIVE_CONFIRM_RESPONSE_TO_TRANSFER_FDU:
    					[[fallthrough]]
					case TransferNotificationType::NEGATIVE_CONFIRM_RESPONSE_TO_TRANSFER_FDU:
						if (!mcChan.channelMutex.tryLockFor(Defs::MutexDelayMs)) {
							fop.signalQueueMutex.unlock();
							return etl::unexpected(ServiceChannelNotification::FAILED_TO_LOCK_MUTEX);
						}

    					if (!Objects::frameOctetPool.poolMutex.tryLockFor(Defs::MutexDelayMs)) {
    						mcChan.channelMutex.unlock();
    						fop.signalQueueMutex.unlock();
    						return etl::unexpected(ServiceChannelNotification::FAILED_TO_LOCK_MUTEX);
    					}

    				Objects::frameOctetPool.deleteBlock(signal.frame.value()->getFrameData(), signal.frame.value()->getFrameLength());
    				mcChan.frameMasterCopies.erase(signal.frame.value());

    				fop.transferNotificationSignalQueue.pop();
    				Objects::frameOctetPool.poolMutex.unlock();
    				mcChan.channelMutex.unlock();
    				break;
    			}
    		}

    		/** Handle a lower layer request signal and respond with a lower layer response
			 * - LOW_LAYER_TRANSMIT: AD/BD/BC request for transmitting frame. If there is space, pass the pointer to the lower layer buffer
			 *   and send *_ACCEPT to FOP. Otherwise, send *_REJECT.
			 * - LOW_LAYER_ABORT: FOP asks to stop ongoing type AD/BC transmission. Delete pointers from lower layer buffer.
			 */
    		if (!fop.fopToLowerLayerRequestSignalQueue.empty()) {
    			encounteredFopSignal = true;
    			FopToLowerLayerRequestSignal& signal = fop.fopToLowerLayerRequestSignalQueue.front();
    			switch (signal.lowerLayerRequestType) {
    				case LowerLayerRequestType::LOW_LAYER_ABORT:
    					if (!mcChan.channelMutex.tryLockFor(Defs::MutexDelayMs)) {
    						fop.signalQueueMutex.unlock();
    						return etl::unexpected(ServiceChannelNotification::FAILED_TO_LOCK_MUTEX);
    					}

    				mcChan.framesAfterVcGeneration.reset();
    				fop.fopToLowerLayerRequestSignalQueue.pop();
    				mcChan.channelMutex.unlock();
    				break;
    				case LowerLayerRequestType::LOW_LAYER_TRANSMIT:
    					if (!mcChan.channelMutex.tryLockFor(Defs::MutexDelayMs)) {
    						fop.signalQueueMutex.unlock();
    						return etl::unexpected(ServiceChannelNotification::FAILED_TO_LOCK_MUTEX);
    					}

    				LowerLayerResponseSignal response;
    				if (!mcChan.framesAfterVcGeneration.isFull()) {
    					if (signal.frame.value()->getServiceType() == Defs::ServiceType::TYPE_AD) {
    						response = LowerLayerResponseSignal::AD_ACCEPT;
    					} else if (signal.frame.value()->getServiceType() == Defs::ServiceType::TYPE_BD) {
    						response = LowerLayerResponseSignal::BD_ACCEPT;
    					} else {
    						response = LowerLayerResponseSignal::BC_ACCEPT;
    					}

    					mcChan.framesAfterVcGeneration.push(signal.frame.value());
    				} else {
    					if (signal.frame.value()->getServiceType() == Defs::ServiceType::TYPE_AD) {
    						response = LowerLayerResponseSignal::AD_REJECT;
    					} else if (signal.frame.value()->getServiceType() == Defs::ServiceType::TYPE_BD) {
    						response = LowerLayerResponseSignal::BD_REJECT;
    					} else {
    						response = LowerLayerResponseSignal::BC_REJECT;
    					}
    				}

    				fop.lowerLayerResponseSignalQueue.push(response);
    				fop.fopToLowerLayerRequestSignalQueue.pop();
    				mcChan.channelMutex.unlock();
    				break;
    			}
    		}

    		/** Handle a directive notification signal that generated a Type-BC frame
			 * - ACCEPT_RESPONSE_TO_DIRECTIVE: FOP accepted the request. Initiate directives with set V(R) or unlock will also
			 *   generate a type BC frame. No other action needs to be taken.
			 * - REJECT_RESPONSE_TO_DIRECTIVE: FOP rejected the request. No other action needs to be taken.
			 * - POSITIVE_CONFIRM_RESPONSE_TO_DIRECTIVE: Can be received for 3 possible directives:
			 *   -- Initiate directives with set V(R) or unlock successfully received by farm. Delete type BC frame master copy.
			 *   -- Initiate with clcw check. No other action needs to be taken.
			 * - NEGATIVE_CONFIRM_RESPONSE_TO_DIRECTIVE: Can be received for 3 possible directives:
			 *   -- Initiate directives with set V(R) or unlock were not received by farm or
			 *      an error occurred. Delete type BC frame master copy.
			 *   -- Initiate with clcw check. No other action needs to be taken.
			 *
			 * Note: Fop sends all directive notifications to two queues:
			 *       directiveNotificationSignalQueue: Used by this function to send respond to directivies causing a Type-BC
			 *                                         frame generation.
			 *       directiveNotificationSignalQueueUser: Used by the 'copManagementServiceDirectiveNotify' service, to pass
			 *                                             the notification to the user. These notifications are the same as
			 *                                             above, but do not expose a frame pointer.
			 */
    		if (!fop.directiveNotificationSignalQueue.empty()) {
    			encounteredFopSignal = true;
    			DirectiveNotificationSignal& signal = fop.directiveNotificationSignalQueue.front();
    			switch (signal.directiveNotificationType) {
    				case DirectiveNotificationType::POSITIVE_CONFIRM_RESPONSE_TO_DIRECTIVE:
    					if (!mcChan.channelMutex.tryLockFor(Defs::MutexDelayMs)) {
    						fop.signalQueueMutex.unlock();
    						return etl::unexpected(ServiceChannelNotification::FAILED_TO_LOCK_MUTEX);
    					}

    				if (!mcChan.framesAfterVcGeneration.isFull()) {
    					mcChan.framesAfterVcGeneration.push(signal.frame.value());
    					fop.directiveNotificationSignalQueue.pop();
    				}

    				mcChan.channelMutex.unlock();
    				break;
    				case DirectiveNotificationType::NEGATIVE_CONFIRM_RESPONSE_TO_DIRECTIVE:
    					if (!mcChan.channelMutex.tryLockFor(Defs::MutexDelayMs)) {
    						fop.signalQueueMutex.unlock();
    						return etl::unexpected(ServiceChannelNotification::FAILED_TO_LOCK_MUTEX);
    					}

	    				if (!Objects::frameOctetPool.poolMutex.tryLockFor(Defs::MutexDelayMs)) {
							mcChan.channelMutex.unlock();
    						fop.signalQueueMutex.unlock();
    						return etl::unexpected(ServiceChannelNotification::FAILED_TO_LOCK_MUTEX);
    					}

    					Objects::frameOctetPool.deleteBlock(signal.frame.value()->getFrameData(),
							signal.frame.value()->getFrameLength());
    					mcChan.frameMasterCopies.erase(signal.frame.value());

    					fop.directiveNotificationSignalQueue.pop();
    					Objects::frameOctetPool.poolMutex.unlock();
    					mcChan.channelMutex.unlock();
    					break;
    				case DirectiveNotificationType::ACCEPT_RESPONSE_TO_DIRECTIVE:
    					[[fallthrough]]
					case DirectiveNotificationType::REJECT_RESPONSE_TO_DIRECTIVE:
						fop.directiveNotificationSignalQueue.pop();
    			}
    		}

    		/**
			 * Push a transfer fdu signal
			 */
    		if (!vcChan.channelMutex.tryLockFor(Defs::MutexDelayMs)) {
    			fop.signalQueueMutex.unlock();
    			return etl::unexpected(ServiceChannelNotification::FAILED_TO_LOCK_MUTEX);
    		}

    		if (!fop.transferFduSignalQueue.full()) {
    			encounteredFopSignal = true;
    			if (!vcChan.framesAfterApplySDLSSecurity.isEmpty()) {
    				fop.transferFduSignalQueue.push(FduTransferSignal(vcChan.framesAfterApplySDLSSecurity.getFront()));
    				vcChan.framesAfterApplySDLSSecurity.popFront();
    			}
    		}
    		vcChan.channelMutex.unlock();

    		fop.signalQueueMutex.unlock();
    	} else {
    		// just pass the frame in the next stage
    		if (!vcChan.channelMutex.tryLockFor(Defs::MutexDelayMs)) {
    			return etl::unexpected(ServiceChannelNotification::FAILED_TO_LOCK_MUTEX);
    		}

    		if (vcChan.framesAfterApplySDLSSecurity.isEmpty()) {
    			vcChan.channelMutex.unlock();
    			return etl::unexpected(ServiceChannelNotification::FRAME_QUEUE_EMPTY);
    		}

    		if (!mcChan.channelMutex.tryLockFor(Defs::MutexDelayMs)) {
    			vcChan.channelMutex.unlock();
    			return etl::unexpected(ServiceChannelNotification::FAILED_TO_LOCK_MUTEX);
    		}

    		if (mcChan.framesAfterVcGeneration.isFull()) {
			    mcChan.channelMutex.unlock();
			    vcChan.channelMutex.unlock();
			    return etl::unexpected(ServiceChannelNotification::FRAME_QUEUE_FULL);
    		}

    		mcChan.framesAfterVcGeneration.push(vcChan.framesAfterApplySDLSSecurity.getFront());
    		vcChan.framesAfterApplySDLSSecurity.popFront();

		    mcChan.channelMutex.unlock();
		    vcChan.channelMutex.unlock();
	    }

	    if (!encounteredFopSignal) {
		    return etl::unexpected(ServiceChannelNotification::NO_FOP_SIGNALS_TO_PROCESS);
	    } else {
		    return {};
	    }
    }

	etl::expected<void, ServiceChannelNotification> GroundSegmentTcDataHandling::allFramesGeneration(
		const PhysicalChannel& phyChan,
		MasterChannelGsTc& mcChan,
		uint8_t *frameDestination) {
		if (!mcChan.channelMutex.tryLockFor(Defs::MutexDelayMs)) {
			return etl::unexpected(ServiceChannelNotification::FAILED_TO_LOCK_MUTEX);
		}

    	if (!Objects::frameOctetPool.poolMutex.tryLockFor(Defs::MutexDelayMs)) {
    		mcChan.channelMutex.unlock();
    		return etl::unexpected(ServiceChannelNotification::FAILED_TO_LOCK_MUTEX);
    	}

    	// ensure there is at least one frame to send
    	if (mcChan.framesAfterVcGeneration.isEmpty()) {
    		Objects::frameOctetPool.poolMutex.unlock();
    		mcChan.channelMutex.unlock();
    		return etl::unexpected(ServiceChannelNotification::FRAME_QUEUE_EMPTY);
    	}

    	TransferFrameTC* frameTcPtr = mcChan.framesAfterVcGeneration.getFront();

    	const Defs::VcidScidKey key = constructVcidScidKey(frameTcPtr->getVirtualChannelId(),
				frameTcPtr->getSpacecraftId());

    	uint8_t numRepetitions;
    	if (frameTcPtr->getServiceType() == Defs::ServiceType::TYPE_AD) {
    		numRepetitions = Objects::virtualChannelGsTcMap.at(key).getVcRepetitionsTypeAD();
    	} else if (frameTcPtr->getServiceType() == Defs::ServiceType::TYPE_BC) {
    		numRepetitions = Objects::virtualChannelGsTcMap.at(key).getVcRepetitionsTypeBC();
    	} else {
    		// type BC frames are expedited and therefore are sequentially transmitted only once
    		numRepetitions = 1;
    	}

    	// check if the frame was retransmitted enough times
    	if (frameTcPtr->getTimesSequentiallyTransmitted() >= numRepetitions) {
    		// remove the frame from the queue
		    mcChan.framesAfterVcGeneration.pop();

    		if (Objects::virtualChannelGsTcMap.at(key).getCopInEffect()) {
    			// Erase the master copy of type BD frames only. Type AD and BC frames are deleted from
    			// virtualChannelGeneration, once their arrival is confirmed
    			if (frameTcPtr->getServiceType() == Defs::ServiceType::TYPE_BD) {
    				Objects::frameOctetPool.deleteBlock(frameTcPtr->getFrameData(), frameTcPtr->getFrameLength());
    				mcChan.frameMasterCopies.erase(frameTcPtr);
    			}
    		} else {
    			Objects::frameOctetPool.deleteBlock(frameTcPtr->getFrameData(), frameTcPtr->getFrameLength());
    			mcChan.frameMasterCopies.erase(frameTcPtr);
    		}

    		// if queue is empty, return
    		if (mcChan.framesAfterVcGeneration.isEmpty()) {
    			Objects::frameOctetPool.poolMutex.unlock();
    			mcChan.channelMutex.unlock();
    			return etl::unexpected(ServiceChannelNotification::FRAME_QUEUE_EMPTY);
    		}

    		// fetch the next frame, append CRC if needed
    		frameTcPtr = mcChan.framesAfterVcGeneration.getFront();
    		if (phyChan.getFrameErrorControlFieldPresent()) {
    			frameTcPtr->appendCRC();
    		}
    	}

    	// copy the frame to the user supplied buffer
    	memcpy(frameDestination, frameTcPtr->getFrameData(), phyChan.getTMFrameLength());

    	// increment repetition count and return
    	frameTcPtr->incrementTimesSequentiallyTransmitted();
    	Objects::frameOctetPool.poolMutex.unlock();
    	mcChan.channelMutex.unlock();
    	return {};
    }

#endif // INCLUDE_GROUND_SEGMENT_CODE
} // CCSDSDataLinkLayer::GroundSegmentTcDataHandling
