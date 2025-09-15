#include "FrameOperationProcedure.hpp"
#include "LoggerImpl.h"
#include "AddressingAndParsingUtilities.hpp"
#include "Mutex.hpp"
#include "ChannelObjects.hpp"

namespace CCSDSDataLinkLayer {
#ifdef INCLUDE_GROUND_SEGMENT_CODE
    FrameOperationProcedure::FrameOperationProcedure(const Defs::Scid scid, const Defs::Vcid vcid, const uint16_t tiInitial,
                        const uint16_t transmissionLimit,
                        const uint8_t fopSlidingWindowWidth)
    : state(Defs::FOPState::INITIAL), transmitterFrameSeqNumber(0), adOut(true),
      bdOut(true), bcOut(true), expectedAcknowledgementSeqNumber(0),
      tiInitial(tiInitial), transmissionLimit(transmissionLimit), transmissionCount(1),
      fopSlidingWindowWidth(fopSlidingWindowWidth), timeoutType(false),
      suspendState(Defs::SuspendVariableState::NOT_SUSPENDED),
      signalQueueMutex(Mutex()), vcChan(Objects::virtualChannelGsTcMap.at(constructVcidScidKey(vcid, scid))),
      mcChan(Objects::masterChannelGsTcMap.at(scid)),
      timer(CountdownTimer()) {}

    void FrameOperationProcedure::resetFOP() {
        state = Defs::FOPState::INITIAL;
        transmitterFrameSeqNumber = 0;
        adOut = true;
        bdOut = true;
        bcOut = true;
        expectedAcknowledgementSeqNumber = 0;
        transmissionCount = 0;
        timer.stopTimer();
        directiveRequestSignalQueue.clear();
        transferFduSignalQueue.clear();
        lowerLayerResponseSignalQueue.clear();
        directiveRequestSignalQueue.clear();
        directiveNotificationSignalQueueUser.clear();
        asynchronousNotificationSignalQueue.clear();
        transferFduSignalQueue.clear();
        fopToLowerLayerRequestSignalQueue.clear();
        initiateWithClcwCheckId.reset();
        initiateWithBcFrameId.reset();

        // clear frame master copies
        TransferFrameTC* frameTcPtr;
        if (!waitQueueFOP.empty()) {
            frameTcPtr = waitQueueFOP.front();
            waitQueueFOP.pop_front();

            for (auto frameTc : mcChan.frameMasterCopies) {
                if (&frameTc == frameTcPtr) {
                    Objects::frameOctetPool.deleteBlock(frameTcPtr->getFrameData());
                    mcChan.frameMasterCopies.erase(frameTcPtr);
                    break;
                }
            }
        }

        while (!sentQueueFOP.empty()) {
            frameTcPtr = sentQueueFOP.front();
            sentQueueFOP.pop_front();

            for (auto frameTc : mcChan.frameMasterCopies) {
                if (&frameTc == frameTcPtr) {
                    Objects::frameOctetPool.deleteBlock(frameTcPtr->getFrameData());
                    mcChan.frameMasterCopies.erase(frameTcPtr);
                    break;
                }
            }
        }
    }

    /** FOP-1 actions **/
    FOPNotification FrameOperationProcedure::purgeSentQueue() {
        etl::ilist<TransferFrameTC *>::iterator sent_queue_it = sentQueueFOP.begin();

        if (sent_queue_it == sentQueueFOP.end()) {
            ccsdsLogNotice(TxRx::Tx, NotificationType::TypeFOPNotif, FOPNotification::SENT_QUEUE_EMPTY);
            return FOPNotification::SENT_QUEUE_EMPTY;
        }

        while (sent_queue_it != sentQueueFOP.end()) {
            if ((*sent_queue_it)->getServiceType() == Defs::ServiceType::TYPE_AD && !
                transferNotificationSignalQueue.full()) {
                transferNotificationSignalQueue.push(
                    TransferNotificationSignal(TransferNotificationType::NEGATIVE_CONFIRM_RESPONSE_TO_TRANSFER_FDU,
                                               *sent_queue_it));
            }

            if ((*sent_queue_it)->getServiceType() == Defs::ServiceType::TYPE_BC &&
                !directiveNotificationSignalQueue.full()) {
                directiveNotificationSignalQueue.push(DirectiveNotificationSignal(initiateWithBcFrameId.value(),
                    DirectiveNotificationType::NEGATIVE_CONFIRM_RESPONSE_TO_DIRECTIVE,
                    *sent_queue_it));
                directiveNotificationSignalQueueUser.push(
                                            DirectiveNotificationSignalUser(initiateWithBcFrameId.value(),
                                                                        DirectiveNotificationType::ACCEPT_RESPONSE_TO_DIRECTIVE));
            }

            // remove from sent queue
            sent_queue_it = sentQueueFOP.erase(sent_queue_it);
        }

        initiateWithBcFrameId = etl::nullopt;

        ccsdsLogNotice(TxRx::Tx, NotificationType::TypeFOPNotif, FOPNotification::NO_FOP_EVENT);
        return FOPNotification::NO_FOP_EVENT;
    }

