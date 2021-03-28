#ifndef CCSDS_FOP_H
#define CCSDS_FOP_H

#pragma once

#include <cstdint>
#include <Packet.hpp>
#include <etl/list.h>
#include <Alert.hpp>

enum FOPState {
    ACTIVE = 1,
    RETRANSMIT_WITHOUT_WAIT = 2,
    RETRANSMIT_WITH_WAIT = 3,
    INITIALIZING_WITHOUT_BC_FRAME = 4,
    INITIALIZING_WITH_BC_FRAME = 5,
    INITIAL = 6
};

enum FlagState {
    NOT_READY = 0,
    READY = 1
};

enum Event {
    VALID_CLCW_RECEIVED = 0,
    INVALID_CLCW_RECEIVED = 1,
};

enum AlertEvent {
    ALRT_SYNCH = 0,
    ALRT_CLCW = 1,
    ALRT_LIMIT = 2,
    ALRT_TERM = 3,
};

class VirtualChannel;
class MAPChannel;

class FrameOperationProcedure {
private:
     etl::list<Packet, MAX_RECEIVED_TC_IN_WAIT_QUEUE> *waitQueue;
     etl::list<Packet, MAX_RECEIVED_TC_IN_SENT_QUEUE> *sentQueue;
    VirtualChannel *vchan;

    FOPState state;
    uint8_t transmitterFrameSeqNumber;
    bool adOut;
    bool bdOut;
    bool bcOut;
    uint8_t expectedAcknowledgementSeqNumber;
    uint16_t tiInitial;
    uint16_t transmissionLimit;
    uint16_t transmissionCount;
    uint8_t fopSlidingWindow;
    bool timeoutType;
    uint8_t suspendState;


    /**
     * @brief Purge the sent queue of the virtual channel and generate a response
     */
    FOPNotif purge_sent_queue();

    /**
     * @brief Purge the wait queue of the virtual channel and generate a response
     */
    FOPNotif purge_wait_queue();

    /**
     * @brief Prepares a Type-AD Frame for transmission
     */
    FOPNotif transmit_ad_frame(Packet ad_frame);

    /**
     * @brief Prepares a Type-BC Frame for transmission
     */
    FOPNotif transmit_bc_frame(Packet bc_frame);

    /**
     * @brief Prepares a Type-BD Frame for transmission
     */
    FOPNotif transmit_bd_frame(Packet bd_frame);

    /**
     * @brief Marks AD Frames stored in the sent queue to be retransmitted
     */
    void initiate_ad_retransmission();

    /**
    * @brief Marks BC Frames stored in the sent queue to be retransmitted
    */
    void initiate_bc_retransmission();

    /**
     * @brief Remove acknowledged frames from sent queue
     */
    void remove_acknowledged_frames();

    /**
     * @brief Search for directives in the sent queue and transmit any eligible frames
     */
    void look_for_directive();

    /**
     * @brief Search for a FDU that can be transmitted in the sent_queueu. If none are found also search in
     * the wait_queue
     */
    void look_for_fdu();

    void initialize();

    void alert(AlertEvent event);

    /**
     * @brief Process event where a valid CLCW arrives
     */
    void event_valid_clcw();

    /* Directives */
    FOPDirectiveResponse initiate_ad_no_clcw(Packet *ad_frame);

    FOPDirectiveResponse initiate_ad_clcw(Packet *ad_frame);

    FOPDirectiveResponse initiate_ad_unlock(Packet *ad_frame);

    FOPDirectiveResponse initiate_ad_vr(Packet *ad_frame);

    FOPDirectiveResponse terminate_ad_service(Packet *ad_frame);

    FOPDirectiveResponse resume_ad_service(Packet *ad_frame);

    FOPDirectiveResponse set_vs(Packet *ad_frame);

    FOPDirectiveResponse set_fop_width(Packet *ad_frame, uint8_t vr);

    FOPDirectiveResponse set_t1_initial(Packet *ad_frame);

    FOPDirectiveResponse set_transmission_limit(Packet *ad_frame, uint8_t vr);

    FOPDirectiveResponse set_timeout_type(Packet *ad_frame, bool vr);

    FOPDirectiveResponse invalid_directive(Packet *ad_frame);

protected:
    /**
     * @brief Process events, take the corresponding actions and change the state of the State Machine
     */
    void process_event(Event event);

public:
    FrameOperationProcedure(VirtualChannel *vchan,
            etl::list<Packet, MAX_RECEIVED_TC_IN_WAIT_QUEUE> *waitQueue,
            etl::list<Packet, MAX_RECEIVED_TC_IN_SENT_QUEUE> *sentQueue,
            const uint8_t repetition_cop_ctrl) :
            waitQueue(waitQueue), sentQueue(sentQueue), state(FOPState::INITIAL), transmitterFrameSeqNumber(0),
            adOut(0), bdOut(0), bcOut(0), expectedAcknowledgementSeqNumber(0),
            tiInitial(FOP_TIMER_INITIAL), transmissionLimit(repetition_cop_ctrl), transmissionCount(1),
            fopSlidingWindow(FOP_SLIDING_WINDOW_INITIAL), timeoutType(0), suspendState(0) {};
};

#endif //CCSDS_FOP_H
