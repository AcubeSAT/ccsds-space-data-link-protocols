#include "SpaceSegmentTmDataHandlingFunctions.hpp"
#include "AddressingAndParsingUtilities.hpp"
#include "OID_GENERATOR.hpp"

namespace CCSDSDataLinkLayer {
#ifdef INCLUDE_SPACE_SEGMENT_CODE
	void SpaceSegmentTmDataHandling::resetVirtualChannel(VirtualChannelSsTm& vcChan) {
		vcChan.packetLengths.reset();
		vcChan.packetOctets.reset();
		vcChan.secondaryHeaderDataFieldOctets.reset();
		vcChan.framesAfterVcGeneration.reset();
		vcChan.framesAfterSecondaryHeaderPlacement.reset();

		vcChan.resetVirtualChannelFrameCount();
	}

	void SpaceSegmentTmDataHandling::resetMasterChannel(MasterChannelSsTm& mcChan) {
		TransferFrameTM* frameTmPtr;
		while (!mcChan.framesAfterSecurityProcessing.isEmpty()) {
			frameTmPtr = mcChan.framesAfterSecurityProcessing.getFront();
			mcChan.framesAfterSecurityProcessing.pop();
			Objects::frameOctetPool.deleteBlock(frameTmPtr->getFrameData(), frameTmPtr->getFrameLength());
			mcChan.frameMasterCopies.erase(frameTmPtr);
		}

		while (!mcChan.framesAfterMcGeneration.isEmpty()) {
			frameTmPtr = mcChan.framesAfterMcGeneration.getFront();
			mcChan.framesAfterMcGeneration.pop();
			Objects::frameOctetPool.deleteBlock(frameTmPtr->getFrameData(), frameTmPtr->getFrameLength());
			mcChan.frameMasterCopies.erase(frameTmPtr);
		}

		mcChan.resetMasterChannelFrameCount();
		mcChan.ocfSduQueue.reset();
		mcChan.waitingBuffer.reset();
	}

    etl::expected<void, ServiceChannelNotification> SpaceSegmentTmDataHandling::storePacket(
        VirtualChannelSsTm& vcChan,
        etl::span<uint8_t> packetSource) {

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
        } else { // Encapsulation packet
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

    	if (!vcChan.channelMutex.tryLockFor(Defs::MutexDelayMs)) {
    		return etl::unexpected(ServiceChannelNotification::FAILED_TO_LOCK_MUTEX);
    	}

        if (vcChan.packetLengths.isFull() || vcChan.packetOctets.remainingCapacity() < packetSource.size()) {
        	vcChan.channelMutex.unlock();
            return etl::unexpected(ServiceChannelNotification::PACKET_QUEUE_FULL);
        }
        vcChan.packetLengths.pushBack(length);
        for (size_t i = 0; i < length; i++) {
            vcChan.packetOctets.pushBack(packetSource[i]);
        }

    	vcChan.channelMutex.unlock();
        return {};
    }

	etl::expected<void, ServiceChannelNotification> SpaceSegmentTmDataHandling::storeVcaSdu(
			const PhysicalChannel& phyChan,
			VirtualChannelSsTm& vcChan,
			etl::span<uint8_t> vcaSduSource,
			bool packetOrderFlag,
			uint8_t segmentLengthIdentifier) {
    	if (!vcChan.channelMutex.tryLockFor(Defs::MutexDelayMs)) {
    		return etl::unexpected(ServiceChannelNotification::FAILED_TO_LOCK_MUTEX);
    	}

    	if (vcChan.packetLengths.isFull() || vcChan.packetOctets.remainingCapacity() < vcaSduSource.size()) {
    		vcChan.channelMutex.unlock();
    		return etl::unexpected(ServiceChannelNotification::PACKET_QUEUE_FULL);
    	}

    	const uint16_t transferFrameDataFieldLength =
				phyChan.getTMFrameLength() -
				Defs::TmPrimaryHeaderSize -
				vcChan.getOperationalControlFieldPresent() * Defs::TmOperationalControlFieldSize -
				phyChan.getFrameErrorControlFieldPresent() * Defs::ErrorControlFieldSize;

		if (vcaSduSource.size() != transferFrameDataFieldLength) {
			vcChan.channelMutex.unlock();
			return etl::unexpected(ServiceChannelNotification::INVALID_LENGTH);
		}

    	vcChan.packetLengths.pushBack((static_cast<uint16_t>(packetOrderFlag) << 2) | static_cast<uint16_t>(segmentLengthIdentifier));
    	for (size_t i = 0; i < vcaSduSource.size(); i++) {
    		vcChan.packetOctets.pushBack(vcaSduSource[i]);
    	}

    	vcChan.channelMutex.unlock();
    	return {};
    }

