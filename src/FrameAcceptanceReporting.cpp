#include <FrameAcceptanceReporting.hpp>
#include "CCSDS_Log.h"

COPDirectiveResponse FrameAcceptanceReporting::frameArrives() {
	TransferFrameTC* frame = waitQueue->front();

	if (frame->getServiceType() == ServiceType::TYPE_A && frame->transferFrameHeader().ctrlAndCmdFlag()) {
		if (frame->transferFrameSequenceNumber() == receiverFrameSeqNumber) {
			if (!sentQueue->full()) {
				// E1
				if (state == FARMState::OPEN) {
					sentQueue->push_back(frame);
					waitQueue->pop_front();
					receiverFrameSeqNumber += 1;
					retransmit = FlagState::NOT_READY;
					ccsdsLog(Rx, TypeCOPDirectiveResponse, ACCEPT);
					return COPDirectiveResponse::ACCEPT;
				} else {
					ccsdsLog(Rx, TypeCOPDirectiveResponse, REJECT);
					return COPDirectiveResponse::REJECT;
				}
			} else {
				// E2
				if (state == FARMState::OPEN) {
					retransmit = FlagState::READY;
					wait = FlagState::READY;
					state = FARMState::WAIT;
				}
				ccsdsLog(Rx, TypeCOPDirectiveResponse, REJECT);
				return COPDirectiveResponse::REJECT;
			}
		} else if ((frame->transferFrameSequenceNumber() > receiverFrameSeqNumber) &&
		           (frame->transferFrameSequenceNumber() <= receiverFrameSeqNumber + farmPositiveWinWidth - 1)) {
			// E3
			if (state == FARMState::OPEN) {
				retransmit = FlagState::READY;
			}
			ccsdsLog(Rx, TypeCOPDirectiveResponse, REJECT);
			return COPDirectiveResponse::REJECT;
		} else if ((frame->transferFrameSequenceNumber() < receiverFrameSeqNumber) &&
		           (frame->transferFrameSequenceNumber() >= receiverFrameSeqNumber - farmNegativeWidth)) {
			// E4
			ccsdsLog(Rx, TypeCOPDirectiveResponse, REJECT);
			return COPDirectiveResponse::REJECT;
		} else if ((frame->transferFrameSequenceNumber() > receiverFrameSeqNumber + farmPositiveWinWidth - 1) &&
		           (frame->transferFrameSequenceNumber() < receiverFrameSeqNumber - farmNegativeWidth)) {
			// E5
			if (state == FARMState::OPEN || state == FARMState::WAIT){
				lockout = FlagState::READY;
			}
			state = FARMState::LOCKOUT;
			ccsdsLog(Rx, TypeCOPDirectiveResponse, REJECT);
			return COPDirectiveResponse::REJECT;
		}
	} else if (frame->getServiceType() == ServiceType::TYPE_B && !frame->transferFrameHeader().ctrlAndCmdFlag()) {
		// E6
        sentQueue->push_back(frame);
		// TODO: This is not exactly accurate, BD frames should be passed to a separate queue and
		//  directly processed from higher procedures
        waitQueue->pop_front();
		farmBCount += 1;
		ccsdsLog(Rx, TypeCOPDirectiveResponse, ACCEPT);
		return COPDirectiveResponse::ACCEPT;
	} else if (frame->getServiceType() == ServiceType::TYPE_B && frame->transferFrameHeader().ctrlAndCmdFlag()) {
        // TODO: Define what user data BD frames will include and how they're identified (E6 state number)
		// TODO: Those depend on the pending CLCW implementation
		if (frame->controlWordType() == 0) {
			if (frame->packetPlData()[4] == 0) {
				// E7
				farmBCount += 1;
				retransmit = FlagState::NOT_READY;

				if (state == FARMState::WAIT) {
					wait = FlagState::NOT_READY;
				} else if (state == FARMState::LOCKOUT) {
					wait = FlagState::NOT_READY;
					lockout = FlagState::NOT_READY;
				}
				state = FARMState::OPEN;
				ccsdsLog(Rx, TypeCOPDirectiveResponse, ACCEPT);
				return COPDirectiveResponse::ACCEPT;
			} else if (frame->packetPlData()[4] == 130 && frame->packetPlData()[5] == 0) {
				// E8
				farmBCount += 1;
				retransmit = FlagState::NOT_READY;
				receiverFrameSeqNumber = frame->packetPlData()[6];
				ccsdsLog(Rx, TypeCOPDirectiveResponse, ACCEPT);
				return COPDirectiveResponse::ACCEPT;
			}
		}
	}
	// Invalid Directive
    ccsdsLog(Rx, TypeCOPDirectiveResponse, REJECT);
	return COPDirectiveResponse::REJECT;
}

COPDirectiveResponse FrameAcceptanceReporting::bufferRelease() {
	if (state == FARMState::LOCKOUT) {
		state = FARMState::OPEN;
		wait = FlagState::NOT_READY;
	} else if (state == FARMState::WAIT) {
		wait = FlagState::NOT_READY;
	}
	// TODO: Will this use the logger reporting mechanism. If so, logger would need to also give the option
	//  to accept a simple string stream
	return COPDirectiveResponse::ACCEPT;
}