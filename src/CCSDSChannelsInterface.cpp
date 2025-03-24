#include "CCSDSChannelsInterface.hpp"
#include "CCSDSLoggerImpl.h"

namespace CCSDSDataLinkLayer {
#ifdef SPACE_SEGMENT

    /** ===================================
     *   MapChannelSpaceSegment operations
     *  ===================================
     */
    BaseMAPChannel ChannelsInterface::upcastToBase(ChannelConfig::MAPChannelSpaceSegmentVariant &mapChannelVariant) {
        return etl::visit([](auto &mapChan) -> BaseMAPChannel {
            return static_cast<BaseMAPChannel>(mapChan);
        }, mapChannelVariant);
    }

    etl::expected<void, MapChannelAlert>
    ChannelsInterface::pushFrameMapChannelSpaceSegment(ChannelConfig::MAPChannelSpaceSegmentVariant &mapChannelVariant,
                                                       TransferFrameTC *frameTC) {
        return etl::visit([&](auto &mapChan) -> etl::expected<void, MapChannelAlert> {
            if (mapChan.framesUnderProcessing.full()) {
                ccsdsLogNotice(TxRx::Rx, NotificationType::TypeMAPChannelAlert, MapChannelAlert::FRAME_LIST_FULL);
                return etl::unexpected(MapChannelAlert::FRAME_LIST_FULL);
            }

            mapChan.framesUnderProcessing.push_back(frameTC);

            return {};
        }, mapChannelVariant);
    }

    etl::expected<void, MapChannelAlert>
    ChannelsInterface::popFrameMapChannelSpaceSegment(ChannelConfig::MAPChannelSpaceSegmentVariant &mapChannelVariant,
                                                      TransferFrameTC *frameTC) {
        return etl::visit([&](auto &mapChan) -> etl::expected<void, MapChannelAlert> {
            auto it = mapChan.framesUnderProcessing.begin();
            while (it != mapChan.framesUnderProcessing.end()) {
                if (frameTC == *it) {
                    mapChan.framesUnderProcessing.erase(it);
                    return {};
                }
                ++it;
            }
            ccsdsLogNotice(TxRx::Rx, NotificationType::TypeMAPChannelAlert, MapChannelAlert::REQUSTED_FRAME_NOT_FOUND);
            return etl::unexpected(MapChannelAlert::REQUSTED_FRAME_NOT_FOUND);
        }, mapChannelVariant);
    }

    etl::expected<TransferFrameTC *, MapChannelAlert>
    ChannelsInterface::getFrameMapChannelSpaceSegment(ChannelConfig::MAPChannelSpaceSegmentVariant &mapChannelVariant,
                                                      const DefsAndUtils::ServiceType serviceType,
                                                      const DefsAndUtils::TcFrameProcessingStage processingStage) {
        return etl::visit([&](auto &mapChan) -> etl::expected<TransferFrameTC *, MapChannelAlert> {
            auto it = mapChan.framesUnderProcessing.begin();
            while (it != mapChan.framesUnderProcessing.end()) {
                if ((*it)->getServiceType() == serviceType && (*it)->getProcessingStage() == processingStage) {
                    return *it;
                }
                ++it;
            }
            ccsdsLogNotice(TxRx::Rx, NotificationType::TypeMAPChannelAlert, MapChannelAlert::REQUSTED_FRAME_NOT_FOUND);
            return etl::unexpected(MapChannelAlert::REQUSTED_FRAME_NOT_FOUND);
        }, mapChannelVariant);
    }

    /** =======================================
     *   VirtualChannelSpaceSegment operations
     *  =======================================
     */
    BaseVirtualChannel ChannelsInterface::upcastToBase(
        ChannelConfig::VirtualChannelSpaceSegmentVariant &virtualChannelVariant) {
        return etl::visit([](auto &vcChan) -> BaseVirtualChannel {
            return static_cast<BaseVirtualChannel>(vcChan);
        }, virtualChannelVariant);
    }

    etl::pair<FARMNotification, uint8_t> ChannelsInterface::applyFarmStateTable(
        ChannelConfig::MasterChannelSpaceSegmentVariant &masterChannelVariant,
        ChannelConfig::VirtualChannelSpaceSegmentVariant &virtualChannelVariant) {
        return etl::visit([&](auto &vcChan) -> std::pair<FARMNotification, uint8_t> {
            return vcChan.farm.applyFarmStateTable(masterChannelVariant, virtualChannelVariant);
        }, virtualChannelVariant);
    }

