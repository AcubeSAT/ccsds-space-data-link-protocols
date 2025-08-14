#include "FrameAcceptanceReporting.hpp"
#include "LoggerImpl.h"
#include "AddressingAndParsingUtilities.hpp"
#include "StructureGeneration.hpp"

namespace CCSDSDataLinkLayer {
#ifdef INCLUDE_SPACE_SEGMENT_CODE
    FrameAcceptanceReporting::FrameAcceptanceReporting(const Defs::Vcid vcid,
                         const uint8_t farmSlidingWinWidth,
                         const uint16_t clcwReportInterval,
                         const uint8_t fopTransmissionLimit)
    : state(FARMState::OPEN), lockout(false), wait(false),
      retransmit(false),
      farmBCount(0),
      receiverFrameSeqNumber(0),
      farmSlidingWinWidth(farmSlidingWinWidth),
      farmPositiveWinWidth(fopTransmissionLimit > 1 ? farmSlidingWinWidth / 2 : farmSlidingWinWidth),
      farmNegativeWidth(fopTransmissionLimit > 1 ? farmSlidingWinWidth / 2 : 0),
      clcwReportInterval(clcwReportInterval),
      timer(CountdownTimer()),
      clcwBuffer(etl::nullopt),
      clcwBufferMutex(Mutex()),
      vcChan(Objects::virtualChannelSsTcMap.at(vcid)){}

    void FrameAcceptanceReporting::resetFARM() {
        state = FARMState::OPEN;
        lockout = false;
        wait = false;
        retransmit = false;
        farmBCount = 0;
        receiverFrameSeqNumber = 0;
        timer.stopTimer();
        clcwBuffer.reset();
    }

    FARMNotification FrameAcceptanceReporting::accept(TransferFrameTC *frame) {
        // push frame to higher layer buffer
        if (frame->getServiceType() == Defs::ServiceType::TYPE_AD) {
            if (vcChan.framesAfterVcReceptionTypeAD.isFull()) {
                return FARMNotification::FARM_HIGH_LAYER_AD_BUFFER_FULL;
            }

            vcChan.framesAfterVcReceptionTypeAD.push(frame);
            return FARMNotification::NO_FARM_EVENT;
        } else if (frame->getServiceType() == Defs::ServiceType::TYPE_BD) {
            if (vcChan.framesAfterVcReceptionTypeBD.isFull()) {
                // we first need to delete master copy of oldest frame in the circular buffer
                if (discard(vcChan.framesAfterVcReceptionTypeBD.getFront()) == FARMNotification::FAILED_TO_LOCK_MUTEX) {
                    return FARMNotification::FAILED_TO_LOCK_MUTEX;
                }
            }
            vcChan.framesAfterVcReceptionTypeBD.push(frame);
            return FARMNotification::NO_FARM_EVENT;
        }

        return FARMNotification::FARM_UNEXPECTED_VALUE;
    }

    FARMNotification FrameAcceptanceReporting::discard(TransferFrameTC *frame) {
        MasterChannelSsTc& mcChan = Objects::masterChannelSsTcMap.at(vcChan.getParentScid());
        if (!mcChan.channelMutex.tryLockFor(Defs::MutexDelayMs)) {
            return FARMNotification::FAILED_TO_LOCK_MUTEX;
        }

        if (Objects::frameOctetPool.poolMutex.tryLockFor(Defs::MutexDelayMs)) {
            mcChan.channelMutex.unlock();
            return FARMNotification::FAILED_TO_LOCK_MUTEX;
        }

        Objects::frameOctetPool.deleteBlock(frame->getFrameData(), frame->getFrameLength());
        mcChan.frameMasterCopies.erase(frame);
        Objects::frameOctetPool.poolMutex.unlock();
        mcChan.channelMutex.unlock();
        return FARMNotification::NO_FARM_EVENT;
    }

    FARMNotification FrameAcceptanceReporting::report() {
        MasterChannelSsTc& mcChan = Objects::masterChannelSsTcMap.at(vcChan.getParentScid());
        if (mcChan.channelMutex.tryLockFor(Defs::MutexDelayMs)) {
            return FARMNotification::FAILED_TO_LOCK_MUTEX;
        }

        clcwBuffer.emplace(CLCW(Defs::ControlWordTypeCLCW,
                          Defs::ClcwVersionNumber,
                          vcChan.getClcwStatusField(),
                          Defs::CopInEffect,
                          vcChan.getVcid(),
                          0,
                          mcChan.getNoRfAvailable(),
                          mcChan.getNoBitLock(),
                          lockout,
                          wait,
                          retransmit,
                          farmBCount,
                          false,
                          receiverFrameSeqNumber));

        mcChan.channelMutex.unlock();
        return FARMNotification::NO_FARM_EVENT;
    }

