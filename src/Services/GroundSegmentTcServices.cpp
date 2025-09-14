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
            const auto it = Objects::mapChannelGsMap.find(key);
            if (it == Objects::mapChannelGsMap.end()) {
                return etl::unexpected(ServiceChannelNotification::INVALID_CHANNEL_NAME);
            }
            MAPChannelGs &mapChan = it->second;

            if (mapChan.getDataFieldContent() != Defs::DataFieldContent::PACKET) {
                return etl::unexpected(ServiceChannelNotification::UNSUPPORTED_SERVICE);
            }

            MasterChannelGsTc& mcChan = Objects::masterChannelGsTcMap.at(mapChan.getParentScid());
            const PhysicalChannel& phyChan = Objects::physicalChannelMap.at(mcChan.getParentPcid());

            return GroundSegmentTcDataHandling::storePacket(phyChan, etl::reference_wrapper{mapChan}, packet, serviceType);
        }

        etl::expected<void, ServiceChannelNotification> GroundSegmentTcServices::mapChannelAccessServiceRequest(
            Objects::MapChannelName mapChanName,
            etl::span<uint8_t> vcaSdu,
            Defs::ServiceType serviceType) {

            if (serviceType != Defs::ServiceType::TYPE_AD && serviceType != Defs::ServiceType::TYPE_BD) {
                return etl::unexpected(ServiceChannelNotification::INVALID_FRAME_SERVICE_TYPE);
            }

            const Defs::MapidVcidScidKey key = static_cast<Defs::MapidVcidScidKey>(mapChanName);
            const auto it = Objects::mapChannelGsMap.find(key);
            if (it == Objects::mapChannelGsMap.end()) {
                return etl::unexpected(ServiceChannelNotification::INVALID_CHANNEL_NAME);
            }
            MAPChannelGs &mapChan = it->second;

            if (mapChan.getDataFieldContent() != Defs::DataFieldContent::VCA_SDU) {
                return etl::unexpected(ServiceChannelNotification::UNSUPPORTED_SERVICE);
            }

            MasterChannelGsTc& mcChan = Objects::masterChannelGsTcMap.at(mapChan.getParentScid());
            const PhysicalChannel& phyChan = Objects::physicalChannelMap.at(mcChan.getParentPcid());

            return GroundSegmentTcDataHandling::storeVcaSdu(phyChan, etl::reference_wrapper{mapChan}, vcaSdu, serviceType);
        }

        etl::expected<void, ServiceChannelNotification> GroundSegmentTcServices::virtualChannelPacketServiceRequest(
            Objects::VirtualChannelTmName vcChanName,
            etl::span<uint8_t> packet,
            Defs::ServiceType serviceType) {

            if (serviceType != Defs::ServiceType::TYPE_AD && serviceType != Defs::ServiceType::TYPE_BD) {
                return etl::unexpected(ServiceChannelNotification::INVALID_FRAME_SERVICE_TYPE);
            }

            const Defs::VcidScidKey key = static_cast<Defs::VcidScidKey>(vcChanName);
            const auto it = Objects::virtualChannelGsTcMap.find(key);
            if (it == Objects::virtualChannelGsTcMap.end()) {
                return etl::unexpected(ServiceChannelNotification::INVALID_CHANNEL_NAME);
            }
            VirtualChannelGsTc &vcChan = it->second;

            if (vcChan.getDataFieldContent() != Defs::DataFieldContent::PACKET || vcChan.getSegmentHeaderPresent()) {
                return etl::unexpected(ServiceChannelNotification::UNSUPPORTED_SERVICE);
            }

            MasterChannelGsTc& mcChan = Objects::masterChannelGsTcMap.at(vcChan.getParentScid());
            const PhysicalChannel& phyChan = Objects::physicalChannelMap.at(mcChan.getParentPcid());

            return GroundSegmentTcDataHandling::storePacket(phyChan, etl::reference_wrapper{vcChan}, packet, serviceType);
        }

        etl::expected<void, ServiceChannelNotification> GroundSegmentTcServices::virtualChannelAccessServiceRequest(
            Objects::VirtualChannelTmName vcChanName,
            etl::span<uint8_t> vcaSdu,
            Defs::ServiceType serviceType) {

            if (serviceType != Defs::ServiceType::TYPE_AD && serviceType != Defs::ServiceType::TYPE_BD) {
                return etl::unexpected(ServiceChannelNotification::INVALID_FRAME_SERVICE_TYPE);
            }

            const Defs::VcidScidKey key = static_cast<Defs::VcidScidKey>(vcChanName);
            const auto it = Objects::virtualChannelGsTcMap.find(key);
            if (it == Objects::virtualChannelGsTcMap.end()) {
                return etl::unexpected(ServiceChannelNotification::INVALID_CHANNEL_NAME);
            }
            VirtualChannelGsTc &vcChan = it->second;

            if (vcChan.getDataFieldContent() != Defs::DataFieldContent::VCA_SDU || vcChan.getSegmentHeaderPresent()) {
                return etl::unexpected(ServiceChannelNotification::UNSUPPORTED_SERVICE);
            }

            MasterChannelGsTc& mcChan = Objects::masterChannelGsTcMap.at(vcChan.getParentScid());
            const PhysicalChannel& phyChan = Objects::physicalChannelMap.at(mcChan.getParentPcid());

            return GroundSegmentTcDataHandling::storeVcaSdu(phyChan, etl::reference_wrapper{vcChan}, vcaSdu, serviceType);
        }

        etl::expected<void, ServiceChannelNotification> GroundSegmentTcServices::groundSegmentTcProcessing(
            Objects::PhysicalChannelName physicalChannelName) {

            const auto it = Objects::physicalChannelMap.find(static_cast<uint8_t>(physicalChannelName));
            if (it == Objects::physicalChannelMap.end()) {
                return etl::unexpected(ServiceChannelNotification::INVALID_CHANNEL_NAME);
            }
            const PhysicalChannel& phyChan = it->second;
            MasterChannelGsTc& mcChan = Objects::masterChannelGsTcMap.at(phyChan.getScidTc());

            etl::expected<void, ServiceChannelNotification> status;
            for (auto &mapidVcidScidKey : Objects::mapChannelGsPrioritySortedKeys) {
                // Act only on map channels that belong to this master channel
                MAPChannelGs& mapChan = Objects::mapChannelGsMap.at(mapidVcidScidKey);
                if (mapChan.getParentScid() == mcChan.getScid()) {
                    // Possible return notifications and actions
                    // FAILED_TO_LOCK_MUTEX -> return to notify user
                    // PACKET_QUEUE_EMPTY -> no packets were available, no action to be taken
                    // NOT_ENOUGH_SPACE_IN_MASTER_COPY_OR_MEMORY_POOL, FRAME_QUEUE_FULL, NOT_ENOUGH_SPACE_IN_MEMORY_POOL ->
                    //  there is congestion in the channel, no action to be taken
                    status = GroundSegmentTcDataHandling::packetProcessing(phyChan, mcChan, etl::reference_wrapper{mapChan}, Defs::ServiceType::TYPE_AD);
                    if (!status.has_value()) {
                        if (status.error() == ServiceChannelNotification::FAILED_TO_LOCK_MUTEX) {
                            return etl::unexpected(ServiceChannelNotification::FAILED_TO_LOCK_MUTEX);
                        }
                    }

                    status = GroundSegmentTcDataHandling::packetProcessing(phyChan, mcChan, etl::reference_wrapper{mapChan}, Defs::ServiceType::TYPE_BD);
                    if (!status.has_value()) {
                        if (status.error() == ServiceChannelNotification::FAILED_TO_LOCK_MUTEX) {
                            return etl::unexpected(ServiceChannelNotification::FAILED_TO_LOCK_MUTEX);
                        }
                    }
                }
            }

            for (auto &vcidScidKey : Objects::virtualChannelGsTcPrioritySortedKeys) {
                // Act only on virtual channels that belong to this master channel. Furthermore, packet processing
                // will be performed only on virtual channels that did not have a map channel
                VirtualChannelGsTc& vcChan = Objects::virtualChannelGsTcMap.at(vcidScidKey);
                if (vcChan.getParentScid() == mcChan.getScid()) {
                    status = GroundSegmentTcDataHandling::packetProcessing(phyChan, mcChan, etl::reference_wrapper{vcChan}, Defs::ServiceType::TYPE_AD);
                    if (!status.has_value()) {
                        if (status.error() == ServiceChannelNotification::FAILED_TO_LOCK_MUTEX) {
                            return etl::unexpected(ServiceChannelNotification::FAILED_TO_LOCK_MUTEX);
                        }
                    }

                    status = GroundSegmentTcDataHandling::packetProcessing(phyChan, mcChan, etl::reference_wrapper{vcChan}, Defs::ServiceType::TYPE_BD);
                    if (!status.has_value()) {
                        if (status.error() == ServiceChannelNotification::FAILED_TO_LOCK_MUTEX) {
                            return etl::unexpected(ServiceChannelNotification::FAILED_TO_LOCK_MUTEX);
                        }
                    }

                    // Possible return notifications and actions
                    // FAILED_TO_LOCK_MUTEX -> return to notify user
                    // FRAME_QUEUE_EMPTY -> upper layer queue empty, break loop
                    // FRAME_QUEUE_FULL -> lower layer queue full, break loop
                    // SDLS_CALCULATION_ERROR -> failed to calculate MAC, notify user
                    do {
                        status = GroundSegmentTcDataHandling::applySDLSSecurity(phyChan, vcChan);
                    } while (status.has_value());

                    if (status.error() == ServiceChannelNotification::FAILED_TO_LOCK_MUTEX) {
                        return etl::unexpected(ServiceChannelNotification::FAILED_TO_LOCK_MUTEX);
                    } else if (status.error() == ServiceChannelNotification::SLDS_CALCULATION_ERROR) {
                        return etl::unexpected(ServiceChannelNotification::SLDS_CALCULATION_ERROR);
                    }

                    do {
                        status = GroundSegmentTcDataHandling::virtualChannelGeneration(mcChan, vcChan);
                    } while (status.has_value());

                    if (status.error() == ServiceChannelNotification::FAILED_TO_LOCK_MUTEX) {
                        return etl::unexpected(ServiceChannelNotification::FAILED_TO_LOCK_MUTEX);
                    }
                }
            }

            return {};
        }

        etl::expected<void, ServiceChannelNotification> GroundSegmentTcServices::getReadyFrameForTransmission(
            Objects::PhysicalChannelName physicalChannelName,
            uint8_t* packetDestination) {

            const auto it = Objects::physicalChannelMap.find(static_cast<uint8_t>(physicalChannelName));
            if (it == Objects::physicalChannelMap.end()) {
                return etl::unexpected(ServiceChannelNotification::INVALID_CHANNEL_NAME);
            }
            const PhysicalChannel& phyChan = it->second;
            MasterChannelGsTc& mcChan = Objects::masterChannelGsTcMap.at(phyChan.getScidTc());

            return GroundSegmentTcDataHandling::allFramesGeneration(phyChan, mcChan, packetDestination);
        }

        etl::expected<void, ServiceChannelNotification> GroundSegmentTcServices::copManagementServiceDirectiveRequest(
            Objects::VirtualChannelTcName vcChanName,
            DirectiveRequestSignal directive
            ) {
            const Defs::VcidScidKey key = static_cast<Defs::VcidScidKey>(vcChanName);
            const auto vcIt = Objects::virtualChannelGsTcMap.find(key);
            if (vcIt == Objects::virtualChannelGsTcMap.end()) {
                return etl::unexpected(ServiceChannelNotification::INVALID_CHANNEL_NAME);
            }
            const VirtualChannelGsTc& vcChan = vcIt->second;

            const auto fopIt = Objects::fopMap.find(key);
            if (!vcChan.getCopInEffect() || fopIt == Objects::fopMap.end()) {
                return etl::unexpected(ServiceChannelNotification::COP_INACTIVE);
            }
            FrameOperationProcedure& fop = fopIt->second;

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
            const auto vcIt = Objects::virtualChannelGsTcMap.find(key);
            if (vcIt == Objects::virtualChannelGsTcMap.end()) {
                return etl::unexpected(ServiceChannelNotification::INVALID_CHANNEL_NAME);
            }
            const VirtualChannelGsTc& vcChan = vcIt->second;

            const auto fopIt = Objects::fopMap.find(key);
            if (!vcChan.getCopInEffect() || fopIt == Objects::fopMap.end()) {
                return etl::unexpected(ServiceChannelNotification::COP_INACTIVE);
            }
            FrameOperationProcedure& fop = fopIt->second;

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
            const auto vcIt = Objects::virtualChannelGsTcMap.find(key);
            if (vcIt == Objects::virtualChannelGsTcMap.end()) {
                return etl::unexpected(ServiceChannelNotification::INVALID_CHANNEL_NAME);
            }
            const VirtualChannelGsTc& vcChan = vcIt->second;

            const auto fopIt = Objects::fopMap.find(key);
            if (!vcChan.getCopInEffect() || fopIt == Objects::fopMap.end()) {
                return etl::unexpected(ServiceChannelNotification::COP_INACTIVE);
            }
            FrameOperationProcedure& fop = fopIt->second;

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
            Objects::PhysicalChannelName physicalChannelName,
            uint32_t clcw
            ) {

            const auto it = Objects::physicalChannelMap.find(static_cast<uint8_t>(physicalChannelName));
            if (it == Objects::physicalChannelMap.end()) {
                return etl::unexpected(ServiceChannelNotification::INVALID_CHANNEL_NAME);
            }
            const PhysicalChannel& phyChan = it->second;
            MasterChannelGsTc& mcChan = Objects::masterChannelGsTcMap.at(phyChan.getScidTc());

            auto clcwObj = CLCW(clcw);
            if (clcwObj.getControlWordType() != Defs::ControlWordTypeCLCW ||
                clcwObj.getClcwVersion() != Defs::ClcwVersionNumber) {
                return etl::unexpected(ServiceChannelNotification::INVALID_CLCW);
            }

            for (auto& pair : Objects::fopMap) {
                if (pair.second.getVcid() != clcwObj.getVcId()) {
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
            Objects::PhysicalChannelName physicalChannelName,
            etl::optional<etl::span<FopOutputData>> fopDataVector) {
            const auto it = Objects::physicalChannelMap.find(static_cast<uint8_t>(physicalChannelName));
            if (it == Objects::physicalChannelMap.end()) {
                return etl::unexpected(ServiceChannelNotification::INVALID_CHANNEL_NAME);
            }
            const PhysicalChannel& phyChan = it->second;
            MasterChannelGsTc& mcChan = Objects::masterChannelGsTcMap.at(phyChan.getScidTc());

            FopOutputData* fopDataPtr;
            if (fopDataVector.has_value()) {
                if (fopDataVector.value().size() < Objects::fopMap.size()) {
                    return etl::unexpected(ServiceChannelNotification::INVALID_LENGTH);
                }
                fopDataPtr = fopDataVector.value().data();
            }

            for (auto& keyFopPair : Objects::fopMap) {
                const Defs::VcidScidKey key = keyFopPair.first;

                if (std::get<1>(extractVcidScid(key)) == mcChan.getScid()) {
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

        etl::expected<void, ServiceChannelNotification> GroundSegmentTcServices::resetChain(Objects::PhysicalChannelName physicalChannelName) {
            const auto it = Objects::physicalChannelMap.find(static_cast<uint8_t>(physicalChannelName));
            if (it == Objects::physicalChannelMap.end()) {
                return etl::unexpected(ServiceChannelNotification::INVALID_CHANNEL_NAME);
            }
            const PhysicalChannel& phyChan = it->second;
            MasterChannelGsTc& mcChan = Objects::masterChannelGsTcMap.at(phyChan.getScidTc());

            GroundSegmentTcDataHandling::resetMasterChannel(mcChan);

            for (auto& vcChan : Objects::virtualChannelGsTcMap) {
                if (std::get<1>(extractVcidScid(vcChan.first)) == mcChan.getScid()) {
                    GroundSegmentTcDataHandling::resetVirtualChannel(vcChan.second);
                }
            }

            for (auto& mapChan : Objects::mapChannelGsMap) {
                if (std::get<1>(extractVcidScid(mapChan.first)) == mcChan.getScid()) {
                    GroundSegmentTcDataHandling::resetMapChannel(mapChan.second);
                }
            }
        }
#endif // INCLUDE_GROUND_SEGMENT_CODE
} // CCSDSDataLinkLayer