    etl::expected<void, VirtualChannelAlert>
    ChannelsInterface::pushFrameVirtualChannelSpaceSegment(
        ChannelConfig::MasterChannelSpaceSegmentVariant &masterChannelVariant,
        ChannelConfig::VirtualChannelSpaceSegmentVariant &virtualChannelVariant,
        etl::variant<TransferFrameTM *, TransferFrameTC *> frame,
        const VchanBuffType vchanBuffType
    ) {
        // check if the correct frame type was given
        if ((vchanBuffType == VchanBuffType::UNDER_PROCESSING_TM && frame.is_type<TransferFrameTC *>()) ||
            (vchanBuffType != VchanBuffType::UNDER_PROCESSING_TM && frame.is_type<TransferFrameTM *>())) {
            ccsdsLogNotice(TxRx::Tx, NotificationType::TypeVirtualChannelAlert,
                           VirtualChannelAlert::INVALID_INPUT);
            return etl::unexpected(VirtualChannelAlert::INVALID_INPUT);
        }

        return etl::visit([&](auto &vcChan) -> etl::expected<void, VirtualChannelAlert> {
            switch (vchanBuffType) {
                case VchanBuffType::UNDER_PROCESSING_TM:
                    if (vcChan.framesUnderProcessingTM.full()) {
                        ccsdsLogNotice(TxRx::Tx, NotificationType::TypeVirtualChannelAlert,
                                       VirtualChannelAlert::FRAME_LIST_FULL);
                        return etl::unexpected(VirtualChannelAlert::FRAME_LIST_FULL);
                    }
                    vcChan.framesUnderProcessingTM.push_back(etl::get<TransferFrameTM *>(frame));
                    break;
                case VchanBuffType::AFTER_ALL_FRAMES_RECEPTION_TC:
                    if (vcChan.framesAfterAllFramesReceptionTC.full()) {
                        ccsdsLogNotice(TxRx::Rx, NotificationType::TypeVirtualChannelAlert,
                                       VirtualChannelAlert::FRAME_LIST_FULL);
                        return etl::unexpected(VirtualChannelAlert::FRAME_LIST_FULL);
                    }
                    vcChan.framesAfterAllFramesReceptionTC.push_back(etl::get<TransferFrameTC *>(frame));
                    break;
                case VchanBuffType::AFTER_VC_GENERATION_TC_TYPE_AD:
                    if (vcChan.framesAfterVCReceptionTCTypeAD.full()) {
                        ccsdsLogNotice(TxRx::Rx, NotificationType::TypeVirtualChannelAlert,
                                       VirtualChannelAlert::FRAME_LIST_FULL);
                        return etl::unexpected(VirtualChannelAlert::FRAME_LIST_FULL);
                    }
                    vcChan.framesAfterVCReceptionTCTypeAD.push_back(etl::get<TransferFrameTC *>(frame));
                    break;
                case VchanBuffType::AFTER_VC_GENERATION_TC_TYPE_BD:
                    // delete front frame master copy if the circular buffer is full
                    if (vcChan.framesAfterVCReceptionTCTypeBD.full()) {
                        removeFrameDataMasterChannelSpaceSegment(masterChannelVariant,
                                                                 vcChan.framesAfterVCReceptionTCTypeBD.front());
                        vcChan.framesAfterVCReceptionTCTypeBD.pop_front();
                    }
                    vcChan.framesAfterVCReceptionTCTypeBD.push_back(etl::get<TransferFrameTC *>(frame));
            }

            return {};
        }, virtualChannelVariant);
    }

    etl::expected<void, VirtualChannelAlert>
    ChannelsInterface::popFrameVirtualChannelSpaceSegment(
        ChannelConfig::VirtualChannelSpaceSegmentVariant &virtualChannelVariant,
        etl::variant<TransferFrameTM *, TransferFrameTC *> frame,
        const VchanBuffType vchanBuffType) {
        // check if the correct frame type was given
        if ((vchanBuffType == VchanBuffType::UNDER_PROCESSING_TM && frame.is_type<TransferFrameTC *>()) ||
            (vchanBuffType != VchanBuffType::UNDER_PROCESSING_TM && frame.is_type<TransferFrameTM *>())) {
            ccsdsLogNotice(TxRx::Tx, NotificationType::TypeVirtualChannelAlert,
                           VirtualChannelAlert::INVALID_INPUT);
            return etl::unexpected(VirtualChannelAlert::INVALID_INPUT);
        }

        return etl::visit([&](auto &vcChan) -> etl::expected<void, VirtualChannelAlert> {
            switch (vchanBuffType) {
                case VchanBuffType::UNDER_PROCESSING_TM: {
                    auto it = vcChan.framesUnderProcessingTM.begin();
                    while (it != vcChan.framesUnderProcessingTM.end()) {
                        if ((*it) == etl::get<TransferFrameTM *>(frame)) {
                            vcChan.framesUnderProcessingTM.erase(it);
                            return {};
                        }
                        ++it;
                    }
                    break;
                }
                case VchanBuffType::AFTER_ALL_FRAMES_RECEPTION_TC: {
                    auto it = vcChan.framesAfterAllFramesReceptionTC.begin();
                    while (it != vcChan.framesAfterAllFramesReceptionTC.end()) {
                        if ((*it) == etl::get<TransferFrameTC *>(frame)) {
                            vcChan.framesAfterAllFramesReceptionTC.erase(it);
                            return {};
                        }
                        ++it;
                    }
                    break;
                }
                case VchanBuffType::AFTER_VC_GENERATION_TC_TYPE_AD: {
                    auto it = vcChan.framesAfterVCReceptionTCTypeAD.begin();
                    while (it != vcChan.framesAfterVCReceptionTCTypeAD.end()) {
                        if ((*it) == etl::get<TransferFrameTC *>(frame)) {
                            vcChan.framesAfterVCReceptionTCTypeAD.erase(it);
                            return {};
                        }
                        ++it;
                    }
                    break;
                }
                case VchanBuffType::AFTER_VC_GENERATION_TC_TYPE_BD: {
                    auto it = vcChan.framesAfterVCReceptionTCTypeBD.begin();
                    while (it != vcChan.framesAfterVCReceptionTCTypeBD.end()) {
                        if ((*it) == etl::get<TransferFrameTC *>(frame)) {
                            vcChan.framesAfterVCReceptionTCTypeBD.erase(it);
                            return {};
                        }
                        ++it;
                    }
                    break;
                }
            }

            ccsdsLogNotice(TxRx::Rx, NotificationType::TypeVirtualChannelAlert,
                           VirtualChannelAlert::REQUSTED_FRAME_NOT_FOUND);
            return etl::unexpected(VirtualChannelAlert::REQUSTED_FRAME_NOT_FOUND);
        }, virtualChannelVariant);
    }

