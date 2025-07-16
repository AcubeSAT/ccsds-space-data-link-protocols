#include "SpaceSegmentTmServices.hpp"
#include "SpaceSegmentTmDataHandlingFunctions.hpp"

namespace CCSDSDataLinkLayer {
#ifdef INCLUDE_SPACE_SEGMENT_CODE
    etl::expected<void, ServiceChannelNotification> SpaceSegmentTmServices::virtualChannelPacketServiceRequest(Objects::VirtualChannelTmName vcChanName,
                etl::span<uint8_t> packet) {

        const Defs::VcidScidKey key = static_cast<Defs::VcidScidKey>(vcChanName);
        if (!Objects::virtualChannelSsTmMap.contains(key)) {
            return etl::unexpected(ServiceChannelNotification::INVALID_CHANNEL_NAME);
        }

        VirtualChannelSsTm &vcChan = Objects::virtualChannelSsTmMap.at(key);
        if (vcChan.getSynchronization() != Defs::SynchronizationFlag::OCTET_SYNCHRONIZED_FORWARD_ORDERED) {
            return etl::unexpected(ServiceChannelNotification::UNSUPPORTED_SERVICE_TYPE);
        }

        return SpaceSegmentTmDataHandling::storePacket(Objects::virtualChannelSsTmMap.at(key), packet);
    }


    etl::expected<void, ServiceChannelNotification> SpaceSegmentTmServices::virtualChannelAccessServiceRequest(
        Objects::VirtualChannelTmName vcChanName,
        etl::span<uint8_t> packet,
        const bool packetOrderFlag,
        const uint8_t segmentLengthIdentifier) {

        const Defs::VcidScidKey key = static_cast<Defs::VcidScidKey>(vcChanName);
        if (!Objects::virtualChannelSsTmMap.contains(key)) {
            return etl::unexpected(ServiceChannelNotification::INVALID_CHANNEL_NAME);
        }

        VirtualChannelSsTm &vcChan = Objects::virtualChannelSsTmMap.at(key);
        if (vcChan.getSynchronization() != Defs::SynchronizationFlag::VCA_SDU) {
            return etl::unexpected(ServiceChannelNotification::UNSUPPORTED_SERVICE_TYPE);
        }

        const MasterChannelSsTm& mcChan = Objects::masterChannelSsTmMap.at(vcChan.getParentScid());
        const PhysicalChannel& phyChan = Objects::physicalChannelMap.at(mcChan.getParentPcid());
        return SpaceSegmentTmDataHandling::storeVcaSdu(phyChan, vcChan, packet, packetOrderFlag, segmentLengthIdentifier);
    }

    etl::expected<void, ServiceChannelNotification> SpaceSegmentTmServices::virtualChannelOperationalControlFieldServiceRequest(
        Objects::MasterChannelTmName mcChanName,
        const uint32_t ocfSdu) {

        const uint16_t key = static_cast<uint16_t>(mcChanName);
        if (!Objects::masterChannelSsTmMap.contains(key)) {
            return etl::unexpected(ServiceChannelNotification::INVALID_CHANNEL_NAME);
        }

        MasterChannelSsTm& mcChan = Objects::masterChannelSsTmMap.at(key);
        if (!mcChan.channelMutex.tryLockFor(Defs::mutexDelayMs)) {
            return etl::unexpected(ServiceChannelNotification::FAILED_TO_LOCK_MUTEX);
        }

        if (mcChan.ocfSduQueue.isFull()) {
            mcChan.channelMutex.unlock();
            return etl::unexpected(ServiceChannelNotification::OCF_SDU_QUEUE_FULL);
        }

        mcChan.ocfSduQueue.push(ocfSdu);
        mcChan.channelMutex.unlock();
        return {};
    }

