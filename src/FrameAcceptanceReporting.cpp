#include <FarmeAcceptanceReporting.h>

COPDirectiveResponse FarmAcceptanceReporting::ad_frame_arrives() {
    Packet *frame = waitQueue->front();

    if (frame->service_type() == ServiceType::TYPE_A && frame->transfer_frame_header().ctrl_and_cmd_flag()) {
        if (frame->transfer_frame_sequence_number() == receiver_frame_seq_number) {
            if (!sentQueue->empty()) {
                // E1
                if (state == FARMState::OPEN) {
                    sentQueue->push_back(frame);
                    sentQueue->pop_front();
                    receiver_frame_seq_number += 1;
                    retransmit = FlagState::NOT_READY;
                    return COPDirectiveResponse::ACCEPT;
                } else {
                    return COPDirectiveResponse::REJECT;
                }
            } else {
                // E2
                if (state == FARMState::OPEN) {
                    retransmit = FlagState::READY;
                    wait = FlagState::READY;
                    state = FARMState::WAIT;
                }
                return COPDirectiveResponse::REJECT;
            }
        } else if ((frame->transfer_frame_sequence_number() > receiver_frame_seq_number) &&
                   (frame->transfer_frame_sequence_number() <= receiver_frame_seq_number + farmPositiveWinWidth - 1)) {
            // E3
            if (state == FARMState::OPEN) {
                retransmit = FlagState::READY;
            }
            return COPDirectiveResponse::REJECT;
        } else if ((frame->transfer_frame_sequence_number() < receiver_frame_seq_number) &&
                   (frame->transfer_frame_sequence_number() >= receiver_frame_seq_number - farmNegativeWidth)) {
            // E4
            return COPDirectiveResponse::REJECT;
        } else if ((frame->transfer_frame_sequence_number() > receiver_frame_seq_number + farmPositiveWinWidth - 1) &&
                   (frame->transfer_frame_sequence_number() < farmPositiveWinWidth - farmNegativeWidth)) {
            // E5
            state = FARMState::LOCKOUT;
            return COPDirectiveResponse::REJECT;
        }
    }
}

