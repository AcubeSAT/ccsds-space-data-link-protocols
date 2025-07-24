#include "SpaceSegmentTcServices.hpp"
#include "SpaceSegmentTcDataHandlingFunctions.hpp"

namespace CCSDSDataLinkLayer {
#ifdef INCLUDE_SPACE_SEGMENT_CODE
        etl::expected<void, ServiceChannelNotification> SpaceSegmentTcServices::insertReceivedFrame(
            Objects::PhysicalChannelName physicalChannelName,
            etl::span<uint8_t> frameSource) {
            const auto it = Objects::physicalChannelMap.find(static_cast<uint8_t>(physicalChannelName));
            if (it == Objects::physicalChannelMap.end()) {
                return etl::unexpected(ServiceChannelNotification::INVALID_CHANNEL_NAME);
            }
            return SpaceSegmentTcDataHandling::allFramesReception(it->second, frameSource);
        }


        etl::expected<void, ServiceChannelNotification> SpaceSegmentTcServices::mapChannelHelperFunc(
            Objects::MapChannelName mapChanName,
            etl::span<uint8_t> packetDestination,
            Defs::ServiceType serviceType,
            Defs::DataFieldContent dataFieldContent) {
            if (serviceType != Defs::ServiceType::TYPE_AD &&
                serviceType != Defs::ServiceType::TYPE_BD) {
                return etl::unexpected(ServiceChannelNotification::INVALID_FRAME_SERVICE_TYPE);
                }

            const Defs::MapidVcidScidKey key = static_cast<Defs::MapidVcidScidKey>(mapChanName);
            const auto it = Objects::mapChannelSsMap.find(key);
            if (it == Objects::mapChannelSsMap.end()) {
                return etl::unexpected(ServiceChannelNotification::INVALID_CHANNEL_NAME);
            }
            MAPChannelSs &mapChan = it->second;

            if (mapChan.getDataFieldContent() != dataFieldContent) {
                return etl::unexpected(ServiceChannelNotification::UNSUPPORTED_SERVICE);
            }

            MasterChannelSsTc &mcChan = Objects::masterChannelSsTcMap.at(mapChan.getParentScid());
            PhysicalChannel &phyChan = Objects::physicalChannelMap.at(mcChan.getScid());

            return SpaceSegmentTcDataHandling::packetExtraction(phyChan, etl::reference_wrapper{mapChan}, serviceType, packetDestination.data());
        }

        etl::expected<void, ServiceChannelNotification> SpaceSegmentTcServices::mapChannelPacketServiceIndication(
            Objects::MapChannelName mapChanName,
            etl::span<uint8_t> packetDestination,
            Defs::ServiceType serviceType) {
            return mapChannelHelperFunc(mapChanName, packetDestination, serviceType, Defs::DataFieldContent::PACKET);
        }

        etl::expected<void, ServiceChannelNotification> SpaceSegmentTcServices::mapChannelAccessServiceIndication(
            Objects::MapChannelName mapChanName,
            etl::span<uint8_t> vcaSduDestination,
            Defs::ServiceType serviceType) {
            return mapChannelHelperFunc(mapChanName, vcaSduDestination, serviceType, Defs::DataFieldContent::VCA_SDU);
        }

        etl::expected<void, ServiceChannelNotification> SpaceSegmentTcServices::virtualChannelHelperFunc(
            Objects::VirtualChannelTcName vcChanName,
            etl::span<uint8_t> packetDestination,
            Defs::ServiceType serviceType,
            Defs::DataFieldContent dataFieldContent) {
            if (serviceType != Defs::ServiceType::TYPE_AD &&
                serviceType != Defs::ServiceType::TYPE_BD) {
                return etl::unexpected(ServiceChannelNotification::INVALID_FRAME_SERVICE_TYPE);
                }

            const Defs::VcidScidKey key = static_cast<Defs::VcidScidKey>(vcChanName);
            const auto it = Objects::virtualChannelSsTcMap.find(key);
            if (it == Objects::virtualChannelSsTcMap.end()) {
                return etl::unexpected(ServiceChannelNotification::INVALID_CHANNEL_NAME);
            }
            VirtualChannelSsTc &vcChan = it->second;

            if (vcChan.getDataFieldContent() != dataFieldContent) {
                return etl::unexpected(ServiceChannelNotification::UNSUPPORTED_SERVICE);
            }

            MasterChannelSsTc &mcChan = Objects::masterChannelSsTcMap.at(vcChan.getParentScid());
            PhysicalChannel &phyChan = Objects::physicalChannelMap.at(mcChan.getScid());

            return SpaceSegmentTcDataHandling::packetExtraction(phyChan, etl::reference_wrapper{vcChan}, serviceType, packetDestination.data());
        }

