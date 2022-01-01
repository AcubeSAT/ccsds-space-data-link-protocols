#include <FrameOperationProcedure.hpp>
#include <CCSDSChannel.hpp>
#include <CCSDS_Log.h>

FOPNotification FrameOperationProcedure::purgeSentQueue() {
    etl::ilist<PacketTC*>::iterator cur_frame = sentQueue->begin();

    while (cur_frame != sentQueue->end()) {
        (*cur_frame)->setConfSignal(FDURequestType::REQUEST_NEGATIVE_CONFIRM);
        sentQueue->erase(cur_frame++);
    }
	ccsdsLog(Tx, TypeFOPNotif, NO_FOP_EVENT);
    return FOPNotification::NO_FOP_EVENT;
}

FOPNotification FrameOperationProcedure::purgeWaitQueue() {
    etl::ilist<PacketTC*>::iterator cur_frame = waitQueue->begin();

    while (cur_frame != waitQueue->end()) {
        (*cur_frame)->setConfSignal(FDURequestType::REQUEST_NEGATIVE_CONFIRM);
        waitQueue->erase(cur_frame++);
    }
	ccsdsLog(Tx, TypeFOPNotif, NO_FOP_EVENT);
    return FOPNotification::NO_FOP_EVENT;
}

FOPNotification FrameOperationProcedure::transmitAdFrame() {
    if (sentQueue->full()) {
		ccsdsLog(Tx, TypeFOPNotif, SENT_QUEUE_FULL);
        return FOPNotification::SENT_QUEUE_FULL;
    }

    if (sentQueue->empty()) {
        transmissionCount = 1;
    }

	PacketTC*ad_frame = waitQueue->front();
    if (waitQueue->empty()){
		ccsdsLog(Tx, TypeFOPNotif, WAIT_QUEUE_EMPTY);
        return FOPNotification::WAIT_QUEUE_EMPTY;
    }

	ad_frame->setTransferFrameSequenceNumber(transmitterFrameSeqNumber);

	ad_frame->setToBeRetransmitted(0);

    sentQueue->push_back(ad_frame);
    adOut = false;

    // TODO start the timer
    // pass the frame into the all frames generation service
	vchan->master_channel().storeOut(ad_frame);
    waitQueue->pop_front();
	ccsdsLog(Tx, TypeFOPNotif, NO_FOP_EVENT);
    return FOPNotification::NO_FOP_EVENT;
}

FOPNotification FrameOperationProcedure::transmitBcFrame(PacketTC*bc_frame) {
	bc_frame->setToBeRetransmitted(0);
    transmissionCount = 1;

    // TODO start the timer
	vchan->master_channel().storeOut(bc_frame);
	ccsdsLog(Tx, TypeFOPNotif, NO_FOP_EVENT);
    return FOPNotification::NO_FOP_EVENT;
}

FOPNotification FrameOperationProcedure::transmitBdFrame(PacketTC*bd_frame) {
    bdOut = NOT_READY;
    // Pass frame to all frames generation service
	vchan->master_channel().storeOut(bd_frame);
	ccsdsLog(Tx, TypeFOPNotif, NO_FOP_EVENT);
    return FOPNotification::NO_FOP_EVENT;
}

void FrameOperationProcedure::initiateAdRetransmission() {
    // TODO generate an `abort` request to lower procedures
    transmissionCount = (transmissionCount == 255) ? 0 : transmissionCount + 1;
    // TODO start the timer

    for (PacketTC*frame : *sentQueue) {
        if (frame->getServiceType() == ServiceType::TYPE_A) {
			frame->setToBeRetransmitted(1);
        }
    }
}

void FrameOperationProcedure::initiateBcRetransmission() {
    // TODO generate an `abort` request to lower procedures
    transmissionCount = (transmissionCount == 255) ? 0 : transmissionCount + 1;
    // TODO start the timer

    for (PacketTC*frame : *sentQueue) {
        if (frame->getServiceType() == ServiceType::TYPE_B) {
			frame->setToBeRetransmitted(1);
        }
    }
}