    etl::expected<etl::variant<TransferFrameTM *, TransferFrameTC *>, VirtualChannelAlert>
    ChannelsInterface::getFrameVirtualChannelSpaceSegment(
        ChannelConfig::VirtualChannelSpaceSegmentVariant &virtualChannelVariant,
        const VchanBuffType vchanBuffType,
        const etl::variant<DefsAndUtils::TcFrameProcessingStage,
            DefsAndUtils::TmFrameProcessingStage> processingStage) {
        if ((vchanBuffType == VchanBuffType::UNDER_PROCESSING_TM && processingStage.is_type<
                 DefsAndUtils::TcFrameProcessingStage>()) ||
            (vchanBuffType != VchanBuffType::UNDER_PROCESSING_TM && processingStage.is_type<
                 DefsAndUtils::TmFrameProcessingStage>())) {
            ccsdsLogNotice(TxRx::Tx, NotificationType::TypeVirtualChannelAlert,
                           VirtualChannelAlert::INVALID_INPUT);
            return etl::unexpected(VirtualChannelAlert::INVALID_INPUT);
        }

        return etl::visit(
            [&](auto &vcChan) -> etl::expected<etl::variant<TransferFrameTM *, TransferFrameTC *>,
        VirtualChannelAlert> {
                switch (vchanBuffType) {
                    case VchanBuffType::UNDER_PROCESSING_TM: {
                        auto it = vcChan.framesUnderProcessingTM.begin();
                        while (it != vcChan.framesUnderProcessingTM.end()) {
                            if ((*it)->getProcessingStage() == etl::get<DefsAndUtils::TmFrameProcessingStage>(
                                    processingStage)) {
                                return etl::variant<TransferFrameTM *, TransferFrameTC *>(*it);
                            }
                            ++it;
                        }
                        break;
                    }
                    case VchanBuffType::AFTER_ALL_FRAMES_RECEPTION_TC: {
                        auto it = vcChan.framesAfterAllFramesReceptionTC.begin();
                        while (it != vcChan.framesAfterAllFramesReceptionTC.end()) {
                            if ((*it)->getProcessingStage() == etl::get<DefsAndUtils::TcFrameProcessingStage>(
                                    processingStage)) {
                                return etl::variant<TransferFrameTM *, TransferFrameTC *>(*it);
                            }
                            ++it;
                        }
                        break;
                    }
                    case VchanBuffType::AFTER_VC_GENERATION_TC_TYPE_AD: {
                        auto it = vcChan.framesAfterVCReceptionTCTypeAD.begin();
                        while (it != vcChan.framesAfterVCReceptionTCTypeAD.end()) {
                            if ((*it)->getProcessingStage() == etl::get<DefsAndUtils::TcFrameProcessingStage>(
                                    processingStage)) {
                                return etl::variant<TransferFrameTM *, TransferFrameTC *>(*it);
                            }
                            ++it;
                        }
                        break;
                    }
                    case VchanBuffType::AFTER_VC_GENERATION_TC_TYPE_BD: {
                        auto it = vcChan.framesAfterVCReceptionTCTypeBD.begin();
                        while (it != vcChan.framesAfterVCReceptionTCTypeBD.end()) {
                            if ((*it)->getProcessingStage() == etl::get<DefsAndUtils::TcFrameProcessingStage>(
                                    processingStage)) {
                                return etl::variant<TransferFrameTM *, TransferFrameTC *>(*it);
                            }
                            ++it;
                        }
                    }
                }

                ccsdsLogNotice(TxRx::Rx, NotificationType::TypeVirtualChannelAlert,
                               VirtualChannelAlert::REQUSTED_FRAME_NOT_FOUND);
                return etl::unexpected(VirtualChannelAlert::REQUSTED_FRAME_NOT_FOUND);
            }, virtualChannelVariant);
    }

    uint8_t ChannelsInterface::readAndUpdateTmFrameCountVirtualChannelSpaceSegment(
        ChannelConfig::VirtualChannelSpaceSegmentVariant &virtualChannelVariant) {
        return etl::visit([](auto &vcChan) -> uint8_t {
            // The C++ standard guarantees that unsigned integers wraparound in case of overflow,
            // which is the desired behavior.
            return vcChan.frameCountTM++;
        }, virtualChannelVariant);
    }

    etl::expected<void, VirtualChannelAlert>
    ChannelsInterface::pushTmPacketVirtualChannelSpaceSegment(
        ChannelConfig::VirtualChannelSpaceSegmentVariant &virtualChannelVariant,
        const uint8_t *packetSource,
        const uint16_t packetLength,
        const bool pushToFront) {
        return etl::visit([&](auto &vcChan) -> etl::expected<void, VirtualChannelAlert> {
            if (vcChan.packetBufferTM.available() >= packetLength &&
                !vcChan.packetLengthBufferTM.full()) {
                if (pushToFront) {
                    vcChan.packetLengthBufferTM.push_front(packetLength);
                    for (uint16_t i = 0; i < packetLength; i++) {
                        vcChan.packetBufferTM.push_front(packetSource[i]);
                    }
                    return {};
                } else {
                    vcChan.packetLengthBufferTM.push_back(packetLength);
                    for (uint16_t i = 0; i < packetLength; i++) {
                        vcChan.packetBufferTM.push_back(packetSource[i]);
                    }
                    return {};
                }
            }

            ccsdsLogNotice(TxRx::Tx, NotificationType::TypeVirtualChannelAlert, VirtualChannelAlert::PACKET_QUEUE_FULL);
            return etl::unexpected(VirtualChannelAlert::PACKET_QUEUE_FULL);
        }, virtualChannelVariant);
    }

