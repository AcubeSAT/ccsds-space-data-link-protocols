#include "SecurityAssociationServices.hpp"
#include "ChannelObjects.hpp"

namespace CCSDSDataLinkLayer {
    etl::expected<void, ServiceChannelNotification> SecurityAssociationServices::resetSequenceNumber(Defs::Spi spi) {
#ifdef INCLUDE_SPACE_SEGMENT_CODE
        auto& saMap = Objects::saSpaceSegmentMap;
#else
        auto& saMap = Objects::saGroundSegmentMap;
#endif

        const auto it = saMap.find(static_cast<uint8_t>(spi));
        if (it == saMap.end()) {
            return etl::unexpected(ServiceChannelNotification::INVALID_SECURITY_PARAMETER_INDEX);
        }
        SecurityAssociation& sa = it->second;

        if (!sa.saMutex.tryLockFor(Defs::MutexDelayMs)) {
            return etl::unexpected(ServiceChannelNotification::FAILED_TO_LOCK_MUTEX);
        }
        sa.resetSequenceNumber();
        sa.saMutex.unlock();
        return {};
    }

    etl::expected<void, ServiceChannelNotification> SecurityAssociationServices::pauseSecurityAssociation(Defs::Spi spi) {
#ifdef INCLUDE_SPACE_SEGMENT_CODE
        auto& saMap = Objects::saSpaceSegmentMap;
#else
        auto& saMap = Objects::saGroundSegmentMap;
#endif

        const auto it = saMap.find(static_cast<uint8_t>(spi));
        if (it == saMap.end()) {
            return etl::unexpected(ServiceChannelNotification::INVALID_SECURITY_PARAMETER_INDEX);
        }
        SecurityAssociation& sa = it->second;

        if (!sa.saMutex.tryLockFor(Defs::MutexDelayMs)) {
            return etl::unexpected(ServiceChannelNotification::FAILED_TO_LOCK_MUTEX);
        }

        if (sa.getSecurityAssociationStatus() == Defs::SecurityAssociationStatus::PAUSED) {
            return etl::unexpected(ServiceChannelNotification::SA_ALREADY_PAUSED);
        }
        sa.setSecurityAssociationStatus(Defs::SecurityAssociationStatus::PAUSED);

        sa.saMutex.unlock();
        return {};
    }

    etl::expected<void, ServiceChannelNotification> SecurityAssociationServices::continueSecurityAssociation(Defs::Spi spi) {
#ifdef INCLUDE_SPACE_SEGMENT_CODE
        auto& saMap = Objects::saSpaceSegmentMap;
#else
        auto& saMap = Objects::saGroundSegmentMap;
#endif

        const auto it = saMap.find(static_cast<uint8_t>(spi));
        if (it == saMap.end()) {
            return etl::unexpected(ServiceChannelNotification::INVALID_SECURITY_PARAMETER_INDEX);
        }
        SecurityAssociation& sa = it->second;

        if (!sa.saMutex.tryLockFor(Defs::MutexDelayMs)) {
            return etl::unexpected(ServiceChannelNotification::FAILED_TO_LOCK_MUTEX);
        }

        if (sa.getSecurityAssociationStatus() == Defs::SecurityAssociationStatus::RUNNING) {
            return etl::unexpected(ServiceChannelNotification::SA_ALREADY_PAUSED);
        }
        sa.setSecurityAssociationStatus(Defs::SecurityAssociationStatus::RUNNING);

        sa.saMutex.unlock();
        return {};
    }
}// CCSDSDataLinkLayer