void FrameOperationProcedure::acknowledgeFrame(uint8_t frame_seq_num){
    for (PacketTC* pckt : *sentQueue){
        if (pckt->transferFrameSequenceNumber() == frame_seq_num){
			pckt->setAcknowledgement(1);
            return;
        }
    }
}

void FrameOperationProcedure::removeAcknowledgedFrames() {
    etl::ilist<PacketTC*>::iterator cur_frame = sentQueue->begin();

    while (cur_frame != sentQueue->end()) {
        if ((*cur_frame)->acknowledged()) {
            expectedAcknowledgementSeqNumber = (*cur_frame)->transferFrameSequenceNumber();
            sentQueue->erase(cur_frame++);
        } else {
            ++cur_frame;
        }
    }

    // Also remove acknowledged frames from Master TX Buffer
    etl::ilist<PacketTC>::iterator cur_packet = vchan->master_channel().txMasterCopyTC.begin();

    while (cur_packet != vchan->master_channel().txMasterCopyTC.end()) {
        if ((*cur_packet).acknowledged()) {
            vchan->master_channel().txMasterCopyTC.erase(cur_packet++);
        } else {
            ++cur_packet;
        }
    }

    transmissionCount = 1;
}

void FrameOperationProcedure::lookForDirective() {
    if (bcOut == FlagState::READY) {
        for (PacketTC*frame : *sentQueue) {
            if (frame->getServiceType() == ServiceType::TYPE_B && frame->getToBeRetransmitted()) {
                bcOut == FlagState::NOT_READY;
				frame->setToBeRetransmitted(0);
            }
            // transmit_bc_frame();
        }
    } else {
        // @TODO? call look_for_fdu once bcOut is set to ready
    }
}

// TODO: Sent Queue as-is is pretty much tx
COPDirectiveResponse FrameOperationProcedure::pushSentQueue(){
    if (vchan->sentQueue.empty()){
		ccsdsLog(Tx, TypeCOPDirectiveResponse, REJECT);
        return COPDirectiveResponse::REJECT;
    }

    PacketTC* pckt = sentQueue->front();

    MasterChannelAlert err = vchan->master_channel().storeOut(pckt);

    if (err == MasterChannelAlert::NO_MC_ALERT){
        //sentQueue->pop_front();
		ccsdsLog(Tx, TypeCOPDirectiveResponse, ACCEPT);
        return COPDirectiveResponse::ACCEPT;
    }
	ccsdsLog(Tx, TypeCOPDirectiveResponse, REJECT);
    return COPDirectiveResponse::REJECT;
}

COPDirectiveResponse FrameOperationProcedure::lookForFdu() {
    if (adOut == FlagState::READY) {
        for (PacketTC*frame : *sentQueue) {
            if (frame->getServiceType() == ServiceType::TYPE_A) {
                // adOut = FlagState::NOT_READY;
				frame->setToBeRetransmitted(0);
				FOPNotification resp = transmitAdFrame();
				ccsdsLog(Tx, TypeCOPDirectiveResponse, ACCEPT);
                return COPDirectiveResponse::ACCEPT;
            }
        }
        // Search the wait queue for a suitable FDU
        // The wait queue is supposed to have a maximum capacity of one
        if (transmitterFrameSeqNumber < expectedAcknowledgementSeqNumber + fopSlidingWindow) {
			PacketTC*frame = waitQueue->front();
            if (frame->getServiceType() == ServiceType::TYPE_A) {
                sentQueue->push_back(frame);
                waitQueue->pop_front();
				ccsdsLog(Tx, TypeCOPDirectiveResponse, ACCEPT);
                return COPDirectiveResponse::ACCEPT;
            }
        }
    } else {
        // TODO? I think that look_for_fdu has to be automatically sent once adOut is set to ready
		ccsdsLog(Tx, TypeCOPDirectiveResponse, REJECT);
        return COPDirectiveResponse::REJECT;
    }
}

void FrameOperationProcedure::initialize() {
	purgeSentQueue();
	purgeWaitQueue();
    transmissionCount = 1;
    suspendState = FOPState::INITIAL;
}