    etl::expected<uint16_t, VirtualChannelAlert>
    ChannelsInterface::popTmPacketVirtualChannelSpaceSegment(
        ChannelConfig::VirtualChannelSpaceSegmentVariant &virtualChannelVariant,
        uint8_t *packetDestination,
        const bool popFromBack) {
        return etl::visit([&](auto &vcChan) -> etl::expected<uint16_t, VirtualChannelAlert> {
            if (!vcChan.packetLengthBufferTM.empty()) {
                if (popFromBack) {
                    uint16_t packetLength = vcChan.packetLengthBufferTM.back();
                    vcChan.packetLengthBufferTM.pop_back();
                    for (uint16_t i = 0; i < packetLength; i++) {
                        packetDestination[i] = vcChan.packetBufferTM.back();
                        vcChan.packetLengthBufferTM.pop_back();
                    }
                    return packetLength;
                } else {
                    uint16_t packetLength = vcChan.packetLengthBufferTM.front();
                    vcChan.packetLengthBufferTM.pop_front();
                    for (uint16_t i = 0; i < packetLength; i++) {
                        packetDestination[i] = vcChan.packetBufferTM.front();
                        vcChan.packetLengthBufferTM.pop_front();
                    }
                    return packetLength;
                }
            }

            ccsdsLogNotice(TxRx::Tx, NotificationType::TypeVirtualChannelAlert,
                           VirtualChannelAlert::PACKET_QUEUE_EMPTY);
            return etl::unexpected(VirtualChannelAlert::PACKET_QUEUE_EMPTY);
        }, virtualChannelVariant);
    }

    /** ======================================
     *   MasterChannelSpaceSegment operations
     *  ======================================
     */
    BaseMasterChannel ChannelsInterface::upcastToBase(
        ChannelConfig::MasterChannelSpaceSegmentVariant &masterChannelVariant) {
        return etl::visit([](auto &mcChan) -> BaseMasterChannel {
            return static_cast<BaseMasterChannel>(mcChan);
        }, masterChannelVariant);
    }

    etl::expected<void, MasterChannelAlert> ChannelsInterface::pushTmFrameMasterChannelSpaceSegment(
        ChannelConfig::MasterChannelSpaceSegmentVariant &masterChannelVariant,
        TransferFrameTM *frameTM) {
        return etl::visit([&](auto &mcChan) -> etl::expected<void, MasterChannelAlert> {
            if (mcChan.framesUnderProcessingTM.full()) {
                ccsdsLogNotice(TxRx::Tx, NotificationType::TypeMasterChannelAlert, MasterChannelAlert::FRAME_LIST_FULL);
                return etl::unexpected(MasterChannelAlert::FRAME_LIST_FULL);
            }
            mcChan.framesUnderProcessingTM.push_back(frameTM);
            return {};
        }, masterChannelVariant);
    }

    etl::expected<void, MasterChannelAlert> ChannelsInterface::popTmFrameMasterChannelSpaceSegment(
        ChannelConfig::MasterChannelSpaceSegmentVariant &masterChannelVariant,
        TransferFrameTM *frameTM) {
        return etl::visit([&](auto &mcChan) -> etl::expected<void, MasterChannelAlert> {
            auto it = mcChan.framesUnderProcessingTM.begin();
            while (it != mcChan.framesUnderProcessingTM.end()) {
                if (frameTM == *it) {
                    mcChan.framesUnderProcessingTM.erase(it);
                    return {};
                }
                ++it;
            }
            ccsdsLogNotice(TxRx::Rx, NotificationType::TypeMasterChannelAlert,
                           MasterChannelAlert::REQUESTED_FRAME_NOT_FOUND);
            return etl::unexpected(MasterChannelAlert::REQUESTED_FRAME_NOT_FOUND);
        }, masterChannelVariant);
    }

    etl::expected<TransferFrameTM *, MasterChannelAlert>
    ChannelsInterface::getTmFrameMasterChannelSpaceSegment(
        ChannelConfig::MasterChannelSpaceSegmentVariant &masterChannelVariant,
        const DefsAndUtils::TmFrameProcessingStage processingStage) {
        return etl::visit([&](auto &mcChan) -> etl::expected<TransferFrameTM *, MasterChannelAlert> {
            auto it = mcChan.framesUnderProcessingTM.begin();
            while (it != mcChan.framesUnderProcessingTM.end()) {
                if ((*it)->getProcessingStage() == processingStage) {
                    return (*it);
                }
            }
            ccsdsLogNotice(TxRx::Tx, NotificationType::TypeMasterChannelAlert,
                           MasterChannelAlert::REQUESTED_FRAME_NOT_FOUND);
            return etl::unexpected(MasterChannelAlert::REQUESTED_FRAME_NOT_FOUND);
        }, masterChannelVariant);
    }

    bool ChannelsInterface::hasCapacityForFrameDataMasterChannelSpaceSegment(
        ChannelConfig::MasterChannelSpaceSegmentVariant &masterChannelVariant,
        const DefsAndUtils::FrameType frameType,
        const uint16_t numberOfFrames,
        const uint16_t numberOfOctets) {
        return etl::visit([&](auto &mcChan) -> bool {
            if (frameType == DefsAndUtils::FrameType::TM &&
                mcChan.masterCopyTM.available() >= numberOfFrames &&
                mcChan.masterChannelPoolTM.findFit(numberOfOctets).second == MasterChannelAlert::NO_MC_ALERT) {
                return true;
            }

            if (frameType == DefsAndUtils::FrameType::TC &&
                mcChan.masterCopyTC.available() >= numberOfFrames &&
                mcChan.masterChannelPoolTC.findFit(numberOfOctets).second == MasterChannelAlert::NO_MC_ALERT) {
                return true;
            }

            return false;
        }, masterChannelVariant);
    }


    etl::expected<uint8_t *, MasterChannelAlert>
    ChannelsInterface::addFrameOctetsToMemPoolMasterChannelSpaceSegment(
        ChannelConfig::MasterChannelSpaceSegmentVariant &masterChannelVariant,
        const DefsAndUtils::FrameType frameType,
        uint8_t *octetsSource, const uint16_t frameLength) {
        return etl::visit([&](auto &mcChan) -> etl::expected<uint8_t *, MasterChannelAlert> {
            uint8_t *ptr;
            if (frameType == DefsAndUtils::FrameType::TM) {
                ptr = mcChan.masterChannelPoolTM.allocatePacket(octetsSource, frameLength);
            } else {
                // TC
                ptr = mcChan.masterChannelPoolTC.allocatePacket(octetsSource, frameLength);
            }

            if (ptr == nullptr) {
                ccsdsLogNotice(TxRx::Rx, NotificationType::TypeMasterChannelAlert,
                               MasterChannelAlert::NOT_ENOUGH_SPACE_IN_MEMORY_POOL);
                return etl::unexpected(MasterChannelAlert::NOT_ENOUGH_SPACE_IN_MEMORY_POOL);
            }

            return ptr;
        }, masterChannelVariant);
    }