        etl::expected<void, ServiceChannelNotification> SpaceSegmentTcServices::virtualChannelPacketServiceIndication(
            Objects::VirtualChannelTcName vcChanName,
            etl::span<uint8_t> packetDestination,
            Defs::ServiceType serviceType) {
            return virtualChannelHelperFunc(vcChanName, packetDestination, serviceType, Defs::DataFieldContent::PACKET);
        }

        etl::expected<void, ServiceChannelNotification> SpaceSegmentTcServices::virtualChannelAccessServiceIndication(
            Objects::VirtualChannelTcName vcChanName,
            etl::span<uint8_t> vcaSduDestination,
            Defs::ServiceType serviceType) {
            return virtualChannelHelperFunc(vcChanName, vcaSduDestination, serviceType, Defs::DataFieldContent::VCA_SDU);
        }

        etl::expected<void, ServiceChannelNotification> SpaceSegmentTcServices::spaceSegmentTcProcessing(
            Objects::PhysicalChannelName physicalChannelName) {
            const auto it = Objects::physicalChannelMap.find(static_cast<uint8_t>(physicalChannelName));
            if (it == Objects::physicalChannelMap.end()) {
                return etl::unexpected(ServiceChannelNotification::INVALID_CHANNEL_NAME);
            }
            PhysicalChannel& phyChan = it->second;
            uint16_t scid = phyChan.getScidTc();

            for (auto &pair : Objects::virtualChannelSsTcMap) {
                // only act on channels that belong to this master channel
                if (etl::get<1>(extractVcidScid(pair.first)) == scid) {
                    // Possible return notifications and actions
                    // NO_SERVICE_EVENT -> no errors encountered, no action
                    // FARM_ERROR -> // TODO
                    // FAILED_TO_LOCK_MUTEX -> return to notify user
                    // FRAME_QUEUE_EMPTY -> no frames were available, no action
                    // FRAME_QUEUE_FULL -> there is congestion in the channel, no action
                    etl::pair<ServiceChannelNotification, uint8_t> vcReceptionStatus;
                    do {
                        vcReceptionStatus = SpaceSegmentTcDataHandling::virtualChannelReception(pair.second);
                    } while (vcReceptionStatus.first == ServiceChannelNotification::NO_SERVICE_EVENT);

                    if (vcReceptionStatus.first == ServiceChannelNotification::FAILED_TO_LOCK_MUTEX) {
                        return etl::unexpected(ServiceChannelNotification::FAILED_TO_LOCK_MUTEX);
                    }

                    // Possible return notifications and actions
                    // NO_SERVICE_EVENT -> no errors encountered, no action
                    // SDLS_CALCULATION_ERROR -> HMAC could not be calculated, notify user
                    // UNAUTHORIZED_SENDER -> frame with invalid spi/wrong HMAC/same sequence number was detected, notify user
                    // FAILED_TO_LOCK_MUTEX -> return to notify user
                    // FRAME_QUEUE_EMPTY -> no frames were available, no action
                    // FRAME_QUEUE_FULL -> there is congestion in the channel, no action
                    etl::expected<void, ServiceChannelNotification> sldsSecStatus;
                    do {
                        sldsSecStatus = SpaceSegmentTcDataHandling::processSdlsSecurity(phyChan, pair.second, Defs::ServiceType::TYPE_AD);
                    } while (sldsSecStatus.has_value());

                    if (!sldsSecStatus.has_value()) {
                        if (sldsSecStatus.error() == ServiceChannelNotification::FAILED_TO_LOCK_MUTEX ||
                            sldsSecStatus.error() == ServiceChannelNotification::SLDS_CALCULATION_ERROR ||
                            sldsSecStatus.error() == ServiceChannelNotification::UNAUTHORIZED_SENDER) {
                            return etl::unexpected(sldsSecStatus.error());
                        }
                    }

                    do {
                        sldsSecStatus = SpaceSegmentTcDataHandling::processSdlsSecurity(phyChan, pair.second, Defs::ServiceType::TYPE_BD);
                    } while (sldsSecStatus.has_value());

                    if (!sldsSecStatus.has_value()) {
                        if (sldsSecStatus.error() == ServiceChannelNotification::FAILED_TO_LOCK_MUTEX ||
                            sldsSecStatus.error() == ServiceChannelNotification::SLDS_CALCULATION_ERROR ||
                            sldsSecStatus.error() == ServiceChannelNotification::UNAUTHORIZED_SENDER) {
                            return etl::unexpected(sldsSecStatus.error());
                            }
                    }
                }
            }

            return {};
        }

