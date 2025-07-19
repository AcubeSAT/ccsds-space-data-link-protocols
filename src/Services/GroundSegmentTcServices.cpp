#include "GroundSegmentTcServices.hpp"
#include "SpaceSegmentTmDataHandlingFunctions.hpp"
#include "GroundSegmentTcDataHandlingFunctions.hpp"

namespace CCSDSDataLinkLayer {
#ifdef INCLUDE_GROUND_SEGMENT_CODE
        etl::expected<void, ServiceChannelNotification> GroundSegmentTcServices::mapChannelPacketServiceRequest(
            Objects::MapChannelName mapChanName,
            etl::span<uint8_t> packet,
            Defs::ServiceType serviceType) {

            if (serviceType != Defs::ServiceType::TYPE_AD && serviceType != Defs::ServiceType::TYPE_BD) {
                return etl::unexpected(ServiceChannelNotification::INVALID_FRAME_SERVICE_TYPE);
            }

            const Defs::MapidVcidScidKey key = static_cast<Defs::MapidVcidScidKey>(mapChanName);
            if (!Objects::mapChannelGsMap.contains(key)) {
                return etl::unexpected(ServiceChannelNotification::INVALID_CHANNEL_NAME);
            }

            MAPChannelGs &mapChan = Objects::mapChannelGsMap.at(key);
            if (mapChan.getDataFieldContent() != Defs::DataFieldContent::PACKET) {
                return etl::unexpected(ServiceChannelNotification::UNSUPPORTED_SERVICE);
            }

            MasterChannelGsTc& mcChan = Objects::masterChannelGsTcMap.at(mapChan.getParentScid());
            const PhysicalChannel& phyChan = Objects::physicalChannelMap.at(mcChan.getParentPcid());

            return GroundSegmentTcDataHandling::storePacket(phyChan, mapChan, packet, serviceType);
        }

        etl::expected<void, ServiceChannelNotification> GroundSegmentTcServices::mapChannelAccessServiceRequest(
            Objects::MapChannelName mapChanName,
            etl::span<uint8_t> vcaSdu,
            Defs::ServiceType serviceType) {

            if (serviceType != Defs::ServiceType::TYPE_AD && serviceType != Defs::ServiceType::TYPE_BD) {
                return etl::unexpected(ServiceChannelNotification::INVALID_FRAME_SERVICE_TYPE);
            }

            const Defs::MapidVcidScidKey key = static_cast<Defs::MapidVcidScidKey>(mapChanName);
            if (!Objects::mapChannelGsMap.contains(key)) {
                return etl::unexpected(ServiceChannelNotification::INVALID_CHANNEL_NAME);
            }

            MAPChannelGs &mapChan = Objects::mapChannelGsMap.at(key);
            if (mapChan.getDataFieldContent() != Defs::DataFieldContent::VCA_SDU) {
                return etl::unexpected(ServiceChannelNotification::UNSUPPORTED_SERVICE);
            }

            MasterChannelGsTc& mcChan = Objects::masterChannelGsTcMap.at(mapChan.getParentScid());
            const PhysicalChannel& phyChan = Objects::physicalChannelMap.at(mcChan.getParentPcid());

            return GroundSegmentTcDataHandling::storeVcaSdu(phyChan, mapChan, vcaSdu, serviceType);
        }

        etl::expected<void, ServiceChannelNotification> GroundSegmentTcServices::virtualChannelPacketServiceRequest(
            Objects::VirtualChannelTmName vcChanName,
            etl::span<uint8_t> packet,
            Defs::ServiceType serviceType) {

            if (serviceType != Defs::ServiceType::TYPE_AD && serviceType != Defs::ServiceType::TYPE_BD) {
                return etl::unexpected(ServiceChannelNotification::INVALID_FRAME_SERVICE_TYPE);
            }

            const Defs::VcidScidKey key = static_cast<Defs::VcidScidKey>(vcChanName);
            if (!Objects::virtualChannelGsTcMap.contains(key)) {
                return etl::unexpected(ServiceChannelNotification::INVALID_CHANNEL_NAME);
            }

            VirtualChannelGsTc &vcChan = Objects::virtualChannelGsTcMap.at(key);
            if (vcChan.getDataFieldContent() != Defs::DataFieldContent::PACKET || vcChan.getsegmentHeaderPresent()) {
                return etl::unexpected(ServiceChannelNotification::UNSUPPORTED_SERVICE);
            }

            MasterChannelGsTc& mcChan = Objects::masterChannelGsTcMap.at(vcChan.getParentScid());
            const PhysicalChannel& phyChan = Objects::physicalChannelMap.at(mcChan.getParentPcid());

            return GroundSegmentTcDataHandling::storePacket(phyChan, vcChan, packet, serviceType);
        }

