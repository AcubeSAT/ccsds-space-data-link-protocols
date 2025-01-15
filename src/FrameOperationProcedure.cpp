#include <FrameOperationProcedure.hpp>
#include <CCSDSChannel.hpp>
#include "CCSDSLoggerImpl.h"

/** FOP-1 actions **/
FOPNotification FrameOperationProcedure::purgeSentQueue() {
	etl::ilist<TransferFrameTC*>::iterator sent_queue_it = sentQueueFOP.begin();
    etl::ilist<TransferFrameTC>::iterator master_copy_it;

    if (sent_queue_it == sentQueueFOP.end()) {
        ccsdsLogNotice(Tx, TypeFOPNotif, SENT_QUEUE_EMPTY);
        return SENT_QUEUE_EMPTY;
    };


	while (sent_queue_it != sentQueueFOP.end()) {
		if ((*sent_queue_it)->getServiceType() == ServiceType::TYPE_AD && !transferNotificationSignalQueue.full()) {
            transferNotificationSignalQueue.push(TransferNotificationSignal((*sent_queue_it)->getTransferRequestId(),
                                                                            NEGATIVE_CONFIRM_RESPONSE_TO_TRANSFER_FDU));
        }

        if ((*sent_queue_it)->getServiceType() == ServiceType::TYPE_BC && !directiveNotificationSignalQueue.full()) {
            directiveNotificationSignalQueue.push(DirectiveNotificationSignal((*sent_queue_it)->getTransferRequestId(),
                                                                              NEGATIVE_CONFIRM_RESPONSE_TO_DIRECTIVE));
        }


        // delete frame octets
		memoryPool.deletePacket((*sent_queue_it)->getFrameData(), (*sent_queue_it)->getFrameLength());

        // delete frame master copy
        for (master_copy_it = frameMasterCopyBuffer.begin(); master_copy_it != frameMasterCopyBuffer.end(); ++master_copy_it) {
            if (&(*master_copy_it) == *sent_queue_it) {
                frameMasterCopyBuffer.erase(master_copy_it);
                break;
            }
        }

        // remove from sent queue
        sentQueueFOP.erase(sent_queue_it++);
	}

	ccsdsLogNotice(Tx, TypeFOPNotif, NO_FOP_EVENT);
	return NO_FOP_EVENT;
}

FOPNotification FrameOperationProcedure::purgeWaitQueue() {

    if (waitQueueFOP.empty()) {
        ccsdsLogNotice(Tx, TypeFOPNotif, WAIT_QUEUE_EMPTY);
        return WAIT_QUEUE_EMPTY;
    };

    if (transferNotificationSignalQueue.full()) {
        ccsdsLogNotice(Tx, TypeFOPNotif, SIGNAL_QUEUE_FULL);
        return SIGNAL_QUEUE_FULL;
    }

    etl::ilist<TransferFrameTC*>::iterator wait_queue_it = waitQueueFOP.begin();
    uint8_t requestIdentifier = (*wait_queue_it)->getTransferRequestId();

    // delete frame octets
    memoryPool.deletePacket((*wait_queue_it)->getFrameData(), (*wait_queue_it)->getFrameLength());

    // delete frame master copy
    etl::ilist<TransferFrameTC>::iterator master_copy_it;
    for (master_copy_it = frameMasterCopyBuffer.begin(); master_copy_it != frameMasterCopyBuffer.end(); ++master_copy_it) {
        if (&master_copy_it == *wait_queue_it) {
            frameMasterCopyBuffer.erase(master_copy_it);
            break;
        }
    }

    // remove pointer from sent queue
    sentQueueFOP.erase(wait_queue_it++);

    transferNotificationSignalQueue.push(TransferNotificationSignal(requestIdentifier, NEGATIVE_CONFIRM_RESPONSE_TO_TRANSFER_FDU));
    ccsdsLogNotice(Tx, TypeFOPNotif, NO_FOP_EVENT);
    return NO_FOP_EVENT;
}

