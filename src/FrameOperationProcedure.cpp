#include <FrameOperationProcedure.hpp>
#include <CCSDSChannel.hpp>

FOPNotif FrameOperationProcedure::purge_sent_queue() {
    etl::ilist<PacketTC*>::iterator cur_frame = sentQueue->begin();

    while (cur_frame != sentQueue->end()) {
        (*cur_frame)->setConfSignal(FDURequestType::REQUEST_NEGATIVE_CONFIRM);
        sentQueue->erase(cur_frame++);
    }

    return FOPNotif::NO_FOP_EVENT;
}

FOPNotif FrameOperationProcedure::purge_wait_queue() {
    etl::ilist<PacketTC*>::iterator cur_frame = waitQueue->begin();

    while (cur_frame != waitQueue->end()) {
        (*cur_frame)->setConfSignal(FDURequestType::REQUEST_NEGATIVE_CONFIRM);
        waitQueue->erase(cur_frame++);
    }

    return FOPNotif::NO_FOP_EVENT;
}

FOPNotif FrameOperationProcedure::transmit_ad_frame() {
    if (sentQueue->full()) {
        return FOPNotif::SENT_QUEUE_FULL;
    }

    if (sentQueue->empty()) {
        transmissionCount = 1;
    }

	PacketTC*ad_frame = waitQueue->front();
    if (waitQueue->empty()){
        return FOPNotif::WAIT_QUEUE_EMPTY;
    }


    sentQueue->push_back(ad_frame);
    ad_frame->set_transfer_frame_sequence_number(transmitterFrameSeqNumber);

    ad_frame->mark_for_retransmission(0);

    sentQueue->push_back(ad_frame);
    adOut = false;

    // TODO start the timer
    // pass the frame into the all frames generation service
    vchan->master_channel().store_out(ad_frame);
    waitQueue->pop_front();

    return FOPNotif::NO_FOP_EVENT;
}

FOPNotif FrameOperationProcedure::transmit_bc_frame(PacketTC*bc_frame) {
    bc_frame->mark_for_retransmission(0);
    transmissionCount = 1;

    // TODO start the timer
    vchan->master_channel().store_out(bc_frame);
    return FOPNotif::NO_FOP_EVENT;
}

FOPNotif FrameOperationProcedure::transmit_bd_frame(PacketTC*bd_frame) {
    bdOut = NOT_READY;
    // Pass frame to all frames generation service
    vchan->master_channel().store_out(bd_frame);
    return FOPNotif::NO_FOP_EVENT;
}

void FrameOperationProcedure::initiate_ad_retransmission() {
    // TODO generate an `abort` request to lower procedures
    transmissionCount = (transmissionCount == 255) ? 0 : transmissionCount + 1;
    // TODO start the timer

    for (PacketTC*frame : *sentQueue) {
        if (frame->service_type() == ServiceType::TYPE_A) {
            frame->mark_for_retransmission(1);
        }
    }
}

void FrameOperationProcedure::initiate_bc_retransmission() {
    // TODO generate an `abort` request to lower procedures
    transmissionCount = (transmissionCount == 255) ? 0 : transmissionCount + 1;
    // TODO start the timer

    for (PacketTC*frame : *sentQueue) {
        if (frame->service_type() == ServiceType::TYPE_B) {
            frame->mark_for_retransmission(1);
        }
    }
}

void FrameOperationProcedure::remove_acknowledged_frames() {
    etl::ilist<PacketTC*>::iterator cur_frame = sentQueue->begin();

    while (cur_frame != sentQueue->end()) {
        if ((*cur_frame)->acknowledged()) {
            (*cur_frame)->setConfSignal(FDURequestType::REQUEST_POSITIVE_CONFIRM);
            expectedAcknowledgementSeqNumber = (*cur_frame)->transfer_frame_sequence_number();
            sentQueue->erase(cur_frame++);
        } else {
            ++cur_frame;
        }
    }
    transmissionCount = 1;
}

void FrameOperationProcedure::look_for_directive() {
    if (bcOut == FlagState::READY) {
        for (PacketTC*frame : *sentQueue) {
            if (frame->service_type() == ServiceType::TYPE_B && frame->to_be_retransmitted()) {
                bcOut == FlagState::NOT_READY;
                frame->mark_for_retransmission(0);
            }
            // transmit_bc_frame();
        }
    } else {
        // @TODO? call look_for_fdu once bcOut is set to ready
    }
}

// TODO: Sent Queue as-is is pretty much tx
COPDirectiveResponse FrameOperationProcedure::push_sent_queue(){
    if (vchan->sentQueue.empty()){
        return COPDirectiveResponse::REJECT;
    }

    PacketTC* pckt = sentQueue->front();

    MasterChannelAlert err = vchan->master_channel().store_out(pckt);

    if (err == MasterChannelAlert::NO_MC_ALERT){
        sentQueue->pop_front();
        return COPDirectiveResponse::ACCEPT;
    }
    return COPDirectiveResponse::REJECT;
}