        etl::expected<void, ServiceChannelNotification> GroundSegmentTcServices::virtualChannelAccessServiceRequest(
            Objects::VirtualChannelTmName vcChanName,
            etl::span<uint8_t> vcaSdu,
            Defs::ServiceType serviceType) {

            if (serviceType != Defs::ServiceType::TYPE_AD && serviceType != Defs::ServiceType::TYPE_BD) {
                return etl::unexpected(ServiceChannelNotification::INVALID_FRAME_SERVICE_TYPE);
            }

            const Defs::VcidScidKey key = static_cast<Defs::VcidScidKey>(vcChanName);
            if (!Objects::virtualChannelGsTcMap.contains(key)) {
                return etl::unexpected(ServiceChannelNotification::INVALID_CHANNEL_NAME);
            }

            VirtualChannelGsTc &vcChan = Objects::virtualChannelGsTcMap.at(key);
            if (vcChan.getDataFieldContent() != Defs::DataFieldContent::VCA_SDU || vcChan.getsegmentHeaderPresent()) {
                return etl::unexpected(ServiceChannelNotification::UNSUPPORTED_SERVICE);
            }

            MasterChannelGsTc& mcChan = Objects::masterChannelGsTcMap.at(vcChan.getParentScid());
            const PhysicalChannel& phyChan = Objects::physicalChannelMap.at(mcChan.getParentPcid());

            return GroundSegmentTcDataHandling::storePacket(phyChan, vcChan, vcaSdu, serviceType);
        }

        etl::expected<void, ServiceChannelNotification> GroundSegmentTcServices::spaceSegmentTcProcessing(
            Objects::MasterChannelTcName mcChanName) {
            const uint16_t key = static_cast<uint16_t>(mcChanName);
            if (!Objects::masterChannelGsTcMap.contains(key)) {
                return etl::unexpected(ServiceChannelNotification::INVALID_CHANNEL_NAME);
            }

            MasterChannelGsTc& mcChan = Objects::masterChannelGsTcMap.at(key);
            const PhysicalChannel& phyChan = Objects::physicalChannelMap.at(mcChan.getParentPcid());

            etl::expected<void, ServiceChannelNotification> status;
            for (auto &pair : Objects::mapChannelGsMap) {
                // Act only on map channels that belong to this master channel
                if (pair.second.getParentScid() == mcChan.getScid()) {
                    // Possible return notifications and actions
                    // FAILED_TO_LOCK_MUTEX -> return to notify user
                    // PACKET_QUEUE_EMPTY -> no packets were available, no action to be taken
                    // NOT_ENOUGH_SPACE_IN_MASTER_COPY_OR_MEMORY_POOL, FRAME_QUEUE_FULL, NOT_ENOUGH_SPACE_IN_MEMORY_POOL ->
                    //  there is congestion in the channel, no action to be taken
                    status = GroundSegmentTcDataHandling::packetProcessing(phyChan, mcChan, pair.second);

                    if (!status.has_value()) {
                        if (status.error() == ServiceChannelNotification::FAILED_TO_LOCK_MUTEX) {
                            return etl::unexpected(ServiceChannelNotification::FAILED_TO_LOCK_MUTEX);
                        }
                    }
                }
            }

            for (auto &pair : Objects::virtualChannelGsTcMap) {
                // Act only on virtual channels that belong to this master channel. Furthermore, packet processing
                // will be performed on virtual channels that did not have a map channel
                if (pair.second.getParentScid() == mcChan.getScid()) {
                    status = GroundSegmentTcDataHandling::packetProcessing(phyChan, mcChan, pair.second);

                    if (!status.has_value()) {
                        if (status.error() == ServiceChannelNotification::FAILED_TO_LOCK_MUTEX) {
                            return etl::unexpected(ServiceChannelNotification::FAILED_TO_LOCK_MUTEX);
                        }
                    }

                    // Possible return notifications and actions
                    // FAILED_TO_LOCK_MUTEX -> return to notify user
                    // FRAME_QUEUE_EMPTY -> upper layer queue empty, break loop
                    // FRAME_QUEUE_FULL -> lower layer queue full, break loop
                    do {
                        status = GroundSegmentTcDataHandling::applySDLSSecurity(phyChan, pair.second);
                    } while (status.has_value());

                    if (status.error() == ServiceChannelNotification::FAILED_TO_LOCK_MUTEX) {
                        return etl::unexpected(ServiceChannelNotification::FAILED_TO_LOCK_MUTEX);
                    }

                    do {
                        status = GroundSegmentTcDataHandling::virtualChannelGeneration(mcChan, pair.second);
                    } while (status.has_value());

                    if (status.error() == ServiceChannelNotification::FAILED_TO_LOCK_MUTEX) {
                        return etl::unexpected(ServiceChannelNotification::FAILED_TO_LOCK_MUTEX);
                    }
                }
            }
        }