void FrameOperationProcedure::alert(AlertEvent event) {
    // TODO: cancel the timer
	purgeSentQueue();
	purgeWaitQueue();
    // TODO: Generate a ‘Negative Confirm Response to Directive’ for any ongoing 'Initiate AD Service' request
    // TODO: Generate Alert notification (also the reason for the alert needs to be passed here)
}

// This is just a representation of the transitions of the state machine. This can be cleaned up a lot and have a
// separate data structure hold down the transitions between each state but this works too... it's just ugly
COPDirectiveResponse FrameOperationProcedure::validClcwArrival() {
	PacketTC*frame = vchan->txUnprocessedPacketListBufferTC.front();

    if (frame->lockout() == 0) {
        if (frame->reportValue() == expectedAcknowledgementSeqNumber) {
            if (frame->retransmit() == 0) {
                if (frame->wait() == 0) {
                    if (expectedAcknowledgementSeqNumber == transmitterFrameSeqNumber) {
                        // E1
                        switch (state) {
                            case FOPState::ACTIVE:
                                break;
                            case FOPState::RETRANSMIT_WITHOUT_WAIT:
                            case FOPState::RETRANSMIT_WITH_WAIT:
                                alert(AlertEvent::ALRT_SYNCH);
                                state = FOPState::INITIAL;
                                break;
                            case FOPState::INITIALIZING_WITHOUT_BC_FRAME:
                                frame->setConfSignal(FDURequestType::REQUEST_POSITIVE_CONFIRM);
                                state = FOPState::ACTIVE;
                                // cancel timer
                                break;
                            case FOPState::INITIALIZING_WITH_BC_FRAME:
                                frame->setConfSignal(FDURequestType::REQUEST_POSITIVE_CONFIRM);
                                // bc_accept()??
                                //  cancel timer
                                state = FOPState::ACTIVE;
                                break;
                            case FOPState::INITIAL:
                                state = FOPState::INITIAL;
                                break;
                        }
                    } else {
                        // E2
                        switch (state) {
                            case FOPState::ACTIVE:
                            case FOPState::RETRANSMIT_WITHOUT_WAIT:
                            case FOPState::RETRANSMIT_WITH_WAIT:
								removeAcknowledgedFrames();
                                // cancel timer
								lookForFdu();
                                state = FOPState::ACTIVE;
                                break;
                            case FOPState::INITIALIZING_WITHOUT_BC_FRAME: // N/A
                            case FOPState::INITIALIZING_WITH_BC_FRAME: // N/A
                            case FOPState::INITIAL:
                                break;
                        }
                    }
                } else {
                    // E3
                    switch (state) {
                        case FOPState::ACTIVE:
                        case FOPState::RETRANSMIT_WITHOUT_WAIT:
                        case FOPState::RETRANSMIT_WITH_WAIT:
                        case FOPState::INITIALIZING_WITHOUT_BC_FRAME:
                        case FOPState::INITIALIZING_WITH_BC_FRAME:
                            alert(AlertEvent::ALRT_CLCW);
                            state = FOPState::INITIAL;
                            break;
                        case FOPState::INITIAL:
                            break;
                    }
                }
            } else {
                // E4
                switch (state) {
                    case FOPState::ACTIVE:
                    case FOPState::RETRANSMIT_WITHOUT_WAIT:
                    case FOPState::RETRANSMIT_WITH_WAIT:
                    case FOPState::INITIALIZING_WITHOUT_BC_FRAME:
                        alert(AlertEvent::ALRT_SYNCH);
                        state = FOPState::INITIAL;
                        break;
                    case FOPState::INITIALIZING_WITH_BC_FRAME:
                    case FOPState::INITIAL:
                        break;
                }
            }
        } else if (frame->reportValue() > expectedAcknowledgementSeqNumber &&
                   expectedAcknowledgementSeqNumber >= transmitterFrameSeqNumber) {
            if (frame->retransmit() == 0) {
                if (frame->wait() == 0) {
                    if (expectedAcknowledgementSeqNumber == transmitterFrameSeqNumber) {
                        // E5
                        switch (state) {
                            case FOPState::ACTIVE:
                                break;
                            case FOPState::RETRANSMIT_WITHOUT_WAIT:
                            case FOPState::RETRANSMIT_WITH_WAIT:
                                alert(AlertEvent::ALRT_SYNCH);
                                state = FOPState::INITIAL;
                                break;
                            case FOPState::INITIALIZING_WITHOUT_BC_FRAME: // N/A
                            case FOPState::INITIALIZING_WITH_BC_FRAME: // N/A
                            case FOPState::INITIAL:
                                break;
                        }
                    } else {
                        // E6
                        switch (state) {
                            case FOPState::ACTIVE:
                            case FOPState::RETRANSMIT_WITHOUT_WAIT:
                            case FOPState::RETRANSMIT_WITH_WAIT:
								removeAcknowledgedFrames();
								lookForFdu();
                                state = FOPState::ACTIVE;
                                break;
                            case FOPState::INITIALIZING_WITHOUT_BC_FRAME: // N/A
                            case FOPState::INITIALIZING_WITH_BC_FRAME: // N/A
                            case FOPState::INITIAL:
                                break;
                        }
                    }
                } else {
                    // E7
                    switch (state) {
                        case FOPState::ACTIVE:
                        case FOPState::RETRANSMIT_WITHOUT_WAIT:
                        case FOPState::RETRANSMIT_WITH_WAIT:
                            alert(AlertEvent::ALRT_CLCW);
                            state = FOPState::INITIAL;
                            break;
                        case FOPState::INITIALIZING_WITHOUT_BC_FRAME: // N/A
                        case FOPState::INITIALIZING_WITH_BC_FRAME: // N/A
                        case FOPState::INITIAL:
                            break;
                    }
                }
            } else {
                if (transmissionLimit == 1) {
                    if (expectedAcknowledgementSeqNumber != transmitterFrameSeqNumber) {
                        // E101
                        switch (state) {
                            case FOPState::ACTIVE:
                            case FOPState::RETRANSMIT_WITHOUT_WAIT:
                            case FOPState::RETRANSMIT_WITH_WAIT:
								removeAcknowledgedFrames();
                                alert(AlertEvent::ALRT_LIMIT);
                                state = FOPState::INITIAL;
                                break;
                            case FOPState::INITIALIZING_WITHOUT_BC_FRAME: // N/A
                            case FOPState::INITIALIZING_WITH_BC_FRAME: // N/A
                            case FOPState::INITIAL:
                                break;
                        }
                    } else {
                        // E102
                        switch (state) {
                            case FOPState::ACTIVE:
                            case FOPState::RETRANSMIT_WITHOUT_WAIT:
                            case FOPState::RETRANSMIT_WITH_WAIT:
                                alert(AlertEvent::ALRT_LIMIT);
                                state = FOPState::INITIAL;
                                break;
                            case FOPState::INITIALIZING_WITHOUT_BC_FRAME: // N/A
                            case FOPState::INITIALIZING_WITH_BC_FRAME: // N/A
                            case FOPState::INITIAL:
                                break;
                        }
                    }
                } else if (transmissionLimit > 1) {
                    if (expectedAcknowledgementSeqNumber != transmitterFrameSeqNumber) {
                        if (frame->wait() == 0) {
                            // E8
                            switch (state) {
                                case FOPState::ACTIVE:
                                case FOPState::RETRANSMIT_WITHOUT_WAIT:
                                case FOPState::RETRANSMIT_WITH_WAIT:
									removeAcknowledgedFrames();
									initiateAdRetransmission();
									lookForFdu();
                                    state = FOPState::RETRANSMIT_WITHOUT_WAIT;
                                    break;
                                case FOPState::INITIALIZING_WITHOUT_BC_FRAME: // N/A
                                case FOPState::INITIALIZING_WITH_BC_FRAME: // N/A
                                case FOPState::INITIAL:
                                    break;
                            }
                        } else {
                            // E9
                            switch (state) {
                                case FOPState::ACTIVE:
                                case FOPState::RETRANSMIT_WITHOUT_WAIT:
                                case FOPState::RETRANSMIT_WITH_WAIT:
									removeAcknowledgedFrames();
                                    state = FOPState::RETRANSMIT_WITH_WAIT;
                                    break;
                                case FOPState::INITIALIZING_WITHOUT_BC_FRAME: // N/A
                                case FOPState::INITIALIZING_WITH_BC_FRAME: // N/A
                                case FOPState::INITIAL:
                                    break;
                            }
                        }
                    } else {
                        if (transmissionCount < transmissionLimit) {
                            if (frame->wait() == 0) {
                                //  E10
                                switch (state) {
                                    case FOPState::ACTIVE:
                                    case FOPState::RETRANSMIT_WITH_WAIT:
										initiateAdRetransmission();
										lookForFdu();
                                        state = FOPState::RETRANSMIT_WITHOUT_WAIT;
                                        break;
                                    case FOPState::RETRANSMIT_WITHOUT_WAIT:
                                    case FOPState::INITIALIZING_WITHOUT_BC_FRAME: // N/A
                                    case FOPState::INITIALIZING_WITH_BC_FRAME: // N/A
                                    case FOPState::INITIAL:
                                        break;
                                }
                            } else {
                                // E11
                                switch (state) {
                                    case FOPState::ACTIVE:
                                    case FOPState::RETRANSMIT_WITHOUT_WAIT:
                                        state = FOPState::RETRANSMIT_WITH_WAIT;
                                        break;
                                    case FOPState::RETRANSMIT_WITH_WAIT:
                                    case FOPState::INITIALIZING_WITHOUT_BC_FRAME: // N/A
                                    case FOPState::INITIALIZING_WITH_BC_FRAME: // N/A
                                    case FOPState::INITIAL:
                                        break;
                                }
                            }
                        } else {
                            if (frame->wait() == 0) {
                                // E12
                                switch (state) {
                                    case FOPState::ACTIVE:
                                    case FOPState::RETRANSMIT_WITH_WAIT:
                                        state = FOPState::RETRANSMIT_WITHOUT_WAIT;
                                        break;
                                    case FOPState::RETRANSMIT_WITHOUT_WAIT:
                                    case FOPState::INITIALIZING_WITHOUT_BC_FRAME: // N/A
                                    case FOPState::INITIALIZING_WITH_BC_FRAME: // N/A
                                    case FOPState::INITIAL:
                                        break;
                                }
                            } else {
                                // E103
                                switch (state) {
                                    case FOPState::ACTIVE:
                                    case FOPState::RETRANSMIT_WITHOUT_WAIT:
                                        state = FOPState::RETRANSMIT_WITH_WAIT;
                                        break;
                                    case FOPState::RETRANSMIT_WITH_WAIT:
                                    case FOPState::INITIALIZING_WITHOUT_BC_FRAME: // N/A
                                    case FOPState::INITIALIZING_WITH_BC_FRAME: // N/A
                                    case FOPState::INITIAL:
                                        break;
                                }
                            }
                        }
                    }
                }
            }
        } else {
            // E13
            switch (state) {
                case FOPState::ACTIVE:
                case FOPState::RETRANSMIT_WITHOUT_WAIT:
                case FOPState::RETRANSMIT_WITH_WAIT:
                case FOPState::INITIALIZING_WITHOUT_BC_FRAME:
                    alert(AlertEvent::ALRT_NNR);
                    state = FOPState::INITIAL;
                    break;
                case FOPState::INITIALIZING_WITH_BC_FRAME:
                case FOPState::INITIAL:
                    break;
            }
        }
    } else {
        switch (state) {
            case FOPState::ACTIVE:
            case FOPState::RETRANSMIT_WITHOUT_WAIT:
            case FOPState::RETRANSMIT_WITH_WAIT:
            case FOPState::INITIALIZING_WITHOUT_BC_FRAME:
                alert(AlertEvent::ALRT_LOCKOUT);
                state = FOPState::INITIAL;
                break;
            case FOPState::INITIALIZING_WITH_BC_FRAME:
            case FOPState::INITIAL:
                break;
        }
    }

    MasterChannelAlert mc = vchan->master_channel().storeOut(frame);
    if (mc != MasterChannelAlert::NO_MC_ALERT){
		ccsdsLog(Tx, TypeCOPDirectiveResponse, REJECT);
        return COPDirectiveResponse::REJECT;
    }
	ccsdsLog(Tx, TypeCOPDirectiveResponse, ACCEPT);
    return COPDirectiveResponse::ACCEPT;
}