    etl::expected<void, ServiceChannelNotification>
    SpaceSegmentTmDataHandling::blockingTM(
        const PhysicalChannel& phyChan,
        MasterChannelSsTm &mcChan,
        VirtualChannelSsTm &vcChan,
        bool &finishedOperationsFlag,
        etl::optional<etl::pair<TransferFrameTM *, uint16_t> > &segmentationData) {

        TransferFrameTM *frameTmPtr;
        if (!mcChan.framesAfterMcGeneration.isEmpty()) {
            // get existing frame under processing
            frameTmPtr = mcChan.framesAfterMcGeneration.getFront();
        } else {
            // a new frame needs to be created
            const uint16_t frameLength = phyChan.getTMFrameLength();

            // check if there is enough space for a new frame
            if (mcChan.frameMasterCopies.isFull() ||
                Objects::frameOctetPool.findFit(frameLength).second == MasterChannelAlert::NOT_ENOUGH_SPACE_IN_MEMORY_POOL) {
                return etl::unexpected(ServiceChannelNotification::NOT_ENOUGH_SPACE_IN_MASTER_COPY_OR_MEMORY_POOL);
            }

            if (vcChan.framesAfterVcGeneration.isFull()) {
                return etl::unexpected(ServiceChannelNotification::FRAME_QUEUE_FULL);
            }

            // allocate frame data and create the frame object
            uint8_t *frameData = Objects::frameOctetPool.allocateBlock(frameLength, nullptr);
            frameTmPtr = mcChan.frameMasterCopies.push(TransferFrameTM(frameData,
                                    frameLength,
                                    vcChan.getVcid(),
                                    mcChan.getScid(),
                                    vcChan.getOperationalControlFieldPresent(),
                                    vcChan.getVirtualChannelFrameCount(),
                                    vcChan.getSecondaryHeaderPresent(),
                                    vcChan.getSecondaryHeaderLength(),
                                    vcChan.getSynchronization(),
                                    Defs::PacketOrderFlag,
                                    Defs::SegmentLengthIdentifierLegacy,
                                    0,
                                    phyChan.getFrameErrorControlFieldPresent()));
            vcChan.incrementVirtualChannelFrameCount();
            frameTmPtr->setFirstDataFieldEmptyOctet(0);

            // push a frame pointer to the queue
            vcChan.framesAfterVcGeneration.push(frameTmPtr);
        }

        const uint16_t transferFrameDataFieldLength =
                phyChan.getTMFrameLength() -
                Defs::TmPrimaryHeaderSize -
                vcChan.getSecondaryHeaderLength() -
                vcChan.getOperationalControlFieldPresent() * Defs::TmOperationalControlFieldSize -
                phyChan.getFrameErrorControlFieldPresent() * Defs::ErrorControlFieldSize;

        // Pop packets from queue and append them to the data field.
        uint16_t packetLength;
        while (!vcChan.packetLengths.isEmpty()) {
            packetLength = vcChan.packetLengths.getFront();
            vcChan.packetLengths.popFront();
            if (frameTmPtr->getFirstDataFieldEmptyOctet() + packetLength > transferFrameDataFieldLength) {
                // Next packet does not fit. Give a request to the segmentation method.
                segmentationData = etl::make_pair(frameTmPtr, packetLength);
                return {};
            }

            // Next packet fits. Copy it to the data field and update first data field empty octet
            for (uint16_t i = 0; i < packetLength; i++) {
                frameTmPtr->getFrameData()[Defs::TmPrimaryHeaderSize + vcChan.getSecondaryHeaderLength() + frameTmPtr->getFirstDataFieldEmptyOctet()] = vcChan.packetOctets.getFront();
                vcChan.packetOctets.popFront();
            }
            frameTmPtr->setFirstDataFieldEmptyOctet(frameTmPtr->getFirstDataFieldEmptyOctet() + packetLength);
        }

        // Ran out of packets and frame is filled, signal end of operations and return
        if (frameTmPtr->getFirstDataFieldEmptyOctet() == transferFrameDataFieldLength) {
            finishedOperationsFlag = true;
            return {};
        }

        // Ran out of packets but frame is not filled. Push idle space packet to queue.
        const uint16_t remainingSpace = transferFrameDataFieldLength - frameTmPtr->getFirstDataFieldEmptyOctet();
        generateIdleSpacePacket(vcChan, remainingSpace);

        if (vcChan.packetLengths.isEmpty()) {
            // The code should probably not reach this point
            return etl::unexpected(ServiceChannelNotification::PACKET_QUEUE_EMPTY);
        }

        packetLength = vcChan.packetLengths.getFront();
        vcChan.packetLengths.popFront();

        if (remainingSpace >= packetLength) {
            // Generated idle packet fits perfectly. Append it to data field, end operations.
            for (uint16_t i = 0; i < packetLength; i++) {
                frameTmPtr->getFrameData()[Defs::TmPrimaryHeaderSize + vcChan.getSecondaryHeaderLength() + frameTmPtr->getFirstDataFieldEmptyOctet()] = vcChan.packetOctets.getFront();
                vcChan.packetOctets.popFront();
            }

            frameTmPtr->setFirstDataFieldEmptyOctet(frameTmPtr->getFirstDataFieldEmptyOctet() + packetLength);
            finishedOperationsFlag = true;
            return {};
        }

        // Generated idle packet does not fit. Give a request to the segmentation method.
        segmentationData = etl::make_pair(frameTmPtr, packetLength);
        return {};
    }

