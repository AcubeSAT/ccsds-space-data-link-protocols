#include <FrameOperationProcedure.h>

FOPNotif FrameOperationProcedure::purge_sent_queue() {
    etl::ilist<Packet>::iterator cur_frame = sentQueue->begin();

    while (cur_frame != sentQueue->end()) {
        cur_frame->setConfSignal(FDURequestType::REQUEST_DENIED);
        sentQueue->erase(cur_frame++);
    }

    return FOPNotif::NO_FOP_EVENT;
}

FOPNotif FrameOperationProcedure::purge_wait_queue() {
    etl::ilist<Packet>::iterator cur_frame = waitQueue->begin();

    while (cur_frame != waitQueue->end()) {
        cur_frame->setConfSignal(FDURequestType::REQUEST_DENIED);
        waitQueue->erase(cur_frame++);
    }

    return FOPNotif::NO_FOP_EVENT;
}

FOPNotif FrameOperationProcedure::transmit_ad_frame(Packet ad_frame) {

    if (sentQueue->full()) {
        return FOPNotif::SENT_QUEUE_FULL;
    }

    if (sentQueue->empty()) {
        transmissionCount = 1;
    }

    sentQueue->push_back(ad_frame);
    ad_frame.transferFrameSeqNumber = transmitterFrameSeqNumber;

    ad_frame.mark_for_retransmission(0);

    waitQueue->pop_front();
    sentQueue->push_back(ad_frame);
    adOut = false;

    // TODO start the timer
    // TODO generate a 'Transmit AD Frame' request to lower procedures
    return FOPNotif::NO_FOP_EVENT;
}

FOPNotif FrameOperationProcedure::transmit_bc_frame(Packet bc_frame) {

    bc_frame.mark_for_retransmission(0);
    transmissionCount = 1;

    // TODO start the timer
    // TODO generate a 'Transmit BC Frame' request
    return FOPNotif::NO_FOP_EVENT;
}

FOPNotif FrameOperationProcedure::transmit_bd_frame(Packet bd_frame) {
    bdOut = NOT_READY;

    // TODO generate a 'Transmit BD Frame' request
    return FOPNotif::NO_FOP_EVENT;
}

void FrameOperationProcedure::initiate_ad_retransmission() {
    // TODO generate an `abort` request to lower procedures
    transmissionCount = (transmissionCount == 255) ? 0 : transmissionCount + 1;
    // TODO start the timer

    for (Packet frame:*sentQueue) {

        if (frame.serviceType == ServiceType::TYPE_A) {
            frame.mark_for_retransmission(1);
        }
    }
}

void FrameOperationProcedure::initiate_bc_retransmission() {
    // TODO generate an `abort` request to lower procedures
    transmissionCount = (transmissionCount == 255) ? 0 : transmissionCount + 1;
    // TODO start the timer

    for (Packet frame:*sentQueue) {
        if (frame.serviceType == ServiceType::TYPE_B) {
            frame.mark_for_retransmission(1);
        }
    }
}

void FrameOperationProcedure::remove_acknowledged_frames() {
    etl::ilist<Packet>::iterator cur_frame = sentQueue->begin();

    while (cur_frame != sentQueue->end()) {
        if (cur_frame->acknowledged) {
            cur_frame->setConfSignal(FDURequestType::REQUEST_CONFIRMED);
            expectedAcknowledgementSeqNumber = cur_frame->transferFrameSeqNumber;
            sentQueue->erase(cur_frame++);
        } else {
            ++cur_frame;
        }
    }
    transmissionCount = 1;
}

void FrameOperationProcedure::look_for_directive() {
    if (bcOut == FlagState::READY) {
        for (Packet frame : *sentQueue) {
            if (frame.serviceType == ServiceType::TYPE_B && frame.to_be_retransmitted()) {
                bcOut == FlagState::NOT_READY;
                frame.mark_for_retransmission(0);
            }
            //transmit_bc_frame();
        }
    } else {
        // @TODO? call look_for_fdu once bcOut is set to ready
    }
}