        etl::expected<void, ServiceChannelNotification> GroundSegmentTcServices::getReadyFrameForTransmission(
            Objects::MasterChannelTcName mcChanName,
            uint8_t* packetDestination) {

            const uint16_t key = static_cast<uint16_t>(mcChanName);
            if (!Objects::masterChannelGsTcMap.contains(key)) {
                return etl::unexpected(ServiceChannelNotification::INVALID_CHANNEL_NAME);
            }

            MasterChannelGsTc& mcChan = Objects::masterChannelGsTcMap.at(key);
            const PhysicalChannel& phyChan = Objects::physicalChannelMap.at(mcChan.getParentPcid());
            return GroundSegmentTcDataHandling::allFramesGeneration(phyChan, mcChan, packetDestination);
        }

        etl::expected<void, ServiceChannelNotification> GroundSegmentTcServices::copManagementServiceDirectiveRequest(
            Objects::VirtualChannelTcName vcChanName,
            DirectiveRequestSignal directive
            ) {
            const Defs::VcidScidKey key = static_cast<Defs::VcidScidKey>(vcChanName);
            if (!Objects::virtualChannelGsTcMap.contains(key)) {
                return etl::unexpected(ServiceChannelNotification::INVALID_CHANNEL_NAME);
            }

            const VirtualChannelGsTc& vcChan = Objects::virtualChannelGsTcMap.at(key);
            if (!vcChan.getCopInEffect() || !Objects::fopMap.contains(key)) {
                return etl::unexpected(ServiceChannelNotification::COP_INACTIVE);
            }

            FrameOperationProcedure& fop = Objects::fopMap.at(key);
            if (!fop.signalQueueMutex.tryLockFor(Defs::MutexDelayMs)) {
                return etl::unexpected(ServiceChannelNotification::FAILED_TO_LOCK_MUTEX);
            }

            if (!fop.directiveRequestSignalQueue.full()) {
                fop.directiveRequestSignalQueue.push(directive);
                fop.signalQueueMutex.unlock();
                return {};
            }

            fop.signalQueueMutex.unlock();
            return etl::unexpected(ServiceChannelNotification::DIRECTIVE_QUEUE_FULL);
        }

        etl::expected<DirectiveNotificationSignalUser, ServiceChannelNotification> GroundSegmentTcServices::copManagementServiceDirectiveNotify(
            Objects::VirtualChannelTcName vcChanName
            ) {
            const Defs::VcidScidKey key = static_cast<Defs::VcidScidKey>(vcChanName);
            if (!Objects::virtualChannelGsTcMap.contains(key)) {
                return etl::unexpected(ServiceChannelNotification::INVALID_CHANNEL_NAME);
            }

            const VirtualChannelGsTc& vcChan = Objects::virtualChannelGsTcMap.at(key);
            if (!vcChan.getCopInEffect() || !Objects::fopMap.contains(key)) {
                return etl::unexpected(ServiceChannelNotification::COP_INACTIVE);
            }

            FrameOperationProcedure& fop = Objects::fopMap.at(key);
            if (!fop.signalQueueMutex.tryLockFor(Defs::MutexDelayMs)) {
                return etl::unexpected(ServiceChannelNotification::FAILED_TO_LOCK_MUTEX);
            }

            if (!fop.directiveNotificationSignalQueueUser.empty()) {
                DirectiveNotificationSignalUser signal = fop.directiveNotificationSignalQueueUser.front();
                fop.directiveNotificationSignalQueueUser.pop();
                fop.signalQueueMutex.unlock();
                return signal;
            }

            fop.signalQueueMutex.unlock();
            return etl::unexpected(ServiceChannelNotification::NO_PENDING_NOTIFICATION);
        }