    etl::expected<void, ServiceChannelNotification>
    SpaceSegmentTmDataHandling::segmentationTM(
        const PhysicalChannel& phyChan,
        MasterChannelSsTm& mcChan,
        VirtualChannelSsTm& vcChan,
        TransferFrameTM *frameTmPtr,
        const uint16_t packetLength) {


        const uint16_t transferFrameDataFieldLength =
                        phyChan.getTMFrameLength() -
                        Defs::TmPrimaryHeaderSize -
                        vcChan.getSecondaryHeaderLength() -
                        vcChan.getOperationalControlFieldPresent() * Defs::TmOperationalControlFieldSize -
                        phyChan.getFrameErrorControlFieldPresent() * Defs::ErrorControlFieldSize;

        // calculate amount of new transfer frames and octets needed to fit the whole packet
        const uint16_t prevFrameCapacity = transferFrameDataFieldLength - frameTmPtr->getFirstDataFieldEmptyOctet();
        const uint8_t numberOfNewTransferFrames = (packetLength - prevFrameCapacity) / transferFrameDataFieldLength +
                                                  ((packetLength - prevFrameCapacity) % transferFrameDataFieldLength
                                                       ? 1
                                                       : 0);
        const uint16_t numberOfNewOctets = numberOfNewTransferFrames * (
                                               Defs::TmPrimaryHeaderSize +
                                               transferFrameDataFieldLength +
                                               vcChan.getSecondaryHeaderLength() +
                                               vcChan.getOperationalControlFieldPresent() *
                                               Defs::TmOperationalControlFieldSize +
                                               phyChan.getFrameErrorControlFieldPresent() *
                                               Defs::ErrorControlFieldSize);

        // Ensure there is enough space for the new frames. If not, then the operation should be halted, and
        // the packet's length must be returned to the front of the packet length queue (since blockingTM popped it).
        if (mcChan.frameMasterCopies.isFull() ||
            Objects::frameOctetPool.findFit(numberOfNewOctets).second == MasterChannelAlert::NOT_ENOUGH_SPACE_IN_MEMORY_POOL) {
            vcChan.packetLengths.pushBack(packetLength);
            return etl::unexpected(ServiceChannelNotification::NOT_ENOUGH_SPACE_IN_MASTER_COPY_OR_MEMORY_POOL);
        }

        if (vcChan.framesAfterVcGeneration.remainingCapacity() < numberOfNewTransferFrames) {
            vcChan.packetLengths.pushBack(packetLength);
            return etl::unexpected(ServiceChannelNotification::FRAME_QUEUE_FULL);
        }

        // fill half-full frame
        for (uint16_t i = 0; i < packetLength; i++) {
            frameTmPtr->getFrameData()[Defs::TmPrimaryHeaderSize + vcChan.getSecondaryHeaderLength() + frameTmPtr->getFirstDataFieldEmptyOctet()] = vcChan.packetOctets.getFront();
            vcChan.packetOctets.popFront();
        }
        frameTmPtr->setFirstDataFieldEmptyOctet(frameTmPtr->getFirstDataFieldEmptyOctet() + packetLength);


        // create the new frames
        uint16_t remainingPacketSegmentLength = packetLength - prevFrameCapacity;
        const uint16_t frameLength = phyChan.getTMFrameLength();
        for (uint8_t i = 0; i < numberOfNewTransferFrames; ++i) {
            uint8_t* frameData = Objects::frameOctetPool.allocateBlock(frameLength, nullptr);

            for (uint16_t j = 0; j < remainingPacketSegmentLength; ++j) {
                frameData[j + Defs::TmPrimaryHeaderSize + vcChan.getSecondaryHeaderLength()] = vcChan.packetOctets.getFront();
                vcChan.packetOctets.popFront();
            }

            uint16_t firstHeaderPointer;
            if (i == numberOfNewTransferFrames - 1) {
                // last frame
                if (remainingPacketSegmentLength == transferFrameDataFieldLength) {
                    firstHeaderPointer = Defs::TmNoPacketStartFirstHeaderPointerVal;
                } else {
                    firstHeaderPointer = remainingPacketSegmentLength;
                }
            } else {
                // intermediate frame
                firstHeaderPointer = Defs::TmNoPacketStartFirstHeaderPointerVal;
            }

            frameTmPtr = mcChan.frameMasterCopies.push(TransferFrameTM(frameData,
                                    frameLength,
                                    vcChan.getVcid(),
                                    mcChan.getScid(),
                                    vcChan.getOperationalControlFieldPresent(),
                                    vcChan.getVirtualChannelFrameCount(),
                                    vcChan.getSecondaryHeaderPresent(),
                                    vcChan.getSecondaryHeaderLength(),
                                    vcChan.getSynchronization(),
                                    Defs::PacketOrderFlag,
                                    Defs::SegmentLengthIdentifierLegacy,
                                    firstHeaderPointer,
                                    phyChan.getFrameErrorControlFieldPresent()));
            frameTmPtr->setFirstDataFieldEmptyOctet((i == numberOfNewTransferFrames - 1) ? firstHeaderPointer : frameLength);
            vcChan.incrementVirtualChannelFrameCount();
            vcChan.framesAfterVcGeneration.push(frameTmPtr);

            remainingPacketSegmentLength -= transferFrameDataFieldLength;
        }

        return {};
    }