// TODO ensure fop does not break in case the sent/wait/signal queues are full. this is true for the other 2 transmit functions as well
FOPNotification FrameOperationProcedure::transmitAdFrame(TransferFrameTC* adFrame) {
	if (sentQueueFOP.full()) {
		ccsdsLogNotice(Tx, TypeFOPNotif, SENT_QUEUE_FULL);
		return FOPNotification::SENT_QUEUE_FULL;
	}

    if (adFrame->getServiceType() != ServiceType::TYPE_AD) {
        ccsdsLogNotice(Tx, TypeFOPNotif, FOP_UNEXPECTED_VALUE);
        return FOPNotification::FOP_UNEXPECTED_VALUE;
    }

    adFrame->setTransferFrameSequenceNumber(transmitterFrameSeqNumber);
    if(!adFrame->isToBeRetransmitted()) {
        transmitterFrameSeqNumber = (transmitterFrameSeqNumber == 255) ? 0 : (transmitterFrameSeqNumber + 1);
    }

	adFrame->setToBeRetransmitted(false);
	sentQueueFOP.push_back(adFrame);

    if (sentQueueFOP.empty()) {
        transmissionCount = 1;
    }

	timer.startTimer(tiInitial);

    adOut = NOT_READY;
    fopToLowerLayerRequestSignalQueue.push(FopToLowerLayerRequestSignal(LOW_LAYER_TRANSMIT, ServiceType::TYPE_AD, adFrame));
    ccsdsLogNotice(Tx, TypeFOPNotif, NO_FOP_EVENT);
	return FOPNotification::NO_FOP_EVENT;
}

FOPNotification FrameOperationProcedure::transmitBcFrame(const DirectiveRequestSignal& directiveSignal) {
    if (directiveSignal.directiveType != INITIATE_AD_SERVICE_WITH_UNLOCK &&
        directiveSignal.directiveType != INITIATE_AD_SERVICE_WITH_SET_VR) {
        ccsdsLogNotice(Tx, TypeFOPNotif, FOP_UNEXPECTED_VALUE);
        return FOPNotification::FOP_UNEXPECTED_VALUE;
    }

    if (!directiveSignal.directiveQualifier) {
        ccsdsLogNotice(Tx, TypeFOPNotif, FOP_UNEXPECTED_VALUE);
        return FOPNotification::FOP_UNEXPECTED_VALUE;
    }

    if (!directiveSignal.requestIdentifier) {
        ccsdsLogNotice(Tx, TypeFOPNotif, FOP_UNEXPECTED_VALUE);
        return FOPNotification::FOP_UNEXPECTED_VALUE;
    }

    if (sentQueueFOP.full()) {
        ccsdsLogNotice(Tx, TypeFOPNotif, SENT_QUEUE_FULL);
        return FOPNotification::SENT_QUEUE_FULL;
    }

    if (fopToLowerLayerRequestSignalQueue.full()) {
        ccsdsLogNotice(Tx, TypeFOPNotif, SIGNAL_QUEUE_FULL);
        return FOPNotification::SIGNAL_QUEUE_FULL;
    }

    // bc frames do not have a segmentation header, security header or security trailer
    uint8_t dataFieldSize = (directiveSignal.directiveType == INITIATE_AD_SERVICE_WITH_UNLOCK) ? UnlockCommandSize : SetVrCommandSize;
    uint8_t bcFrameLen = TcPrimaryHeaderSize + dataFieldSize + vchan->frameErrorControlFieldPresent * ErrorControlFieldSize;

    if (memoryPool.findFit(bcFrameLen).second != NO_MC_ALERT) {
        ccsdsLogNotice(Tx, TypeFOPNotif, FOP_MEMORY_POOL_FULL);
        return FOP_MEMORY_POOL_FULL;
    }

    if (frameMasterCopyBuffer.full()) {
        ccsdsLogNotice(Tx, TypeFOPNotif, FOP_MASTER_COPY_BUFFER_FULL);
        return FOP_MASTER_COPY_BUFFER_FULL;
    }

    static uint8_t tmpData[TcPrimaryHeaderSize + SetVrCommandSize + ErrorControlFieldSize];

    if (directiveSignal.directiveType == INITIATE_AD_SERVICE_WITH_UNLOCK) {
        tmpData[TcPrimaryHeaderSize] = UnlockCommand;
    }
    else {
        tmpData[TcPrimaryHeaderSize] = SetVrCommandOctet1;
        tmpData[TcPrimaryHeaderSize + 1] = SetVrCommandOctet2;
        tmpData[TcPrimaryHeaderSize + 2] = static_cast<uint8_t>(directiveSignal.directiveQualifier.value());
    }

    // place octets in the memory pool
    uint8_t* data = memoryPool.allocatePacket(tmpData, bcFrameLen);

    TransferFrameTC bcFrame = TransferFrameTC(data,
                                              ServiceType::TYPE_BC,
                                              vchan->VCID,
                                              bcFrameLen,
                                              false);
    bcFrame.setToBeRetransmitted(false);
    bcFrame.setTransferFrameSequenceNumber(0); /// @see p. 4.2.1.8 of TC Data Link
    bcFrame.setTransferRequestId(directiveSignal.requestIdentifier);

    // store master copy
    frameMasterCopyBuffer.push_back(bcFrame);

    // store to sent queue
    TransferFrameTC* bcFramePtr = &bcFrame;
    sentQueueFOP.push_back(bcFramePtr);

	transmissionCount = 1;

	timer.startTimer(tiInitial);

    bcOut = NOT_READY;

    fopToLowerLayerRequestSignalQueue.push(FopToLowerLayerRequestSignal(LOW_LAYER_TRANSMIT, ServiceType::TYPE_BC, bcFramePtr));
	ccsdsLogNotice(Tx, TypeFOPNotif, NO_FOP_EVENT);
	return FOPNotification::NO_FOP_EVENT;
}

