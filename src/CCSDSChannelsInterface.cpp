#include "CCSDSChannelsInterface.hpp"
#include "CCSDSLoggerImpl.h"

namespace CCSDSDataLinkLayer {
#ifdef SPACE_SEGMENT

    /** ===================================
     *   MapChannelSpaceSegment operations
     *  ===================================
     */
    BaseMAPChannel* ChannelsInterface::upcastToBase(MAPChannelSpaceSegmentVariant &mapChannelVariant) {
        return etl::visit([](auto &mapChan) -> BaseMAPChannel* {
            return static_cast<BaseMAPChannel*>(&mapChan);
        }, mapChannelVariant);
    }

    uint16_t ChannelsInterface::frameListAvailableMapChannelSpaceSegment(MAPChannelSpaceSegmentVariant &mapChannelVariant) {
        return etl::visit([](auto &mapChan) -> uint16_t {
            return mapChan.framesUnderProcessing.available();
        }, mapChannelVariant);
    }

    etl::expected<void, MapChannelAlert>
    ChannelsInterface::pushFrameMapChannelSpaceSegment(MAPChannelSpaceSegmentVariant &mapChannelVariant,
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
    ChannelsInterface::popFrameMapChannelSpaceSegment(MAPChannelSpaceSegmentVariant &mapChannelVariant,
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
    ChannelsInterface::getFrameMapChannelSpaceSegment(MAPChannelSpaceSegmentVariant &mapChannelVariant,
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
    BaseVirtualChannel* ChannelsInterface::upcastToBase(
        VirtualChannelSpaceSegmentVariant &virtualChannelVariant) {
        return etl::visit([](auto &vcChan) -> BaseVirtualChannel* {
            return static_cast<BaseVirtualChannel*>(&vcChan);
        }, virtualChannelVariant);
    }

    uint16_t ChannelsInterface::frameListAvailableVirtualChannelSpaceSegment(
            VirtualChannelSpaceSegmentVariant &virtualChannelVariant,
            const VchanBuffType vchanBuffType
        ) {
        return etl::visit([&](auto &vcChan) -> uint16_t {
            switch (vchanBuffType) {
                case VchanBuffType::UNDER_PROCESSING_TM:
                    return !vcChan.framesUnderProcessingTM.available();
                case VchanBuffType::AFTER_ALL_FRAMES_RECEPTION_TC:
                    return !vcChan.framesAfterAllFramesReceptionTC.available();
                case VchanBuffType::AFTER_VC_GENERATION_TC_TYPE_AD:
                    return !vcChan.framesAfterVCReceptionTCTypeAD.available();
                case VchanBuffType::AFTER_VC_GENERATION_TC_TYPE_BD:
                    return !vcChan.framesAfterVCReceptionTCTypeBD.available();
            }
        }, virtualChannelVariant);
    }

    etl::expected<void, VirtualChannelAlert>
    ChannelsInterface::pushFrameVirtualChannelSpaceSegment(
        MasterChannelSpaceSegmentVariant &masterChannelVariant,
        VirtualChannelSpaceSegmentVariant &virtualChannelVariant,
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
        VirtualChannelSpaceSegmentVariant &virtualChannelVariant,
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
        VirtualChannelSpaceSegmentVariant &virtualChannelVariant,
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
        VirtualChannelSpaceSegmentVariant &virtualChannelVariant) {
        return etl::visit([](auto &vcChan) -> uint8_t {
            // The C++ standard guarantees that unsigned integers wraparound in case of overflow,
            // which is the desired behavior.
            return vcChan.frameCountTM++;
        }, virtualChannelVariant);
    }

    bool ChannelsInterface::packetAvailableVirtualChannelSpaceSegment(
        VirtualChannelSpaceSegmentVariant &virtualChannelVariant) {
        return etl::visit([](auto &vcChan) -> bool {
            return !vcChan.packetLengthBufferTM.empty();
        }, virtualChannelVariant);
    }

    etl::expected<void, VirtualChannelAlert>
    ChannelsInterface::pushTmPacketVirtualChannelSpaceSegment(
        VirtualChannelSpaceSegmentVariant &virtualChannelVariant,
        const uint8_t *packetSource,
        const uint16_t packetLength) {
        return etl::visit([&](auto &vcChan) -> etl::expected<void, VirtualChannelAlert> {
            if (vcChan.packetBufferTM.available() >= packetLength &&
                !vcChan.packetLengthBufferTM.full()) {
                vcChan.packetLengthBufferTM.push_back(packetLength);
                for (uint16_t i = 0; i < packetLength; i++) {
                    vcChan.packetBufferTM.push(packetSource[i]);
                }
                return {};
            }

            ccsdsLogNotice(TxRx::Tx, NotificationType::TypeVirtualChannelAlert, VirtualChannelAlert::PACKET_QUEUE_FULL);
            return etl::unexpected(VirtualChannelAlert::PACKET_QUEUE_FULL);
        }, virtualChannelVariant);
    }

    etl::expected<uint16_t, VirtualChannelAlert> ChannelsInterface::popTmPacketLengthVirtualChannelSpaceSegment(
        VirtualChannelSpaceSegmentVariant &virtualChannelVariant) {
        return etl::visit([&](auto &vcChan) -> etl::expected<uint16_t, VirtualChannelAlert> {
            if (!vcChan.packetLengthBufferTM.empty()) {
                uint16_t length = 0;
                    length = vcChan.packetLengthBufferTM.front();
                    vcChan.packetLengthBufferTM.pop_front();
                    return length;
            }

            ccsdsLogNotice(TxRx::Tx, NotificationType::TypeVirtualChannelAlert,
                           VirtualChannelAlert::PACKET_QUEUE_EMPTY);
            return etl::unexpected(VirtualChannelAlert::PACKET_QUEUE_EMPTY);
        }, virtualChannelVariant);
    }

    etl::expected<void, VirtualChannelAlert>
    ChannelsInterface::popTmPacketSegmentVirtualChannelSpaceSegment(
        VirtualChannelSpaceSegmentVariant &virtualChannelVariant,
        const uint16_t numOctets,
        uint8_t *packetDestination) {
        return etl::visit([&](auto &vcChan) -> etl::expected<void, VirtualChannelAlert> {
            if (vcChan.packetBufferTM.available() >= numOctets) {
                for (uint16_t i = 0; i < numOctets; i++) {
                    if (packetDestination != nullptr) {
                        packetDestination[i] = vcChan.packetBufferTM.front();
                    }
                    vcChan.packetBufferTM.pop();
                }
                return {};
            }

            ccsdsLogNotice(TxRx::Tx, NotificationType::TypeVirtualChannelAlert,
                           VirtualChannelAlert::INVALID_REQUESTED_PACKET_SEGMENT);
            return etl::unexpected(VirtualChannelAlert::INVALID_REQUESTED_PACKET_SEGMENT);
        }, virtualChannelVariant);
    }

    /** ======================================
     *   MasterChannelSpaceSegment operations
     *  ======================================
     */
    BaseMasterChannel* ChannelsInterface::upcastToBase(
        MasterChannelSpaceSegmentVariant &masterChannelVariant) {
        return etl::visit([](auto &mcChan) -> BaseMasterChannel* {
            return static_cast<BaseMasterChannel*>(&mcChan);
        }, masterChannelVariant);
    }

    uint16_t ChannelsInterface::frameListAvailableMasterChannelSpaceSegment(MasterChannelSpaceSegmentVariant &masterChannelVariant) {
        return etl::visit([](auto &mcChan) -> uint16_t {
            return mcChan.framesUnderProcessingTM.available();
        }, masterChannelVariant);
    }

    etl::expected<void, MasterChannelAlert> ChannelsInterface::pushTmFrameMasterChannelSpaceSegment(
        MasterChannelSpaceSegmentVariant &masterChannelVariant,
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
        MasterChannelSpaceSegmentVariant &masterChannelVariant,
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
        MasterChannelSpaceSegmentVariant &masterChannelVariant,
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
        MasterChannelSpaceSegmentVariant &masterChannelVariant,
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
    allocateBlockFromMemPoolMasterChannelSpaceSegment(
    MasterChannelSpaceSegmentVariant &masterChannelVariant,
    const DefsAndUtils::FrameType frameType,
    uint16_t blockLength,
    uint8_t *frameDataSource) {
        return etl::visit([&](auto &mcChan) -> etl::expected<uint8_t *, MasterChannelAlert> {
            uint8_t *ptr;
            if (frameType == DefsAndUtils::FrameType::TM) {
                ptr = mcChan.masterChannelPoolTM.allocateBlock(blockLength, frameDataSource);
            } else {
                // TC
                ptr = mcChan.masterChannelPoolTC.allocateBlock(blockLength, frameDataSource);
            }

            if (ptr == nullptr) {
                ccsdsLogNotice(TxRx::Rx, NotificationType::TypeMasterChannelAlert,
                               MasterChannelAlert::NOT_ENOUGH_SPACE_IN_MEMORY_POOL);
                return etl::unexpected(MasterChannelAlert::NOT_ENOUGH_SPACE_IN_MEMORY_POOL);
            }

            return ptr;
        }, masterChannelVariant);
    }

    etl::expected<etl::variant<TransferFrameTM *, TransferFrameTC *>, MasterChannelAlert>
    ChannelsInterface::addFrameObjectToMasterCopyBufferMasterChannelSpaceSegment(
        MasterChannelSpaceSegmentVariant &masterChannelVariant,
        const DefsAndUtils::FrameType frameType,
        etl::variant<TransferFrameTM &, TransferFrameTC &> frame) {
        return etl::visit(
            [&](auto &mcChan) -> etl::expected<etl::variant<TransferFrameTM *, TransferFrameTC *>, MasterChannelAlert> {
                if (frameType == DefsAndUtils::FrameType::TM &&
                    frame.is_type<TransferFrameTM &>() &&
                    !mcChan.masterCopyTM.full()) {
                    mcChan.masterCopyTM.push_back(etl::get<TransferFrameTM &>(frame));
                    return etl::variant<TransferFrameTM *, TransferFrameTC *>(&(mcChan.masterCopyTM.back()));
                }

                if (frameType == DefsAndUtils::FrameType::TC &&
                    frame.is_type<TransferFrameTC &>() &&
                    !mcChan.masterCopyTC.full()) {
                    mcChan.masterCopyTC.push_back(etl::get<TransferFrameTC &>(frame));
                    return etl::variant<TransferFrameTM *, TransferFrameTC *>(&(mcChan.masterCopyTC.back()));
                }

                ccsdsLogNotice(TxRx::Rx, NotificationType::TypeMasterChannelAlert,
                               MasterChannelAlert::MASTER_COPY_BUFFER_FULL);
                return etl::unexpected(MasterChannelAlert::MASTER_COPY_BUFFER_FULL);
            }, masterChannelVariant);
    }

    etl::expected<void, MasterChannelAlert>
    ChannelsInterface::removeFrameDataMasterChannelSpaceSegment(
        MasterChannelSpaceSegmentVariant &masterChannelVariant,
        etl::variant<TransferFrameTM *, TransferFrameTC *> frame) {
        return etl::visit([&](auto &mcChan) -> etl::expected<void, MasterChannelAlert> {
            if (frame.is_type<TransferFrameTM *>()) {
                const auto framePtr = etl::get<TransferFrameTM *>(frame);
                auto it = mcChan.masterCopyTM.begin();
                while (it != mcChan.masterCopyTM.end()) {
                    if ((&(*it)) == framePtr) {
                        mcChan.masterChannelPoolTM.deleteBlock(framePtr->getFrameData(), framePtr->getFrameLength());
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
                        mcChan.masterChannelPoolTC.deleteBlock(framePtr->getFrameData(), framePtr->getFrameLength());
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
        MasterChannelSpaceSegmentVariant &masterChannelVariant) {
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
    BaseMAPChannel* ChannelsInterface::upcastToBase(MAPChannelGroundSegmentVariant &mapChannelVariant) {
        return etl::visit([](auto &mapChan) -> BaseMAPChannel* {
            return static_cast<BaseMAPChannel*>(&mapChan);
        }, mapChannelVariant);
    }

    etl::expected<void, MapChannelAlert>
    ChannelsInterface::pushPacketMAPChannelGroundSegment(
        MAPChannelGroundSegmentVariant &mapChannelVariant,
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
    ChannelsInterface::popPacketLengthMAPChannelGroundSegment(MAPChannelGroundSegmentVariant &mapChannelVariant,
                                                              DefsAndUtils::ServiceType serviceType) {
        if (serviceType == DefsAndUtils::ServiceType::TYPE_BC ||
            serviceType == DefsAndUtils::ServiceType::TYPE_RESERVED) {
            ccsdsLogNotice(TxRx::Tx, NotificationType::TypeMAPChannelAlert, MapChannelAlert::INVALID_SERVICE_TYPE);
            return etl::unexpected(MapChannelAlert::INVALID_SERVICE_TYPE);
        }

        return etl::visit([&](auto &mapChan) -> etl::expected<uint16_t, MapChannelAlert> {
            uint16_t length = 0;
            if (serviceType == DefsAndUtils::ServiceType::TYPE_AD &&
                !mapChan.packetLengthBufferTypeAD.empty()) {
                length = mapChan.packetLengthBufferTypeAD.front();
                mapChan.packetLengthBufferTypeAD.pop();
                return length;
            }
            if (serviceType == DefsAndUtils::ServiceType::TYPE_BD &&
                !mapChan.packetLengthBufferTypeBD.empty()) {
                length = mapChan.packetLengthBufferTypeBD.front();
                mapChan.packetLengthBufferTypeBD.pop();
                return length;
            }


            ccsdsLogNotice(TxRx::Tx, NotificationType::TypeMAPChannelAlert, MapChannelAlert::PACKET_QUEUE_EMPTY);
            return etl::unexpected(MapChannelAlert::PACKET_QUEUE_EMPTY);
        }, mapChannelVariant);
    }

    etl::expected<void, MapChannelAlert>
    ChannelsInterface::popPacketSegmentMAPChannelGroundSegment(
        MAPChannelGroundSegmentVariant &mapChannelVariant,
        const DefsAndUtils::ServiceType serviceType,
        uint8_t *packetDestination,
        uint16_t numOctets) {
        if (serviceType == DefsAndUtils::ServiceType::TYPE_BC ||
            serviceType == DefsAndUtils::ServiceType::TYPE_RESERVED) {
            ccsdsLogNotice(TxRx::Tx, NotificationType::TypeMAPChannelAlert, MapChannelAlert::INVALID_SERVICE_TYPE);
            return etl::unexpected(MapChannelAlert::INVALID_SERVICE_TYPE);
        }

        return etl::visit([&](auto &mapChan) -> etl::expected<void, MapChannelAlert> {
            if (serviceType == DefsAndUtils::ServiceType::TYPE_AD &&
                mapChan.packetBufferTypeAD.available() >= numOctets) {
                for (uint16_t i = 0; i < numOctets; ++i) {
                    packetDestination[i] = mapChan.packetBufferTypeAD.front();
                }
                return {};
            }

            if (serviceType == DefsAndUtils::ServiceType::TYPE_BD &&
                mapChan.packetBufferTypeBD.available() >= numOctets) {
                for (uint16_t i = 0; i < numOctets; ++i) {
                    packetDestination[i] = mapChan.packetBufferTypeBD.front();
                }
                return {};
            }

            ccsdsLogNotice(TxRx::Tx, NotificationType::TypeMAPChannelAlert, MapChannelAlert::INVALID_REQUESTED_PACKET_SEGMENT);
            return etl::unexpected(MapChannelAlert::INVALID_REQUESTED_PACKET_SEGMENT);
        }, mapChannelVariant);
    }

    uint16_t ChannelsInterface::frameListAvailableMapChannelGroundSegment(MAPChannelGroundSegmentVariant &mapChannelVariant) {
        return etl::visit([&](auto &mapChan) -> uint16_t {
            return mapChan.framesUnderProcessing.available();
        }, mapChannelVariant);
    }

    etl::expected<void, MapChannelAlert>
    ChannelsInterface::pushFrameMapChannelGroundSegment(
        MAPChannelGroundSegmentVariant &mapChannelVariant,
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
    ChannelsInterface::popFrameMapChannelGroundSegment(MAPChannelGroundSegmentVariant &mapChannelVariant,
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
    ChannelsInterface::getFrameMapChannelGroundSegment(MAPChannelGroundSegmentVariant &mapChannelVariant,
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
    BaseVirtualChannel* ChannelsInterface::upcastToBase(
        VirtualChannelGroundSegmentVariant &virtualChannelVariant) {
        return etl::visit([](auto &vcChan) -> BaseVirtualChannel* {
            return static_cast<BaseVirtualChannel*>(&vcChan);
        }, virtualChannelVariant);
    }

    uint16_t ChannelsInterface::frameListAvailableVirtualChannelGroundSegment(VirtualChannelGroundSegmentVariant &virtualChannelVariant) {
        return etl::visit([&](auto &vcChan) -> uint16_t {
            return vcChan.framesUnderProcessingTC.available();
        }, virtualChannelVariant);
    }

    etl::expected<void, VirtualChannelAlert>
    ChannelsInterface::pushFrameVirtualChannelGroundSegment(
        VirtualChannelGroundSegmentVariant &virtualChannelVariant,
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
        VirtualChannelGroundSegmentVariant &virtualChannelVariant,
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
        VirtualChannelGroundSegmentVariant &virtualChannelVariant,
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
    BaseMasterChannel* ChannelsInterface::upcastToBase(
        MasterChannelGroundSegmentVariant &masterChannelVariant) {
        return etl::visit([](auto &mcChan) -> BaseMasterChannel* {
            return static_cast<BaseMasterChannel*>(&mcChan);
        }, masterChannelVariant);
    }

    bool ChannelsInterface::hasCapacityForFrameDataMasterChannelGroundSegment(
        MasterChannelGroundSegmentVariant &masterChannelVariant,
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
    ChannelsInterface::allocateBlockFromMemPoolMasterChannelGroundSegment(
    MasterChannelGroundSegmentVariant &masterChannelVariant,
    const uint16_t blockLength,
    uint8_t *frameDataSource) {
        return etl::visit([&](auto &mcChan) -> etl::expected<uint8_t *, MasterChannelAlert> {
            uint8_t *ptr = mcChan.masterChannelPoolTC.allocateBlock(blockLength, frameDataSource);

            if (ptr == nullptr) {
                ccsdsLogNotice(TxRx::Rx, NotificationType::TypeMasterChannelAlert,
                               MasterChannelAlert::NOT_ENOUGH_SPACE_IN_MEMORY_POOL);
                return etl::unexpected(MasterChannelAlert::NOT_ENOUGH_SPACE_IN_MEMORY_POOL);
            }

            return ptr;
        }, masterChannelVariant);
    }

    etl::expected<etl::variant<TransferFrameTM *, TransferFrameTC *>, MasterChannelAlert>
    ChannelsInterface::addFrameObjectToMasterCopyBufferMasterChannelGroundSegment(
        MasterChannelGroundSegmentVariant &masterChannelVariant,
        const TransferFrameTC &frame) {
        return etl::visit(
            [&](auto &mcChan) -> etl::expected<etl::variant<TransferFrameTM *, TransferFrameTC *>, MasterChannelAlert> {
                if (!mcChan.masterCopyTC.full()) {
                    mcChan.masterCopyTC.push_back(frame);
                    return etl::expected<etl::variant<TransferFrameTM *, TransferFrameTC *>,
                        MasterChannelAlert>(&(mcChan.masterCopyTC.back()));
                }

                ccsdsLogNotice(TxRx::Rx, NotificationType::TypeMasterChannelAlert,
                               MasterChannelAlert::MASTER_COPY_BUFFER_FULL);
                return etl::unexpected(MasterChannelAlert::MASTER_COPY_BUFFER_FULL);
            }, masterChannelVariant);
    }

    etl::expected<void, MasterChannelAlert>
    ChannelsInterface::removeFrameDataMasterChannelGroundSegment(
        MasterChannelGroundSegmentVariant &masterChannelVariant,
        TransferFrameTC &frame) {
        return etl::visit([&](auto &mcChan) -> etl::expected<void, MasterChannelAlert> {
            const auto framePtr = &frame;
            auto it = mcChan.masterCopyTC.begin();
            while (it != mcChan.masterCopyTC.end()) {
                if ((&(*it)) == framePtr) {
                    mcChan.masterChannelPoolTC.deleteBlock(framePtr->getFrameData(), framePtr->getFrameLength());
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