    FOPNotification FrameOperationProcedure::purgeWaitQueue() {
        if (waitQueueFOP.empty()) {
            ccsdsLogNotice(TxRx::Tx, NotificationType::TypeFOPNotif, FOPNotification::WAIT_QUEUE_EMPTY);
            return FOPNotification::WAIT_QUEUE_EMPTY;
        }

        if (transferNotificationSignalQueue.full()) {
            ccsdsLogNotice(TxRx::Tx, NotificationType::TypeFOPNotif, FOPNotification::SIGNAL_QUEUE_FULL);
            return FOPNotification::SIGNAL_QUEUE_FULL;
        }

        etl::ilist<TransferFrameTC *>::iterator wait_queue_it = waitQueueFOP.begin();

        transferNotificationSignalQueue.push(
            TransferNotificationSignal(TransferNotificationType::NEGATIVE_CONFIRM_RESPONSE_TO_TRANSFER_FDU,
                                       *wait_queue_it));

        // remove pointer from sent queue
        sentQueueFOP.erase(wait_queue_it);

        ccsdsLogNotice(TxRx::Tx, NotificationType::TypeFOPNotif, FOPNotification::NO_FOP_EVENT);
        return FOPNotification::NO_FOP_EVENT;
    }

    // TODO ensure fop does not break in case the sent/wait/signal queues are full. this is true for the other 2 transmit functions as well
    FOPNotification FrameOperationProcedure::transmitAdFrame(TransferFrameTC *adFrame) {
        if (sentQueueFOP.full()) {
            ccsdsLogNotice(TxRx::Tx, NotificationType::TypeFOPNotif, FOPNotification::SENT_QUEUE_FULL);
            return FOPNotification::SENT_QUEUE_FULL;
        }

        if (adFrame->getServiceType() != Defs::ServiceType::TYPE_AD) {
            ccsdsLogNotice(TxRx::Tx, NotificationType::TypeFOPNotif, FOPNotification::FOP_UNEXPECTED_VALUE);
            return FOPNotification::FOP_UNEXPECTED_VALUE;
        }

        adFrame->setTransferFrameSequenceNumber(transmitterFrameSeqNumber);
        if (!adFrame->getToBeRetransmittedFlag()) {
            transmitterFrameSeqNumber = (transmitterFrameSeqNumber == 255) ? 0 : (transmitterFrameSeqNumber + 1);
        }

        adFrame->setToBeRetransmittedFlag(false);
        sentQueueFOP.push_back(adFrame);

        if (sentQueueFOP.empty()) {
            transmissionCount = 1;
        }

        timer.startTimer(tiInitial);

        adOut = false;
        fopToLowerLayerRequestSignalQueue.push(
            FopToLowerLayerRequestSignal(LowerLayerRequestType::LOW_LAYER_TRANSMIT, adFrame));
        ccsdsLogNotice(TxRx::Tx, NotificationType::TypeFOPNotif, FOPNotification::NO_FOP_EVENT);
        return FOPNotification::NO_FOP_EVENT;
    }

