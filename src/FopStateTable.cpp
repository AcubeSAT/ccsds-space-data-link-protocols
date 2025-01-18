#include <FrameOperationProcedure.hpp>


std::pair<FOPNotification, uint8_t> FrameOperationProcedure::applyFopStateTable() {
    // clear output signal queues
    directiveNotificationSignalQueue.clear();
    transferNotificationSignalQueue.clear();
    asynchronousNotificationSignalQueue.clear();
    fopToLowerLayerRequestSignalQueue.clear();

    // The returned FopNotification will be: NON_APPLICABLE_COMBINATION_OF_STATE_AND_EVENT, if somehow an
    // impossible combination of event and state are reached.

    /** clcw arrival events **/
    uint8_t eventCode = 0;
    FOPNotification fopNotification = NO_FOP_EVENT;
    if (!clcwQueue.empty()) {
        CLCW clcw = clcwQueue.front();
        clcwQueue.pop();

        bool validClcw = true;
        // standard validity checks
        if ((clcw.getControlWordType() != ControlWordType) ||
            (clcw.getClcwVersion() != ClcwVersionNumber) ||
            (clcw.getCopInEffect() != CopInEffect) ||
            (clcw.getVcId() != vchan->VCID)) {
            // E15
            eventCode = 15;
            validClcw = false;
            switch (state) {
                case ACTIVE:
                case RETRANSMIT_WITHOUT_WAIT:
                case RETRANSMIT_WITH_WAIT:
                case INITIALIZING_WITHOUT_BC_FRAME:
                case INITIALIZING_WITH_BC_FRAME:
                    alert(ALRT_CLCW);
                    state = INITIAL;
            }

            return std::make_pair(fopNotification, eventCode);
        }

        // validity checks of special fields that are not part of cop
        // TODO choose which of the following mission specific fields should be used and perform
        //       checks here: status field, no rf flag, no bit lock flag, farm-B counter
        //       (farm-B counter is updated by farm, but not checked by fop. It can provide
        //       (limited confirmation of type BC and BD frame reception)

        if (clcw.getLockout() && validClcw) {
            if (clcw.getReportValue() == transmitterFrameSeqNumber) {
                if (clcw.getRetransmit() == 0) {
                    if (clcw.getWait() == 0) {
                        if (clcw.getReportValue() == expectedAcknowledgementSeqNumber) {
                            // E1
                            eventCode = 1;
                            switch (state) {
                                case RETRANSMIT_WITHOUT_WAIT:
                                case RETRANSMIT_WITH_WAIT:
                                    alert(ALRT_SYNCH);
                                    state = INITIAL;
                                    break;
                                case INITIALIZING_WITHOUT_BC_FRAME:
                                    if (!initiateWithClcwCheckId) {
                                        fopNotification = FOP_UNEXPECTED_VALUE;
                                    }
                                    directiveNotificationSignalQueue.push(
                                            DirectiveNotificationSignal(initiateWithClcwCheckId.value(), POSITIVE_CONFIRM_RESPONSE_TO_DIRECTIVE));
                                    initiateWithClcwCheckId = etl::nullopt;
                                    timer.stopTimer();
                                    state = ACTIVE;
                                    break;
                                case INITIALIZING_WITH_BC_FRAME:
                                    etl::ilist<TransferFrameTC*>::iterator sent_queue_it = sentQueueFOP.begin();
                                    etl::ilist<TransferFrameTC>::iterator master_copy_it;
                                    TransferFrameTC* frame;
                                    while (sent_queue_it != sentQueueFOP.end()) {
                                        frame = *sent_queue_it;
                                        if (frame->getServiceType() == ServiceType::TYPE_BC) {
                                            // message higher layers about the successful directive (type bc frame reception)
                                            if (!directiveNotificationSignalQueue.full()) {
                                                directiveNotificationSignalQueue.push(DirectiveNotificationSignal(initiateWithBcFrameId.value(),
                                                                                                                  POSITIVE_CONFIRM_RESPONSE_TO_DIRECTIVE, frame));
                                                initiateWithBcFrameId = etl::nullopt;
                                            }

                                            // delete pointer from the sent queue
                                            sentQueueFOP.erase(sent_queue_it);
                                            break;
                                        }
                                        sent_queue_it++; // This frame was not type BC, move to the next one
                                    }
                                    timer.stopTimer();
                                    state = ACTIVE;
                                    break;
                            }
                        } else { // N(R) <> NN(R)
                            // E2
                            eventCode = 2;
                            switch (state) {
                                case ACTIVE:
                                case RETRANSMIT_WITHOUT_WAIT:
                                case RETRANSMIT_WITH_WAIT:
                                    removeAcknowledgedFramesFromSentQueue(clcw.getReportValue());
                                    timer.stopTimer();
                                    lookForFdu();
                                    state = ACTIVE;
                                    break;
                                case INITIALIZING_WITHOUT_BC_FRAME:
                                case INITIALIZING_WITH_BC_FRAME:
                                    fopNotification = NON_APPLICABLE_COMBINATION_OF_STATE_AND_EVENT;
                            }
                        }

                    } else { // wait flag == 1
                        // E3
                        eventCode = 3;
                        switch (state) {
                            case ACTIVE:
                            case RETRANSMIT_WITHOUT_WAIT:
                            case RETRANSMIT_WITH_WAIT:
                            case INITIALIZING_WITHOUT_BC_FRAME:
                            case INITIALIZING_WITH_BC_FRAME:
                                alert(ALRT_CLCW);
                                state = INITIAL;
                        }
                    }
                } else {  // retransmit flag = 1
                    // E4
                    eventCode = 4;
                    switch (state) {
                        case ACTIVE:
                        case RETRANSMIT_WITHOUT_WAIT:
                        case RETRANSMIT_WITH_WAIT:
                        case INITIALIZING_WITHOUT_BC_FRAME:
                            alert(ALRT_SYNCH);
                            state = INITIAL;
                    }
                }
            } else if (withinWindow(clcw.getReportValue(), expectedAcknowledgementSeqNumber,
                                    (transmitterFrameSeqNumber == 0) ? 255 : transmitterFrameSeqNumber - 1)) {
                if (!clcw.getRetransmit()) {
                    if (!clcw.getWait()) {
                        if (clcw.getReportValue() == expectedAcknowledgementSeqNumber) {
                            // E5
                            eventCode = 5;
                            switch (state) {
                                case RETRANSMIT_WITHOUT_WAIT:
                                case RETRANSMIT_WITH_WAIT:
                                    alert(ALRT_SYNCH);
                                    break;
                                case INITIALIZING_WITHOUT_BC_FRAME:
                                case INITIALIZING_WITH_BC_FRAME:
                                    fopNotification = NON_APPLICABLE_COMBINATION_OF_STATE_AND_EVENT;
                                    break;
                            }
                        } else {
                            // E6
                            eventCode = 6;
                            switch (state) {
                                case ACTIVE:
                                case RETRANSMIT_WITHOUT_WAIT:
                                case RETRANSMIT_WITH_WAIT:
                                    removeAcknowledgedFramesFromSentQueue(clcw.getReportValue());
                                    fopNotification = lookForFdu();
                                    break;
                                case INITIALIZING_WITHOUT_BC_FRAME:
                                case INITIALIZING_WITH_BC_FRAME:
                                    fopNotification = NON_APPLICABLE_COMBINATION_OF_STATE_AND_EVENT;
                            }
                        }
                    } else {  // wait flag = 1
                        // E7 rev. B
                        eventCode = 7;
                        switch (state) {
                            case ACTIVE:
                            case RETRANSMIT_WITHOUT_WAIT:
                            case RETRANSMIT_WITH_WAIT:
                                alert(ALRT_CLCW);
                                state = INITIAL;
                                break;
                            case INITIALIZING_WITHOUT_BC_FRAME:
                            case INITIALIZING_WITH_BC_FRAME:
                                fopNotification = NON_APPLICABLE_COMBINATION_OF_STATE_AND_EVENT;
                                break;
                        }
                    }
                } else {  // retransmit flag = 1
                    if (transmissionLimit == 1) {
                        if (clcw.getReportValue() != expectedAcknowledgementSeqNumber) {
                            // E101
                            eventCode = 101;
                            switch (state) {
                                case ACTIVE:
                                case RETRANSMIT_WITHOUT_WAIT:
                                case RETRANSMIT_WITH_WAIT:
                                    fopNotification = removeAcknowledgedFramesFromSentQueue(clcw.getReportValue());
                                    alert(ALRT_LIMIT);
                                    state = INITIAL;
                                    break;
                                case INITIALIZING_WITHOUT_BC_FRAME:
                                case INITIALIZING_WITH_BC_FRAME:
                                    fopNotification = NON_APPLICABLE_COMBINATION_OF_STATE_AND_EVENT;
                                    break;
                            }
                        } else {
                            // E102
                            eventCode = 102;
                            switch (state) {
                                case ACTIVE:
                                case RETRANSMIT_WITHOUT_WAIT:
                                case RETRANSMIT_WITH_WAIT:
                                    alert(ALRT_LIMIT);
                                    state = INITIAL;
                                    break;
                                case INITIALIZING_WITHOUT_BC_FRAME:
                                case INITIALIZING_WITH_BC_FRAME:
                                    fopNotification = NON_APPLICABLE_COMBINATION_OF_STATE_AND_EVENT;
                                    break;
                            }
                        }
                    }
                    else { // transmissionLimit > 1
                        if (clcw.getReportValue() == expectedAcknowledgementSeqNumber) {
                            if (transmissionCount < transmissionLimit) {
                                if (clcw.getWait() == 0) {
                                    // E10
                                    eventCode = 10;
                                    switch (state) {
                                        case ACTIVE:
                                        case RETRANSMIT_WITH_WAIT:
                                            initiateRetransmission(ServiceType::TYPE_AD);
                                            fopNotification = lookForFdu();
                                            state = RETRANSMIT_WITHOUT_WAIT;
                                            break;
                                        case INITIALIZING_WITHOUT_BC_FRAME:
                                        case INITIALIZING_WITH_BC_FRAME:
                                            fopNotification = NON_APPLICABLE_COMBINATION_OF_STATE_AND_EVENT;
                                    }
                                } else { // wait flag = 1
                                    // E11
                                    eventCode = 11;
                                    switch (state) {
                                        case ACTIVE:
                                        case RETRANSMIT_WITHOUT_WAIT:
                                            state = RETRANSMIT_WITH_WAIT;
                                            break;
                                        case INITIALIZING_WITHOUT_BC_FRAME:
                                        case INITIALIZING_WITH_BC_FRAME:
                                            fopNotification = NON_APPLICABLE_COMBINATION_OF_STATE_AND_EVENT;
                                    }
                                }
                            } else { // transmission count >= transmission limit
                                if (clcw.getWait() == 0) {
                                    // E12
                                    eventCode = 12;
                                    switch (state) {
                                        case ACTIVE:
                                        case RETRANSMIT_WITH_WAIT:
                                            state = RETRANSMIT_WITHOUT_WAIT;
                                            break;
                                        case INITIALIZING_WITHOUT_BC_FRAME:
                                        case INITIALIZING_WITH_BC_FRAME:
                                            fopNotification = NON_APPLICABLE_COMBINATION_OF_STATE_AND_EVENT;
                                    }
                                } else { // wait flag = 1
                                    // E103
                                    eventCode = 103;
                                    switch (state) {
                                        case ACTIVE:
                                        case RETRANSMIT_WITHOUT_WAIT:
                                            state = RETRANSMIT_WITH_WAIT;
                                            break;
                                        case INITIALIZING_WITHOUT_BC_FRAME:
                                        case INITIALIZING_WITH_BC_FRAME:
                                            fopNotification = NON_APPLICABLE_COMBINATION_OF_STATE_AND_EVENT;
                                    }
                                }
                            }
                        } else {  // N(R) <> NN(R)
                            if (clcw.getWait() == 0) {
                                // E8
                                eventCode = 8;
                                switch (state) {
                                    case ACTIVE:
                                    case RETRANSMIT_WITHOUT_WAIT:
                                    case RETRANSMIT_WITH_WAIT:
                                        removeAcknowledgedFramesFromSentQueue(clcw.getReportValue());
                                        initiateRetransmission(ServiceType::TYPE_AD);
                                        fopNotification = lookForFdu();
                                        state = RETRANSMIT_WITHOUT_WAIT;
                                        break;
                                    case INITIALIZING_WITHOUT_BC_FRAME:
                                    case INITIALIZING_WITH_BC_FRAME:
                                        fopNotification = NON_APPLICABLE_COMBINATION_OF_STATE_AND_EVENT;
                                }
                            } else {  // wait flag = 1
                                // E9
                                eventCode = 9;
                                switch (state) {
                                    case ACTIVE:
                                    case RETRANSMIT_WITHOUT_WAIT:
                                    case RETRANSMIT_WITH_WAIT:
                                        fopNotification = removeAcknowledgedFramesFromSentQueue(clcw.getReportValue());
                                        state = RETRANSMIT_WITH_WAIT;
                                        break;
                                    case INITIALIZING_WITHOUT_BC_FRAME:
                                    case INITIALIZING_WITH_BC_FRAME:
                                        fopNotification = NON_APPLICABLE_COMBINATION_OF_STATE_AND_EVENT;
                                }
                            }
                        }
                    }
                }
            } else {  // invalid N(R)
                // E13
                eventCode = 13;
                switch (state) {
                    case ACTIVE:
                    case RETRANSMIT_WITHOUT_WAIT:
                    case RETRANSMIT_WITH_WAIT:
                    case INITIALIZING_WITHOUT_BC_FRAME:
                        alert(ALRT_NNR);
                        state = INITIAL;
                        break;
                }
            }
        } else { // lockout flag = 1
            // E14
            eventCode = 14;
            switch (state) {
                case ACTIVE:
                case RETRANSMIT_WITHOUT_WAIT:
                case RETRANSMIT_WITH_WAIT:
                case INITIALIZING_WITHOUT_BC_FRAME:
                    alert(ALRT_LOCKOUT);
                    state = INITIAL;
                    break;
            }
        }
    }

    // Return if an event was detected, or something went wrong
    if (fopNotification != NO_FOP_EVENT || eventCode != 0) {
        return std::make_pair(fopNotification, eventCode);
    }

    /** Timer expiration **/
    if (timer.getRunning() && (timer.getRemainingTime() == 0)) {
        if (transmissionCount < transmissionLimit) {
            if (timeoutType == 0) {
                // E16 rev. B
                eventCode = 16;
                switch (state) {
                    case ACTIVE:
                    case RETRANSMIT_WITHOUT_WAIT:
                        initiateRetransmission(ServiceType::TYPE_AD);
                        fopNotification = lookForFdu();
                        break;
                    case INITIALIZING_WITHOUT_BC_FRAME:
                        alert(ALRT_T1);
                        state = INITIAL;
                        break;
                    case INITIALIZING_WITH_BC_FRAME:
                        initiateRetransmission(ServiceType::TYPE_BC);
                        fopNotification = lookForDirective();
                        break;
                    case INITIAL:
                        fopNotification = NON_APPLICABLE_COMBINATION_OF_STATE_AND_EVENT;
                }
            } else { // TT = 1
                // E104
                eventCode = 104;
                switch (state) {
                    case ACTIVE:
                    case RETRANSMIT_WITHOUT_WAIT:
                        initiateRetransmission(ServiceType::TYPE_AD);
                        fopNotification = lookForFdu();
                        break;
                    case INITIALIZING_WITHOUT_BC_FRAME:
                        suspendState = SUSPENDED_PREV_STATE_INITIALIZING_WITHOUT_BC_FRAME;
                        asynchronousNotificationSignalQueue.push(AsynchronousNotificationSignal(SUSPEND));
                        state = INITIAL;
                        break;
                    case INITIALIZING_WITH_BC_FRAME:
                        initiateRetransmission(ServiceType::TYPE_BC);
                        fopNotification = lookForDirective();
                        break;
                    case INITIAL:
                        fopNotification = NON_APPLICABLE_COMBINATION_OF_STATE_AND_EVENT;
                }
            }
        } else { // transmission count >= transmission limit
            if (timeoutType == 0) {
                // E17 rev. B
                eventCode = 17;
                switch (state) {
                    case INITIAL:
                        fopNotification = NON_APPLICABLE_COMBINATION_OF_STATE_AND_EVENT;
                        break;
                    default:
                        alert(ALRT_T1);
                        state = INITIAL;
                }
            } else { // TT = 1
                // E18
                eventCode = 18;
                switch (state) {
                    case ACTIVE:
                        suspendState = SUSPENDED_PREV_STATE_ACTIVE; // fallthrough
                    case RETRANSMIT_WITHOUT_WAIT:
                        suspendState = SUSPENDED_PREV_STATE_RETRANSMIT_WITHOUT_WAIT; // fallthrough
                    case RETRANSMIT_WITH_WAIT:
                        suspendState = SUSPENDED_PREV_STATE_RETRANSMIT_WITH_WAIT; // fallthrough
                    case INITIALIZING_WITHOUT_BC_FRAME:
                        suspendState = SUSPENDED_PREV_STATE_INITIALIZING_WITHOUT_BC_FRAME;
                        asynchronousNotificationSignalQueue.push(AsynchronousNotificationSignal(SUSPEND));
                        state = INITIAL;
                        break;
                    case INITIALIZING_WITH_BC_FRAME:
                        alert(ALRT_T1);
                        state = INITIAL;
                        break;
                    case INITIAL:
                        fopNotification = NON_APPLICABLE_COMBINATION_OF_STATE_AND_EVENT;
                }
            }
        }
    }

    // Return if an event was detected, or something went wrong
    if (fopNotification != NO_FOP_EVENT || eventCode != 0) {
        return std::make_pair(fopNotification, eventCode);
    }

    /** Receive request to transfer fdu **/
    if (!transferFduSignalQueue.empty()) {
        FduTransferSignal fduTransferSignal = transferFduSignalQueue.front();
        transferNotificationSignalQueue.pop();

        if (fduTransferSignal.serviceType != ServiceType::TYPE_AD &&
            fduTransferSignal.serviceType != ServiceType::TYPE_BD) {
            fopNotification = FOP_UNEXPECTED_VALUE;
        }

        if (fduTransferSignal.frame->getServiceType() != ServiceType::TYPE_AD &&
            fduTransferSignal.frame->getServiceType() != ServiceType::TYPE_BD) {
            fopNotification = FOP_UNEXPECTED_VALUE;
        }

        if (fduTransferSignal.serviceType == ServiceType::TYPE_AD) {
            if (waitQueueFOP.empty()) {
                // E19
                eventCode = 19;
                switch (state) {
                    case ACTIVE:
                    case RETRANSMIT_WITHOUT_WAIT:
                        waitQueueFOP.push_back(fduTransferSignal.frame);
                        fopNotification = lookForFdu();
                        break;
                    case RETRANSMIT_WITH_WAIT:
                        waitQueueFOP.push_back((fduTransferSignal.frame));
                        break;
                    case INITIALIZING_WITHOUT_BC_FRAME:
                    case INITIALIZING_WITH_BC_FRAME:
                    case INITIAL:
                        transferNotificationSignalQueue.push(
                                TransferNotificationSignal(REJECT_RESPONSE_TO_TRANSFER_FDU, fduTransferSignal.frame));
                }
            } else { // wait queue not empty
                // E20
                eventCode = 20;
                transferNotificationSignalQueue.push(
                        TransferNotificationSignal(REJECT_RESPONSE_TO_TRANSFER_FDU, fduTransferSignal.frame));
            }
        } else { // TYPE BD request
            if (bdOut == READY) {
                // E21 rev. B
                eventCode = 21;
                transferNotificationSignalQueue.push(TransferNotificationSignal(ACCEPT_RESPONSE_TO_TRANSFER_FDU, fduTransferSignal.frame));
                fopNotification = transmitBdFrame(fduTransferSignal.frame);
            } else { // bd_out_flag not ready
                // E22
                eventCode = 22;
                transferNotificationSignalQueue.push(
                        TransferNotificationSignal(REJECT_RESPONSE_TO_TRANSFER_FDU, fduTransferSignal.frame));
            }

        }
    }

    // Return if an event was detected, or something went wrong
    if (fopNotification != NO_FOP_EVENT || eventCode != 0) {
        return std::make_pair(fopNotification, eventCode);
    }

    /** Directive request reception**/
    if (!directiveRequestSignalQueue.empty()) {
        DirectiveRequestSignal directiveRequestSignal = directiveRequestSignalQueue.front();
        directiveRequestSignalQueue.pop();

        switch (directiveRequestSignal.directiveType) {
            case INITIATE_AD_SERVICE_WITHOUT_CLCW_CHECK:
                // E23
                eventCode = 23;
                switch (state) {
                    case INITIAL:
                        directiveNotificationSignalQueue.push(
                                DirectiveNotificationSignal(directiveRequestSignal.requestIdentifier, ACCEPT_RESPONSE_TO_DIRECTIVE));
                        initialize();
                        directiveNotificationSignalQueue.push(
                                DirectiveNotificationSignal(directiveRequestSignal.requestIdentifier, POSITIVE_CONFIRM_RESPONSE_TO_DIRECTIVE));
                        state = ACTIVE;
                        break;
                    default:
                        directiveNotificationSignalQueue.push(
                                DirectiveNotificationSignal(directiveRequestSignal.requestIdentifier, REJECT_RESPONSE_TO_DIRECTIVE));
                }
                break;
            case INITIATE_AD_SERVICE_WITH_CLCW_CHECK:
                // E24
                eventCode = 24;
                switch (state) {
                    case INITIAL:
                        initiateWithClcwCheckId.emplace(directiveRequestSignal.requestIdentifier); // store request identifier
                        directiveNotificationSignalQueue.push(
                                DirectiveNotificationSignal(directiveRequestSignal.requestIdentifier, ACCEPT_RESPONSE_TO_DIRECTIVE));
                        initialize();
                        timer.startTimer(tiInitial);
                        state = INITIALIZING_WITHOUT_BC_FRAME;
                        break;
                    default:
                        directiveNotificationSignalQueue.push(
                                DirectiveNotificationSignal(directiveRequestSignal.requestIdentifier, REJECT_RESPONSE_TO_DIRECTIVE));
                }
                break;
            case INITIATE_AD_SERVICE_WITH_UNLOCK:
                if (!directiveRequestSignal.directiveQualifier) {
                    fopNotification = FOP_UNEXPECTED_VALUE;
                    break;
                }

                if (bcOut == READY) {
                    // E25
                    eventCode = 25;
                    switch (state) {
                        case INITIAL:
                            directiveNotificationSignalQueue.push(
                                    DirectiveNotificationSignal(directiveRequestSignal.requestIdentifier, ACCEPT_RESPONSE_TO_DIRECTIVE));
                            initialize();
                            fopNotification = transmitBcFrame(directiveRequestSignal);
                            initiateWithBcFrameId.emplace(directiveRequestSignal.requestIdentifier);
                            state = INITIALIZING_WITH_BC_FRAME;
                            break;
                        default:
                            directiveNotificationSignalQueue.push(
                                    DirectiveNotificationSignal(directiveRequestSignal.requestIdentifier, REJECT_RESPONSE_TO_DIRECTIVE));
                    }
                } else { // bc_out_flag not ready
                    // E26
                    eventCode = 26;
                    directiveNotificationSignalQueue.push(
                            DirectiveNotificationSignal(directiveRequestSignal.requestIdentifier, REJECT_RESPONSE_TO_DIRECTIVE));
                }
                break;
            case INITIATE_AD_SERVICE_WITH_SET_VR:
                if (!directiveRequestSignal.directiveQualifier) {
                    fopNotification = FOP_UNEXPECTED_VALUE;
                    break;
                }

                if (bcOut == READY) {
                    // E27 rev. B
                    eventCode = 27;
                    switch (state) {
                        case INITIAL:
                            directiveNotificationSignalQueue.push(
                                    DirectiveNotificationSignal(directiveRequestSignal.requestIdentifier, ACCEPT_RESPONSE_TO_DIRECTIVE));
                            transmitterFrameSeqNumber = static_cast<uint8_t>(directiveRequestSignal.directiveQualifier.value());
                            expectedAcknowledgementSeqNumber = static_cast<uint8_t>(directiveRequestSignal.directiveQualifier.value());
                            fopNotification = transmitBcFrame(directiveRequestSignal);
                            initiateWithBcFrameId.emplace(directiveRequestSignal.requestIdentifier);
                            state = INITIALIZING_WITH_BC_FRAME;
                            break;
                        default:
                            directiveNotificationSignalQueue.push(
                                    DirectiveNotificationSignal(directiveRequestSignal.requestIdentifier, REJECT_RESPONSE_TO_DIRECTIVE));
                    }
                } else { // bc_out_flag not ready
                    // E28
                    eventCode = 28;
                    directiveNotificationSignalQueue.push(
                            DirectiveNotificationSignal(directiveRequestSignal.requestIdentifier, REJECT_RESPONSE_TO_DIRECTIVE));
                }
                break;
            case TERMINATE_AD_SERVICE:
                // E29
                eventCode = 29;
                switch (state) {
                    case INITIAL:
                        directiveNotificationSignalQueue.push(
                                DirectiveNotificationSignal(directiveRequestSignal.requestIdentifier, ACCEPT_RESPONSE_TO_DIRECTIVE));
                        directiveNotificationSignalQueue.push(
                                DirectiveNotificationSignal(directiveRequestSignal.requestIdentifier, POSITIVE_CONFIRM_RESPONSE_TO_DIRECTIVE));
                        break;
                    default:
                        directiveNotificationSignalQueue.push(
                                DirectiveNotificationSignal(directiveRequestSignal.requestIdentifier, ACCEPT_RESPONSE_TO_DIRECTIVE));
                        alert(ALRT_TERM);
                        directiveNotificationSignalQueue.push(
                                DirectiveNotificationSignal(directiveRequestSignal.requestIdentifier, POSITIVE_CONFIRM_RESPONSE_TO_DIRECTIVE));
                        state = INITIAL;
                }
                break;
            case RESUME_AD_SERVICE:
                // E30 to E34
                if (suspendState == 0) {
                    eventCode = 30;
                    directiveNotificationSignalQueue.push(
                            DirectiveNotificationSignal(directiveRequestSignal.requestIdentifier, REJECT_RESPONSE_TO_DIRECTIVE));
                } else {
                    eventCode = 30 + static_cast<uint8_t>(suspendState);
                    switch (state) {
                        case INITIAL:
                            directiveNotificationSignalQueue.push(
                                    DirectiveNotificationSignal(directiveRequestSignal.requestIdentifier, ACCEPT_RESPONSE_TO_DIRECTIVE));
                            resume();
                            directiveNotificationSignalQueue.push(
                                    DirectiveNotificationSignal(directiveRequestSignal.requestIdentifier, POSITIVE_CONFIRM_RESPONSE_TO_DIRECTIVE));
                            state = static_cast<FOPState>(suspendState);
                            break;
                        default:
                            fopNotification = NON_APPLICABLE_COMBINATION_OF_STATE_AND_EVENT;
                    }
                }
                break;
            case SET_NEW_VS:
                if (!directiveRequestSignal.directiveQualifier) {
                    fopNotification = FOP_UNEXPECTED_VALUE;
                    break;
                }

                // E35 rev. B
                eventCode = 35;
                if (state == INITIAL && suspendState == NOT_SUSPENDED ) {
                    directiveNotificationSignalQueue.push(
                            DirectiveNotificationSignal(directiveRequestSignal.requestIdentifier, ACCEPT_RESPONSE_TO_DIRECTIVE));
                    transmitterFrameSeqNumber = static_cast<uint8_t>(directiveRequestSignal.directiveQualifier.value());
                    expectedAcknowledgementSeqNumber = static_cast<uint8_t>(directiveRequestSignal.directiveQualifier.value());
                    directiveNotificationSignalQueue.push(
                            DirectiveNotificationSignal(directiveRequestSignal.requestIdentifier, POSITIVE_CONFIRM_RESPONSE_TO_DIRECTIVE));
                } else {
                    directiveNotificationSignalQueue.push(
                            DirectiveNotificationSignal(directiveRequestSignal.requestIdentifier, REJECT_RESPONSE_TO_DIRECTIVE));
                }
                break;
            case SET_FOP_SLIDING_WINDOW_WIDTH:
                if (!directiveRequestSignal.directiveQualifier) {
                    fopNotification = FOP_UNEXPECTED_VALUE;
                    break;
                }

                // E36
                eventCode = 36;
                directiveNotificationSignalQueue.push(
                        DirectiveNotificationSignal(directiveRequestSignal.requestIdentifier, ACCEPT_RESPONSE_TO_DIRECTIVE));
                fopSlidingWindowWidth = static_cast<uint8_t>(directiveRequestSignal.directiveQualifier.value());
                directiveNotificationSignalQueue.push(
                        DirectiveNotificationSignal(directiveRequestSignal.requestIdentifier, POSITIVE_CONFIRM_RESPONSE_TO_DIRECTIVE));
                break;
            case SET_T1_INITIAL:
                if (!directiveRequestSignal.directiveQualifier) {
                    fopNotification = FOP_UNEXPECTED_VALUE;
                    break;
                }

                // E37
                eventCode = 37;
                directiveNotificationSignalQueue.push(
                        DirectiveNotificationSignal(directiveRequestSignal.requestIdentifier, ACCEPT_RESPONSE_TO_DIRECTIVE));
                tiInitial = directiveRequestSignal.directiveQualifier.value();
                directiveNotificationSignalQueue.push(
                        DirectiveNotificationSignal(directiveRequestSignal.requestIdentifier, POSITIVE_CONFIRM_RESPONSE_TO_DIRECTIVE));
                break;
            case SET_TRANSMISSION_LIMIT:
                if (!directiveRequestSignal.directiveQualifier) {
                    fopNotification = FOP_UNEXPECTED_VALUE;
                    break;
                }

                // E38
                eventCode = 38;
                directiveNotificationSignalQueue.push(
                        DirectiveNotificationSignal(directiveRequestSignal.requestIdentifier, ACCEPT_RESPONSE_TO_DIRECTIVE));
                transmissionLimit = static_cast<uint8_t>(directiveRequestSignal.directiveQualifier.value());
                directiveNotificationSignalQueue.push(
                        DirectiveNotificationSignal(directiveRequestSignal.requestIdentifier, POSITIVE_CONFIRM_RESPONSE_TO_DIRECTIVE));
                break;
            case SET_TIMEOUT_TYPE:
                if (!directiveRequestSignal.directiveQualifier) {
                    fopNotification = FOP_UNEXPECTED_VALUE;
                    break;
                }

                // E39
                eventCode = 39;
                directiveNotificationSignalQueue.push(
                        DirectiveNotificationSignal(directiveRequestSignal.requestIdentifier, ACCEPT_RESPONSE_TO_DIRECTIVE));
                timeoutType = static_cast<bool>(directiveRequestSignal.directiveQualifier.value());
                directiveNotificationSignalQueue.push(
                        DirectiveNotificationSignal(directiveRequestSignal.requestIdentifier, POSITIVE_CONFIRM_RESPONSE_TO_DIRECTIVE));
                break;
            default:  // invalid directive
                // E40
                eventCode = 40;
                directiveNotificationSignalQueue.push(
                        DirectiveNotificationSignal(directiveRequestSignal.requestIdentifier, REJECT_RESPONSE_TO_DIRECTIVE));
                break;
        }
    }

    // Return if an event was detected, or something went wrong
    if (fopNotification != NO_FOP_EVENT || eventCode != 0) {
        return std::make_pair(fopNotification, eventCode);
    }

    /** Response from lower layers **/
    if (!lowerLayerResponseSignalQueue.empty()) {
        LowerLayerResponseSignal lowerLayerResponseSignal = lowerLayerResponseSignalQueue.front();
        lowerLayerResponseSignalQueue.pop();

        switch (lowerLayerResponseSignal) {
            case AD_REJECT: // fallthrough
                // E42
                eventCode = 42;
            case BC_REJECT: // fallthrough
                // E44
                eventCode = 44;
            case BD_REJECT:
                // E46
                eventCode = 46;
                alert(ALRT_LLIF);
                state = INITIAL;
                break;
            case AD_ACCEPT:
                // E41
                eventCode = 41;
                adOut = READY;

                if ((state == ACTIVE) || (state == RETRANSMIT_WITHOUT_WAIT)) {
                    fopNotification = lookForFdu();
                }
                break;
            case BC_ACCEPT:
                // E43
                eventCode = 43;
                bcOut = READY;

                if (state == INITIALIZING_WITH_BC_FRAME) {
                    fopNotification = lookForDirective();
                }
                break;
            case BD_ACCEPT:
                // E45
                eventCode = 45;
                bdOut = READY;
                // An ACCEPT_RESPONSE_TO_TRANSFER_FDU is already sent at E21, there is no point
                // sending a duplicate here (as the protocol suggests).
                // transferNotificationSignalQueue.push(TransferNotificationSignal(ACCEPT_RESPONSE_TO_TRANSFER_FDU);
                break;
        }
    }

    return std::make_pair(fopNotification, eventCode);
}