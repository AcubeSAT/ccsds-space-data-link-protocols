#include <FrameOperationProcedure.hpp>
#include <CCSDSChannel.hpp>
#include "CCSDSLoggerImpl.h"
FOPNotification FrameOperationProcedure::purgeSentQueue() {
	etl::ilist<TransferFrameTC*>::iterator cur_frame = sentQueueFOP->begin();

	while (cur_frame != sentQueueFOP->end()) {
		(*cur_frame)->setConfSignal(FDURequestType::REQUEST_NEGATIVE_CONFIRM);
		sentQueueFOP->erase(cur_frame++);
	}
	ccsdsLogNotice(Tx, TypeFOPNotif, NO_FOP_EVENT);
	return FOPNotification::NO_FOP_EVENT;
}

FOPNotification FrameOperationProcedure::purgeWaitQueue() {
	etl::ilist<TransferFrameTC*>::iterator cur_frame = waitQueueFOP->begin();

	while (cur_frame != waitQueueFOP->end()) {
		(*cur_frame)->setConfSignal(FDURequestType::REQUEST_NEGATIVE_CONFIRM);
		waitQueueFOP->erase(cur_frame++);
	}
	ccsdsLogNotice(Tx, TypeFOPNotif, NO_FOP_EVENT);
	return FOPNotification::NO_FOP_EVENT;
}

FOPNotification FrameOperationProcedure::transmitAdFrame() {
	if (sentQueueFOP->full()) {
		ccsdsLogNotice(Tx, TypeFOPNotif, SENT_QUEUE_FULL);
		return FOPNotification::SENT_QUEUE_FULL;
	}

	if (sentQueueFOP->empty()) {
		transmissionCount = 1;
	}

	TransferFrameTC* adFrame = waitQueueFOP->front();
	if (waitQueueFOP->empty()) {
		ccsdsLogNotice(Tx, TypeFOPNotif, WAIT_QUEUE_EMPTY);
		return FOPNotification::WAIT_QUEUE_EMPTY;
	}

	adFrame->hdr.setTransferFrameSequenceNumber(transmitterFrameSeqNumber);
	if(!adFrame->isToBeRetransmitted()){
		transmitterFrameSeqNumber++;;
	}
	adFrame->setToBeRetransmitted(false);
	adFrame->setToProcessedByFOP();

	sentQueueFOP->push_back(adFrame);
	adOut = false;

	// TODO start the timer
	// pass the frame into the all frames generation service
	vchan->master_channel().storeOut(adFrame);
	waitQueueFOP->pop_front();
	ccsdsLogNotice(Tx, TypeFOPNotif, NO_FOP_EVENT);
	return FOPNotification::NO_FOP_EVENT;
}

FOPNotification FrameOperationProcedure::transmitBcFrame(TransferFrameTC* bc_frame) {
	bc_frame->setToBeRetransmitted(0);
	transmissionCount = 1;

	// TODO start the timer
	vchan->master_channel().storeOut(bc_frame);
	ccsdsLogNotice(Tx, TypeFOPNotif, NO_FOP_EVENT);
	return FOPNotification::NO_FOP_EVENT;
}

FOPNotification FrameOperationProcedure::transmitBdFrame(TransferFrameTC* bd_frame) {
	bdOut = NOT_READY;
	// Pass frame to all frames generation service
	vchan->master_channel().storeOut(bd_frame);
	ccsdsLogNotice(Tx, TypeFOPNotif, NO_FOP_EVENT);
	return FOPNotification::NO_FOP_EVENT;
}

void FrameOperationProcedure::initiateAdRetransmission() {
	// TODO generate an `abort` request to lower procedures
	transmissionCount = (transmissionCount == 255) ? 0 : transmissionCount + 1;
	// TODO start the timer

	for (TransferFrameTC* frame : *sentQueueFOP) {
		if (frame->getServiceType() == ServiceType::TYPE_AD) {
			frame->setToBeRetransmitted(true);
		}
	}
}