    void SpaceSegmentTmDataHandling::generateIdleSpacePacket(VirtualChannelSsTm& vcChan,
                                                             const uint16_t remainingDataFieldSpace) {
        uint16_t idlePacketDataFieldLength;
        if (remainingDataFieldSpace >= Defs::SpacePacketPrimaryHeaderLength + 1) {
            idlePacketDataFieldLength = remainingDataFieldSpace - Defs::SpacePacketPrimaryHeaderLength;
        } else {
            idlePacketDataFieldLength = 1;
        }

        uint8_t tmpData[Defs::SpacePacketPrimaryHeaderLength + idlePacketDataFieldLength];

        // Static primary header fields
        for (uint8_t i = 0; i < Defs::SpacePacketPrimaryHeaderLength - 2; ++i) {
            tmpData[i] = Defs::IdleSpPrimaryHeader[i];
        }

        // Data length field
        // The idle packet data length needs to be reduced by 1  (see p. 4.1.3.5 of space packet protocol)
        tmpData[Defs::SpacePacketPrimaryHeaderLength - 2] = static_cast<uint8_t>(
            (idlePacketDataFieldLength - 1) >> 8);
        tmpData[Defs::SpacePacketPrimaryHeaderLength - 1] = static_cast<uint8_t>(idlePacketDataFieldLength - 1);

        // Data field (idle data)
        for (uint16_t i = 0; i < idlePacketDataFieldLength; i++) {
            tmpData[i + Defs::SpacePacketPrimaryHeaderLength] = getNextOidByte();
        }

        if (vcChan.packetLengths.isFull() ||
            vcChan.packetOctets.remainingCapacity() < Defs::SpacePacketPrimaryHeaderLength +
            idlePacketDataFieldLength) {
            return;
        }
        vcChan.packetLengths.pushBack(Defs::SpacePacketPrimaryHeaderLength + idlePacketDataFieldLength);
        for (size_t i = 0; i < Defs::SpacePacketPrimaryHeaderLength + idlePacketDataFieldLength; i++) {
            vcChan.packetOctets.pushBack(tmpData[i]);
        }
    }