void FrameOperationProcedure::look_for_fdu() {
    if (adOut == FlagState::READY) {
        for (Packet frame : *sentQueue) {
            if (frame.serviceType == ServiceType::TYPE_A) {
                adOut = FlagState::NOT_READY;
                frame.mark_for_retransmission(0);
                // TODO Generate ‘Transmit Request for (AD) Frame’ request for this frame
                break;
            }

            // Check if no Type-A transfer frames have been found in the sent queue
            if (adOut == FlagState::READY) {
                // Search the wait queue for a suitable FDU
                // The wait queue is supposed to have a maximum capacity of one but anyway
                if (transmitterFrameSeqNumber < expectedAcknowledgementSeqNumber + fopSlidingWindow) {
                    etl::ilist<Packet>::iterator cur_frame = waitQueue->begin();
                    while (cur_frame != waitQueue->end()) {
                        if (cur_frame->serviceType == ServiceType::TYPE_A) {
                            // TODO Generate ‘Accept Response to Request to Transfer FDU’
                            sentQueue->push_front(*cur_frame);
                            waitQueue->erase(cur_frame);
                        }
                    }
                }
            }
            //transmit_ad_frame();
        }
    } else {
        // TODO? I think that look_for_fdu has to be automatically sent once adOut is set to ready
    }
}

