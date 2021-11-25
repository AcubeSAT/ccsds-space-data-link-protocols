#include <FarmeAcceptanceReporting.hpp>
#include "CCSDS_Log.h"

COPDirectiveResponse FarmAcceptanceReporting::frame_arrives() {
    PacketTC *frame = waitQueue->front();

    if (frame->service_type() == ServiceType::TYPE_A && frame->transfer_frame_header().ctrl_and_cmd_flag()) {
        if (frame->transfer_frame_sequence_number() == receiverFrameSeqNumber) {
            if (!sentQueue->empty()) {
                // E1
                if (state == FARMState::OPEN) {
                    sentQueue->push_back(frame);
                    sentQueue->pop_front();
                    receiverFrameSeqNumber += 1;
                    retransmit = FlagState::NOT_READY;
					ccsds_log(Tx, TypeCOPDirectiveResponse, ACCEPT);
                    return COPDirectiveResponse::ACCEPT;
                } else {
					ccsds_log(Tx, TypeCOPDirectiveResponse, REJECT);
                    return COPDirectiveResponse::REJECT;
                }
            } else {
                // E2
                if (state == FARMState::OPEN) {
                    retransmit = FlagState::READY;
                    wait = FlagState::READY;
                    state = FARMState::WAIT;
                }
				ccsds_log(Tx, TypeCOPDirectiveResponse, REJECT);
                return COPDirectiveResponse::REJECT;
            }
        } else if ((frame->transfer_frame_sequence_number() > receiverFrameSeqNumber) &&
                   (frame->transfer_frame_sequence_number() <= receiverFrameSeqNumber + farmPositiveWinWidth - 1)) {
            // E3
            if (state == FARMState::OPEN) {
                retransmit = FlagState::READY;
            }
			ccsds_log(Tx, TypeCOPDirectiveResponse, REJECT);
            return COPDirectiveResponse::REJECT;
        } else if ((frame->transfer_frame_sequence_number() < receiverFrameSeqNumber) &&
                   (frame->transfer_frame_sequence_number() >= receiverFrameSeqNumber - farmNegativeWidth)) {
            // E4
			ccsds_log(Tx, TypeCOPDirectiveResponse, REJECT);
            return COPDirectiveResponse::REJECT;
        } else if ((frame->transfer_frame_sequence_number() > receiverFrameSeqNumber + farmPositiveWinWidth - 1) &&
                   (frame->transfer_frame_sequence_number() < farmPositiveWinWidth - farmNegativeWidth)) {
            // E5
            state = FARMState::LOCKOUT;
			ccsds_log(Tx, TypeCOPDirectiveResponse, REJECT);
            return COPDirectiveResponse::REJECT;
        }
    } else if (frame->service_type() == ServiceType::TYPE_B && !frame->transfer_frame_header().ctrl_and_cmd_flag()) {
        // E6
        farmBCount += 1;
		ccsds_log(Tx, TypeCOPDirectiveResponse, ACCEPT);
        return COPDirectiveResponse::ACCEPT;
    } else if (frame->service_type() == ServiceType::TYPE_B && frame->transfer_frame_header().ctrl_and_cmd_flag()) {
        if (frame->control_word_type() == 0) {
            if (frame->packet_pl_data()[4] == 0) {
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
				ccsds_log(Tx, TypeCOPDirectiveResponse, ACCEPT);
                return COPDirectiveResponse::ACCEPT;
            } else if (frame->packet_pl_data()[4] == 130 && frame->packet_pl_data()[5] == 0) {
                // E8
                farmBCount += 1;
                retransmit = FlagState::NOT_READY;
                receiverFrameSeqNumber = frame->packet_pl_data()[6];
				ccsds_log(Tx, TypeCOPDirectiveResponse, ACCEPT);
                return COPDirectiveResponse::ACCEPT;
            }
        }
    }
}

COPDirectiveResponse FarmAcceptanceReporting::buffer_release() {
    if (state == FARMState::LOCKOUT) {
        state = FARMState::OPEN;
        wait = FlagState::NOT_READY;
    } else if (state == FARMState::WAIT) {
        wait = FlagState::NOT_READY;
    }
}