    etl::expected<void, ServiceChannelNotification>
    SpaceSegmentTmDataHandling::virtualChannelGeneration(
	    const PhysicalChannel &phyChan,
	    MasterChannelSsTm& mcChan,
        VirtualChannelSsTm& vcChan) {

    	if (!vcChan.channelMutex.tryLockFor(Defs::MutexDelayMs)) {
    		return etl::unexpected(ServiceChannelNotification::FAILED_TO_LOCK_MUTEX);
    	}

    	if (!mcChan.channelMutex.tryLockFor(Defs::MutexDelayMs)) {
    		vcChan.channelMutex.unlock();
    		return etl::unexpected(ServiceChannelNotification::FAILED_TO_LOCK_MUTEX);
    	}

    	if (!Objects::frameOctetPool.poolMutex.tryLockFor(Defs::MutexDelayMs)) {
    		mcChan.channelMutex.unlock();
    		vcChan.channelMutex.unlock();
    		return etl::unexpected(ServiceChannelNotification::FAILED_TO_LOCK_MUTEX);
    	}

    	// Return immediately if there are no packets available
    	if (vcChan.packetLengths.isEmpty()) {
    		Objects::frameOctetPool.poolMutex.unlock();
		    mcChan.channelMutex.unlock();
		    vcChan.channelMutex.unlock();
		    return etl::unexpected(ServiceChannelNotification::PACKET_QUEUE_EMPTY);
    	}

    	// Handle simple case, where channel transfers vca sdu instead of packets
    	if (vcChan.getSynchronization() == Defs::SynchronizationFlag::VCA_SDU) {
    		const uint16_t frameLength = phyChan.getTMFrameLength();
    		const uint16_t transferFrameDataFieldLength =
						frameLength -
						Defs::TmPrimaryHeaderSize -
						vcChan.getSecondaryHeaderLength() -
						vcChan.getOperationalControlFieldPresent() * Defs::TmOperationalControlFieldSize -
						phyChan.getFrameErrorControlFieldPresent() * Defs::ErrorControlFieldSize;

    		// check if there is enough space for a new frame
    		if (mcChan.frameMasterCopies.isFull() ||
				Objects::frameOctetPool.findFit(frameLength).second == MasterChannelAlert::NOT_ENOUGH_SPACE_IN_MEMORY_POOL) {
    			Objects::frameOctetPool.poolMutex.unlock();
			    mcChan.channelMutex.unlock();
			    vcChan.channelMutex.unlock();
			    return etl::unexpected(ServiceChannelNotification::NOT_ENOUGH_SPACE_IN_MASTER_COPY_OR_MEMORY_POOL);
			}

    		if (vcChan.framesAfterVcGeneration.isFull()) {
    			Objects::frameOctetPool.poolMutex.unlock();
			    mcChan.channelMutex.unlock();
			    vcChan.channelMutex.unlock();
			    return etl::unexpected(ServiceChannelNotification::FRAME_QUEUE_FULL);
    		}

    		const bool packetOrderFlag = vcChan.packetLengths.getFront() >> 2;
			const uint8_t segmentLengthIdentifier = vcChan.packetLengths.getFront() & 0x3;
    		vcChan.packetLengths.popFront();

    		// allocate frame data and create the frame object
    		uint8_t *frameData = Objects::frameOctetPool.allocateBlock(frameLength, nullptr);

			for (uint16_t i = 0; i < transferFrameDataFieldLength; i++) {
				frameData[i + Defs::TmPrimaryHeaderSize + vcChan.getSecondaryHeaderLength()] = vcChan.packetOctets.getFront();
				vcChan.packetOctets.popFront();
			}

    		TransferFrameTM* frameTmPtr = mcChan.frameMasterCopies.push(TransferFrameTM(frameData,
									frameLength,
									vcChan.getVcid(),
									mcChan.getScid(),
									vcChan.getOperationalControlFieldPresent(),
									vcChan.getVirtualChannelFrameCount(),
									vcChan.getSecondaryHeaderPresent(),
									vcChan.getSecondaryHeaderLength(),
									vcChan.getSynchronization(),
									packetOrderFlag,
									segmentLengthIdentifier,
									0,
									phyChan.getFrameErrorControlFieldPresent()));
    		vcChan.incrementVirtualChannelFrameCount();

    		// push a frame pointer to the queue
    		vcChan.framesAfterVcGeneration.push(frameTmPtr);

    		Objects::frameOctetPool.poolMutex.unlock();
		    mcChan.channelMutex.unlock();
		    vcChan.channelMutex.unlock();
		    return {};
    	}

    	// Packets
		bool finishedOperationsFlag = false;
	    etl::optional<etl::pair<TransferFrameTM *, uint16_t>> segmentationData;
    	while (true) {
    		// Block packets until queue empties or a packet that does not fit is encountered.
		    etl::expected<void, ServiceChannelNotification> opResult = blockingTM(
			    phyChan, mcChan, vcChan, finishedOperationsFlag,
			    segmentationData);

    		if (!opResult.has_value()) {
    			// Not enough space for new frames.
    			Objects::frameOctetPool.poolMutex.unlock();
			    mcChan.channelMutex.unlock();
			    vcChan.channelMutex.unlock();
			    return etl::unexpected(opResult.error());
    		}

    		if (finishedOperationsFlag) {
    			Objects::frameOctetPool.poolMutex.unlock();
			    mcChan.channelMutex.unlock();
			    vcChan.channelMutex.unlock();
			    return {};
    		}

		    if (segmentationData.has_value()) {
		    	// Got request by blockingTM for segmentation.
			    opResult = segmentationTM(phyChan, mcChan, vcChan, segmentationData.value().first,
			                   segmentationData.value().second);
		    	if (!opResult.has_value()) {
		    		Objects::frameOctetPool.poolMutex.unlock();
				    mcChan.channelMutex.unlock();
				    vcChan.channelMutex.unlock();
				    return  etl::unexpected(opResult.error());
		    	}
		    	segmentationData = etl::nullopt;
		    }
    	}
    }

