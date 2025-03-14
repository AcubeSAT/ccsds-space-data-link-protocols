#include "FrameAcceptanceReporting.hpp"
#include "CCSDSLoggerImpl.h"

namespace CCSDSDataLinkLayer {
#ifdef SPACE_SEGMENT
    FARMNotification FrameAcceptanceReporting::accept(TransferFrameTC *frame, ServiceType serviceType) {
        if (serviceType == ServiceType::TYPE_AD) {
            if (higherLayerBufferTypeAD.full()) {
                ccsdsLogNotice(TxRx::Rx, NotificationType::TypeFARMNotif, FARMNotification:: FARM_HIGH_LAYER_AD_BUFFER_FULL);
                return FARMNotification::FARM_HIGH_LAYER_AD_BUFFER_FULL;
            }

            higherLayerBufferTypeAD.push_back(frame);
            ccsdsLogNotice(TxRx::Rx, NotificationType::TypeFARMNotif, FARMNotification:: NO_FARM_EVENT);
            return FARMNotification::NO_FARM_EVENT;
        } else if (serviceType == ServiceType::TYPE_BD) {
            if (higherLayerBufferTypeBD.full()) {
                TransferFrameTC *frameToDiscard = higherLayerBufferTypeBD.front(); // Oldest frame that will be deleted by the circular buffer

                memoryPool.deletePacket(frameToDiscard->getFrameData(), frameToDiscard->getFrameLength());

                etl::ilist<TransferFrameTC>::iterator master_copy_it = frameMasterCopyBuffer.begin();
                while (master_copy_it != frameMasterCopyBuffer.end()) {
                    if (&(*master_copy_it) == frameToDiscard) {
                        frameMasterCopyBuffer.erase(master_copy_it);
                        break;
                    }
                    ++master_copy_it;
                }
            }

            higherLayerBufferTypeBD.push(frame);
            ccsdsLogNotice(TxRx::Rx, NotificationType::TypeFARMNotif, FARMNotification:: NO_FARM_EVENT);
            return FARMNotification::NO_FARM_EVENT;
        }

        ccsdsLogNotice(TxRx::Rx, NotificationType::TypeFARMNotif, FARMNotification:: FARM_UNEXPECTED_VALUE);
        return FARMNotification::FARM_UNEXPECTED_VALUE;
    }

    void FrameAcceptanceReporting::discard(TransferFrameTC *frame) {
        memoryPool.deletePacket(frame->getFrameData(), frame->getFrameLength());

        etl::ilist<TransferFrameTC>::iterator master_copy_it = frameMasterCopyBuffer.begin();
        while (master_copy_it != frameMasterCopyBuffer.end()) {
            if (&(*master_copy_it) == frame) {
                frameMasterCopyBuffer.erase(master_copy_it);
                break;
            }
            ++master_copy_it;
        }
    }

    void FrameAcceptanceReporting::report() {
        if (clcwBuffer->full()) {
            clcwBuffer->clear();
        }
        // TODO: See if there is any use for the optional fields and report them here (not part of COP-1):
        //       statusField, noRfAvailable, noBitLock and farmBCount (the last one is updated, but
        //       not used by COP-1)
        clcwBuffer->push(CLCW(ControlWordType,
                             ClcwVersionNumber,
                             0,
                             CopInEffect,
                             vid,
                             0,
                             false,
                             false,
                             lockout,
                             wait,
                             retransmit,
                             farmBCount,
                             false,
                             receiverFrameSeqNumber));

    }

    Window FrameAcceptanceReporting::getWindow(uint8_t frameSeqNumber) const {
        auto positiveWindowBorder = static_cast<uint8_t>(
                (static_cast<uint16_t>(receiverFrameSeqNumber) + farmPositiveWinWidth - 1) & 0xFF);
        // Note: the case V(R) = N(s) is included in the positive window area
        if (withinWindow(frameSeqNumber, receiverFrameSeqNumber, positiveWindowBorder)) {
            return Window::POSITIVE_WINDOW;
        }

        uint8_t negativeWindowBorder;
        if (receiverFrameSeqNumber >= farmNegativeWidth - 1) {
            negativeWindowBorder = receiverFrameSeqNumber - (farmNegativeWidth - 1);
        } else {
            negativeWindowBorder = static_cast<uint8_t>(
                    (static_cast<uint16_t>(receiverFrameSeqNumber) + 0xFF - (farmNegativeWidth - 1)) & 0xFF);
        }

        if (withinWindow(frameSeqNumber, negativeWindowBorder,
                         (receiverFrameSeqNumber == 0) ? 255 : (receiverFrameSeqNumber - 1))) {
            return Window::NEGATIVE_WINDOW;
        }

        return Window::OUTSIDE_WINDOWS;
    }