void FrameOperationProcedure::invalidClcwArrival() {
    switch (state) {
        case FOPState::ACTIVE:
        case FOPState::RETRANSMIT_WITHOUT_WAIT:
        case FOPState::RETRANSMIT_WITH_WAIT:
        case FOPState::INITIALIZING_WITHOUT_BC_FRAME:
        case FOPState::INITIALIZING_WITH_BC_FRAME:
            alert(AlertEvent::ALRT_CLCW);
            state = FOPState::INITIAL;
            break;
        case FOPState::INITIAL:
            break;
    }
}

FDURequestType FrameOperationProcedure::initiateAdNoClcw() {
    // E23
    if (state == FOPState::INITIAL) {
        initialize();
        state = FOPState::ACTIVE;
    }
	ccsdsLog(Tx, TypeCOPDirectiveResponse, ACCEPT);
    return FDURequestType::REQUEST_POSITIVE_CONFIRM;
}

FDURequestType FrameOperationProcedure::initiateAdClcw() {
    // E24
    if (state == FOPState::INITIAL) {
        initialize();
        state = FOPState::ACTIVE;
    }
    return FDURequestType::REQUEST_POSITIVE_CONFIRM;
}

FDURequestType FrameOperationProcedure::initiateAdUnlock() {
    if (state == FOPState::INITIAL && bcOut == FlagState::READY) {
        // E25
        initialize();
        state = FOPState::INITIALIZING_WITH_BC_FRAME;
        // @todo transmit unlock frame
    }
    // E26
	ccsdsLog(Tx, TypeFDURequestType, REQUEST_POSITIVE_CONFIRM);
    return FDURequestType::REQUEST_POSITIVE_CONFIRM;
}