COPDirectiveResponse FrameOperationProcedure::look_for_fdu() {
    if (adOut == FlagState::READY) {
        for (PacketTC*frame : *sentQueue) {
            if (frame->service_type() == ServiceType::TYPE_A) {
                // adOut = FlagState::NOT_READY;
                frame->mark_for_retransmission(0);
                FOPNotif resp = transmit_ad_frame();
                return COPDirectiveResponse::ACCEPT;
            }
        }
        // Search the wait queue for a suitable FDU
        // The wait queue is supposed to have a maximum capacity of one
        if (transmitterFrameSeqNumber < expectedAcknowledgementSeqNumber + fopSlidingWindow) {
			PacketTC*frame = waitQueue->front();
            if (frame->service_type() == ServiceType::TYPE_A) {
                sentQueue->push_back(frame);
                waitQueue->pop_front();
                return COPDirectiveResponse::ACCEPT;
            }
        }
    } else {
        // TODO? I think that look_for_fdu has to be automatically sent once adOut is set to ready
        return COPDirectiveResponse::REJECT;
    }
}

void FrameOperationProcedure::initialize() {
    purge_sent_queue();
    purge_wait_queue();
    transmissionCount = 1;
    suspendState = FOPState::INITIAL;
}

void FrameOperationProcedure::alert(AlertEvent event) {
    // TODO: cancel the timer
    purge_sent_queue();
    purge_wait_queue();
    // TODO: Generate a ‘Negative Confirm Response to Directive’ for any ongoing 'Initiate AD Service' request
    // TODO: Generate Alert notification (also the reason for the alert needs to be passed here)
}

