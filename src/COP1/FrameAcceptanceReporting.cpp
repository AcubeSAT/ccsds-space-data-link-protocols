#include "FrameAcceptanceReporting.hpp"
#include "LoggerImpl.h"

namespace CCSDSDataLinkLayer {
#ifdef INCLUDE_SPACE_SEGMENT_CODE
    FARMNotification FrameAcceptanceReporting::accept(TransferFrameTC *frame,
        const DefsAndUtils::ServiceType serviceType) {
        // push frame to higher layer buffer
        if (serviceType == DefsAndUtils::ServiceType::TYPE_AD) {
            VirtualChannelSsTc& vcChan = virtualChannelSsTcMap.at(vcid);
            if (vcChan.framesAfterVcReceptionTypeAD.isFull()) {
                ccsdsLogNotice(TxRx::Rx, NotificationType::TypeFARMNotif,
                               FARMNotification::FARM_HIGH_LAYER_AD_BUFFER_FULL);
                return FARMNotification::FARM_HIGH_LAYER_AD_BUFFER_FULL;
            }

            vcChan.framesAfterVcReceptionTypeAD.push(frame);
            return FARMNotification::NO_FARM_EVENT;
        } else if (serviceType == DefsAndUtils::ServiceType::TYPE_BD) {
            frame->updateProcessingStage(DefsAndUtils::TcFrameProcessingStage::PROCESSED_BY_FARM);
            ChannelsInterface::pushFrameVirtualChannelSpaceSegment(
                masterChannelVariant, virtualChannelVariant, frame,
                ChannelsInterface::VchanBuffType::AFTER_VC_GENERATION_TC_TYPE_BD);

            return FARMNotification::NO_FARM_EVENT;
        }

        ccsdsLogNotice(TxRx::Rx, NotificationType::TypeFARMNotif, FARMNotification::FARM_UNEXPECTED_VALUE);
        return FARMNotification::FARM_UNEXPECTED_VALUE;
    }

    void FrameAcceptanceReporting::discard(MasterChannelSpaceSegmentVariant &masterChannelVariant,
                                           TransferFrameTC *frame) {
        ChannelsInterface::removeFrameDataMasterChannelSpaceSegment(masterChannelVariant, frame);
    }

    void FrameAcceptanceReporting::report() const {
        // TODO: See if there is any use for the optional fields and report them here (not part of COP-1):
        //       statusField, noRfAvailable, noBitLock and farmBCount (the last one is updated, but
        //       not used by COP-1)
        clcwBuffer = CLCW(DefsAndUtils::ControlWordTypeCLCW,
                          DefsAndUtils::ClcwVersionNumber,
                          0,
                          DefsAndUtils::CopInEffect,
                          vcid,
                          0,
                          false,
                          false,
                          lockout,
                          wait,
                          retransmit,
                          farmBCount,
                          false,
                          receiverFrameSeqNumber);
    }

    Window FrameAcceptanceReporting::getWindow(const uint8_t frameSeqNumber) const {
        const auto positiveWindowBorder = static_cast<uint8_t>(
            (static_cast<uint16_t>(receiverFrameSeqNumber) + farmPositiveWinWidth - 1) & 0xFF);
        // Note: the case V(R) = N(s) is included in the positive window area
        if (DefsAndUtils::withinWindow(frameSeqNumber, receiverFrameSeqNumber, positiveWindowBorder)) {
            return Window::POSITIVE_WINDOW;
        }

        uint8_t negativeWindowBorder;
        if (receiverFrameSeqNumber >= farmNegativeWidth - 1) {
            negativeWindowBorder = receiverFrameSeqNumber - (farmNegativeWidth - 1);
        } else {
            negativeWindowBorder = static_cast<uint8_t>(
                (static_cast<uint16_t>(receiverFrameSeqNumber) + 0xFF - (farmNegativeWidth - 1)) & 0xFF);
        }

        if (DefsAndUtils::withinWindow(frameSeqNumber, negativeWindowBorder,
                                       (receiverFrameSeqNumber == 0) ? 255 : (receiverFrameSeqNumber - 1))) {
            return Window::NEGATIVE_WINDOW;
        }

        return Window::OUTSIDE_WINDOWS;
    }