FOPNotification FrameOperationProcedure::transmitBdFrame(TransferFrameTC* bdFrame) {
	if (bdFrame->getServiceType() != ServiceType::TYPE_BD) {
        ccsdsLogNotice(Tx, TypeFOPNotif, FOP_UNEXPECTED_VALUE);
        return FOPNotification::FOP_UNEXPECTED_VALUE;
    }

    if (fopToLowerLayerRequestSignalQueue.full()) {
        ccsdsLogNotice(Tx, TypeFOPNotif, SIGNAL_QUEUE_FULL);
        return FOPNotification::SIGNAL_QUEUE_FULL;
    }

    bdOut = NOT_READY;
    bdFrame->setTransferFrameSequenceNumber(0);  /// @see p. 4.2.1.8 of TC Data Link
    bdFrame->setTransferRequestId(bdFrame->getTransferRequestId());
    bdFrameRequestIdentifier.emplace(bdFrame->getTransferRequestId());

    fopToLowerLayerRequestSignalQueue.push(FopToLowerLayerRequestSignal(LOW_LAYER_TRANSMIT, ServiceType::TYPE_BD, bdFrame));
	ccsdsLogNotice(Tx, TypeFOPNotif, NO_FOP_EVENT);
	return FOPNotification::NO_FOP_EVENT;
}

FOPNotification FrameOperationProcedure::initiateRetransmission(ServiceType serviceType) {
    if ((serviceType != ServiceType::TYPE_AD) && (serviceType != ServiceType::TYPE_BC)) {
        ccsdsLogNotice(Tx, TypeFOPNotif, FOP_UNEXPECTED_VALUE);
        return FOPNotification::FOP_UNEXPECTED_VALUE;
    }

    if (sentQueueFOP.empty()) {
        ccsdsLogNotice(Tx, TypeFOPNotif, SENT_QUEUE_EMPTY);
        return FOPNotification::SENT_QUEUE_EMPTY;
    }

    if (fopToLowerLayerRequestSignalQueue.full()) {
        ccsdsLogNotice(Tx, TypeFOPNotif, SIGNAL_QUEUE_FULL);
        return FOPNotification::SIGNAL_QUEUE_FULL;
    }

	fopToLowerLayerRequestSignalQueue.push(FopToLowerLayerRequestSignal(LOW_LAYER_ABORT, serviceType));
	transmissionCount = (transmissionCount == 255) ? 0 : transmissionCount + 1;
    timer.startTimer(tiInitial);

	for (TransferFrameTC* frame : sentQueueFOP) {
		if (frame->getServiceType() == serviceType) {
			frame->setToBeRetransmitted(true);
		}
	}

    ccsdsLogNotice(Tx, TypeFOPNotif, NO_FOP_EVENT);
    return FOPNotification::NO_FOP_EVENT;
}

