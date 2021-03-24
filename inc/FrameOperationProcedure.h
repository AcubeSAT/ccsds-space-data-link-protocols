#ifndef CCSDS_FOP_H
#define CCSDS_FOP_H

#include <CCSDSChannel.hpp>

enum FOPState{
    ACTIVE = 1,
    RETRANSMIT_WITHOUT_WAIT = 2,
    RETRANSMIT_WITH_WAIT = 3,
    INITIALIZING_WITHOUT_BC_FRAME = 4,
    INITIALIZING_WITH_BC_FRAME = 5,
    INITIAL = 6
};

class FrameOperationProcedure{
private:
    FOPState state;
    uint8_t transmitterFrameSeqNumber;
    bool toBeRetransmitted;
    bool adOut;
    bool bdOut;
    bool bcOut;
    uint8_t expectedAcknowledgementSeqNumber;
    uint16_t tiInitial;
    uint16_t transmissionLimit;
    uint16_t transmissionCount;
    uint8_t fopSlidingWindow;
    uint8_t timeoutState;
    uint8_t suspendState;

};
#endif //CCSDS_FOP_H
