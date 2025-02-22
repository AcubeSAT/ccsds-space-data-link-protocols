#include <FrameAcceptanceReporting.hpp>
#include <CCSDSLogger.h>

FARMNotification FrameAcceptanceReporting::accept(TransferFrameTC* frame, ServiceType serviceType) {
    if (serviceType == ServiceType::TYPE_AD) {
        if (higherLayerBufferTypeAD.full()) {
            ccsdsLogNotice(Rx, TypeFARMNotif, FARM_HIGH_LAYER_AD_BUFFER_FULL);
            return FARM_HIGH_LAYER_AD_BUFFER_FULL;
        }

        higherLayerBufferTypeAD.push_back(frame);
        ccsdsLogNotice(Rx, TypeFARMNotif, NO_FARM_EVENT);
        return NO_FARM_EVENT;
    } else if (serviceType == ServiceType::TYPE_BD) {
        if (higherLayerBufferTypeBD.full()) {
            TransferFrameTC* frameToDiscard = higherLayerBufferTypeBD.front(); // Oldest frame that will be deleted by the circular buffer

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
        ccsdsLogNotice(Rx, TypeFARMNotif, NO_FARM_EVENT);
        return NO_FARM_EVENT;
    }

    ccsdsLogNotice(Rx, TypeFARMNotif, FARM_UNEXPECTED_VALUE);
    return FARM_UNEXPECTED_VALUE;
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
    if (clcwBuffer.full()) {
        clcwBuffer.clear();
    }
    // TODO: See if there is any use for the optional fields and report them here (not part of COP-1):
    //       statusField, noRfAvailable, noBitLock and farmBCount (the last one is updated, but
    //       not used by COP-1)
    clcwBuffer.push(CLCW(ControlWordType,
                         ClcwVersionNumber,
                         0,
                         CopInEffect,
                         vid,
                         0,
                         FlagState::NOT_READY,
                         FlagState::NOT_READY,
                         lockout,
                         wait,
                         retransmit,
                         farmBCount,
                         0,
                         receiverFrameSeqNumber));

}

Window FrameAcceptanceReporting::getWindow(uint8_t frameSeqNumber) {
    uint8_t positiveWindowBorder = static_cast<uint8_t>((static_cast<uint16_t>(receiverFrameSeqNumber) + farmPositiveWinWidth - 1) & 0xFF);
    // Note: the case V(R) = N(s) is included in the positive window area
    if (withinWindow(frameSeqNumber, receiverFrameSeqNumber, positiveWindowBorder)) {
        return POSITIVE_WINDOW;
    }

    uint8_t negativeWindowBorder;
    if (receiverFrameSeqNumber >= farmNegativeWidth - 1) {
        negativeWindowBorder = receiverFrameSeqNumber - (farmNegativeWidth - 1);
    } else {
        negativeWindowBorder = static_cast<uint8_t>((static_cast<uint16_t>(receiverFrameSeqNumber) + 0xFF - (farmNegativeWidth - 1)) & 0xFF);
    }

    if (withinWindow(frameSeqNumber, negativeWindowBorder, (receiverFrameSeqNumber == 0) ? 255 : (receiverFrameSeqNumber - 1))) {
        return NEGATIVE_WINDOW;
    }

    return OUTSIDE_WINDOWS;
}

std::pair<FARMNotification, uint8_t> FrameAcceptanceReporting::applyFarmStateTable() {
    uint8_t eventCode = 0;
    FARMNotification farmNotification = NO_FARM_EVENT;

    /** CLCW report time **/
    if (timer.getRemainingTime() == 0) {
        // E11
        eventCode = 11;
        report();
        timer.startTimer(clcwReportInterval);
        ccsdsLogNotice(Rx, TypeFARMNotif, farmNotification);
        return std::make_pair(farmNotification, eventCode);
    }

    /** Check if upper layer buffer has free space **/
    if (!higherLayerBufferTypeAD.full() && wait == FlagState::READY) {
        // E10
        eventCode = 10;
        wait = FlagState::NOT_READY;
        if (state == WAIT) {
            state = OPEN;
        }
        ccsdsLogNotice(Rx,TypeFARMNotif, farmNotification);
        return std::make_pair(farmNotification, eventCode);
    }

    /** Frame arrival **/
    if (!lowerLayerBuffer.empty()) {
        TransferFrameTC* frameTc = lowerLayerBuffer.front();
        lowerLayerBuffer.pop_front();

        if (frameTc->getServiceType() == ServiceType::TYPE_AD) {
            if (frameTc->getTransferFrameSequenceNumber() == receiverFrameSeqNumber) {
                if (!higherLayerBufferTypeAD.full()) {
                    // E1
                    eventCode = 1;
                    if (state == OPEN) {
                        accept(frameTc, ServiceType::TYPE_AD);
                        receiverFrameSeqNumber = (receiverFrameSeqNumber == 255) ? 0 : (receiverFrameSeqNumber + 1);
                        retransmit = FlagState::NOT_READY;
                    } else if (state == WAIT) {
                        farmNotification = FARM_NON_APPLICABLE_COMBINATION_OF_STATE_AND_EVENT;
                    } else { // state LOCKOUT
                        discard(frameTc);
                    }
                } else { // no buffer available for TYPE-AD frames
                    // E2
                    eventCode = 2;
                    discard(frameTc);
                    if (state == OPEN) {
                        retransmit = FlagState::READY;
                        wait = FlagState::READY;
                        state = WAIT;
                    }
                }
            } else { // check if N(S) is within the positive or negative window
                Window window = getWindow(frameTc->getTransferFrameSequenceNumber());
                if (window == POSITIVE_WINDOW) {
                    // E3
                    eventCode = 3;
                    discard(frameTc);
                    if (state == OPEN) {
                        retransmit = FlagState::READY;
                    }
                } else if (window == NEGATIVE_WINDOW) {
                    // E4
                    eventCode = 4;
                    discard(frameTc);
                } else { // Outside windows range
                    // E5
                    eventCode = 5;
                    discard(frameTc);
                    lockout = FlagState::READY;
                    state = LOCKOUT;
                }
            }
        } else if (frameTc->getServiceType() == ServiceType::TYPE_BD) {
            // E6
            eventCode = 6;
            accept(frameTc, ServiceType::TYPE_BD);
            farmBCount = (farmBCount == 3) ? 0 : (farmBCount + 1);
        } else if (frameTc->getServiceType() == ServiceType::TYPE_BC) {
            uint16_t expectedLenWithoutDataField = TcPrimaryHeaderSize + errorControlFieldPresent * ErrorControlFieldSize;
            uint16_t frameLength = frameTc->getFrameLength();
            uint8_t* frameData = frameTc->getFrameData();
            if ((frameLength == expectedLenWithoutDataField + UnlockCommandSize) &&
                (frameData[TcPrimaryHeaderSize] == UnlockCommandOctet)) {
                // E7 (unlock command)
                eventCode = 7;
                farmBCount = (farmBCount == 3) ? 0 : (farmBCount + 1);
                retransmit = FlagState::NOT_READY;
                wait = FlagState::NOT_READY;
                lockout = FlagState::NOT_READY;
                state = OPEN;
            } else if ((frameLength == expectedLenWithoutDataField + SetVrCommandSize) &&
                    (frameData[TcPrimaryHeaderSize] == SetVrCommandOctet1) &&
                    (frameData[TcPrimaryHeaderSize + 1] == SetVrCommandOctet2)) {
                // E8 (set V(R) command)
                eventCode = 8;
                farmBCount = (farmBCount == 3) ? 0 : (farmBCount + 1);
                if (state == OPEN || state == WAIT) {
                    retransmit = FlagState::NOT_READY;
                    wait = FlagState::NOT_READY;
                    receiverFrameSeqNumber = frameData[TcPrimaryHeaderSize + 2];
                    state = OPEN;
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
    ccsdsLogNotice(Rx,TypeFARMNotif, farmNotification);
    return std::make_pair(farmNotification, eventCode);
}