FOPNotification FrameOperationProcedure::removeAcknowledgedFramesFromSentQueue(uint8_t reportValue) {
    if (frameMasterCopyBuffer.empty()) {
        ccsdsLogNotice(Tx, TypeFOPNotif, FOP_MASTER_COPY_BUFFER_FULL);
        return FOPNotification::FOP_MASTER_COPY_BUFFER_FULL;
    }

    if (sentQueueFOP.empty()) {
        ccsdsLogNotice(Tx, TypeFOPNotif, SENT_QUEUE_EMPTY);
        return FOPNotification::SENT_QUEUE_EMPTY;
    }

	etl::ilist<TransferFrameTC*>::iterator sent_queue_it = sentQueueFOP.begin();
    etl::ilist<TransferFrameTC>::iterator master_copy_it;
    TransferFrameTC* adFrame;
    while (sent_queue_it != sentQueueFOP.end()) {
        adFrame = *sent_queue_it;
        if ((adFrame->getServiceType() == ServiceType::TYPE_AD) && (adFrame->getTransferFrameSequenceNumber() <= reportValue)) {
            // message higher layers about the successful reception
            if (!transferNotificationSignalQueue.full()) {
                transferNotificationSignalQueue.push(TransferNotificationSignal(adFrame->getTransferRequestId(),
                                                                                POSITIVE_CONFIRM_RESPONSE_TO_TRANSFER_FDU));
            }

            // delete octets
            memoryPool.deletePacket(adFrame->getFrameData(), adFrame->getFrameLength());

            // delete master copy
            for (master_copy_it = frameMasterCopyBuffer.begin(); master_copy_it != frameMasterCopyBuffer.end(); ++master_copy_it) {
                if (&(*master_copy_it) == *sent_queue_it) {
                    frameMasterCopyBuffer.erase(master_copy_it);
                    break;
                }
            }

            // delete pointer from the sent queue
            sent_queue_it = sentQueueFOP.erase(sent_queue_it); // erase() returns the iterator to the next element
            continue;
        }
        sent_queue_it++; // This frame was either not TYPE_AD or not acknowledged, move to the next one.
    }

    expectedAcknowledgementSeqNumber = reportValue;
	transmissionCount = 1;
    ccsdsLogNotice(Tx, TypeFOPNotif, NO_FOP_EVENT);
    return FOPNotification::NO_FOP_EVENT;
}

FOPNotification FrameOperationProcedure::lookForDirective() {
    if (bcOut == FlagState::NOT_READY) {
        ccsdsLogNotice(Tx, TypeFOPNotif, NO_FOP_EVENT);
        return FOPNotification::NO_FOP_EVENT;
    }

    if (sentQueueFOP.empty()) {
        ccsdsLogNotice(Tx, TypeFOPNotif, SENT_QUEUE_EMPTY);
        return FOPNotification::SENT_QUEUE_EMPTY;
    }

    etl::ilist<TransferFrameTC*>::iterator sent_queue_it = sentQueueFOP.begin();
    while (sent_queue_it != sentQueueFOP.end()) {
        if (((*sent_queue_it)->getServiceType() == ServiceType::TYPE_BC) && ((*sent_queue_it)->isToBeRetransmitted())) {
           if (!fopToLowerLayerRequestSignalQueue.full()) {
               fopToLowerLayerRequestSignalQueue.push(
                       FopToLowerLayerRequestSignal(LOW_LAYER_TRANSMIT, ServiceType::TYPE_BC, *sent_queue_it));
           }

            // NOTE: Resting the retransmission flag to false is not included in the protocol, but the corresponding action
            // for TYPE-AD frames does (see lookForFdu()), so it might have been an omission.
            (*sent_queue_it)->setToBeRetransmitted(false);
            bcOut = FlagState::NOT_READY;
            break;
        }
    }

    ccsdsLogNotice(Tx, TypeFOPNotif, NO_FOP_EVENT);
    return FOPNotification::NO_FOP_EVENT;
}