        etl::expected<uint32_t, ServiceChannelNotification> SpaceSegmentTcServices::copManagementServiceGetCLCW(
            Objects::PhysicalChannelName physicalChannelName) {
            const auto it = Objects::physicalChannelMap.find(static_cast<uint8_t>(physicalChannelName));
            if (it == Objects::physicalChannelMap.end()) {
                return etl::unexpected(ServiceChannelNotification::INVALID_CHANNEL_NAME);
            }
            uint16_t scid = it->second.getScidTc();

            for (auto &pair : Objects::farmMap) {
                // extract a clcw only from FARMs that belong to this physical channel
                if (etl::get<1>(extractVcidScid(pair.first))  == scid) {
                    FrameAcceptanceReporting& farm = pair.second;

                    if (!farm.clcwBufferMutex.tryLockFor(Defs::MutexDelayMs)) {
                        return etl::unexpected(ServiceChannelNotification::FAILED_TO_LOCK_MUTEX);
                    }

                    if (farm.clcwBuffer.has_value()) {
                        uint32_t clcw = farm.clcwBuffer.has_value();
                        farm.clcwBuffer.reset();
                        farm.clcwBufferMutex.unlock();
                        return clcw;
                    }

                    farm.clcwBufferMutex.unlock();
                }
            }

            // if reached this point, no CLCWs were available
            return etl::unexpected(ServiceChannelNotification::NO_CLCW_AVAILABLE);
        }

        etl::expected<void, ServiceChannelNotification> SpaceSegmentTcServices::modifyOptionalClcwFields(
            Objects::PhysicalChannelName physicalChannelName,
            etl::optional<StatusFieldVector> statusFieldVector,
            etl::optional<bool> noRfAvailable,
            etl::optional<bool> noBitLock) {

            // update no rf available and no bit lock
            if (noRfAvailable.has_value() || noBitLock.has_value()) {
                const auto phyIt = Objects::physicalChannelMap.find(static_cast<uint8_t>(physicalChannelName));
                if (phyIt == Objects::physicalChannelMap.end()) {
                    return etl::unexpected(ServiceChannelNotification::INVALID_CHANNEL_NAME);
                }
                const PhysicalChannel& phyChan = phyIt->second;
                MasterChannelSsTc& mcChan = Objects::masterChannelSsTcMap.at(phyChan.getScidTc());

                if (!mcChan.channelMutex.tryLockFor(Defs::MutexDelayMs)) {
                    return etl::unexpected(ServiceChannelNotification::FAILED_TO_LOCK_MUTEX);
                }

                if (noRfAvailable.has_value()) {
                    mcChan.setNoRfAvailable(noRfAvailable.value());
                }

                if (noBitLock.has_value()) {
                    mcChan.setNoBitLock(noBitLock.value());
                }
                mcChan.channelMutex.unlock();
            }

            // update status fields
            if (statusFieldVector.has_value()) {
                etl::pair<Objects::VirtualChannelTcName, uint8_t>* statusFieldVectorPtr = statusFieldVector->data();
                for (uint8_t i = 0; i < statusFieldVector->size(); i++) {
                    const Defs::VcidScidKey key = static_cast<Defs::VcidScidKey>(statusFieldVectorPtr->first);
                    const auto it = Objects::virtualChannelSsTcMap.find(key);

                    if (it != Objects::virtualChannelSsTcMap.end()) {
                        VirtualChannelSsTc &vcChan = it->second;
                        if (vcChan.channelMutex.tryLockFor(Defs::MutexDelayMs)) {
                            return etl::unexpected(ServiceChannelNotification::FAILED_TO_LOCK_MUTEX);
                        }

                        vcChan.setClcwStatusField(statusFieldVectorPtr->second);
                        vcChan.channelMutex.unlock();
                    }
                    statusFieldVectorPtr++;
                }
            }
        }
#endif // INCLUDE_SPACE_SEGMENT_CODE
} // CCSDSDataLinkLayer