    etl::expected<void, ServiceChannelNotification> SpaceSegmentTmDataHandling::generateOidFrame(
		const PhysicalChannel &phyChan,
		MasterChannelSsTm& mcChan,
		VirtualChannelSsTm& vcChan) {

    	if (!vcChan.channelMutex.tryLockFor(Defs::MutexDelayMs)) {
    		return etl::unexpected(ServiceChannelNotification::FAILED_TO_LOCK_MUTEX);
    	}

    	if (!mcChan.channelMutex.tryLockFor(Defs::MutexDelayMs)) {
    		vcChan.channelMutex.unlock();
    		return etl::unexpected(ServiceChannelNotification::FAILED_TO_LOCK_MUTEX);
    	}

    	if (!Objects::frameOctetPool.poolMutex.tryLockFor(Defs::MutexDelayMs)) {
    		mcChan.channelMutex.unlock();
    		vcChan.channelMutex.unlock();
    		return etl::unexpected(ServiceChannelNotification::FAILED_TO_LOCK_MUTEX);
    	}

    	// check if there is enough space for an OID frame
        const uint16_t frameLength = phyChan.getTMFrameLength();
    	if (mcChan.frameMasterCopies.isFull() ||
			Objects::frameOctetPool.findFit(frameLength).second == MasterChannelAlert::NOT_ENOUGH_SPACE_IN_MEMORY_POOL) {
			Objects::frameOctetPool.poolMutex.unlock();
    		mcChan.channelMutex.unlock();
		    vcChan.channelMutex.unlock();
		    return etl::unexpected(ServiceChannelNotification::NOT_ENOUGH_SPACE_IN_MASTER_COPY_OR_MEMORY_POOL);
		}

    	if (vcChan.framesAfterVcGeneration.isFull()) {
    		Objects::frameOctetPool.poolMutex.unlock();
    		mcChan.channelMutex.unlock();
		    vcChan.channelMutex.unlock();
		    return etl::unexpected(ServiceChannelNotification::FRAME_QUEUE_FULL);
    	}

        const uint16_t transferFrameDataFieldLength =
                        frameLength -
                        Defs::TmPrimaryHeaderSize -
                        vcChan.getSecondaryHeaderLength() -
                        vcChan.getOperationalControlFieldPresent() * Defs::TmOperationalControlFieldSize -
                        phyChan.getFrameErrorControlFieldPresent() * Defs::ErrorControlFieldSize;

        uint8_t* frameData = Objects::frameOctetPool.allocateBlock(frameLength, nullptr);

    	for (uint16_t i = 0; i < transferFrameDataFieldLength; i++) {
    		frameData[i + Defs::TmPrimaryHeaderSize + vcChan.getSecondaryHeaderLength()] = getNextOidByte();
    	}

        TransferFrameTM* oidFramePtr = mcChan.frameMasterCopies.push(TransferFrameTM(frameData,
                                            frameLength,
                                            vcChan.getVcid(),
                                            mcChan.getScid(),
                                            vcChan.getOperationalControlFieldPresent(),
                                            vcChan.getVirtualChannelFrameCount(),
                                            vcChan.getSecondaryHeaderPresent(),
                                            vcChan.getSecondaryHeaderLength(),
                                            vcChan.getSynchronization(),
                                            Defs::PacketOrderFlag,
                                            Defs::SegmentLengthIdentifierLegacy,
                                            Defs::TmOIDFrameFirstHeaderPointer,
                                            phyChan.getFrameErrorControlFieldPresent()));
        vcChan.incrementVirtualChannelFrameCount();
        vcChan.framesAfterVcGeneration.push(oidFramePtr);

    	Objects::frameOctetPool.poolMutex.unlock();
	    mcChan.channelMutex.unlock();
	    vcChan.channelMutex.unlock();
	    return {};
    }

	etl::expected<void, ServiceChannelNotification> SpaceSegmentTmDataHandling::appendSecondaryHeaderDataField(
		VirtualChannelSsTm &vcChan) {
		if (!vcChan.channelMutex.tryLockFor(Defs::MutexDelayMs)) {
			return etl::unexpected(ServiceChannelNotification::FAILED_TO_LOCK_MUTEX);
		}

		if (vcChan.framesAfterVcGeneration.isEmpty()) {
			vcChan.channelMutex.unlock();
			return etl::unexpected(ServiceChannelNotification::FRAME_QUEUE_EMPTY);
		}

		if (vcChan.framesAfterSecondaryHeaderPlacement.isFull()) {
			vcChan.channelMutex.unlock();
			return etl::unexpected(ServiceChannelNotification::FRAME_QUEUE_FULL);
		}

		TransferFrameTM* frameTmPtr = vcChan.framesAfterVcGeneration.getFront();
		if (vcChan.getSecondaryHeaderPresent()) {
			if (vcChan.secondaryHeaderDataFieldOctets.currentSize() < vcChan.getSecondaryHeaderLength() - Defs::TmSecondaryHeaderIdLength) {
				vcChan.channelMutex.unlock();
				return etl::unexpected(ServiceChannelNotification::NO_SECONDARY_HEADER_DATA_FIELD_AVAILABLE);
			}

			uint8_t* frameData = frameTmPtr->getFrameData();
			for (uint16_t i = 0; i < vcChan.getSecondaryHeaderLength() - Defs::TmSecondaryHeaderIdLength; i++) {
				frameData[i + Defs::TmPrimaryHeaderSize + Defs::TmSecondaryHeaderIdLength] = vcChan.secondaryHeaderDataFieldOctets.getFront();
				vcChan.secondaryHeaderDataFieldOctets.pop();
			}
		}

		//push to the next stage
		vcChan.framesAfterVcGeneration.pop();
		vcChan.framesAfterSecondaryHeaderPlacement.push(frameTmPtr);

		vcChan.channelMutex.unlock();
		return {};
	}