FDURequestType FrameOperationProcedure::initiateAdVr(uint8_t vr) {
    if (state == FOPState::INITIAL && bcOut == FlagState::READY) {
        // E27
        initialize();
        transmitterFrameSeqNumber = vr;
        expectedAcknowledgementSeqNumber = vr;
        // @todo transmit Set V(R) frame
        state = FOPState::INITIALIZING_WITH_BC_FRAME;
    }
    // E28
	ccsdsLog(Tx, TypeFDURequestType, REQUEST_POSITIVE_CONFIRM);
    return FDURequestType::REQUEST_POSITIVE_CONFIRM;
}

FDURequestType FrameOperationProcedure::terminateAdService() {
    // E29
    if (state != FOPState::INITIAL) {
        alert(AlertEvent::ALRT_TERM);
        state = FOPState::INITIAL;
    }
	ccsdsLog(Tx, TypeFDURequestType, REQUEST_POSITIVE_CONFIRM);
    return FDURequestType::REQUEST_POSITIVE_CONFIRM;
}

FDURequestType FrameOperationProcedure::resumeAdService() {
    state = suspendState;
	ccsdsLog(Tx, TypeFDURequestType, REQUEST_POSITIVE_CONFIRM);
    return FDURequestType::REQUEST_POSITIVE_CONFIRM;
}