void FrameOperationProcedure::initiateBcRetransmission() {
	// TODO generate an `abort` request to lower procedures
	transmissionCount = (transmissionCount == 255) ? 0 : transmissionCount + 1;
	// TODO start the timer

	for (TransferFrameTC* frame : *sentQueueFOP) {
		if ((frame->getServiceType() == ServiceType::TYPE_BC) || (frame->getServiceType() == ServiceType::TYPE_BD)) {
			frame->setToBeRetransmitted(1);
		}
	}
}

void FrameOperationProcedure::acknowledgeFrame(uint8_t frameSeqNumber) {
	for (TransferFrameTC* frame : *sentQueueFOP) {
		if (frame->hdr.getTransferFrameSequenceNumber() == frameSeqNumber) {
			frame->setAcknowledgement(true);
		}
	}
}

void FrameOperationProcedure::removeAcknowledgedFrames() {
	etl::ilist<TransferFrameTC>::iterator cur_packet = vchan->master_channel().txMasterCopyTC.begin();

	while (cur_packet != vchan->master_channel().txMasterCopyTC.end()) {
		if ((*cur_packet).acknowledged()) {
            vchan->master_channel().masterChannelPoolTC.deletePacket(cur_packet->packetData(), cur_packet->hdr.transferFrameLength());
			vchan->master_channel().txMasterCopyTC.erase(cur_packet++);
		} else {
			++cur_packet;
		}
	}

	transmissionCount = 1;
}

void FrameOperationProcedure::lookForDirective() {
	if (bcOut == FlagState::READY) {
		for (TransferFrameTC* frame : *sentQueueFOP) {
			if (((frame->getServiceType() == ServiceType::TYPE_BC) ||
			     (frame->getServiceType() == ServiceType::TYPE_BD)) &&
			    frame->isToBeRetransmitted()) {
				bcOut = FlagState::NOT_READY;
				frame->setToBeRetransmitted(0);
			}
			// transmit_bc_frame();
		}
	} else {
		// @TODO? call look_for_fdu once bcOut is set to ready
	}
}

// TODO: Sent Queue as-is is pretty much tx
COPDirectiveResponse FrameOperationProcedure::pushSentQueue() {
	if (vchan->sentQueueTxTC.empty()) {
		ccsdsLogNotice(Tx, TypeCOPDirectiveResponse, REJECT);
		return COPDirectiveResponse::REJECT;
	}

	TransferFrameTC* pckt = sentQueueFOP->front();

	MasterChannelAlert err = vchan->master_channel().storeOut(pckt);

	if (err == MasterChannelAlert::NO_MC_ALERT) {
		// sentQueueTC->pop_front();
		ccsdsLogNotice(Tx, TypeCOPDirectiveResponse, ACCEPT);
		return COPDirectiveResponse::ACCEPT;
	}
	ccsdsLogNotice(Tx, TypeCOPDirectiveResponse, REJECT);
	return COPDirectiveResponse::REJECT;
}