    FOPNotification FrameOperationProcedure::transmitBcFrame(const DirectiveRequestSignal &directiveSignal) {
        if (directiveSignal.directiveType != DirectiveRequestType::INITIATE_AD_SERVICE_WITH_UNLOCK &&
            directiveSignal.directiveType != DirectiveRequestType::INITIATE_AD_SERVICE_WITH_SET_VR) {
            ccsdsLogNotice(TxRx::Tx, NotificationType::TypeFOPNotif, FOPNotification::FOP_UNEXPECTED_VALUE);
            return FOPNotification::FOP_UNEXPECTED_VALUE;
        }

        if (!directiveSignal.directiveQualifier) {
            ccsdsLogNotice(TxRx::Tx, NotificationType::TypeFOPNotif, FOPNotification::FOP_UNEXPECTED_VALUE);
            return FOPNotification::FOP_UNEXPECTED_VALUE;
        }

        if (!directiveSignal.requestIdentifier) {
            ccsdsLogNotice(TxRx::Tx, NotificationType::TypeFOPNotif, FOPNotification::FOP_UNEXPECTED_VALUE);
            return FOPNotification::FOP_UNEXPECTED_VALUE;
        }

        if (sentQueueFOP.full()) {
            ccsdsLogNotice(TxRx::Tx, NotificationType::TypeFOPNotif, FOPNotification::SENT_QUEUE_FULL);
            return FOPNotification::SENT_QUEUE_FULL;
        }

        if (fopToLowerLayerRequestSignalQueue.full()) {
            ccsdsLogNotice(TxRx::Tx, NotificationType::TypeFOPNotif, FOPNotification::SIGNAL_QUEUE_FULL);
            return FOPNotification::SIGNAL_QUEUE_FULL;
        }

        if (!mcChan.channelMutex.tryLockFor(Defs::MutexDelayMs)) {
            return FOPNotification::FAILED_TO_LOCK_MUTEX;
        }

        if (!Objects::frameOctetPool.poolMutex.tryLockFor(Defs::MutexDelayMs)) {
            mcChan.channelMutex.unlock();
            return FOPNotification::FAILED_TO_LOCK_MUTEX;
        }

        // bc frames do not have a segmentation header, security header or security trailer
        bool errorControlFieldPresent = Objects::physicalChannelMap.at(mcChan.getParentPcid()).getFrameErrorControlFieldPresent();
        const uint8_t dataFieldSize = (directiveSignal.directiveType == DirectiveRequestType::INITIATE_AD_SERVICE_WITH_UNLOCK)
                                    ? Defs::UnlockCommandSize
                                    : Defs::SetVrCommandSize;
        uint8_t bcFrameLen = Defs::TcPrimaryHeaderSize + dataFieldSize + errorControlFieldPresent *
            Defs::ErrorControlFieldSize;


        if (mcChan.frameMasterCopies.isFull() ||
            Objects::frameOctetPool.findFit(bcFrameLen).second != MasterChannelAlert::NO_MC_ALERT) {
            Objects::frameOctetPool.poolMutex.unlock();
            mcChan.channelMutex.unlock();
            return FOPNotification::FOP_MEMORY_POOL_OR_MASTER_COPY_BUFFER_FULL;
        }

        // allocate a block for the frame data in the memory pool
        uint8_t *frameData = Objects::frameOctetPool.allocateBlock(bcFrameLen, nullptr);

        if (directiveSignal.directiveType == DirectiveRequestType::INITIATE_AD_SERVICE_WITH_UNLOCK) {
            frameData[Defs::TcPrimaryHeaderSize] = Defs::UnlockCommandOctet;
        } else {
            frameData[Defs::TcPrimaryHeaderSize] = Defs::SetVrCommandOctet1;
            frameData[Defs::TcPrimaryHeaderSize + 1] = Defs::SetVrCommandOctet2;
            frameData[Defs::TcPrimaryHeaderSize + 2] = static_cast<uint8_t>(directiveSignal.
                directiveQualifier.value());
        }

        // create and store frame master copy
        TransferFrameTC* frameTcPtr = mcChan.frameMasterCopies.push(
            TransferFrameTC(
                frameData,
                Defs::ServiceType::TYPE_BC,
                vcChan.getVcid(),
                mcChan.getScid(),
                bcFrameLen,
                false,
                Defs::SequenceFlag::NO_SEGMENTATION));


        frameTcPtr->setTransferFrameSequenceNumber(0); /// @see p. 4.2.1.8 of TC Data Link
        mcChan.channelMutex.unlock();
        
        // store to sent queue
        sentQueueFOP.push_back(frameTcPtr);

        transmissionCount = 1;

        timer.startTimer(tiInitial);

        bcOut = false;

        fopToLowerLayerRequestSignalQueue.push(
            FopToLowerLayerRequestSignal(LowerLayerRequestType::LOW_LAYER_TRANSMIT, frameTcPtr));

        Objects::frameOctetPool.poolMutex.unlock();
        mcChan.channelMutex.unlock();
        return FOPNotification::NO_FOP_EVENT;
    }