    std::pair<FARMNotification, uint8_t> FrameAcceptanceReporting::applyFarmStateTable(
        MasterChannelSpaceSegmentVariant &
        masterChannelVariant,
        VirtualChannelSpaceSegmentVariant &virtualChannelVariant) {
        uint8_t eventCode = 0;
        FARMNotification farmNotification = FARMNotification::NO_FARM_EVENT;

        /** CLCW report time **/
        if (timer.getRemainingTime() == 0) {
            // E11
            eventCode = 11;
            report();
            timer.startTimer(clcwReportInterval);
            ccsdsLogNotice(TxRx::Rx, NotificationType::TypeFARMNotif, farmNotification);
            return std::make_pair(farmNotification, eventCode);
        }

        /** Check if upper layer buffer has free space **/
        const bool higherLayerAdBufferFull = etl::visit(
            [&](auto &vcChan) -> bool {
                return vcChan.framesAfterVCReceptionTCTypeAD.full();
            }, virtualChannelVariant);

        if (!higherLayerAdBufferFull && wait) {
            // E10
            eventCode = 10;
            wait = false;
            if (state == FARMState::WAIT) {
                state = FARMState::OPEN;
            }
            ccsdsLogNotice(TxRx::Rx, NotificationType::TypeFARMNotif, farmNotification);
            return std::make_pair(farmNotification, eventCode);
        }

        /** Frame arrival **/
        const auto expectedFrame = ChannelsInterface::getFrameVirtualChannelSpaceSegment(virtualChannelVariant,
            ChannelsInterface::VchanBuffType::AFTER_ALL_FRAMES_RECEPTION_TC,
            DefsAndUtils::TcFrameProcessingStage::PROCESSED_BY_ALL_FRAMES_RECEPTION);
        if (expectedFrame.has_value()) {
            TransferFrameTC *frameTc = etl::get<TransferFrameTC *>(expectedFrame.value());
            ChannelsInterface::popFrameVirtualChannelSpaceSegment(virtualChannelVariant,
                                                                  expectedFrame,
                                                                  ChannelsInterface::VchanBuffType::AFTER_ALL_FRAMES_RECEPTION_TC);

            if (frameTc->getServiceType() == DefsAndUtils::ServiceType::TYPE_AD) {
                if (frameTc->getTransferFrameSequenceNumber() == receiverFrameSeqNumber) {
                    if (!higherLayerAdBufferFull) {
                        // E1
                        eventCode = 1;
                        if (state == FARMState::OPEN) {
                            accept(masterChannelVariant, virtualChannelVariant, frameTc,
                                   DefsAndUtils::ServiceType::TYPE_AD);
                            receiverFrameSeqNumber = (receiverFrameSeqNumber == 255) ? 0 : (receiverFrameSeqNumber + 1);
                            retransmit = false;
                        } else if (state == FARMState::WAIT) {
                            farmNotification = FARMNotification::FARM_NON_APPLICABLE_COMBINATION_OF_STATE_AND_EVENT;
                        } else {
                            // state LOCKOUT
                            discard(masterChannelVariant, frameTc);
                        }
                    } else {
                        // no buffer available for TYPE-AD frames
                        // E2
                        eventCode = 2;
                        discard(masterChannelVariant, frameTc);
                        if (state == FARMState::OPEN) {
                            retransmit = true;
                            wait = true;
                            state = FARMState::WAIT;
                        }
                    }
                } else {
                    // check if N(S) is within the positive or negative window
                    Window window = getWindow(frameTc->getTransferFrameSequenceNumber());
                    if (window == Window::POSITIVE_WINDOW) {
                        // E3
                        eventCode = 3;
                        discard(masterChannelVariant, frameTc);
                        if (state == FARMState::OPEN) {
                            retransmit = true;
                        }
                    } else if (window == Window::NEGATIVE_WINDOW) {
                        // E4
                        eventCode = 4;
                        discard(masterChannelVariant, frameTc);
                    } else {
                        // Outside windows range
                        // E5
                        eventCode = 5;
                        discard(masterChannelVariant, frameTc);
                        lockout = true;
                        state = FARMState::LOCKOUT;
                    }
                }
            } else if (frameTc->getServiceType() == DefsAndUtils::ServiceType::TYPE_BD) {
                // E6
                eventCode = 6;
                accept(masterChannelVariant, virtualChannelVariant, frameTc, DefsAndUtils::ServiceType::TYPE_BD);
                farmBCount = (farmBCount == 3) ? 0 : (farmBCount + 1);
            } else if (frameTc->getServiceType() == DefsAndUtils::ServiceType::TYPE_BC) {
                const uint16_t expectedLenWithoutDataField =
                        DefsAndUtils::TcPrimaryHeaderSize + errorControlFieldPresent *
                        DefsAndUtils::ErrorControlFieldSize;
                const uint16_t frameLength = frameTc->getFrameLength();
                uint8_t *frameData = frameTc->getFrameData();
                if ((frameLength == expectedLenWithoutDataField + DefsAndUtils::UnlockCommandSize) &&
                    (frameData[DefsAndUtils::TcPrimaryHeaderSize] ==
                     DefsAndUtils::UnlockCommandOctet)) {
                    // E7 (unlock command)
                    eventCode = 7;
                    farmBCount = (farmBCount == 3) ? 0 : (farmBCount + 1);
                    retransmit = false;
                    wait = false;
                    lockout = false;
                    state = FARMState::OPEN;
                } else if ((frameLength == expectedLenWithoutDataField + DefsAndUtils::SetVrCommandSize) &&
                           (frameData[DefsAndUtils::TcPrimaryHeaderSize] ==
                            DefsAndUtils::SetVrCommandOctet1) &&
                           (frameData[DefsAndUtils::TcPrimaryHeaderSize + 1] ==
                            DefsAndUtils::SetVrCommandOctet2)) {
                    // E8 (set V(R) command)
                    eventCode = 8;
                    farmBCount = (farmBCount == 3) ? 0 : (farmBCount + 1);
                    if (state == FARMState::OPEN || state == FARMState::WAIT) {
                        retransmit = false;
                        wait = false;
                        receiverFrameSeqNumber = frameData[DefsAndUtils::TcPrimaryHeaderSize + 2];
                        state = FARMState::OPEN;
                    }
                } else {
                    // E9 (frame with invalid command)
                    eventCode = 9;
                }

                // Dispose frame. Invalid frames are disposed without any further action being taken.
                discard(masterChannelVariant, frameTc);
            } else {
                // E9 (received a TYPE-RESERVED frame, which is invalid)
                eventCode = 9;
                discard(masterChannelVariant, frameTc);
            }
        }
        ccsdsLogNotice(TxRx::Rx, NotificationType::TypeFARMNotif, farmNotification);
        return std::make_pair(farmNotification, eventCode);
    }
#endif // INCLUDE_SPACE_SEGMENT_CODE
} // namespace CCSDSDataLinkLayer