	etl::expected<void, ServiceChannelNotification> SpaceSegmentTmDataHandling::applySDLSSecurity(
		const PhysicalChannel &phyChan,
		MasterChannelSsTm &mcChan,
		VirtualChannelSsTm &vcChan) {
		if (!vcChan.channelMutex.tryLockFor(Defs::MutexDelayMs)) {
			return etl::unexpected(ServiceChannelNotification::FAILED_TO_LOCK_MUTEX);
		}

		if (!mcChan.channelMutex.tryLockFor(Defs::MutexDelayMs)) {
			vcChan.channelMutex.unlock();
			return etl::unexpected(ServiceChannelNotification::FAILED_TO_LOCK_MUTEX);
		}

		if (vcChan.framesAfterSecondaryHeaderPlacement.isEmpty()) {
			mcChan.channelMutex.unlock();
			vcChan.channelMutex.unlock();
			return etl::unexpected(ServiceChannelNotification::FRAME_QUEUE_EMPTY);
		}

		if (mcChan.framesAfterSecurityProcessing.isFull()) {
			mcChan.channelMutex.unlock();
			vcChan.channelMutex.unlock();
			return etl::unexpected(ServiceChannelNotification::FRAME_QUEUE_FULL);
		}

		TransferFrameTM *frameTmPtr = vcChan.framesAfterSecondaryHeaderPlacement.getFront();

		etl::optional<uint16_t> associatedSdlsSpi = vcChan.getAssociatedSdlsSPI();
		if (associatedSdlsSpi.has_value()) {
			// security processing is required for this frame
			SecurityAssociation &sa = Objects::saSpaceSegmentMap.at(associatedSdlsSpi.value());

			if (!sa.saMutex.tryLockFor(Defs::MutexDelayMs)) {
				mcChan.channelMutex.unlock();
				vcChan.channelMutex.unlock();
				return etl::unexpected(ServiceChannelNotification::FAILED_TO_LOCK_MUTEX);
			}

			const uint16_t transferFrameDataFieldLength = frameTmPtr->getFrameLength() -
			                                              Defs::TmPrimaryHeaderSize -
			                                              frameTmPtr->getSecondaryHeaderLength() -
			                                              sa.getSecurityHeaderLength() -
			                                              sa.getSecurityTrailerLength() -
			                                              frameTmPtr->getOperationalControlFieldFlag() *
			                                              Defs::TmOperationalControlFieldSize -
			                                              phyChan.getFrameErrorControlFieldPresent() *
			                                              Defs::ErrorControlFieldSize;
			if (sa.getSecurityAssociationStatus() == Defs::SecurityAssociationStatus::RUNNING) {
				if (const auto status = sa.applySecurity(frameTmPtr, transferFrameDataFieldLength);
					!status.has_value()) {
					// because of the previous checks, MAC_CALCULATION_ERROR is the only possible error
					sa.saMutex.unlock();
					mcChan.channelMutex.unlock();
					vcChan.channelMutex.unlock();
					return etl::unexpected(ServiceChannelNotification::SLDS_CALCULATION_ERROR);
					}
			}
			sa.saMutex.unlock();
		}

		// push to next stage
		vcChan.framesAfterSecondaryHeaderPlacement.pop();
		mcChan.framesAfterSecurityProcessing.push(frameTmPtr);

		mcChan.channelMutex.unlock();
		vcChan.channelMutex.unlock();
		return {};
	}