    FOPNotification FrameOperationProcedure::transmitBdFrame(TransferFrameTC *bdFrame) {
        if (bdFrame->getServiceType() != Defs::ServiceType::TYPE_BD) {
            ccsdsLogNotice(TxRx::Tx, NotificationType::TypeFOPNotif, FOPNotification::FOP_UNEXPECTED_VALUE);
            return FOPNotification::FOP_UNEXPECTED_VALUE;
        }

        if (fopToLowerLayerRequestSignalQueue.full()) {
            ccsdsLogNotice(TxRx::Tx, NotificationType::TypeFOPNotif, FOPNotification::SIGNAL_QUEUE_FULL);
            return FOPNotification::SIGNAL_QUEUE_FULL;
        }

        bdOut = false;
        bdFrame->setTransferFrameSequenceNumber(0); /// @see p. 4.2.1.8 of TC Data Link

        fopToLowerLayerRequestSignalQueue.push(
            FopToLowerLayerRequestSignal(LowerLayerRequestType::LOW_LAYER_TRANSMIT, bdFrame));
        ccsdsLogNotice(TxRx::Tx, NotificationType::TypeFOPNotif, FOPNotification::NO_FOP_EVENT);
        return FOPNotification::NO_FOP_EVENT;
    }

    FOPNotification FrameOperationProcedure::initiateRetransmission(Defs::ServiceType serviceType) {
        if ((serviceType != Defs::ServiceType::TYPE_AD) && (
                serviceType != Defs::ServiceType::TYPE_BC)) {
            ccsdsLogNotice(TxRx::Tx, NotificationType::TypeFOPNotif, FOPNotification::FOP_UNEXPECTED_VALUE);
            return FOPNotification::FOP_UNEXPECTED_VALUE;
        }

        if (sentQueueFOP.empty()) {
            ccsdsLogNotice(TxRx::Tx, NotificationType::TypeFOPNotif, FOPNotification::SENT_QUEUE_EMPTY);
            return FOPNotification::SENT_QUEUE_EMPTY;
        }

        if (fopToLowerLayerRequestSignalQueue.full()) {
            ccsdsLogNotice(TxRx::Tx, NotificationType::TypeFOPNotif, FOPNotification::SIGNAL_QUEUE_FULL);
            return FOPNotification::SIGNAL_QUEUE_FULL;
        }

        fopToLowerLayerRequestSignalQueue.push(
            FopToLowerLayerRequestSignal(LowerLayerRequestType::LOW_LAYER_ABORT));
        transmissionCount = (transmissionCount == 255) ? 0 : (transmissionCount + 1);
        timer.startTimer(tiInitial);

        for (TransferFrameTC *frame: sentQueueFOP) {
            if (frame->getServiceType() == serviceType) {
                frame->setToBeRetransmittedFlag(true);
            }
        }

        ccsdsLogNotice(TxRx::Tx, NotificationType::TypeFOPNotif, FOPNotification::NO_FOP_EVENT);
        return FOPNotification::NO_FOP_EVENT;
    }