COPDirectiveResponse FrameOperationProcedure::setVs(uint8_t vs) {
    // E35
    if (state == FOPState::INITIAL && suspendState == FOPState::INITIAL) {
        transmitterFrameSeqNumber = vs;
        expectedAcknowledgementSeqNumber = vs;
		ccsdsLog(Tx, TypeCOPDirectiveResponse, ACCEPT);
        return COPDirectiveResponse::ACCEPT;
    } else {
		ccsdsLog(Tx, TypeCOPDirectiveResponse, REJECT);
        return COPDirectiveResponse::REJECT;
    }
}

COPDirectiveResponse FrameOperationProcedure::setFopWidth(uint8_t width) {
    // E36
    fopSlidingWindow = width;
	ccsdsLog(Tx, TypeCOPDirectiveResponse, ACCEPT);
    return COPDirectiveResponse::ACCEPT;
}

COPDirectiveResponse FrameOperationProcedure::setT1Initial(uint16_t t1_init) {
    // E37
    tiInitial = t1_init;
	ccsdsLog(Tx, TypeCOPDirectiveResponse, ACCEPT);
    return COPDirectiveResponse::ACCEPT;
}

COPDirectiveResponse FrameOperationProcedure::setTransmissionLimit(uint8_t vr) {
    // E38
    transmissionLimit = vr;
    return COPDirectiveResponse::ACCEPT;
}