        etl::expected<AsynchronousNotificationSignal, ServiceChannelNotification> GroundSegmentTcServices::copManagementServiceAsyncNotification(
            Objects::VirtualChannelTcName vcChanName
            ) {
            const Defs::VcidScidKey key = static_cast<Defs::VcidScidKey>(vcChanName);
            if (!Objects::virtualChannelGsTcMap.contains(key)) {
                return etl::unexpected(ServiceChannelNotification::INVALID_CHANNEL_NAME);
            }

            const VirtualChannelGsTc& vcChan = Objects::virtualChannelGsTcMap.at(key);
            if (!vcChan.getCopInEffect() || !Objects::fopMap.contains(key)) {
                return etl::unexpected(ServiceChannelNotification::COP_INACTIVE);
            }

            FrameOperationProcedure& fop = Objects::fopMap.at(key);
            if (!fop.signalQueueMutex.tryLockFor(Defs::MutexDelayMs)) {
                return etl::unexpected(ServiceChannelNotification::FAILED_TO_LOCK_MUTEX);
            }

            if (!fop.asynchronousNotificationSignalQueue.empty()) {
                AsynchronousNotificationSignal signal = fop.asynchronousNotificationSignalQueue.front();
                fop.asynchronousNotificationSignalQueue.pop();
                fop.signalQueueMutex.unlock();
                return signal;
            }

            fop.signalQueueMutex.unlock();
            return etl::unexpected(ServiceChannelNotification::NO_PENDING_NOTIFICATION);
        }

        etl::expected<void, ServiceChannelNotification> GroundSegmentTcServices::copManagementServicePushCLCW(
            Objects::MasterChannelTcName mcChanName,
            uint32_t clcw
            ) {
            const uint16_t key = static_cast<uint16_t>(mcChanName);
            if (!Objects::virtualChannelGsTcMap.contains(key)) {
                return etl::unexpected(ServiceChannelNotification::INVALID_CHANNEL_NAME);
            }

            auto clcwObj = CLCW(clcw);
            if (clcwObj.getControlWordType() != Defs::ControlWordTypeCLCW ||
                clcwObj.getClcwVersion() != Defs::ClcwVersionNumber) {
                return etl::unexpected(ServiceChannelNotification::INVALID_CLCW);
            }

            for (auto& pair : Objects::fopMap) {
                if (!pair.second.vcChan->getVcid() == clcwObj.getVcId()) {
                    continue;
                }

                // found correct fop
                if (!pair.second.signalQueueMutex.tryLockFor(Defs::MutexDelayMs)) {
                    return etl::unexpected(ServiceChannelNotification::FAILED_TO_LOCK_MUTEX);
                }

                pair.second.clcwBuffer.emplace(clcwObj);
                pair.second.signalQueueMutex.unlock();
                return {};
            }

            // if we reach this point, then a virtual channel with the specific vcid was not found
            return etl::unexpected(ServiceChannelNotification::INVALID_CLCW_VCID);
        }

        etl::expected<void, ServiceChannelNotification> GroundSegmentTcServices::executeFopStateMachines(
            Objects::MasterChannelTcName mcChanName,
            etl::optional<etl::span<FopOutputData>> fopDataVector) {
            const uint16_t scid = static_cast<uint16_t>(mcChanName);
            if (!Objects::virtualChannelGsTcMap.contains(scid)) {
                return etl::unexpected(ServiceChannelNotification::INVALID_CHANNEL_NAME);
            }

            FopOutputData* fopDataPtr;
            if (fopDataVector.has_value()) {
                fopDataPtr = fopDataVector.value().data();
            }

            for (auto& keyFopPair : Objects::fopMap) {
                const Defs::VcidScidKey key = keyFopPair.first;

                if (etl::get<1>(extractVcidScid(key)) == scid) {
                    // found FOP that belongs to this master channel
                    etl::pair<FOPNotification, uint8_t> status = keyFopPair.second.applyFopStateTable();

                    // construct outputData tuple
                    if (fopDataVector.has_value()) {
                        *fopDataPtr = FopOutputData{key, status.first, keyFopPair.second.getCurrentState(), status.second};
                        fopDataPtr++;
                    }
                }
            }

            return {};
        }

#endif // INCLUDE_GROUND_SEGMENT_CODE
} // CCSDSDataLinkLayer