// This is just a representation of the transitions of the state machine. This can be cleaned up a lot and have a
// separate data structure hold down the transitions between each state but this works too... it's just ugly
COPDirectiveResponse FrameOperationProcedure::valid_clcw_arrival() {
	PacketTC*frame = vchan->txUnprocessedPacketList.front();

    if (frame->lckout() == 0) {
        if (frame->report_value() == expectedAcknowledgementSeqNumber) {
            if (frame->retransmit() == 0) {
                if (frame->wt() == 0) {
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
                                remove_acknowledged_frames();
                                // cancel timer
                                look_for_fdu();
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
        } else if (frame->report_value() > expectedAcknowledgementSeqNumber &&
                   expectedAcknowledgementSeqNumber >= transmitterFrameSeqNumber) {
            if (frame->retransmit() == 0) {
                if (frame->wt() == 0) {
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
                                remove_acknowledged_frames();
                                look_for_fdu();
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
                                remove_acknowledged_frames();
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
                        if (frame->wt() == 0) {
                            // E8
                            switch (state) {
                                case FOPState::ACTIVE:
                                case FOPState::RETRANSMIT_WITHOUT_WAIT:
                                case FOPState::RETRANSMIT_WITH_WAIT:
                                    remove_acknowledged_frames();
                                    initiate_ad_retransmission();
                                    look_for_fdu();
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
                                    remove_acknowledged_frames();
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
                            if (frame->wt() == 0) {
                                //  E10
                                switch (state) {
                                    case FOPState::ACTIVE:
                                    case FOPState::RETRANSMIT_WITH_WAIT:
                                        initiate_ad_retransmission();
                                        look_for_fdu();
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
                            if (frame->wt() == 0) {
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

    MasterChannelAlert mc = vchan->master_channel().store_out(frame);
    if (mc != MasterChannelAlert::NO_MC_ALERT){
        return COPDirectiveResponse::REJECT;
    }

    return COPDirectiveResponse::ACCEPT;
}

void FrameOperationProcedure::invalid_clcw_arrival() {
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

FDURequestType FrameOperationProcedure::initiate_ad_no_clcw() {
    // E23
    if (state == FOPState::INITIAL) {
        initialize();
        state = FOPState::ACTIVE;
    }
    return FDURequestType::REQUEST_POSITIVE_CONFIRM;
}

FDURequestType FrameOperationProcedure::initiate_ad_clcw() {
    // E24
    if (state == FOPState::INITIAL) {
        initialize();
        state = FOPState::ACTIVE;
    }
    return FDURequestType::REQUEST_POSITIVE_CONFIRM;
}

FDURequestType FrameOperationProcedure::initiate_ad_unlock() {
    if (state == FOPState::INITIAL && bcOut == FlagState::READY) {
        // E25
        initialize();
        state = FOPState::INITIALIZING_WITH_BC_FRAME;
        // @todo transmit unlock frame
    }
    // E26
    return FDURequestType::REQUEST_POSITIVE_CONFIRM;
}

FDURequestType FrameOperationProcedure::initiate_ad_vr(uint8_t vr) {
    if (state == FOPState::INITIAL && bcOut == FlagState::READY) {
        // E27
        initialize();
        transmitterFrameSeqNumber = vr;
        expectedAcknowledgementSeqNumber = vr;
        // @todo transmit Set V(R) frame
        state = FOPState::INITIALIZING_WITH_BC_FRAME;
    }
    // E28
    return FDURequestType::REQUEST_POSITIVE_CONFIRM;
}

FDURequestType FrameOperationProcedure::terminate_ad_service() {
    // E29
    if (state != FOPState::INITIAL) {
        alert(AlertEvent::ALRT_TERM);
        state = FOPState::INITIAL;
    }
    return FDURequestType::REQUEST_POSITIVE_CONFIRM;
}

FDURequestType FrameOperationProcedure::resume_ad_service() {
    state = suspendState;
    return FDURequestType::REQUEST_POSITIVE_CONFIRM;
}

COPDirectiveResponse FrameOperationProcedure::set_vs(uint8_t vs) {
    // E35
    if (state == FOPState::INITIAL && suspendState == FOPState::INITIAL) {
        transmitterFrameSeqNumber = vs;
        expectedAcknowledgementSeqNumber = vs;
        return COPDirectiveResponse::ACCEPT;
    } else {
        return COPDirectiveResponse::REJECT;
    }
}

COPDirectiveResponse FrameOperationProcedure::set_fop_width(uint8_t vr) {
    // E36
    fopSlidingWindow = vr;
    return COPDirectiveResponse::ACCEPT;
}

COPDirectiveResponse FrameOperationProcedure::set_t1_initial(uint16_t t1_init) {
    // E37
    tiInitial = t1_init;
    return COPDirectiveResponse::ACCEPT;
}

COPDirectiveResponse FrameOperationProcedure::set_transmission_limit(uint8_t vr) {
    // E38
    transmissionLimit = vr;
    return COPDirectiveResponse::ACCEPT;
}

COPDirectiveResponse FrameOperationProcedure::set_timeout_type(bool vr) {
    // E39
    timeoutType = vr;
    return COPDirectiveResponse::ACCEPT;
}

COPDirectiveResponse FrameOperationProcedure::invalid_directive() {
    // E40
    return COPDirectiveResponse::REJECT;
}

void FrameOperationProcedure::ad_accept() {
    // E41
    adOut = FlagState::READY;
    if (state == FOPState::ACTIVE || state == FOPState::RETRANSMIT_WITHOUT_WAIT) {
        look_for_fdu();
    }
}

void FrameOperationProcedure::ad_reject() {
    // E42
    alert(AlertEvent::ALRT_LLIF);
    state = FOPState::INITIAL;
}

void FrameOperationProcedure::bc_accept() {
    // E43
    bcOut = FlagState::READY;
    if (state == FOPState::INITIALIZING_WITH_BC_FRAME) {
        look_for_directive();
    }
}

void FrameOperationProcedure::bc_reject() {
    alert(AlertEvent::ALRT_LLIF);
    state = FOPState::INITIAL;
}

COPDirectiveResponse FrameOperationProcedure::bd_accept() {
    bdOut = FlagState::READY;
    return COPDirectiveResponse::ACCEPT;
}

void FrameOperationProcedure::bd_reject() {
    alert(AlertEvent::ALRT_LLIF);
    state = FOPState::INITIAL;
}

COPDirectiveResponse FrameOperationProcedure::transfer_fdu() {
	PacketTC*frame = vchan->txUnprocessedPacketList.front();

    if (frame->transfer_frame_header().bypass_flag() == 0) {
        if (frame->service_type() == ServiceType::TYPE_A) {
            if (!waitQueue->full()) {
                // E19
                if (state == FOPState::ACTIVE || state == FOPState::RETRANSMIT_WITHOUT_WAIT) {
                    waitQueue->push_back(frame);
                    look_for_fdu();
                } else if (state == FOPState::RETRANSMIT_WITH_WAIT) {
                    waitQueue->push_back(frame);
                } else {
                    return COPDirectiveResponse::REJECT;
                }
                return COPDirectiveResponse::ACCEPT;
            } else {
                // E20
                return COPDirectiveResponse::REJECT;
            }
        } else if (frame->service_type() == ServiceType::TYPE_B) {

            if (bdOut == FlagState::READY) {
                //
                transmit_bc_frame(frame);
                return COPDirectiveResponse::ACCEPT;
            } else {
                // E22
                COPDirectiveResponse::REJECT;
            }
        }
    } else {
        // transfer directly to lower procedure

        MasterChannelAlert mc = vchan->master_channel().store_out(frame);
        if (mc != MasterChannelAlert::NO_MC_ALERT){
            return COPDirectiveResponse::REJECT;
        }
    }
}
