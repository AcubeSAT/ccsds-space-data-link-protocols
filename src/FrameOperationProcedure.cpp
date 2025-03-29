#include "FrameOperationProcedure.hpp"
#include "CCSDSChannel.hpp"
#include "CCSDSChannelsInterface.hpp"
#include "CCSDSLoggerImpl.h"

namespace CCSDSDataLinkLayer {
#ifdef GROUND_SEGMENT
    /** FOP-1 actions **/
    FOPNotification FrameOperationProcedure::purgeSentQueue() {
        etl::ilist<TransferFrameTC *>::iterator sent_queue_it = sentQueueFOP.begin();

        if (sent_queue_it == sentQueueFOP.end()) {
            ccsdsLogNotice(TxRx::Tx, NotificationType::TypeFOPNotif, FOPNotification::SENT_QUEUE_EMPTY);
            return FOPNotification::SENT_QUEUE_EMPTY;
        }

        while (sent_queue_it != sentQueueFOP.end()) {
            if ((*sent_queue_it)->getServiceType() == DefsAndUtils::ServiceType::TYPE_AD && !
                transferNotificationSignalQueue.full()) {
                transferNotificationSignalQueue.push(
                    DefsAndUtils::TransferNotificationSignal(DefsAndUtils::TransferNotificationType::NEGATIVE_CONFIRM_RESPONSE_TO_TRANSFER_FDU,
                                               *sent_queue_it));
            }

            if ((*sent_queue_it)->getServiceType() == DefsAndUtils::ServiceType::TYPE_BC &&
                !directiveNotificationSignalQueue.full()) {
                directiveNotificationSignalQueue.push(DefsAndUtils::DirectiveNotificationSignal(initiateWithBcFrameId.value(),
                    DefsAndUtils::DirectiveNotificationType::NEGATIVE_CONFIRM_RESPONSE_TO_DIRECTIVE,
                    *sent_queue_it));
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
            DefsAndUtils::TransferNotificationSignal(DefsAndUtils::TransferNotificationType::NEGATIVE_CONFIRM_RESPONSE_TO_TRANSFER_FDU,
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

        if (adFrame->getServiceType() != DefsAndUtils::ServiceType::TYPE_AD) {
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
            DefsAndUtils::FopToLowerLayerRequestSignal(DefsAndUtils::LowerLayerRequestType::LOW_LAYER_TRANSMIT,
                                         DefsAndUtils::ServiceType::TYPE_AD, adFrame));
        ccsdsLogNotice(TxRx::Tx, NotificationType::TypeFOPNotif, FOPNotification::NO_FOP_EVENT);
        return FOPNotification::NO_FOP_EVENT;
    }

    FOPNotification FrameOperationProcedure::transmitBcFrame(MasterChannelGroundSegmentVariant& masterChannelVariant,
        const DefsAndUtils::DirectiveRequestSignal &directiveSignal) {
        if (directiveSignal.directiveType != DefsAndUtils::DirectiveRequestType::INITIATE_AD_SERVICE_WITH_UNLOCK &&
            directiveSignal.directiveType != DefsAndUtils::DirectiveRequestType::INITIATE_AD_SERVICE_WITH_SET_VR) {
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

        // bc frames do not have a segmentation header, security header or security trailer
        const uint8_t dataFieldSize = (directiveSignal.directiveType == DefsAndUtils::DirectiveRequestType::INITIATE_AD_SERVICE_WITH_UNLOCK)
                                    ? DefsAndUtils::UnlockCommandSize
                                    : DefsAndUtils::SetVrCommandSize;
        uint8_t bcFrameLen = DefsAndUtils::TcPrimaryHeaderSize + dataFieldSize + errorControlFieldPresent *
                             DefsAndUtils::ErrorControlFieldSize;

        if (!ChannelsInterface::hasCapacityForFrameDataMasterChannelGroundSegment(
            masterChannelVariant,
            1, bcFrameLen)) {
            ccsdsLogNotice(TxRx::Tx, NotificationType::TypeFOPNotif, FOPNotification::FOP_MEMORY_POOL_OR_MASTER_COPY_BUFFER_FULL);
            return FOPNotification::FOP_MEMORY_POOL_OR_MASTER_COPY_BUFFER_FULL;
        }

        static uint8_t tmpData[DefsAndUtils::TcPrimaryHeaderSize + DefsAndUtils::SetVrCommandSize
                               + DefsAndUtils::ErrorControlFieldSize];

        if (directiveSignal.directiveType == DefsAndUtils::DirectiveRequestType::INITIATE_AD_SERVICE_WITH_UNLOCK) {
            tmpData[DefsAndUtils::TcPrimaryHeaderSize] = DefsAndUtils::UnlockCommandOctet;
        } else {
            tmpData[DefsAndUtils::TcPrimaryHeaderSize] = DefsAndUtils::SetVrCommandOctet1;
            tmpData[DefsAndUtils::TcPrimaryHeaderSize + 1] = DefsAndUtils::SetVrCommandOctet2;
            tmpData[DefsAndUtils::TcPrimaryHeaderSize + 2] = static_cast<uint8_t>(directiveSignal.
                directiveQualifier.value());
        }

        // place octets in the memory pool
        uint8_t *data = ChannelsInterface::addFrameOctetsToMemPoolMasterChannelGroundSegment(masterChannelVariant, tmpData, bcFrameLen).value();

        const BaseMasterChannel* baseMcChanPtr = ChannelsInterface::upcastToBase(masterChannelVariant);
        TransferFrameTC bcFrame = TransferFrameTC(data,
                                                  DefsAndUtils::ServiceType::TYPE_BC,
                                                  vid,
                                                  DefsAndUtils::extractScidFromMcid(baseMcChanPtr->getMscid()),
                                                  bcFrameLen,
                                                  false);

        bcFrame.setTransferFrameSequenceNumber(0); /// @see p. 4.2.1.8 of TC Data Link

        // store master copy
        ChannelsInterface::addFrameObjectToMasterCopyBufferMasterChannelGroundSegment(masterChannelVariant, bcFrame);

        // store to sent queue
        TransferFrameTC *bcFramePtr = &bcFrame;
        sentQueueFOP.push_back(bcFramePtr);

        transmissionCount = 1;

        timer.startTimer(tiInitial);

        bcOut = false;

        fopToLowerLayerRequestSignalQueue.push(
            DefsAndUtils::FopToLowerLayerRequestSignal(DefsAndUtils::LowerLayerRequestType::LOW_LAYER_TRANSMIT,
                                         DefsAndUtils::ServiceType::TYPE_BC, bcFramePtr));
        ccsdsLogNotice(TxRx::Tx, NotificationType::TypeFOPNotif, FOPNotification::NO_FOP_EVENT);
        return FOPNotification::NO_FOP_EVENT;
    }

    FOPNotification FrameOperationProcedure::transmitBdFrame(TransferFrameTC *bdFrame) {
        if (bdFrame->getServiceType() != DefsAndUtils::ServiceType::TYPE_BD) {
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
            DefsAndUtils::FopToLowerLayerRequestSignal(DefsAndUtils::LowerLayerRequestType::LOW_LAYER_TRANSMIT,
                                         DefsAndUtils::ServiceType::TYPE_BD, bdFrame));
        ccsdsLogNotice(TxRx::Tx, NotificationType::TypeFOPNotif, FOPNotification::NO_FOP_EVENT);
        return FOPNotification::NO_FOP_EVENT;
    }

    FOPNotification FrameOperationProcedure::initiateRetransmission(DefsAndUtils::ServiceType serviceType) {
        if ((serviceType != DefsAndUtils::ServiceType::TYPE_AD) && (
                serviceType != DefsAndUtils::ServiceType::TYPE_BC)) {
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
            DefsAndUtils::FopToLowerLayerRequestSignal(DefsAndUtils::LowerLayerRequestType::LOW_LAYER_ABORT, serviceType));
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
            if ((adFrame->getServiceType() == DefsAndUtils::ServiceType::TYPE_AD) &&
                DefsAndUtils::withinWindow(adFrame->getTransferFrameSequenceNumber(),
                                                      expectedAcknowledgementSeqNumber,
                                                      reportValue)) {
                // message higher layers about the successful reception
                if (!transferNotificationSignalQueue.full()) {
                    transferNotificationSignalQueue.push(
                        DefsAndUtils::TransferNotificationSignal(DefsAndUtils::TransferNotificationType::POSITIVE_CONFIRM_RESPONSE_TO_TRANSFER_FDU,
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
            if (((*sent_queue_it)->getServiceType() == DefsAndUtils::ServiceType::TYPE_BC) &&
                ((*sent_queue_it)->getToBeRetransmittedFlag())) {
                if (!fopToLowerLayerRequestSignalQueue.full()) {
                    fopToLowerLayerRequestSignalQueue.push(
                        DefsAndUtils::FopToLowerLayerRequestSignal(DefsAndUtils::LowerLayerRequestType::LOW_LAYER_TRANSMIT,
                                                     DefsAndUtils::ServiceType::TYPE_BC, *sent_queue_it));
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
            if (((*sent_queue_it)->getServiceType() == DefsAndUtils::ServiceType::TYPE_AD) &&
                ((*sent_queue_it)->getToBeRetransmittedFlag())) {
                if (!fopToLowerLayerRequestSignalQueue.full()) {
                    fopToLowerLayerRequestSignalQueue.push(
                        DefsAndUtils::FopToLowerLayerRequestSignal(DefsAndUtils::LowerLayerRequestType::LOW_LAYER_TRANSMIT,
                                                     DefsAndUtils::ServiceType::TYPE_AD, *sent_queue_it));
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

        if ((waitQueueFOP.front()->getServiceType() == DefsAndUtils::ServiceType::TYPE_AD) &&
            DefsAndUtils::withinWindow(transmitterFrameSeqNumber, expectedAcknowledgementSeqNumber,
                                                  upperBound)) {
            TransferFrameTC *adFrame = waitQueueFOP.front();
            const FOPNotification notification = transmitAdFrame(adFrame);
            if (notification == FOPNotification::NO_FOP_EVENT) {
                waitQueueFOP.pop_front();
                transferNotificationSignalQueue.push(
                    DefsAndUtils::TransferNotificationSignal(DefsAndUtils::TransferNotificationType::ACCEPT_RESPONSE_TO_TRANSFER_FDU, adFrame));
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
        clcwQueue.clear();

        transmissionCount = 1;
        suspendState = DefsAndUtils::SuspendVariableState::NOT_SUSPENDED;
    }

    void FrameOperationProcedure::alert(const DefsAndUtils::AlertEvent event) {
        timer.stopTimer();
        purgeSentQueue();
        purgeWaitQueue();

        /** A NEGATIVE_CONFIRM_RESPONSE_TO_DIRECTIVE for
         * 'initialize AD service with set V(R) or unlock' was sent in purgeSentQueue
         * Here, the same signal should also be sent for the 'initialize AD service with CLCW check' directive
         * @see p. 5.2.15 of COP-1 CCSDS
         */
        if (initiateWithClcwCheckId) {
            directiveNotificationSignalQueue.push(DefsAndUtils::DirectiveNotificationSignal(initiateWithClcwCheckId.value(),
                                                                              DefsAndUtils::DirectiveNotificationType::NEGATIVE_CONFIRM_RESPONSE_TO_DIRECTIVE));
            initiateWithClcwCheckId = etl::nullopt;
        }

        // send an alert signal
        asynchronousNotificationSignalQueue.push(
            DefsAndUtils::AsynchronousNotificationSignal(DefsAndUtils::AsynchronousNotificationType::ALERT, event));
    }

    void FrameOperationProcedure::resume() {
        timer.startTimer(tiInitial);
        suspendState = DefsAndUtils::SuspendVariableState::NOT_SUSPENDED;
    }
#endif // GROUND_SEGMENT
} // namespace CCSDSDataLinkLayer