    etl::expected<void, ServiceChannelNotification> SpaceSegmentTmServices::spaceSegmentTmProcessing(
            Objects::MasterChannelTmName mcChanName) {
        //TODO maybe notify the user if many consecutive congestions occur

        const uint16_t key = static_cast<uint16_t>(mcChanName);
        if (!Objects::masterChannelSsTmMap.contains(key)) {
            return etl::unexpected(ServiceChannelNotification::INVALID_CHANNEL_NAME);
        }

        MasterChannelSsTm& mcChan = Objects::masterChannelSsTmMap.at(key);
        const PhysicalChannel& phyChan = Objects::physicalChannelMap.at(mcChan.getParentPcid());

        etl::expected<void, ServiceChannelNotification> status;
        for (auto &pair : Objects::virtualChannelSsTmMap) {
            // act only on virtual channels that belong to this master channel
            if (pair.second.getParentScid() == mcChan.getScid()) {
                // Possible return notifications and actions
                // FAILED_TO_LOCK_MUTEX -> return to notify user
                // PACKET_QUEUE_EMPTY -> no packets were available, attempt to generate an oid frame
                // NOT_ENOUGH_SPACE_IN_MASTER_COPY_OR_MEMORY_POOL, FRAME_QUEUE_FULL, NOT_ENOUGH_SPACE_IN_MEMORY_POOL ->
                //  there is congestion in the channel, no action to be taken
                status = SpaceSegmentTmDataHandling::virtualChannelGeneration(phyChan, mcChan, pair.second);

                if (!status.has_value()) {
                    if (status.error() == ServiceChannelNotification::FAILED_TO_LOCK_MUTEX) {
                        return etl::unexpected(ServiceChannelNotification::FAILED_TO_LOCK_MUTEX);
                    }

                    if (status.error() == ServiceChannelNotification::PACKET_QUEUE_EMPTY) {
                        // Possible return notifications and actions
                        // FAILED_TO_LOCK_MUTEX -> return to notify user
                        // NOT_ENOUGH_SPACE_IN_MASTER_COPY_OR_MEMORY_POOL, FRAME_QUEUE_FULL, NOT_ENOUGH_SPACE_IN_MEMORY_POOL ->
                        //  there is congestion in the channel, no action to be taken
                        status = SpaceSegmentTmDataHandling::generateOidFrame(phyChan, mcChan, pair.second);

                        if (!status.has_value()) {
                            if (status.error() == ServiceChannelNotification::FAILED_TO_LOCK_MUTEX) {
                                return etl::unexpected(ServiceChannelNotification::FAILED_TO_LOCK_MUTEX);
                            }
                        }
                    }
                }
            }
        }

        status = {};
        // Possible return notifications and actions
        // FAILED_TO_LOCK_MUTEX -> return to notify user
        // DISCARDED_FRAME -> ocf sdu queue was empty for long enough that frames are starting to get discarded, return to notify user
        // FRAME_QUEUE_FULL -> congestion, no action to be taken
        // FRAME_QUEUE_EMPTY -> this what we would normally expect

        do {
            status = SpaceSegmentTmDataHandling::masterChannelGeneration(phyChan, mcChan);
        } while (status.has_value());

        if (status.error() == ServiceChannelNotification::FAILED_TO_LOCK_MUTEX) {
            return etl::unexpected(ServiceChannelNotification::FAILED_TO_LOCK_MUTEX);
        }

        if (status.error() == ServiceChannelNotification::FAILED_TO_LOCK_MUTEX || status.error() == ServiceChannelNotification::DISCARDED_FRAME) {
            return status;
        }

        return {};
    }

    etl::expected<void, ServiceChannelNotification> SpaceSegmentTmServices::getReadyFrameForTransmission(
        Objects::MasterChannelTmName mcChanName,
        uint8_t* packetDestination) {

        const uint16_t key = static_cast<uint16_t>(mcChanName);
        if (!Objects::masterChannelSsTmMap.contains(key)) {
            return etl::unexpected(ServiceChannelNotification::INVALID_CHANNEL_NAME);
        }

        MasterChannelSsTm& mcChan = Objects::masterChannelSsTmMap.at(key);
        const PhysicalChannel& phyChan = Objects::physicalChannelMap.at(mcChan.getParentPcid());
        return SpaceSegmentTmDataHandling::allFramesGeneration(phyChan, mcChan, packetDestination);
    }
#endif // INCLUDE_SPACE_SEGMENT_CODE
} // CCSDSDataLinkLayer