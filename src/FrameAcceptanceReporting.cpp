#include <FrameAcceptanceReporting.hpp>
#include "CCSDS_Log.h"

COPDirectiveResponse FrameAcceptanceReporting::frameArrives() {
    TransferFrameTC* frame = waitQueue->front();

    if (frame->getServiceType() == ServiceType::TYPE_AD && frame->transferFrameHeader().ctrlAndCmdFlag()) {
        if (frame->transferFrameSequenceNumber() == receiverFrameSeqNumber) {
            if (!sentQueue->empty()) {
                // E1
                if (state == FARMState::OPEN) {
                    sentQueue->push_back(frame);
                    sentQueue->pop_front();
                    receiverFrameSeqNumber += 1;
                    retransmit = FlagState::NOT_READY;
                    ccsdsLog(Tx, TypeCOPDirectiveResponse, ACCEPT);
                    return COPDirectiveResponse::ACCEPT;
                } else {
                    ccsdsLog(Tx, TypeCOPDirectiveResponse, REJECT);
                    return COPDirectiveResponse::REJECT;
                }
            } else {
                // E2
                if (state == FARMState::OPEN) {
                    retransmit = FlagState::READY;
                    wait = FlagState::READY;
                    state = FARMState::WAIT;
                }
                ccsdsLog(Tx, TypeCOPDirectiveResponse, REJECT);
                return COPDirectiveResponse::REJECT;
            }
        } else if ((frame->transferFrameSequenceNumber() > receiverFrameSeqNumber) &&
                   (frame->transferFrameSequenceNumber() <= receiverFrameSeqNumber + farmPositiveWinWidth - 1)) {
            // E3
            if (state == FARMState::OPEN) {
                retransmit = FlagState::READY;
            }
            ccsdsLog(Tx, TypeCOPDirectiveResponse, REJECT);
            return COPDirectiveResponse::REJECT;
        } else if ((frame->transferFrameSequenceNumber() < receiverFrameSeqNumber) &&
                   (frame->transferFrameSequenceNumber() >= receiverFrameSeqNumber - farmNegativeWidth)) {
            // E4
            ccsdsLog(Tx, TypeCOPDirectiveResponse, REJECT);
            return COPDirectiveResponse::REJECT;
        } else if ((frame->transferFrameSequenceNumber() > receiverFrameSeqNumber + farmPositiveWinWidth - 1) &&
                   (frame->transferFrameSequenceNumber() < farmPositiveWinWidth - farmNegativeWidth)) {
            // E5
            state = FARMState::LOCKOUT;
            ccsdsLog(Tx, TypeCOPDirectiveResponse, REJECT);
            return COPDirectiveResponse::REJECT;
        }
    } else if (((frame->getServiceType() == ServiceType::TYPE_BC) || (frame->getServiceType() == ServiceType::TYPE_BD)) &&
                !frame->transferFrameHeader().ctrlAndCmdFlag()) {
        // E6
        farmBCount += 1;
        ccsdsLog(Tx, TypeCOPDirectiveResponse, ACCEPT);
        return COPDirectiveResponse::ACCEPT;
    } else if (((frame->getServiceType() == ServiceType::TYPE_BC) || (frame->getServiceType() == ServiceType::TYPE_BD)) &&
	           frame->transferFrameHeader().ctrlAndCmdFlag()) {
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
                ccsdsLog(Tx, TypeCOPDirectiveResponse, ACCEPT);
                return COPDirectiveResponse::ACCEPT;
            } else if (frame->packetPlData()[4] == 130 && frame->packetPlData()[5] == 0) {
                // E8
                farmBCount += 1;
                retransmit = FlagState::NOT_READY;
                receiverFrameSeqNumber = frame->packetPlData()[6];
                ccsdsLog(Tx, TypeCOPDirectiveResponse, ACCEPT);
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