    etl::expected<void, MasterChannelAlert>
    ChannelsInterface::addFrameObjectToMasterCopyBufferMasterChannelSpaceSegment(
        ChannelConfig::MasterChannelSpaceSegmentVariant &masterChannelVariant,
        const DefsAndUtils::FrameType frameType,
        etl::variant<TransferFrameTM &, TransferFrameTC &> frame) {
        return etl::visit([&](auto &mcChan) -> etl::expected<void, MasterChannelAlert> {
            if (frameType == DefsAndUtils::FrameType::TM &&
                frame.is_type<TransferFrameTM &>() &&
                !mcChan.masterCopyTM.full()) {
                mcChan.masterCopyTM.push_back(etl::get<TransferFrameTM &>(frame));
                return {};
            }

            if (frameType == DefsAndUtils::FrameType::TC &&
                frame.is_type<TransferFrameTC &>() &&
                !mcChan.masterCopyTC.full()) {
                mcChan.masterCopyTC.push_back(etl::get<TransferFrameTC &>(frame));
                return {};
            }

            ccsdsLogNotice(TxRx::Rx, NotificationType::TypeMasterChannelAlert,
                           MasterChannelAlert::MASTER_COPY_BUFFER_FULL);
            return etl::unexpected(MasterChannelAlert::MASTER_COPY_BUFFER_FULL);
        }, masterChannelVariant);
    }

    etl::expected<void, MasterChannelAlert>
    ChannelsInterface::removeFrameDataMasterChannelSpaceSegment(
        ChannelConfig::MasterChannelSpaceSegmentVariant &masterChannelVariant,
        etl::variant<TransferFrameTM *, TransferFrameTC *> frame) {
        return etl::visit([&](auto &mcChan) -> etl::expected<void, MasterChannelAlert> {
            if (frame.is_type<TransferFrameTM *>()) {
                const auto framePtr = etl::get<TransferFrameTM *>(frame);
                auto it = mcChan.masterCopyTM.begin();
                while (it != mcChan.masterCopyTM.end()) {
                    if ((&(*it)) == framePtr) {
                        mcChan.masterChannelPoolTM.deletePacket(framePtr->getFrameData(), framePtr->getFrameLength());
                        mcChan.masterCopyTM.erase(it);
                        return {};
                    }
                    ++it;
                }
            } else {
                // Type TC
                const auto framePtr = etl::get<TransferFrameTC *>(frame);
                auto it = mcChan.masterCopyTC.begin();
                while (it != mcChan.masterCopyTC.end()) {
                    if ((&(*it)) == framePtr) {
                        mcChan.masterChannelPoolTC.deletePacket(framePtr->getFrameData(), framePtr->getFrameLength());
                        mcChan.masterCopyTC.erase(it);
                        return {};
                    }
                    ++it;
                }
            }

            ccsdsLogNotice(TxRx::Rx, NotificationType::TypeMasterChannelAlert,
                           MasterChannelAlert::REQUESTED_FRAME_NOT_FOUND);
            return etl::unexpected(MasterChannelAlert::REQUESTED_FRAME_NOT_FOUND);
        }, masterChannelVariant);
    }

    uint8_t ChannelsInterface::readAndUpdateTmFrameCountMasterChannelSpaceSegment(
        ChannelConfig::MasterChannelSpaceSegmentVariant &masterChannelVariant) {
        return etl::visit([](auto &mcChan) -> uint8_t {
            // The C++ standard guarantees that unsigned integers wraparound in case of overflow,
            // which is the desired behavior.
            return mcChan.masterChannelFrameCountTM++;
        }, masterChannelVariant);
    }
#endif // SPACE_SEGMENT

#ifdef GROUND_SEGMENT
    /** ====================================
     *   MapChannelGroundSegment operations
     *  ====================================
     */
    BaseMAPChannel ChannelsInterface::upcastToBase(ChannelConfig::MAPChannelGroundSegmentVariant &mapChannelVariant) {
        return etl::visit([](auto &mapChan) -> BaseMAPChannel {
            return static_cast<BaseMAPChannel>(mapChan);
        }, mapChannelVariant);
    }

    etl::expected<void, MapChannelAlert>
    ChannelsInterface::pushPacketMAPChannelGroundSegment(
        ChannelConfig::MAPChannelGroundSegmentVariant &mapChannelVariant,
        const DefsAndUtils::ServiceType serviceType,
        uint8_t *packetSource,
        const uint16_t packetLength) {
        if (serviceType == DefsAndUtils::ServiceType::TYPE_BC ||
            serviceType == DefsAndUtils::ServiceType::TYPE_RESERVED) {
            ccsdsLogNotice(TxRx::Tx, NotificationType::TypeMAPChannelAlert, MapChannelAlert::INVALID_SERVICE_TYPE);
            return etl::unexpected(MapChannelAlert::INVALID_SERVICE_TYPE);
        }

        return etl::visit([&](auto &mapChan) -> etl::expected<void, MapChannelAlert> {
            if (serviceType == DefsAndUtils::ServiceType::TYPE_AD &&
                mapChan.packetBufferTypeAD.available() >= packetLength &&
                !mapChan.packetLengthBufferTypeAD.full()) {
                mapChan.packetLengthBufferTypeAD.push(packetLength);
                for (uint16_t i = 0; i < packetLength; ++i) {
                    mapChan.packetBufferTypeAD.push(packetSource[i]);
                }
                return {};
            }

            if (serviceType == DefsAndUtils::ServiceType::TYPE_BD &&
                mapChan.packetBufferTypeBD.available() >= packetLength &&
                !mapChan.packetLengthBufferTypeBD.full()) {
                mapChan.packetLengthBufferTypeBD.push(packetLength);
                for (uint16_t i = 0; i < packetLength; ++i) {
                    mapChan.packetBufferTypeBD.push(packetSource[i]);
                }

                return {};
            }

            ccsdsLogNotice(TxRx::Tx, NotificationType::TypeMAPChannelAlert, MapChannelAlert::PACKET_QUEUE_FULL);
            return etl::unexpected(MapChannelAlert::PACKET_QUEUE_FULL);
        }, mapChannelVariant);
    }