COPDirectiveResponse FrameOperationProcedure::lookForFdu() {
	// If Ad Out Flag isn't set to ready, the process shall be aborted
	if (adOut == FlagState::READY) {
		// Check if some transmitted packet is set to be retransmitted
		for(TransferFrameTC* frame : *sentQueueFOP){
			if ((frame->getServiceType() == ServiceType::TYPE_AD)) {
				if (frame->isToBeRetransmitted()) {
					adOut = FlagState::NOT_READY;
					waitQueueFOP->push_front(frame);
					transmitAdFrame();
					return COPDirectiveResponse::ACCEPT;
				}
			}
		}
	}

	// Check if there is an AD frame in the wait queue such as V(S) < NN(R) + K
	if (waitQueueFOP->empty()) {
		return COPDirectiveResponse::REJECT;
	}
	TransferFrameTC* frame = waitQueueFOP->front();
	if ((frame->getServiceType() == ServiceType::TYPE_AD) &&
	    (frame->hdr.getTransferFrameSequenceNumber() < expectedAcknowledgementSeqNumber + fopSlidingWindow)) {
		transmitAdFrame();
		ccsdsLogNotice(Tx, TypeCOPDirectiveResponse, ACCEPT);
		return COPDirectiveResponse::ACCEPT;
	}

	ccsdsLogNotice(Tx, TypeCOPDirectiveResponse, REJECT);
	return COPDirectiveResponse::REJECT;
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
	CLCW clcw = vchan->currentlyProcessedCLCW.getClcw();

	if (clcw.getLockout() == 0) {
		if (clcw.getReportValue() == transmitterFrameSeqNumber) {
			if (clcw.getRetransmit() == 0) {
				if (clcw.getWait() == 0) {
					if (clcw.getReportValue() == expectedAcknowledgementSeqNumber) {
						// E1
						switch (state) {
							case FOPState::ACTIVE:
								break;
							case FOPState::RETRANSMIT_WITHOUT_WAIT:
							case FOPState::RETRANSMIT_WITH_WAIT:
								alert(AlertEvent::ALRT_SYNCH);
								state = FOPState::INITIAL;
								break;
							case FOPState::INITIALIZING_WITH_BC_FRAME:
								state = FOPState::ACTIVE;
								// cancel timer
								break;
							case FOPState::INITIALIZING_WITHOUT_BC_FRAME:
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
		} else if (clcw.getReportValue() < transmitterFrameSeqNumber &&
		           clcw.getReportValue() >= expectedAcknowledgementSeqNumber) {
			if (clcw.getRetransmit() == 0) {
				if (clcw.getWait() == 0) {
					if (expectedAcknowledgementSeqNumber == clcw.getReportValue()) {
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
					if (expectedAcknowledgementSeqNumber != clcw.getReportValue()) {
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
					if (expectedAcknowledgementSeqNumber != clcw.getReportValue()) {
						if (clcw.getWait() == 0) {
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
							if (clcw.getWait() == 0) {
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
							if (clcw.getWait() == 0) {
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

	ccsdsLogNotice(Tx, TypeCOPDirectiveResponse, ACCEPT);
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
	ccsdsLogNotice(Tx, TypeCOPDirectiveResponse, ACCEPT);
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
	ccsdsLogNotice(Tx, TypeFDURequestType, REQUEST_POSITIVE_CONFIRM);
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
	ccsdsLogNotice(Tx, TypeFDURequestType, REQUEST_POSITIVE_CONFIRM);
	return FDURequestType::REQUEST_POSITIVE_CONFIRM;
}

FDURequestType FrameOperationProcedure::terminateAdService() {
	// E29
	if (state != FOPState::INITIAL) {
		alert(AlertEvent::ALRT_TERM);
		state = FOPState::INITIAL;
	}
	ccsdsLogNotice(Tx, TypeFDURequestType, REQUEST_POSITIVE_CONFIRM);
	return FDURequestType::REQUEST_POSITIVE_CONFIRM;
}

FDURequestType FrameOperationProcedure::resumeAdService() {
	state = suspendState;
	ccsdsLogNotice(Tx, TypeFDURequestType, REQUEST_POSITIVE_CONFIRM);
	return FDURequestType::REQUEST_POSITIVE_CONFIRM;
}

COPDirectiveResponse FrameOperationProcedure::setVs(uint8_t vs) {
	// E35
	if (state == FOPState::INITIAL && suspendState == FOPState::INITIAL) {
		transmitterFrameSeqNumber = vs;
		expectedAcknowledgementSeqNumber = vs;
		ccsdsLogNotice(Tx, TypeCOPDirectiveResponse, ACCEPT);
		return COPDirectiveResponse::ACCEPT;
	} else {
		ccsdsLogNotice(Tx, TypeCOPDirectiveResponse, REJECT);
		return COPDirectiveResponse::REJECT;
	}
}

COPDirectiveResponse FrameOperationProcedure::setFopWidth(uint8_t width) {
	// E36
	fopSlidingWindow = width;
	ccsdsLogNotice(Tx, TypeCOPDirectiveResponse, ACCEPT);
	return COPDirectiveResponse::ACCEPT;
}

COPDirectiveResponse FrameOperationProcedure::setT1Initial(uint16_t t1_init) {
	// E37
	tiInitial = t1_init;
	ccsdsLogNotice(Tx, TypeCOPDirectiveResponse, ACCEPT);
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
	ccsdsLogNotice(Tx, TypeCOPDirectiveResponse, ACCEPT);
	return COPDirectiveResponse::ACCEPT;
}

COPDirectiveResponse FrameOperationProcedure::invalidDirective() {
	// E40
	ccsdsLogNotice(Tx, TypeCOPDirectiveResponse, REJECT);
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
	ccsdsLogNotice(Tx, TypeCOPDirectiveResponse, ACCEPT);
	return COPDirectiveResponse::ACCEPT;
}

void FrameOperationProcedure::bdReject() {
	alert(AlertEvent::ALRT_LLIF);
	state = FOPState::INITIAL;
}

COPDirectiveResponse FrameOperationProcedure::transferFdu() {
	TransferFrameTC* frame = vchan->txUnprocessedPacketListBufferTC.front();

	if (frame->hdr.bypassFlag() == 0) {
		if (frame->getServiceType() == ServiceType::TYPE_AD) {
			if (!waitQueueFOP->full()) {
				// E19
				if (state == FOPState::ACTIVE || state == FOPState::RETRANSMIT_WITHOUT_WAIT) {
					waitQueueFOP->push_back(frame);
					lookForFdu();
				} else if (state == FOPState::RETRANSMIT_WITH_WAIT) {
					waitQueueFOP->push_back(frame);
				} else {
					ccsdsLogNotice(Tx, TypeCOPDirectiveResponse, REJECT);
					return COPDirectiveResponse::REJECT;
				}
				ccsdsLogNotice(Tx, TypeCOPDirectiveResponse, ACCEPT);
				return COPDirectiveResponse::ACCEPT;
			} else {
				// E20
				ccsdsLogNotice(Tx, TypeCOPDirectiveResponse, REJECT);
				return COPDirectiveResponse::REJECT;
			}
		} else if ((frame->getServiceType() == ServiceType::TYPE_BC) ||
		           (frame->getServiceType() == ServiceType::TYPE_BD)) {
			if (bdOut == FlagState::READY) {
				//
				transmitBcFrame(frame);
				ccsdsLogNotice(Tx, TypeCOPDirectiveResponse, ACCEPT);
				return COPDirectiveResponse::ACCEPT;
			} else {
				// E22
				COPDirectiveResponse::REJECT;
			}
		}
	} else {
		// transfer directly to lower procedure

		MasterChannelAlert mc = vchan->master_channel().storeOut(frame);
		if (mc != MasterChannelAlert::NO_MC_ALERT) {
			ccsdsLogNotice(Tx, TypeCOPDirectiveResponse, REJECT);
			return COPDirectiveResponse::REJECT;
		}
	}
	return COPDirectiveResponse::ACCEPT;
}

void FrameOperationProcedure::acknowledgePreviousFrames(uint8_t frameSequenceNumber) {
	for (TransferFrameTC* frame : *sentQueueFOP) {
		if ((frame->hdr.getTransferFrameSequenceNumber() < frameSequenceNumber ||
		     frame->hdr.getTransferFrameSequenceNumber() > transmitterFrameSeqNumber)) {
			acknowledgeFrame(frame->hdr.getTransferFrameSequenceNumber());
		}
	}
	expectedAcknowledgementSeqNumber = frameSequenceNumber;
}