FOPNotification FrameOperationProcedure::lookForFdu() {
    if (adOut == FlagState::NOT_READY) {
        ccsdsLogNotice(Tx, TypeFOPNotif, NO_FOP_EVENT);
        return FOPNotification::NO_FOP_EVENT;
    }

    if (sentQueueFOP.empty()) {
        ccsdsLogNotice(Tx, TypeFOPNotif, SENT_QUEUE_EMPTY);
        return FOPNotification::SENT_QUEUE_EMPTY;
    }

    // Check if there are TYPE-AD marked 'toBeRetransmitted'. If so, retransmit the first one
    etl::ilist<TransferFrameTC*>::iterator sent_queue_it = sentQueueFOP.begin();
    while (sent_queue_it != sentQueueFOP.end()) {
        if (((*sent_queue_it)->getServiceType() == ServiceType::TYPE_AD) && ((*sent_queue_it)->isToBeRetransmitted())) {
            if (!fopToLowerLayerRequestSignalQueue.full()) {
                fopToLowerLayerRequestSignalQueue.push(
                        FopToLowerLayerRequestSignal(LOW_LAYER_TRANSMIT, ServiceType::TYPE_AD, *sent_queue_it));
            }

            (*sent_queue_it)->setToBeRetransmitted(false);
            adOut = FlagState::NOT_READY;

            ccsdsLogNotice(Tx, TypeFOPNotif, NO_FOP_EVENT);
            return FOPNotification::NO_FOP_EVENT;
        }
    }

    // No TYPE-AD frame is marked 'toBeRetransmitted'. See if the wait queue has an fdu
    // such that V(S) < NN(R) + K, and transmit it
    if (waitQueueFOP.empty()) {
        ccsdsLogNotice(Tx, TypeFOPNotif, NO_FOP_EVENT);
        return FOPNotification::NO_FOP_EVENT;
    }

    if ((waitQueueFOP.front()->getServiceType() == ServiceType::TYPE_AD) && (transmitterFrameSeqNumber < expectedAcknowledgementSeqNumber + fopSlidingWindowWidth)) {
        TransferFrameTC* adFrame = waitQueueFOP.front();
        FOPNotification notification = transmitAdFrame(adFrame);
        if (notification == NO_FOP_EVENT) {
            waitQueueFOP.pop_front();
            transferNotificationSignalQueue.push(TransferNotificationSignal(adFrame->getTransferRequestId(), ACCEPT_RESPONSE_TO_TRANSFER_FDU));
        }
        else {
            ccsdsLogNotice(Tx, TypeFOPNotif, notification);
            return notification;
        }
    }

    ccsdsLogNotice(Tx, TypeFOPNotif, NO_FOP_EVENT);
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
    suspendState = SuspendVariableState::NOT_SUSPENDED;
}

void FrameOperationProcedure::alert(AlertEvent event) {
    timer.stopTimer();
    purgeSentQueue();
    purgeWaitQueue();
    // TODO: Generate a ‘Negative Confirm Response to Directive’ for any ongoing 'Initiate AD Service' request
    // TODO: should all signal queues be cleared here?
    asynchronousNotificationSignalQueue.push(AsynchronousNotificationSignal(ALERT, event));
}

void FrameOperationProcedure::resume() {
    timer.startTimer(tiInitial);
    suspendState = SuspendVariableState::NOT_SUSPENDED;
}

/** Implementation specific FOP-1 methods (for usage inside vcGeneration service)**/

FOPNotification FrameOperationProcedure::pushTransferFduSignal(DfuTransferSignal signal) {
    if (transferFduSignalQueue.full()) {
        ccsdsLogNotice(Tx, TypeFOPNotif, SIGNAL_QUEUE_FULL);
        return FOPNotification::SIGNAL_QUEUE_FULL;
    }

    transferFduSignalQueue.push(signal);
    ccsdsLogNotice(Tx, TypeFOPNotif, NO_FOP_EVENT);
    return FOPNotification::NO_FOP_EVENT;
}

FOPNotification FrameOperationProcedure::pushLowerLayerResponseSignal(LowerLayerResponseSignal signal) {
    if (lowerLayerResponseSignalQueue.full()) {
        ccsdsLogNotice(Tx, TypeFOPNotif, SIGNAL_QUEUE_FULL);
        return FOPNotification::SIGNAL_QUEUE_FULL;
    }

    lowerLayerResponseSignalQueue.push(signal);
    ccsdsLogNotice(Tx, TypeFOPNotif, NO_FOP_EVENT);
    return FOPNotification::NO_FOP_EVENT;
}