    etl::expected<uint16_t, MapChannelAlert>
    ChannelsInterface::popPacketMAPChannelGroundSegment(
        ChannelConfig::MAPChannelGroundSegmentVariant &mapChannelVariant,
        const DefsAndUtils::ServiceType serviceType,
        uint8_t *packetDestination) {
        if (serviceType == DefsAndUtils::ServiceType::TYPE_BC ||
            serviceType == DefsAndUtils::ServiceType::TYPE_RESERVED) {
            ccsdsLogNotice(TxRx::Tx, NotificationType::TypeMAPChannelAlert, MapChannelAlert::INVALID_SERVICE_TYPE);
            return etl::unexpected(MapChannelAlert::INVALID_SERVICE_TYPE);
        }

        return etl::visit([&](auto &mapChan) -> etl::expected<uint16_t, MapChannelAlert> {
            if (serviceType == DefsAndUtils::ServiceType::TYPE_AD &&
                !mapChan.packetLengthBufferTypeAD.empty()) {
                const uint16_t length = mapChan.packetLengthBufferTypeAD.front();
                mapChan.packetLengthBufferTypeAD.pop();
                for (uint16_t i = 0; i < length; ++i) {
                    packetDestination[i] = mapChan.packetBufferTypeAD.front();
                }
                return length;
            }

            if (serviceType == DefsAndUtils::ServiceType::TYPE_BD &&
                !mapChan.packetLengthBufferTypeBD.empty()) {
                const uint16_t length = mapChan.packetLengthBufferTypeBD.front();
                mapChan.packetLengthBufferTypeBD.pop();
                for (uint16_t i = 0; i < length; ++i) {
                    packetDestination[i] = mapChan.packetBufferTypeBD.front();
                }
                return length;
            }

            ccsdsLogNotice(TxRx::Tx, NotificationType::TypeMAPChannelAlert, MapChannelAlert::PACKET_QUEUE_EMPTY);
            return etl::unexpected(MapChannelAlert::PACKET_QUEUE_EMPTY);
        }, mapChannelVariant);
    }

    etl::expected<void, MapChannelAlert>
    ChannelsInterface::pushFrameMapChannelGroundSegment(
        ChannelConfig::MAPChannelGroundSegmentVariant &mapChannelVariant,
        TransferFrameTC *frameTC) {
        return etl::visit([&](auto &mapChan) -> etl::expected<void, MapChannelAlert> {
            if (mapChan.framesUnderProcessing.full()) {
                ccsdsLogNotice(TxRx::Tx, NotificationType::TypeMAPChannelAlert, MapChannelAlert::FRAME_LIST_FULL);
                return etl::unexpected(MapChannelAlert::FRAME_LIST_FULL);
            }

            mapChan.framesUnderProcessing.push_back(frameTC);

            return {};
        }, mapChannelVariant);
    }

    etl::expected<void, MapChannelAlert>
    ChannelsInterface::popFrameMapChannelGroundSegment(ChannelConfig::MAPChannelGroundSegmentVariant &mapChannelVariant,
                                                       TransferFrameTC *frameTC) {
        return etl::visit([&](auto &mapChan) -> etl::expected<void, MapChannelAlert> {
            auto it = mapChan.framesUnderProcessing.begin();
            while (it != mapChan.framesUnderProcessing.end()) {
                if (frameTC == *it) {
                    mapChan.framesUnderProcessing.erase(it);
                    return {};
                }
                ++it;
            }
            ccsdsLogNotice(TxRx::Tx, NotificationType::TypeMAPChannelAlert, MapChannelAlert::REQUSTED_FRAME_NOT_FOUND);
            return etl::unexpected(MapChannelAlert::REQUSTED_FRAME_NOT_FOUND);
        }, mapChannelVariant);
    }

    etl::expected<TransferFrameTC *, MapChannelAlert>
    ChannelsInterface::getFrameMapChannelGroundSegment(ChannelConfig::MAPChannelGroundSegmentVariant &mapChannelVariant,
                                                       const DefsAndUtils::ServiceType serviceType,
                                                       const DefsAndUtils::TcFrameProcessingStage processingStage) {
        return etl::visit([&](auto &mapChan) -> etl::expected<TransferFrameTC *, MapChannelAlert> {
            auto it = mapChan.framesUnderProcessing.begin();
            while (it != mapChan.framesUnderProcessing.end()) {
                if ((*it)->getServiceType() == serviceType && (*it)->getProcessingStage() == processingStage) {
                    return *it;
                }
                ++it;
            }
            ccsdsLogNotice(TxRx::Tx, NotificationType::TypeMAPChannelAlert, MapChannelAlert::REQUSTED_FRAME_NOT_FOUND);
            return etl::unexpected(MapChannelAlert::REQUSTED_FRAME_NOT_FOUND);
        }, mapChannelVariant);
    }