    FOPNotification FrameOperationProcedure::removeAcknowledgedFramesFromSentQueue(const uint8_t reportValue) {
        if (sentQueueFOP.empty()) {
            ccsdsLogNotice(TxRx::Tx, NotificationType::TypeFOPNotif, FOPNotification::SENT_QUEUE_EMPTY);
            return FOPNotification::SENT_QUEUE_EMPTY;
        }

        etl::ilist<TransferFrameTC *>::iterator sent_queue_it = sentQueueFOP.begin();
        TransferFrameTC *adFrame;
        while (sent_queue_it != sentQueueFOP.end()) {
            adFrame = *sent_queue_it;
            if ((adFrame->getServiceType() == Defs::ServiceType::TYPE_AD) &&
                withinWindow(adFrame->getTransferFrameSequenceNumber(),
                                                      expectedAcknowledgementSeqNumber,
                                                      reportValue)) {
                // message higher layers about the successful reception
                if (!transferNotificationSignalQueue.full()) {
                    transferNotificationSignalQueue.push(
                        TransferNotificationSignal(TransferNotificationType::POSITIVE_CONFIRM_RESPONSE_TO_TRANSFER_FDU,
                                                   adFrame));
                }

                // delete pointer from the sent queue
                sent_queue_it = sentQueueFOP.erase(sent_queue_it); // erase() returns the iterator to the next element
                continue;
            }
            ++sent_queue_it; // This frame was either not TYPE_AD or not acknowledged, move to the next one.
        }

        expectedAcknowledgementSeqNumber = reportValue;
        transmissionCount = 1;
        ccsdsLogNotice(TxRx::Tx, NotificationType::TypeFOPNotif, FOPNotification::NO_FOP_EVENT);
        return FOPNotification::NO_FOP_EVENT;
    }

    FOPNotification FrameOperationProcedure::lookForDirective() {
        if (!bcOut) {
            ccsdsLogNotice(TxRx::Tx, NotificationType::TypeFOPNotif, FOPNotification::NO_FOP_EVENT);
            return FOPNotification::NO_FOP_EVENT;
        }

        if (sentQueueFOP.empty()) {
            ccsdsLogNotice(TxRx::Tx, NotificationType::TypeFOPNotif, FOPNotification::SENT_QUEUE_EMPTY);
            return FOPNotification::SENT_QUEUE_EMPTY;
        }

        etl::ilist<TransferFrameTC *>::iterator sent_queue_it = sentQueueFOP.begin();
        while (sent_queue_it != sentQueueFOP.end()) {
            if (((*sent_queue_it)->getServiceType() == Defs::ServiceType::TYPE_BC) &&
                ((*sent_queue_it)->getToBeRetransmittedFlag())) {
                if (!fopToLowerLayerRequestSignalQueue.full()) {
                    fopToLowerLayerRequestSignalQueue.push(
                        FopToLowerLayerRequestSignal(LowerLayerRequestType::LOW_LAYER_TRANSMIT, *sent_queue_it));
                }

                // NOTE: Resting the retransmission flag to false is not included in the protocol, but the corresponding action
                // for TYPE-AD frames does (see lookForFdu()), so it might have been an omission.
                (*sent_queue_it)->setToBeRetransmittedFlag(false);
                bcOut = false;
                break;
            }
        }

        ccsdsLogNotice(TxRx::Tx, NotificationType::TypeFOPNotif, FOPNotification::NO_FOP_EVENT);
        return FOPNotification::NO_FOP_EVENT;
    }