    std::pair<FARMNotification, uint8_t> FrameAcceptanceReporting::applyFarmStateTable() {
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
        if (!higherLayerBufferTypeAD.full() && wait) {
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
        if (!lowerLayerBuffer.empty()) {
            TransferFrameTC *frameTc = lowerLayerBuffer.front();
            lowerLayerBuffer.pop_front();

            if (frameTc->getServiceType() == ServiceType::TYPE_AD) {
                if (frameTc->getTransferFrameSequenceNumber() == receiverFrameSeqNumber) {
                    if (!higherLayerBufferTypeAD.full()) {
                        // E1
                        eventCode = 1;
                        if (state == FARMState::OPEN) {
                            accept(frameTc, ServiceType::TYPE_AD);
                            receiverFrameSeqNumber = (receiverFrameSeqNumber == 255) ? 0 : (receiverFrameSeqNumber + 1);
                            retransmit = false;
                        } else if (state == FARMState::WAIT) {
                            farmNotification = FARMNotification::FARM_NON_APPLICABLE_COMBINATION_OF_STATE_AND_EVENT;
                        } else { // state LOCKOUT
                            discard(frameTc);
                        }
                    } else { // no buffer available for TYPE-AD frames
                        // E2
                        eventCode = 2;
                        discard(frameTc);
                        if (state == FARMState::OPEN) {
                            retransmit = true;
                            wait = true;
                            state = FARMState::WAIT;
                        }
                    }
                } else { // check if N(S) is within the positive or negative window
                    Window window = getWindow(frameTc->getTransferFrameSequenceNumber());
                    if (window == Window::POSITIVE_WINDOW) {
                        // E3
                        eventCode = 3;
                        discard(frameTc);
                        if (state == FARMState::OPEN) {
                            retransmit = true;
                        }
                    } else if (window == Window::NEGATIVE_WINDOW) {
                        // E4
                        eventCode = 4;
                        discard(frameTc);
                    } else { // Outside windows range
                        // E5
                        eventCode = 5;
                        discard(frameTc);
                        lockout = true;
                        state = FARMState::LOCKOUT;
                    }
                }
            } else if (frameTc->getServiceType() == ServiceType::TYPE_BD) {
                // E6
                eventCode = 6;
                accept(frameTc, ServiceType::TYPE_BD);
                farmBCount = (farmBCount == 3) ? 0 : (farmBCount + 1);
            } else if (frameTc->getServiceType() == ServiceType::TYPE_BC) {
                uint16_t expectedLenWithoutDataField =
                        TcPrimaryHeaderSize + errorControlFieldPresent * ErrorControlFieldSize;
                uint16_t frameLength = frameTc->getFrameLength();
                uint8_t *frameData = frameTc->getFrameData();
                if ((frameLength == expectedLenWithoutDataField + UnlockCommandSize) &&
                    (frameData[TcPrimaryHeaderSize] == UnlockCommandOctet)) {
                    // E7 (unlock command)
                    eventCode = 7;
                    farmBCount = (farmBCount == 3) ? 0 : (farmBCount + 1);
                    retransmit = false;
                    wait = false;
                    lockout = false;
                    state = FARMState::OPEN;
                } else if ((frameLength == expectedLenWithoutDataField + SetVrCommandSize) &&
                           (frameData[TcPrimaryHeaderSize] == SetVrCommandOctet1) &&
                           (frameData[TcPrimaryHeaderSize + 1] == SetVrCommandOctet2)) {
                    // E8 (set V(R) command)
                    eventCode = 8;
                    farmBCount = (farmBCount == 3) ? 0 : (farmBCount + 1);
                    if (state == FARMState::OPEN || state == FARMState::WAIT) {
                        retransmit = false;
                        wait = false;
                        receiverFrameSeqNumber = frameData[TcPrimaryHeaderSize + 2];
                        state = FARMState::OPEN;
                    }
                } else {
                    // E9 (frame with invalid command)
                    eventCode = 9;
                }

                // Dispose frame. Invalid frames are disposed without any further action being taken.
                discard(frameTc);
            } else {
                // E9 (received TYPE-BC frame)
                eventCode = 9;
                discard(frameTc);
            }
        }
        ccsdsLogNotice(TxRx::Rx, NotificationType::TypeFARMNotif, farmNotification);
        return std::make_pair(farmNotification, eventCode);
    }
#endif // SPACE_SEGMENT
} // namespace CCSDSDataLinkLayer