    /** ========================================
     *   VirtualChannelGroundSegment operations
     *  ========================================
     */
    BaseVirtualChannel ChannelsInterface::upcastToBase(
        ChannelConfig::VirtualChannelGroundSegmentVariant &virtualChannelVariant) {
        return etl::visit([](auto &vcChan) -> BaseVirtualChannel {
            return static_cast<BaseVirtualChannel>(vcChan);
        }, virtualChannelVariant);
    }

    std::pair<FOPNotification, uint8_t> ChannelsInterface::applyFopStateTable(
        ChannelConfig::MasterChannelGroundSegmentVariant &masterChannelVariant,
        ChannelConfig::VirtualChannelGroundSegmentVariant &virtualChannelVariant) {
        return etl::visit([&](auto &vcChan) -> std::pair<FOPNotification, uint8_t> {
            return vcChan.fop.applyFopStateTable(masterChannelVariant);
        }, virtualChannelVariant);
    }

    etl::expected<void, VirtualChannelAlert>
    ChannelsInterface::pushSignalToFop(ChannelConfig::VirtualChannelGroundSegmentVariant &virtualChannelVariant,
                                       etl::variant<DefsAndUtils::DirectiveRequestSignal,
                                           DefsAndUtils::FduTransferSignal,
                                           DefsAndUtils::LowerLayerResponseSignal,
                                           CLCW> signal) {
        return etl::visit([&](auto &vcChan) -> etl::expected<void, VirtualChannelAlert> {
            if (signal.is_type<DefsAndUtils::DirectiveRequestSignal>() &&
                !vcChan.fop.directiveRequestSignalQueue.full()
            ) {
                vcChan.fop.directiveRequestSignalQueue.push(etl::get<DefsAndUtils::DirectiveRequestSignal>(signal));
            } else if (signal.is_type<DefsAndUtils::FduTransferSignal>() &&
                       !vcChan.fop.transferFduSignalQueue.full()
            ) {
                vcChan.fop.transferFduSignalQueue.push(etl::get<DefsAndUtils::FduTransferSignal>(signal));
            } else if (signal.is_type<DefsAndUtils::LowerLayerResponseSignal>() &&
                       !vcChan.fop.lowerLayerResponseSignalQueue.full()
            ) {
                vcChan.fop.lowerLayerResponseSignalQueue.push(etl::get<DefsAndUtils::LowerLayerResponseSignal>(signal));
            } else if (signal.is_type<CLCW>() &&
                       !vcChan.fop.clcwQueue.full()
            ) {
                vcChan.fop.clcwQueue.push(etl::get<CLCW>(signal));
            }

            ccsdsLogNotice(TxRx::Tx, NotificationType::TypeVirtualChannelAlert,
                           VirtualChannelAlert::FOP_SIGNAL_QUEUE_FULL);
            return etl::unexpected(VirtualChannelAlert::FOP_SIGNAL_QUEUE_FULL);
        }, virtualChannelVariant);
    }

    etl::expected<etl::variant<DefsAndUtils::DirectiveNotificationSignal,
        DefsAndUtils::TransferNotificationSignal,
        DefsAndUtils::AsynchronousNotificationSignal,
        DefsAndUtils::FopToLowerLayerRequestSignal>, VirtualChannelAlert>
    ChannelsInterface::popSignalFromFop(ChannelConfig::VirtualChannelGroundSegmentVariant &virtualChannelVariant,
                                        const FopOutputQueueType queueType) {
        using signalVariant = etl::variant<DefsAndUtils::DirectiveNotificationSignal,
            DefsAndUtils::TransferNotificationSignal,
            DefsAndUtils::AsynchronousNotificationSignal,
            DefsAndUtils::FopToLowerLayerRequestSignal>;

        return etl::visit([&](auto &vcChan) -> signalVariant {
            switch (queueType) {
                case FopOutputQueueType::TRANSFER_NOTIFICATION_QUEUE:
                    if (!vcChan.fop.transferNotificationSignalQueue.empty()) {
                        auto signal = vcChan.fop.transferNotificationSignalQueue.front();
                        vcChan.fop.transferNotificationSignalQueue.pop();
                        return signalVariant(signal);
                    }
                    break;
                case FopOutputQueueType::DIRECTIVE_NOTIFICATION_QUEUE:
                    if (!vcChan.fop.directiveNotificationSignalQueue.empty()) {
                        auto signal = vcChan.fop.directiveNotificationSignalQueue.front();
                        vcChan.fop.directiveNotificationSignalQueue.pop();
                        return signalVariant(signal);
                    }
                    break;
                case FopOutputQueueType::FOP_TO_LOWER_LAYER_REQUEST_QUEUE:
                    if (!vcChan.fop.fopToLowerLayerRequestSignalQueue.empty()) {
                        auto signal = vcChan.fop.fopToLowerLayerRequestSignalQueue.front();
                        vcChan.fop.fopToLowerLayerRequestSignalQueue.pop();
                        return signalVariant(signal);
                    }
                    break;
                case FopOutputQueueType::ASYNCHRONOUS_NOTIFICATION_QUEUE:
                    if (!vcChan.fop.asynchronousNotificationSignalQueue.empty()) {
                        auto signal = vcChan.fop.asynchronousNotificationSignalQueue.front();
                        vcChan.fop.asynchronousNotificationSignalQueue.pop();
                        return signalVariant(signal);
                    }
            }
            ccsdsLogNotice(TxRx::Tx, NotificationType::TypeVirtualChannelAlert,
                           VirtualChannelAlert::FOP_SIGNAL_QUEUE_EMPTY);
            return etl::unexpected(VirtualChannelAlert::FOP_SIGNAL_QUEUE_EMPTY);
        }, virtualChannelVariant);
    }