COPDirectiveResponse FrameOperationProcedure::setTimeoutType(bool vr) {
    // E39
    timeoutType = vr;
	ccsdsLog(Tx, TypeCOPDirectiveResponse, ACCEPT);
    return COPDirectiveResponse::ACCEPT;
}

COPDirectiveResponse FrameOperationProcedure::invalidDirective() {
    // E40
	ccsdsLog(Tx, TypeCOPDirectiveResponse, REJECT);
    return COPDirectiveResponse::REJECT;
}

void FrameOperationProcedure::adAccept() {
    // E41
    adOut = FlagState::READY;
    if (state == FOPState::ACTIVE || state == FOPState::RETRANSMIT_WITHOUT_WAIT) {
		lookForFdu();
    }
}

void FrameOperationProcedure::adReject() {
    // E42
    alert(AlertEvent::ALRT_LLIF);
    state = FOPState::INITIAL;
}

void FrameOperationProcedure::bcAccept() {
    // E43
    bcOut = FlagState::READY;
    if (state == FOPState::INITIALIZING_WITH_BC_FRAME) {
		lookForDirective();
    }
}

void FrameOperationProcedure::bcReject() {
    alert(AlertEvent::ALRT_LLIF);
    state = FOPState::INITIAL;
}

COPDirectiveResponse FrameOperationProcedure::bdAccept() {
    bdOut = FlagState::READY;
	ccsdsLog(Tx, TypeCOPDirectiveResponse, ACCEPT);
    return COPDirectiveResponse::ACCEPT;
}

void FrameOperationProcedure::bdReject() {
    alert(AlertEvent::ALRT_LLIF);
    state = FOPState::INITIAL;
}

COPDirectiveResponse FrameOperationProcedure::transferFdu() {
	PacketTC*frame = vchan->txUnprocessedPacketListBufferTC.front();

    if (frame->transferFrameHeader().bypassFlag() == 0) {
        if (frame->getServiceType() == ServiceType::TYPE_A) {
            if (!waitQueue->full()) {
                // E19
                if (state == FOPState::ACTIVE || state == FOPState::RETRANSMIT_WITHOUT_WAIT) {
                    waitQueue->push_back(frame);
					lookForFdu();
                } else if (state == FOPState::RETRANSMIT_WITH_WAIT) {
                    waitQueue->push_back(frame);
                } else {
					ccsdsLog(Tx, TypeCOPDirectiveResponse, REJECT);
                    return COPDirectiveResponse::REJECT;
                }
				ccsdsLog(Tx, TypeCOPDirectiveResponse, ACCEPT);
                return COPDirectiveResponse::ACCEPT;
            } else {
                // E20
				ccsdsLog(Tx, TypeCOPDirectiveResponse, REJECT);
                return COPDirectiveResponse::REJECT;
            }
        } else if (frame->getServiceType() == ServiceType::TYPE_B) {

            if (bdOut == FlagState::READY) {
                //
				transmitBcFrame(frame);
				ccsdsLog(Tx, TypeCOPDirectiveResponse, ACCEPT);
                return COPDirectiveResponse::ACCEPT;
            } else {
                // E22
                COPDirectiveResponse::REJECT;
            }
        }
    } else {
        // transfer directly to lower procedure

        MasterChannelAlert mc = vchan->master_channel().storeOut(frame);
        if (mc != MasterChannelAlert::NO_MC_ALERT){
			ccsdsLog(Tx, TypeCOPDirectiveResponse, REJECT);
            return COPDirectiveResponse::REJECT;
        }
    }
}