void FrameOperationProcedure::initialize() {
    purge_sent_queue();
    purge_wait_queue();
    transmissionCount = 1;
    suspendState = 0;
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
void FrameOperationProcedure::valid_clcw_arrival(CLCW *clcw) {
    if (clcw->lockout() == 0) {
        if (clcw->report_value() == expectedAcknowledgementSeqNumber) {
            if (clcw->retransmission() == 0) {
                if (clcw->wait() == 0) {
                    if (expectedAcknowledgementSeqNumber == transmitterFrameSeqNumber) {
                        // E1
                        switch (state) {
                            case FOPState::ACTIVE:
                                break;
                            case FOPState::RETRANSMIT_WITHOUT_WAIT:
                                alert(AlertEvent::ALRT_SYNCH);
                                state = FOPState::INITIAL;
                                break;
                            case FOPState::RETRANSMIT_WITH_WAIT:
                                alert(AlertEvent::ALRT_SYNCH);
                                state = FOPState::INITIAL;
                                break;
                            case FOPState::INITIALIZING_WITHOUT_BC_FRAME:
                                clcw->setConfSignal(FDURequestType::REQUEST_CONFIRMED);
                                state = FOPState::ACTIVE;
                                // cancel timer
                                break;
                            case FOPState::INITIALIZING_WITH_BC_FRAME:
                                clcw->setConfSignal(FDURequestType::REQUEST_CONFIRMED);
                                //bc_accept()??
                                // cancel timer
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
                                remove_acknowledged_frames();
                                // cancel timer
                                look_for_fdu();
                                break;
                            case FOPState::RETRANSMIT_WITHOUT_WAIT:
                                remove_acknowledged_frames();
                                // cancel timer
                                look_for_fdu();
                                state = FOPState::ACTIVE;
                                break;
                            case FOPState::RETRANSMIT_WITH_WAIT:
                                remove_acknowledged_frames();
                                // cancel timer
                                look_for_fdu();
                                state = FOPState::ACTIVE;
                                break;
                            case FOPState::INITIALIZING_WITHOUT_BC_FRAME:
                                // N/A
                                break;
                            case FOPState::INITIALIZING_WITH_BC_FRAME:
                                // N/A
                                break;
                            case FOPState::INITIAL:
                                break;
                        }
                    }
                } else {
                    // E3
                    switch (state) {
                        case FOPState::ACTIVE:
                            alert(AlertEvent::ALRT_CLCW);
                            state = FOPState::INITIAL;
                            break;
                        case FOPState::RETRANSMIT_WITHOUT_WAIT:
                            alert(AlertEvent::ALRT_CLCW);
                            state = FOPState::INITIAL;
                            break;
                        case FOPState::RETRANSMIT_WITH_WAIT:
                            alert(AlertEvent::ALRT_CLCW);
                            state = FOPState::INITIAL;
                            break;
                        case FOPState::INITIALIZING_WITHOUT_BC_FRAME:
                            alert(AlertEvent::ALRT_CLCW);
                            state = FOPState::INITIAL;
                            break;
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
                        alert(AlertEvent::ALRT_SYNCH);
                        state = FOPState::INITIAL;
                        break;
                    case FOPState::RETRANSMIT_WITHOUT_WAIT:
                        alert(AlertEvent::ALRT_SYNCH);
                        state = FOPState::INITIAL;
                        break;
                    case FOPState::RETRANSMIT_WITH_WAIT:
                        alert(AlertEvent::ALRT_SYNCH);
                        state = FOPState::INITIAL;
                        break;
                    case FOPState::INITIALIZING_WITHOUT_BC_FRAME:
                        alert(AlertEvent::ALRT_SYNCH);
                        state = FOPState::INITIAL;
                        break;
                    case FOPState::INITIALIZING_WITH_BC_FRAME:
                        break;
                    case FOPState::INITIAL:
                        break;
                }
            }
        } else if (clcw->report_value() > expectedAcknowledgementSeqNumber &&
                   expectedAcknowledgementSeqNumber >= transmitterFrameSeqNumber) {
            if (clcw->retransmission() == 0) {
                if (clcw->wait() == 0) {
                    if (expectedAcknowledgementSeqNumber == transmitterFrameSeqNumber) {
                        // E5
                        switch (state) {
                            case FOPState::ACTIVE:
                                break;
                            case FOPState::RETRANSMIT_WITHOUT_WAIT:
                                alert(AlertEvent::ALRT_SYNCH);
                                state = FOPState::INITIAL;
                                break;
                            case FOPState::RETRANSMIT_WITH_WAIT:
                                alert(AlertEvent::ALRT_SYNCH);
                                state = FOPState::INITIAL;
                                break;
                            case FOPState::INITIALIZING_WITHOUT_BC_FRAME:
                                // N/A
                                break;
                            case FOPState::INITIALIZING_WITH_BC_FRAME:
                                // N/A
                                break;
                            case FOPState::INITIAL:
                                break;
                        }
                    } else {
                        // E6
                        switch (state) {
                            case FOPState::ACTIVE:
                                remove_acknowledged_frames();
                                look_for_fdu();
                                break;
                            case FOPState::RETRANSMIT_WITHOUT_WAIT:
                                remove_acknowledged_frames();
                                look_for_fdu();
                                state = FOPState::ACTIVE;
                                break;
                            case FOPState::RETRANSMIT_WITH_WAIT:
                                remove_acknowledged_frames();
                                look_for_fdu();
                                state = FOPState::ACTIVE;
                                break;
                            case FOPState::INITIALIZING_WITHOUT_BC_FRAME:
                                // N/A
                                break;
                            case FOPState::INITIALIZING_WITH_BC_FRAME:
                                // N/A
                                break;
                            case FOPState::INITIAL:
                                break;
                        }
                    }
                } else {
                    // E7
                    switch (state) {
                        case FOPState::ACTIVE:
                            alert(AlertEvent::ALRT_CLCW);
                            state = FOPState::INITIAL;
                            break;
                        case FOPState::RETRANSMIT_WITHOUT_WAIT:
                            alert(AlertEvent::ALRT_CLCW);
                            state = FOPState::INITIAL;
                            break;
                        case FOPState::RETRANSMIT_WITH_WAIT:
                            alert(AlertEvent::ALRT_CLCW);
                            state = FOPState::INITIAL;
                            break;
                        case FOPState::INITIALIZING_WITHOUT_BC_FRAME:
                            // N/A
                            break;
                        case FOPState::INITIALIZING_WITH_BC_FRAME:
                            // N/A
                            break;
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
                                remove_acknowledged_frames();
                                alert(AlertEvent::ALRT_LIMIT);
                                state = FOPState::INITIAL;
                                break;
                            case FOPState::RETRANSMIT_WITHOUT_WAIT:
                                remove_acknowledged_frames();
                                alert(AlertEvent::ALRT_LIMIT);
                                state = FOPState::INITIAL;
                                break;
                            case FOPState::RETRANSMIT_WITH_WAIT:
                                remove_acknowledged_frames();
                                alert(AlertEvent::ALRT_LIMIT);
                                state = FOPState::INITIAL;
                                break;
                            case FOPState::INITIALIZING_WITHOUT_BC_FRAME:
                                // N/A
                                break;
                            case FOPState::INITIALIZING_WITH_BC_FRAME:
                                // N/A
                                break;
                            case FOPState::INITIAL:
                                break;
                        }
                    } else {
                        // E102
                        switch (state) {
                            case FOPState::ACTIVE:
                                alert(AlertEvent::ALRT_LIMIT);
                                state = FOPState::INITIAL;
                                break;
                            case FOPState::RETRANSMIT_WITHOUT_WAIT:
                                alert(AlertEvent::ALRT_LIMIT);
                                state = FOPState::INITIAL;
                                break;
                            case FOPState::RETRANSMIT_WITH_WAIT:
                                alert(AlertEvent::ALRT_LIMIT);
                                state = FOPState::INITIAL;
                                break;
                            case FOPState::INITIALIZING_WITHOUT_BC_FRAME:
                                // N/A
                                break;
                            case FOPState::INITIALIZING_WITH_BC_FRAME:
                                // N/A
                                break;
                            case FOPState::INITIAL:
                                break;
                        }
                    }
                } else if (transmissionLimit > 1) {
                    if (expectedAcknowledgementSeqNumber != transmitterFrameSeqNumber) {
                        if (clcw->wait() == 0) {
                            // E8
                            switch (state) {
                                case FOPState::ACTIVE:
                                    remove_acknowledged_frames();
                                    initiate_ad_retransmission();
                                    look_for_fdu();
                                    state = FOPState::RETRANSMIT_WITHOUT_WAIT;
                                    break;
                                case FOPState::RETRANSMIT_WITHOUT_WAIT:
                                    remove_acknowledged_frames();
                                    initiate_ad_retransmission();
                                    look_for_fdu();
                                    break;
                                case FOPState::RETRANSMIT_WITH_WAIT:
                                    remove_acknowledged_frames();
                                    initiate_ad_retransmission();
                                    look_for_fdu();
                                    state = FOPState::RETRANSMIT_WITHOUT_WAIT;
                                    break;
                                case FOPState::INITIALIZING_WITHOUT_BC_FRAME:
                                    // N/A
                                    break;
                                case FOPState::INITIALIZING_WITH_BC_FRAME:
                                    // N/A
                                    break;
                                case FOPState::INITIAL:
                                    break;
                            }
                        } else {
                            // E9
                            switch (state) {
                                case FOPState::ACTIVE:
                                    remove_acknowledged_frames();
                                    state = FOPState::RETRANSMIT_WITH_WAIT;
                                    break;
                                case FOPState::RETRANSMIT_WITHOUT_WAIT:
                                    remove_acknowledged_frames();
                                    state = FOPState::RETRANSMIT_WITH_WAIT;
                                    break;
                                case FOPState::RETRANSMIT_WITH_WAIT:
                                    remove_acknowledged_frames();
                                    break;
                                case FOPState::INITIALIZING_WITHOUT_BC_FRAME:
                                    // N/A
                                    break;
                                case FOPState::INITIALIZING_WITH_BC_FRAME:
                                    // N/A
                                    break;
                                case FOPState::INITIAL:
                                    break;
                            }
                        }
                    } else {
                        if (transmissionCount < transmissionLimit) {
                            if (clcw->wait() == 0) {
                                //  E10
                                switch (state) {
                                    case FOPState::ACTIVE:
                                        initiate_ad_retransmission();
                                        look_for_fdu();
                                        state = FOPState::RETRANSMIT_WITHOUT_WAIT;
                                        break;
                                    case FOPState::RETRANSMIT_WITHOUT_WAIT:
                                        break;
                                    case FOPState::RETRANSMIT_WITH_WAIT:
                                        initiate_ad_retransmission();
                                        look_for_fdu();
                                        state = FOPState::RETRANSMIT_WITHOUT_WAIT;
                                        break;
                                    case FOPState::INITIALIZING_WITHOUT_BC_FRAME:
                                        // N/A
                                        break;
                                    case FOPState::INITIALIZING_WITH_BC_FRAME:
                                        // N/A
                                        break;
                                    case FOPState::INITIAL:
                                        break;
                                }
                            } else {
                                // E11
                                switch (state) {
                                    case FOPState::ACTIVE:
                                        state = FOPState::RETRANSMIT_WITH_WAIT;
                                        break;
                                    case FOPState::RETRANSMIT_WITHOUT_WAIT:
                                        state = FOPState::RETRANSMIT_WITH_WAIT;
                                        break;
                                    case FOPState::RETRANSMIT_WITH_WAIT:
                                        break;
                                    case FOPState::INITIALIZING_WITHOUT_BC_FRAME:
                                        // N/A
                                        break;
                                    case FOPState::INITIALIZING_WITH_BC_FRAME:
                                        // N/A
                                        break;
                                    case FOPState::INITIAL:
                                        break;
                                }
                            }
                        } else {
                            if (clcw->wait() == 0) {
                                // E12
                                switch (state) {
                                    case FOPState::ACTIVE:
                                        state = FOPState::RETRANSMIT_WITHOUT_WAIT;
                                        break;
                                    case FOPState::RETRANSMIT_WITHOUT_WAIT:
                                        break;
                                    case FOPState::RETRANSMIT_WITH_WAIT:
                                        state = FOPState::RETRANSMIT_WITHOUT_WAIT;
                                        break;
                                    case FOPState::INITIALIZING_WITHOUT_BC_FRAME:
                                        // N/A
                                        break;
                                    case FOPState::INITIALIZING_WITH_BC_FRAME:
                                        // N/A
                                        break;
                                    case FOPState::INITIAL:
                                        break;
                                }
                            } else {
                                // E103
                                switch (state) {
                                    case FOPState::ACTIVE:
                                        state = FOPState::RETRANSMIT_WITH_WAIT;
                                        break;
                                    case FOPState::RETRANSMIT_WITHOUT_WAIT:
                                        state = FOPState::RETRANSMIT_WITH_WAIT;
                                        break;
                                    case FOPState::RETRANSMIT_WITH_WAIT:
                                        break;
                                    case FOPState::INITIALIZING_WITHOUT_BC_FRAME:
                                        // N/A
                                        break;
                                    case FOPState::INITIALIZING_WITH_BC_FRAME:
                                        // N/A
                                        break;
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
                    alert(AlertEvent::ALRT_NNR);
                    state = FOPState::INITIAL;
                    break;
                case FOPState::RETRANSMIT_WITHOUT_WAIT:
                    alert(AlertEvent::ALRT_NNR);
                    state = FOPState::INITIAL;
                    break;
                case FOPState::RETRANSMIT_WITH_WAIT:
                    alert(AlertEvent::ALRT_NNR);
                    state = FOPState::INITIAL;
                    break;
                case FOPState::INITIALIZING_WITHOUT_BC_FRAME:
                    alert(AlertEvent::ALRT_NNR);
                    state = FOPState::INITIAL;
                    break;
                case FOPState::INITIALIZING_WITH_BC_FRAME:
                    break;
                case FOPState::INITIAL:
                    break;
            }
        }
    } else {
        switch (state) {
            case FOPState::ACTIVE:
                alert(AlertEvent::ALRT_LOCKOUT);
                state = FOPState::INITIAL;
                break;
            case FOPState::RETRANSMIT_WITHOUT_WAIT:
                alert(AlertEvent::ALRT_LOCKOUT);
                state = FOPState::INITIAL;
                break;
            case FOPState::RETRANSMIT_WITH_WAIT:
                alert(AlertEvent::ALRT_LOCKOUT);
                state = FOPState::INITIAL;
                break;
            case FOPState::INITIALIZING_WITHOUT_BC_FRAME:
                alert(AlertEvent::ALRT_LOCKOUT);
                state = FOPState::INITIAL;
                break;
            case FOPState::INITIALIZING_WITH_BC_FRAME:
                break;
            case FOPState::INITIAL:
                break;
        }
    }
}

void FrameOperationProcedure::invalid_clcw_arrival() {
    switch (state) {
        case FOPState::ACTIVE:
            alert(AlertEvent::ALRT_CLCW);
            state = FOPState::INITIAL;
            break;
        case FOPState::RETRANSMIT_WITHOUT_WAIT:
            alert(AlertEvent::ALRT_CLCW);
            state = FOPState::INITIAL;
            break;
        case FOPState::RETRANSMIT_WITH_WAIT:
            alert(AlertEvent::ALRT_CLCW);
            state = FOPState::INITIAL;
            break;
        case FOPState::INITIALIZING_WITHOUT_BC_FRAME:
            alert(AlertEvent::ALRT_CLCW);
            state = FOPState::INITIAL;
            break;
        case FOPState::INITIALIZING_WITH_BC_FRAME:
            alert(AlertEvent::ALRT_CLCW);
            state = FOPState::INITIAL;
            break;
        case FOPState::INITIAL:
            break;
    }
}

FOPDirectiveResponse FrameOperationProcedure::initiate_ad_no_clcw(Packet *ad_frame) {
    // E23
    if (state == FOPState::INITIAL) {
        initialize();
        ad_frame->setConfSignal(FDURequestType::REQUEST_CONFIRMED);
        state = FOPState::ACTIVE;
        return FOPDirectiveResponse::ACCEPT;
    }
    return FOPDirectiveResponse::REJECT;
}

FOPDirectiveResponse FrameOperationProcedure::initiate_ad_clcw(Packet *ad_frame) {
    // E24
    if (state == FOPState::INITIAL) {
        initialize();
        // @todo start timer
        state = FOPState::ACTIVE;
        return FOPDirectiveResponse::ACCEPT;
    }
    return FOPDirectiveResponse::REJECT;
}

FOPDirectiveResponse FrameOperationProcedure::initiate_ad_unlock(Packet *ad_frame) {
    if (state == FOPState::INITIAL && bcOut == FlagState::READY) {
        // E25
        initialize();
        // @todo transmit unlock frame
        return FOPDirectiveResponse::ACCEPT;
    }
    // E26
    return FOPDirectiveResponse::REJECT;
}

FOPDirectiveResponse FrameOperationProcedure::initiate_ad_vr(Packet *ad_frame) {
    if (state == FOPState::INITIAL && bcOut == FlagState::READY) {
        // E27
        initialize();
        transmitterFrameSeqNumber = ad_frame->transferFrameSeqNumber;
        expectedAcknowledgementSeqNumber = ad_frame->transferFrameSeqNumber;
        // @todo transmit Set V(R) frame
        state = FOPState::INITIALIZING_WITH_BC_FRAME;
        return FOPDirectiveResponse::ACCEPT;
    }
    // E28
    return FOPDirectiveResponse::REJECT;
}

FOPDirectiveResponse FrameOperationProcedure::terminate_ad_service(Packet *ad_frame) {
    // E29
    if (state != FOPState::INITIAL) {
        alert(AlertEvent::ALRT_TERM);
    }
    ad_frame->setConfSignal(FDURequestType::REQUEST_CONFIRMED);
    return FOPDirectiveResponse::ACCEPT;
}

FOPDirectiveResponse FrameOperationProcedure::resume_ad_service(Packet *ad_frame) {
    switch (suspendState) {
        case 0:
            // E30
            return FOPDirectiveResponse::REJECT;
        case 1:
            // E31
            if (state == FOPState::INITIAL) {
                ad_frame->setConfSignal(FDURequestType::REQUEST_CONFIRMED);
                // @todo resume
                state = FOPState::ACTIVE;
                return FOPDirectiveResponse::ACCEPT;
            } else {
                // Not applicable. Should not get here, maybe raise an error?
                return FOPDirectiveResponse::REJECT;
            }
        case 2:
            // E32
            if (state == FOPState::INITIAL) {
                ad_frame->setConfSignal(FDURequestType::REQUEST_CONFIRMED);
                // @todo resume
                state = FOPState::RETRANSMIT_WITHOUT_WAIT;
                return FOPDirectiveResponse::ACCEPT;
            } else {
                // Not applicable
                return FOPDirectiveResponse::REJECT;
            }
        case 3:
            // E33
            if (state == FOPState::INITIAL) {
                ad_frame->setConfSignal(FDURequestType::REQUEST_CONFIRMED);
                // @todo resume
                state = FOPState::RETRANSMIT_WITH_WAIT;
                return FOPDirectiveResponse::ACCEPT;
            } else {
                // Not applicable
                return FOPDirectiveResponse::REJECT;
            }
        case 4:
            // E34
            if (state == FOPState::INITIAL) {
                ad_frame->setConfSignal(FDURequestType::REQUEST_CONFIRMED);
                // @todo resume
                state = FOPState::INITIALIZING_WITHOUT_BC_FRAME;
                return FOPDirectiveResponse::ACCEPT;
            } else {
                // Not applicable
                return FOPDirectiveResponse::REJECT;
            }
    }
}

FOPDirectiveResponse FrameOperationProcedure::set_vs(Packet *ad_frame) {
    // E35
    if (state == FOPState::INITIAL && suspendState == 0) {
        transmitterFrameSeqNumber = ad_frame->transferFrameSeqNumber;
        expectedAcknowledgementSeqNumber = ad_frame->transferFrameSeqNumber;
        return FOPDirectiveResponse::ACCEPT;
    } else {
        return FOPDirectiveResponse::REJECT;
    }
}

FOPDirectiveResponse FrameOperationProcedure::set_fop_width(Packet *ad_frame, uint8_t vr) {
    // E36
    fopSlidingWindow = vr;
    ad_frame->setConfSignal(FDURequestType::REQUEST_CONFIRMED);
    return FOPDirectiveResponse::ACCEPT;
}

FOPDirectiveResponse FrameOperationProcedure::set_t1_initial(Packet *ad_frame) {
    // E37
    set_t1_initial(ad_frame);
    ad_frame->setConfSignal(FDURequestType::REQUEST_CONFIRMED);
    return FOPDirectiveResponse::ACCEPT;
}

FOPDirectiveResponse FrameOperationProcedure::set_transmission_limit(Packet *ad_frame, uint8_t vr) {
    // E38
    transmissionLimit = vr;
    ad_frame->setConfSignal(FDURequestType::REQUEST_CONFIRMED);
    return FOPDirectiveResponse::ACCEPT;
}

FOPDirectiveResponse FrameOperationProcedure::set_timeout_type(Packet *ad_frame, bool vr) {
    // E39
    timeoutType = vr;
    ad_frame->setConfSignal(FDURequestType::REQUEST_CONFIRMED);
    return FOPDirectiveResponse::ACCEPT;
}


FOPDirectiveResponse FrameOperationProcedure::invalid_directive(Packet *ad_frame) {
    // E40
    return FOPDirectiveResponse::REJECT;
}

void FrameOperationProcedure::ad_accept(Packet *ad_frame) {
    // E41
    adOut = FlagState::READY;
    if (state == FOPState::ACTIVE || state == FOPState::RETRANSMIT_WITHOUT_WAIT) {
        look_for_fdu();
    }
}

void FrameOperationProcedure::ad_reject(Packet *ad_frame) {
    // E42
    alert(AlertEvent::ALRT_LLIF);
    state = FOPState::INITIAL;
}

void FrameOperationProcedure::bc_accept(Packet *ad_frame) {
    // E43
    bcOut = FlagState::READY;
    if (state == FOPState::INITIALIZING_WITH_BC_FRAME) {
        look_for_directive();
    }
}

void FrameOperationProcedure::bc_reject(Packet *ad_frame) {
    alert(AlertEvent::ALRT_LLIF);
    state = FOPState::INITIAL;
}

FOPDirectiveResponse FrameOperationProcedure::bd_accept(Packet *ad_frame) {
    bdOut = FlagState::READY;
    return FOPDirectiveResponse::ACCEPT;
}

void FrameOperationProcedure::bd_reject(Packet *ad_frame) {
    alert(AlertEvent::ALRT_LLIF);
    state = FOPState::INITIAL;
}