std::pair<FOPNotification, etl::optional<TransferNotificationSignal>> FrameOperationProcedure::popTransferNotificationSignal() {
    if (transferNotificationSignalQueue.empty()) {
        ccsdsLogNotice(Tx, TypeFOPNotif, SIGNAL_QUEUE_EMPTY);
        return std::make_pair(FOPNotification::SIGNAL_QUEUE_EMPTY, etl::nullopt);
    }

    TransferNotificationSignal signal = transferNotificationSignalQueue.front();
    transferNotificationSignalQueue.pop();

    ccsdsLogNotice(Tx, TypeFOPNotif, NO_FOP_EVENT);
    return std::make_pair(NO_FOP_EVENT, signal);
}

std::pair<FOPNotification, etl::optional<FopToLowerLayerRequestSignal>> FrameOperationProcedure::popFopToLowerLayerRequestSignal() {
    if (fopToLowerLayerRequestSignalQueue.empty()) {
        ccsdsLogNotice(Tx, TypeFOPNotif, SIGNAL_QUEUE_EMPTY);
        return std::make_pair(FOPNotification::SIGNAL_QUEUE_EMPTY, etl::nullopt);
    }

    FopToLowerLayerRequestSignal signal = fopToLowerLayerRequestSignalQueue.front();
    fopToLowerLayerRequestSignalQueue.pop();

    ccsdsLogNotice(Tx, TypeFOPNotif, NO_FOP_EVENT);
    return std::make_pair(NO_FOP_EVENT, signal);
}

/** Implementation specific FOP-1 methods (for the the TC Data Link User). Wrapper functions are provided
  * in ServiceChannel
  */

FOPNotification FrameOperationProcedure::pushDirectiveRequestSignal(const DirectiveRequestSignal& signal) {
    if (directiveRequestSignalQueue.full()) {
        ccsdsLogNotice(Tx, TypeFOPNotif, SIGNAL_QUEUE_FULL);
        return FOPNotification::SIGNAL_QUEUE_FULL;
    }

    directiveRequestSignalQueue.push(signal);
    ccsdsLogNotice(Tx, TypeFOPNotif, NO_FOP_EVENT);
    return FOPNotification::NO_FOP_EVENT;
}

FOPNotification FrameOperationProcedure::pushClcw(CLCW clcw) {
    if (clcwQueue.full()) {
        ccsdsLogNotice(Tx, TypeFOPNotif, SIGNAL_QUEUE_FULL);
        return FOPNotification::SIGNAL_QUEUE_FULL;
    }

    clcwQueue.push(clcw);
    ccsdsLogNotice(Tx, TypeFOPNotif, NO_FOP_EVENT);
    return FOPNotification::NO_FOP_EVENT;
}

std::pair<FOPNotification, etl::optional<DirectiveNotificationSignal>> FrameOperationProcedure::popDirectiveNotificationSignal() {
    if (directiveNotificationSignalQueue.empty()) {
        ccsdsLogNotice(Tx, TypeFOPNotif, SIGNAL_QUEUE_EMPTY);
        return std::make_pair(FOPNotification::SIGNAL_QUEUE_EMPTY, etl::nullopt);
    }

    DirectiveNotificationSignal signal = directiveNotificationSignalQueue.front();
    directiveNotificationSignalQueue.pop();

    ccsdsLogNotice(Tx, TypeFOPNotif, NO_FOP_EVENT);
    return std::make_pair(NO_FOP_EVENT, signal);
}

std::pair<FOPNotification, etl::optional<AsynchronousNotificationSignal>> FrameOperationProcedure::popAsynchronousNotificationSignal() {
    if (asynchronousNotificationSignalQueue.empty()) {
        ccsdsLogNotice(Tx, TypeFOPNotif, SIGNAL_QUEUE_EMPTY);
        return std::make_pair(FOPNotification::SIGNAL_QUEUE_EMPTY, etl::nullopt);
    }

    AsynchronousNotificationSignal signal = asynchronousNotificationSignalQueue.front();
    asynchronousNotificationSignalQueue.pop();

    ccsdsLogNotice(Tx, TypeFOPNotif, NO_FOP_EVENT);
    return std::make_pair(NO_FOP_EVENT, signal);
}