    Window FrameAcceptanceReporting::getWindow(const uint8_t frameSeqNumber) const {
        const auto positiveWindowBorder = static_cast<uint8_t>(
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

    etl::pair<FARMNotification, uint8_t> FrameAcceptanceReporting::applyFarmStateTable() {
        uint8_t eventCode = 0;
        FARMNotification farmNotification = FARMNotification::NO_FARM_EVENT;

        if (!clcwBufferMutex.tryLockFor(Defs::MutexDelayMs)) {
            return etl::make_pair(FARMNotification::FAILED_TO_LOCK_MUTEX, 0);
        }

        if (!vcChan.channelMutex.tryLockFor(Defs::MutexDelayMs)) {
            clcwBufferMutex.unlock();
            return etl::make_pair(FARMNotification::FAILED_TO_LOCK_MUTEX, 0);
        }

        /** CLCW report time **/
        if (timer.getRemainingTime() == 0) {
            // E11
            eventCode = 11;
            report();
            timer.startTimer(clcwReportInterval);
            ccsdsLogNotice(TxRx::Rx, NotificationType::TypeFARMNotif, farmNotification);
            clcwBufferMutex.unlock();
            vcChan.channelMutex.unlock();
            return etl::make_pair(farmNotification, eventCode);
        }

        /** Check if enough space is freed for a maximum sized Type-AD transfer frame **/
        bool mcChanHasEnoughSpace = false;
        MasterChannelSsTc& mcChan = Objects::masterChannelSsTcMap.at(vcChan.getParentScid());
        PhysicalChannel& phyChan = Objects::physicalChannelMap.at(mcChan.getParentPcid());
        if (!mcChan.channelMutex.tryLockFor(Defs::MutexDelayMs)) {
            vcChan.channelMutex.unlock();
            return etl::make_pair(FARMNotification::FAILED_TO_LOCK_MUTEX, 0);
        }
        if (!mcChan.frameMasterCopies.isFull() &&
            Objects::frameOctetPool.findFit(phyChan.getMaxTcFrameLength()).second != MasterChannelAlert::NO_MC_ALERT) {
            mcChanHasEnoughSpace = true;
        }
        mcChan.channelMutex.unlock();

        if (!vcChan.framesAfterVcReceptionTypeAD.isFull() &&
            mcChanHasEnoughSpace &&
            wait) {
            // E10
            eventCode = 10;
            wait = false;
            if (state == FARMState::WAIT) {
                state = FARMState::OPEN;
            }
            vcChan.channelMutex.unlock();
            return etl::make_pair(farmNotification, eventCode);
        }

        /** Frame arrival **/
        if (!vcChan.framesAfterAllFramesReception.isEmpty()) {
            TransferFrameTC *frameTc = vcChan.framesAfterAllFramesReception.getFront();

            if (frameTc->getServiceType() == Defs::ServiceType::TYPE_AD) {
                if (frameTc->getTransferFrameSequenceNumber() == receiverFrameSeqNumber) {
                    if (!vcChan.framesAfterAllFramesReception.isFull()) {
                        // E1
                        eventCode = 1;
                        if (state == FARMState::OPEN) {
                            if (accept(frameTc) == FARMNotification::FAILED_TO_LOCK_MUTEX) {
                                vcChan.channelMutex.unlock();
                                return etl::make_pair(FARMNotification::FAILED_TO_LOCK_MUTEX, 1);
                            }
                            receiverFrameSeqNumber = (receiverFrameSeqNumber == 255) ? 0 : (receiverFrameSeqNumber + 1);
                            retransmit = false;
                        } else if (state == FARMState::WAIT) {
                            farmNotification = FARMNotification::FARM_NON_APPLICABLE_COMBINATION_OF_STATE_AND_EVENT;
                        } else {
                            // state LOCKOUT
                            if (discard(frameTc) == FARMNotification::FAILED_TO_LOCK_MUTEX) {
                                vcChan.channelMutex.unlock();
                                return etl::make_pair(FARMNotification::FAILED_TO_LOCK_MUTEX, 1);
                            }
                        }
                    } else {
                        // no buffer available for TYPE-AD frames
                        // E2
                        eventCode = 2;
                        if (discard(frameTc) == FARMNotification::FAILED_TO_LOCK_MUTEX) {
                            vcChan.channelMutex.unlock();
                            return etl::make_pair(FARMNotification::FAILED_TO_LOCK_MUTEX, 2);
                        }
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
                        if (discard(frameTc) == FARMNotification::FAILED_TO_LOCK_MUTEX) {
                            vcChan.channelMutex.unlock();
                            return etl::make_pair(FARMNotification::FAILED_TO_LOCK_MUTEX, 3);
                        }
                        if (state == FARMState::OPEN) {
                            retransmit = true;
                        }
                    } else if (window == Window::NEGATIVE_WINDOW) {
                        // E4
                        eventCode = 4;
                        if (discard(frameTc) == FARMNotification::FAILED_TO_LOCK_MUTEX) {
                            vcChan.channelMutex.unlock();
                            return etl::make_pair(FARMNotification::FAILED_TO_LOCK_MUTEX, 4);
                        }
                    } else {
                        // Outside windows range
                        // E5
                        eventCode = 5;
                        if (discard(frameTc) == FARMNotification::FAILED_TO_LOCK_MUTEX) {
                            vcChan.channelMutex.unlock();
                            return etl::make_pair(FARMNotification::FAILED_TO_LOCK_MUTEX, 5);
                        }
                        lockout = true;
                        state = FARMState::LOCKOUT;
                    }
                }
            } else if (frameTc->getServiceType() == Defs::ServiceType::TYPE_BD) {
                // E6
                eventCode = 6;
                if (accept(frameTc) == FARMNotification::FAILED_TO_LOCK_MUTEX) {
                    vcChan.channelMutex.unlock();
                    return etl::make_pair(FARMNotification::FAILED_TO_LOCK_MUTEX, 6);
                }
                farmBCount = (farmBCount == 3) ? 0 : (farmBCount + 1);
            } else if (frameTc->getServiceType() == Defs::ServiceType::TYPE_BC) {
                const uint16_t expectedLenWithoutDataField =
                        Defs::TcPrimaryHeaderSize + phyChan.getFrameErrorControlFieldPresent() *
                        Defs::ErrorControlFieldSize;
                const uint16_t frameLength = frameTc->getFrameLength();
                uint8_t *frameData = frameTc->getFrameData();
                if ((frameLength == expectedLenWithoutDataField + Defs::UnlockCommandSize) &&
                    (frameData[Defs::TcPrimaryHeaderSize] ==
                     Defs::UnlockCommandOctet)) {
                    // E7 (unlock command)
                    eventCode = 7;
                    farmBCount = (farmBCount == 3) ? 0 : (farmBCount + 1);
                    retransmit = false;
                    wait = false;
                    lockout = false;
                    state = FARMState::OPEN;
                } else if ((frameLength == expectedLenWithoutDataField + Defs::SetVrCommandSize) &&
                           (frameData[Defs::TcPrimaryHeaderSize] ==
                            Defs::SetVrCommandOctet1) &&
                           (frameData[Defs::TcPrimaryHeaderSize + 1] ==
                            Defs::SetVrCommandOctet2)) {
                    // E8 (set V(R) command)
                    eventCode = 8;
                    farmBCount = (farmBCount == 3) ? 0 : (farmBCount + 1);
                    if (state == FARMState::OPEN || state == FARMState::WAIT) {
                        retransmit = false;
                        wait = false;
                        receiverFrameSeqNumber = frameData[Defs::TcPrimaryHeaderSize + 2];
                        state = FARMState::OPEN;
                    }
                } else {
                    // E9 (frame with invalid command)
                    eventCode = 9;
                }

                // Dispose frame. Invalid frames are disposed without any further action being taken.
                if (discard(frameTc) == FARMNotification::FAILED_TO_LOCK_MUTEX) {
                    vcChan.channelMutex.unlock();
                    return etl::make_pair(FARMNotification::FAILED_TO_LOCK_MUTEX, 0);
                }
            } else {
                // E9 (received a TYPE-RESERVED frame, which is invalid)
                eventCode = 9;
                if (discard(frameTc) == FARMNotification::FAILED_TO_LOCK_MUTEX) {
                    vcChan.channelMutex.unlock();
                    return etl::make_pair(FARMNotification::FAILED_TO_LOCK_MUTEX, 9);
                }
            }

            vcChan.framesAfterAllFramesReception.pop();
        }
        vcChan.channelMutex.unlock();
        return etl::make_pair(farmNotification, eventCode);
    }
#endif // INCLUDE_SPACE_SEGMENT_CODE
} // namespace CCSDSDataLinkLayer
