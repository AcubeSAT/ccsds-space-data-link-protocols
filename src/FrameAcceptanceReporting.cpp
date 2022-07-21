#include <FrameAcceptanceReporting.hpp>
#include "CCSDSLogger.h"

COPDirectiveResponse FrameAcceptanceReporting::frameArrives() {
	TransferFrameTC* frame = waitQueue->front();
    waitQueue->pop_front();

	if ((frame->getServiceType() == ServiceType::TYPE_AD) && (frame->transferFrameHeader().ctrlAndCmdFlag())) {
		if (frame->transferFrameSequenceNumber() == receiverFrameSeqNumber) {
			if (!sentQueue->full()) {
				// E1
				if (state == FARMState::OPEN) {
					sentQueue->push_back(frame);
					receiverFrameSeqNumber += 1;
					retransmit = FlagState::NOT_READY;
					ccsdsLogNotice(Tx, TypeCOPDirectiveResponse, ACCEPT);
					return COPDirectiveResponse::ACCEPT;
				} else {
					ccsdsLogNotice(Tx, TypeCOPDirectiveResponse, REJECT);
					return COPDirectiveResponse::REJECT;
				}
			} else {
				// E2
				if (state == FARMState::OPEN) {
					retransmit = FlagState::READY;
					wait = FlagState::READY;
					state = FARMState::WAIT;
				}
				ccsdsLogNotice(Tx, TypeCOPDirectiveResponse, REJECT);
				return COPDirectiveResponse::REJECT;
			}
		} else if ((frame->transferFrameSequenceNumber() > receiverFrameSeqNumber) &&
		           (frame->transferFrameSequenceNumber() <= receiverFrameSeqNumber + farmPositiveWinWidth - 1)) {
			// E3
			if (state == FARMState::OPEN) {
				retransmit = FlagState::READY;
			}
			ccsdsLogNotice(Tx, TypeCOPDirectiveResponse, REJECT);
			return COPDirectiveResponse::REJECT;
		} else if ((frame->transferFrameSequenceNumber() < receiverFrameSeqNumber) &&
		           (frame->transferFrameSequenceNumber() >= receiverFrameSeqNumber - farmNegativeWidth)) {
			// E4
			ccsdsLogNotice(Tx, TypeCOPDirectiveResponse, REJECT);
			return COPDirectiveResponse::REJECT;
		} else if ((frame->transferFrameSequenceNumber() > receiverFrameSeqNumber + farmPositiveWinWidth - 1) &&
		           (frame->transferFrameSequenceNumber() < (receiverFrameSeqNumber > farmNegativeWidth) ? receiverFrameSeqNumber - farmNegativeWidth : 256 - farmNegativeWidth)) {
			// E5
			state = FARMState::LOCKOUT;
            lockout = FlagState::READY;
			ccsdsLogNotice(Tx, TypeCOPDirectiveResponse, REJECT);
			return COPDirectiveResponse::REJECT;
		}
	} else if (((frame->getServiceType() == ServiceType::TYPE_BC) ||
	            (frame->getServiceType() == ServiceType::TYPE_BD)) &&
	           !frame->transferFrameHeader().ctrlAndCmdFlag()) {
		// E6
		farmBCount += 1;
		ccsdsLogNotice(Tx, TypeCOPDirectiveResponse, ACCEPT);
		return COPDirectiveResponse::ACCEPT;
	} else if (((frame->getServiceType() == ServiceType::TYPE_BC) ||
	            (frame->getServiceType() == ServiceType::TYPE_BD)) &&
	           frame->transferFrameHeader().ctrlAndCmdFlag()) {
		if (frame->controlWordType() == 0) {
			if (frame->packetPlData()[5] == 0) {
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
				ccsdsLogNotice(Tx, TypeCOPDirectiveResponse, ACCEPT);
				return COPDirectiveResponse::ACCEPT;
			} else if (frame->packetPlData()[5] == 130 && frame->packetPlData()[6] == 0) {
				// E8
				farmBCount += 1;
				retransmit = FlagState::NOT_READY;
				receiverFrameSeqNumber = frame->packetPlData()[6];
				ccsdsLogNotice(Tx, TypeCOPDirectiveResponse, ACCEPT);
				return COPDirectiveResponse::ACCEPT;
			}
		}
	}
	// Invalid Directive
	return COPDirectiveResponse::REJECT;
}

COPDirectiveResponse FrameAcceptanceReporting::bufferRelease() {
	if (state == FARMState::LOCKOUT) {
		state = FARMState::OPEN;
		wait = FlagState::NOT_READY;
	} else if (state == FARMState::WAIT) {
		wait = FlagState::NOT_READY;
	}
	return COPDirectiveResponse::ACCEPT;
}