    etl::expected<void, ServiceChannelNotification> SpaceSegmentTmDataHandling::masterChannelGeneration(
	    const PhysicalChannel &phyChan,
	    MasterChannelSsTm &mcChan) {

    	if (!mcChan.channelMutex.tryLockFor(Defs::MutexDelayMs)) {
    		return etl::unexpected(ServiceChannelNotification::FAILED_TO_LOCK_MUTEX);
    	}

    	// check if there is space in the next queue
    	if (mcChan.framesAfterMcGeneration.isFull()) {
    		mcChan.channelMutex.unlock();
    		return etl::unexpected(ServiceChannelNotification::FRAME_QUEUE_FULL);
    	}

    	TransferFrameTM* frameTmPtr;
    	// frames waiting for an ocf sdu
		if (!mcChan.waitingBuffer.isEmpty()) {
			frameTmPtr = mcChan.waitingBuffer.getFront();
			if (!mcChan.ocfSduQueue.isEmpty()) {
				// ocf sdu exists
				frameTmPtr->setOperationalControlField(mcChan.ocfSduQueue.getFront());
				mcChan.ocfSduQueue.pop();
				mcChan.waitingBuffer.pop();
				mcChan.framesAfterMcGeneration.push(frameTmPtr);
				mcChan.channelMutex.unlock();
				return {};
			}
		}

    	// frames in framesAfterSecurityProcessing queue
	    if (!mcChan.framesAfterSecurityProcessing.isEmpty()) {
    		frameTmPtr = mcChan.framesAfterSecurityProcessing.getFront();

    		if (frameTmPtr->getOperationalControlFieldFlag()) {
			    if (!mcChan.ocfSduQueue.isEmpty()) {
			    	// ocf sdu exists
			    	frameTmPtr->setOperationalControlField(mcChan.ocfSduQueue.getFront());
			    	mcChan.ocfSduQueue.pop();
			    	mcChan.framesAfterSecurityProcessing.pop();
					mcChan.framesAfterMcGeneration.push(frameTmPtr);
			    	mcChan.channelMutex.unlock();
					return {};
				}

				// could not find an available ocf sdu, so push to circular buffer and return
    			bool discardedFrame = false;
				if (mcChan.waitingBuffer.isFull()) {
					// if the circular buffer is full, we first need to discard the frame in the front
					if (!Objects::frameOctetPool.poolMutex.tryLockFor(Defs::MutexDelayMs)) {
						mcChan.channelMutex.unlock();
						return etl::unexpected(ServiceChannelNotification::FAILED_TO_LOCK_MUTEX);
					}

					discardedFrame = true;
					TransferFrameTM* frameToDiscard = mcChan.waitingBuffer.getFront();
					Objects::frameOctetPool.deleteBlock(frameToDiscard->getFrameData(), phyChan.getTMFrameLength());
					mcChan.frameMasterCopies.erase(frameToDiscard);

					Objects::frameOctetPool.poolMutex.unlock();
				}
    			mcChan.waitingBuffer.push(frameTmPtr);

    			// if a frame is discarded, then notify about it (it is implied that an ocf sdu was
    			// not available)
    			mcChan.framesAfterSecurityProcessing.pop();
    			mcChan.channelMutex.unlock();
    			if (discardedFrame) {
    				return etl::unexpected(ServiceChannelNotification::DISCARDED_FRAME);
    			}
    			return etl::unexpected(ServiceChannelNotification::OCF_SDU_QUEUE_EMPTY);
    		} else {
    			// Next frame in queue has no ocf field, just push to the next queue
    			mcChan.framesAfterSecurityProcessing.pop();
    			mcChan.framesAfterMcGeneration.push(frameTmPtr);
    			mcChan.channelMutex.unlock();
    			return {};
    		}
    	}

		// If we reached this point then no frames were found for processing
    	mcChan.channelMutex.unlock();
    	return etl::unexpected(ServiceChannelNotification::FRAME_QUEUE_EMPTY);
    }

	etl::expected<void, ServiceChannelNotification> SpaceSegmentTmDataHandling::allFramesGeneration(
	    const PhysicalChannel &phyChan,
	    MasterChannelSsTm &mcChan,
	    uint8_t *frameDestination) {
	    if (!mcChan.channelMutex.tryLockFor(Defs::MutexDelayMs)) {
	        return etl::unexpected(ServiceChannelNotification::FAILED_TO_LOCK_MUTEX);
	    }

    	if (!Objects::frameOctetPool.poolMutex.tryLockFor(Defs::MutexDelayMs)) {
    		mcChan.channelMutex.unlock();
    		return etl::unexpected(ServiceChannelNotification::FAILED_TO_LOCK_MUTEX);
    	}

	    // ensure there is at least one frame to send
	    if (mcChan.framesAfterMcGeneration.isEmpty()) {
	    	Objects::frameOctetPool.poolMutex.unlock();
	        mcChan.channelMutex.unlock();
	        return etl::unexpected(ServiceChannelNotification::FRAME_QUEUE_EMPTY);
	    }

	    TransferFrameTM* frameTmPtr = mcChan.framesAfterMcGeneration.getFront();

	    const Defs::VcidScidKey key = constructVcidScidKey(frameTmPtr->getVirtualChannelId(),
	            frameTmPtr->getSpacecraftId());
	    const uint8_t numRepetitions = Objects::virtualChannelSsTmMap.at(key).getVcRepetitions();

	    // check if the frame was retransmitted enough times
	    if (frameTmPtr->getTimesSequentiallyTransmitted() >= numRepetitions) {
	        // remove the frame from the queue and erase the master copy
	        mcChan.framesAfterMcGeneration.pop();
	        Objects::frameOctetPool.deleteBlock(frameTmPtr->getFrameData(), phyChan.getTMFrameLength());
	        mcChan.frameMasterCopies.erase(frameTmPtr);

	        // if queue is empty, return
	        if (mcChan.framesAfterMcGeneration.isEmpty()) {
	        	Objects::frameOctetPool.poolMutex.unlock();
	            mcChan.channelMutex.unlock();
	            return etl::unexpected(ServiceChannelNotification::FRAME_QUEUE_EMPTY);
	        }

	        // fetch the next frame, append CRC if needed
	        frameTmPtr = mcChan.framesAfterMcGeneration.getFront();
	        if (phyChan.getFrameErrorControlFieldPresent()) {
	            frameTmPtr->appendCRC();
	        }
	    }

	    // copy the frame to the user supplied buffer
	    memcpy(frameDestination, frameTmPtr->getFrameData(), phyChan.getTMFrameLength());

	    // increment repetition count and return
	    frameTmPtr->incrementTimesSequentiallyTransmitted();
    	Objects::frameOctetPool.poolMutex.unlock();
	    mcChan.channelMutex.unlock();
	    return {};
	}
#endif // INCLUDE_SPACE_SEGMENT_CODE
} // CCSDSDataLinkLayer