    FOPNotification FrameOperationProcedure::lookForFdu() {
        if (!adOut) {
            ccsdsLogNotice(TxRx::Tx, NotificationType::TypeFOPNotif, FOPNotification::NO_FOP_EVENT);
            return FOPNotification::NO_FOP_EVENT;
        }

        if (sentQueueFOP.empty()) {
            ccsdsLogNotice(TxRx::Tx, NotificationType::TypeFOPNotif, FOPNotification::SENT_QUEUE_EMPTY);
            return FOPNotification::SENT_QUEUE_EMPTY;
        }

        // Check if there are TYPE-AD marked 'toBeRetransmitted'. If so, retransmit the first one
        etl::ilist<TransferFrameTC *>::iterator sent_queue_it = sentQueueFOP.begin();
        while (sent_queue_it != sentQueueFOP.end()) {
            if (((*sent_queue_it)->getServiceType() == Defs::ServiceType::TYPE_AD) &&
                ((*sent_queue_it)->getToBeRetransmittedFlag())) {
                if (!fopToLowerLayerRequestSignalQueue.full()) {
                    fopToLowerLayerRequestSignalQueue.push(
                        FopToLowerLayerRequestSignal(LowerLayerRequestType::LOW_LAYER_TRANSMIT, *sent_queue_it));
                }

                (*sent_queue_it)->setToBeRetransmittedFlag(false);
                adOut = false;

                ccsdsLogNotice(TxRx::Tx, NotificationType::TypeFOPNotif, FOPNotification::NO_FOP_EVENT);
                return FOPNotification::NO_FOP_EVENT;
            }
        }

        // No TYPE-AD frame is marked 'toBeRetransmitted'. See if the wait queue has an fdu
        // such that V(S) < NN(R) + K, and transmit it
        if (waitQueueFOP.empty()) {
            ccsdsLogNotice(TxRx::Tx, NotificationType::TypeFOPNotif, FOPNotification::NO_FOP_EVENT);
            return FOPNotification::NO_FOP_EVENT;
        }

        // calculate NN(R) + (K - 1)
        const auto upperBound = static_cast<uint8_t>(
            (static_cast<uint16_t>(expectedAcknowledgementSeqNumber) + fopSlidingWindowWidth - 1) & 0xFF);

        if ((waitQueueFOP.front()->getServiceType() == Defs::ServiceType::TYPE_AD) &&
            withinWindow(transmitterFrameSeqNumber, expectedAcknowledgementSeqNumber,
                                                  upperBound)) {
            TransferFrameTC *adFrame = waitQueueFOP.front();
            const FOPNotification notification = transmitAdFrame(adFrame);
            if (notification == FOPNotification::NO_FOP_EVENT) {
                waitQueueFOP.pop_front();
                transferNotificationSignalQueue.push(
                    TransferNotificationSignal(TransferNotificationType::ACCEPT_RESPONSE_TO_TRANSFER_FDU, adFrame));
            } else {
                ccsdsLogNotice(TxRx::Tx, NotificationType::TypeFOPNotif, notification);
                return notification;
            }
        }

        ccsdsLogNotice(TxRx::Tx, NotificationType::TypeFOPNotif, FOPNotification::NO_FOP_EVENT);
        return FOPNotification::NO_FOP_EVENT;
    }

    void FrameOperationProcedure::initialize() {
        purgeSentQueue();
        purgeWaitQueue();

        directiveNotificationSignalQueue.clear();
        transferFduSignalQueue.clear();
        lowerLayerResponseSignalQueue.clear();
        clcwBuffer.reset();

        transmissionCount = 1;
        suspendState = Defs::SuspendVariableState::NOT_SUSPENDED;
    }

    void FrameOperationProcedure::alert(const AlertEvent event) {
        timer.stopTimer();
        purgeSentQueue();
        purgeWaitQueue();

        /** A NEGATIVE_CONFIRM_RESPONSE_TO_DIRECTIVE for
         * 'initialize AD service with set V(R) or unlock' was sent in purgeSentQueue
         * Here, the same signal should also be sent for the 'initialize AD service with CLCW check' directive
         * @see p. 5.2.15 of COP-1 CCSDS
         */
        if (initiateWithClcwCheckId) {
            directiveNotificationSignalQueue.push(DirectiveNotificationSignal(initiateWithClcwCheckId.value(),
                                                                              DirectiveNotificationType::NEGATIVE_CONFIRM_RESPONSE_TO_DIRECTIVE));
            directiveNotificationSignalQueueUser.push(
                                            DirectiveNotificationSignalUser(initiateWithBcFrameId.value(),
                                                                        DirectiveNotificationType::NEGATIVE_CONFIRM_RESPONSE_TO_DIRECTIVE));
            initiateWithClcwCheckId = etl::nullopt;
        }

        // send an alert signal
        asynchronousNotificationSignalQueue.push(
            AsynchronousNotificationSignal(AsynchronousNotificationType::ALERT, event));
    }

    void FrameOperationProcedure::resume() {
        timer.startTimer(tiInitial);
        suspendState = Defs::SuspendVariableState::NOT_SUSPENDED;
    }
#endif // INCLUDE_GROUND_SEGMENT_CODE
} // namespace CCSDSDataLinkLayer