    etl::expected<void, VirtualChannelAlert>
    ChannelsInterface::pushFrameVirtualChannelGroundSegment(
        ChannelConfig::VirtualChannelGroundSegmentVariant &virtualChannelVariant,
        TransferFrameTC *frameTC) {
        return etl::visit([&](auto &vcChan) -> etl::expected<void, VirtualChannelAlert> {
            if (vcChan.framesUnderProcessingTC.full()) {
                ccsdsLogNotice(TxRx::Tx, NotificationType::TypeMAPChannelAlert, VirtualChannelAlert::FRAME_LIST_FULL);
                return etl::unexpected(VirtualChannelAlert::FRAME_LIST_FULL);
            }

            vcChan.framesUnderProcessingTC.push_back(frameTC);

            return {};
        }, virtualChannelVariant);
    }

    etl::expected<void, VirtualChannelAlert>
    ChannelsInterface::popFrameVirtualChannelGroundSegment(
        ChannelConfig::VirtualChannelGroundSegmentVariant &virtualChannelVariant,
        TransferFrameTC *frameTC) {
        return etl::visit([&](auto &vcChan) -> etl::expected<void, VirtualChannelAlert> {
            auto it = vcChan.framesUnderProcessingTC.begin();
            while (it != vcChan.framesUnderProcessingTC.end()) {
                if (frameTC == *it) {
                    vcChan.framesUnderProcessingTC.erase(it);
                    return {};
                }
                ++it;
            }
            ccsdsLogNotice(TxRx::Tx, NotificationType::TypeMAPChannelAlert,
                           VirtualChannelAlert::REQUSTED_FRAME_NOT_FOUND);
            return etl::unexpected(VirtualChannelAlert::REQUSTED_FRAME_NOT_FOUND);
        }, virtualChannelVariant);
    }

    etl::expected<TransferFrameTC *, VirtualChannelAlert>
    ChannelsInterface::getFrameVirtualChannelGroundSegment(
        ChannelConfig::VirtualChannelGroundSegmentVariant &virtualChannelVariant,
        const DefsAndUtils::ServiceType serviceType,
        const DefsAndUtils::TcFrameProcessingStage processingStage) {
        return etl::visit([&](auto &vcChan) -> etl::expected<TransferFrameTC *, VirtualChannelAlert> {
            auto it = vcChan.framesUnderProcessingTC.begin();
            while (it != vcChan.framesUnderProcessingTC.end()) {
                if ((*it)->getServiceType() == serviceType && (*it)->getProcessingStage() == processingStage) {
                    return *it;
                }
                ++it;
            }
            ccsdsLogNotice(TxRx::Tx, NotificationType::TypeMAPChannelAlert,
                           VirtualChannelAlert::REQUSTED_FRAME_NOT_FOUND);
            return etl::unexpected(VirtualChannelAlert::REQUSTED_FRAME_NOT_FOUND);
        }, virtualChannelVariant);
    }

    /** =======================================
     *   MasterChannelGroundSegment operations
     *  =======================================
     */
    BaseMasterChannel ChannelsInterface::upcastToBase(
        ChannelConfig::MasterChannelGroundSegmentVariant &masterChannelVariant) {
        return etl::visit([](auto &mcChan) -> BaseMasterChannel {
            return static_cast<BaseMasterChannel>(mcChan);
        }, masterChannelVariant);
    }

    bool ChannelsInterface::hasCapacityForFrameDataMasterChannelGroundSegment(
        ChannelConfig::MasterChannelGroundSegmentVariant &masterChannelVariant,
        const uint16_t numberOfFrames,
        const uint16_t numberOfOctets) {
        return etl::visit([&](auto &mcChan) -> bool {
            if (mcChan.masterCopyTC.available() >= numberOfFrames &&
                mcChan.masterChannelPoolTC.findFit(numberOfOctets).second == MasterChannelAlert::NO_MC_ALERT) {
                return true;
            }

            return false;
        }, masterChannelVariant);
    }


    etl::expected<uint8_t *, MasterChannelAlert>
    ChannelsInterface::addFrameOctetsToMemPoolMasterChannelGroundSegment(
        ChannelConfig::MasterChannelGroundSegmentVariant &masterChannelVariant,
        uint8_t *octetsSource, const uint16_t frameLength) {
        return etl::visit([&](auto &mcChan) -> etl::expected<uint8_t *, MasterChannelAlert> {
            uint8_t *ptr = mcChan.masterChannelPoolTC.allocatePacket(octetsSource, frameLength);

            if (ptr == nullptr) {
                ccsdsLogNotice(TxRx::Tx, NotificationType::TypeMasterChannelAlert,
                               MasterChannelAlert::NOT_ENOUGH_SPACE_IN_MEMORY_POOL);
                return etl::unexpected(MasterChannelAlert::NOT_ENOUGH_SPACE_IN_MEMORY_POOL);
            }

            return ptr;
        }, masterChannelVariant);
    }

    etl::expected<void, MasterChannelAlert>
    ChannelsInterface::removeFrameDataMasterChannelGroundSegment(
        ChannelConfig::MasterChannelGroundSegmentVariant &masterChannelVariant,
        TransferFrameTC &frame) {
        return etl::visit([&](auto &mcChan) -> etl::expected<void, MasterChannelAlert> {
            const auto framePtr = &frame;
            auto it = mcChan.masterCopyTC.begin();
            while (it != mcChan.masterCopyTC.end()) {
                if ((&(*it)) == framePtr) {
                    mcChan.masterChannelPoolTC.deletePacket(framePtr->getFrameData(), framePtr->getFrameLength());
                    mcChan.masterCopyTC.erase(it);
                    return {};
                }
                ++it;
            }

            ccsdsLogNotice(TxRx::Rx, NotificationType::TypeMasterChannelAlert,
                           MasterChannelAlert::REQUESTED_FRAME_NOT_FOUND);
            return etl::unexpected(MasterChannelAlert::REQUESTED_FRAME_NOT_FOUND);
        }, masterChannelVariant);
    }
#endif // GROUND_SEGMENT
} // namespace CCSDSDataLinkLayer
