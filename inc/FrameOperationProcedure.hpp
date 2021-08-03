#ifndef CCSDS_FOP_H
#define CCSDS_FOP_H

#pragma once

#include <cstdint>
#include <PacketTC.hpp>
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

enum Event {
    VALID_CLCW_RECEIVED = 0,
    INVALID_CLCW_RECEIVED = 1,
};

enum AlertEvent {
    ALRT_SYNCH = 0,
    ALRT_CLCW = 1,
    ALRT_LIMIT = 2,
    ALRT_TERM = 3,
    ALRT_LLIF = 4,
    ALRT_NNR = 5,
    ALRT_LOCKOUT = 6,
};

class VirtualChannel;

class MAPChannel;

class FrameOperationProcedure {
    friend class ServiceChannel;

public:
    etl::list<PacketTC *, max_received_tx_tc_in_wait_queue> *waitQueue;
    etl::list<PacketTC *, max_received_tx_tc_in_sent_queue> *sentQueue;
    VirtualChannel *vchan;

private:
    FOPState state;
    FOPState suspendState;

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
    FOPNotif transmit_ad_frame();

    /**
     * @brief Prepares a Type-BC Frame for transmission
     */
    FOPNotif transmit_bc_frame(PacketTC *bc_frame);

    /**
     * @brief Prepares a Type-BD Frame for transmission
     */
    FOPNotif transmit_bd_frame(PacketTC *bd_frame);

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

    COPDirectiveResponse push_sent_queue();

    /**
     * @brief Search for a FDU that can be transmitted in the sent_queueu. If none are found also search in
     * the wait_queue
     */
    COPDirectiveResponse look_for_fdu();

    void initialize();

    void alert(AlertEvent event);

    /* CLCW arrival*/

    /**
     * @brief Process event where a valid CLCW arrives
     */
    COPDirectiveResponse valid_clcw_arrival();

    // TODO: Check for invalid CLCW
    /**
     * @brief Process invalid CLCW arrival
     */
    void invalid_clcw_arrival();

    /* Directives */
    FDURequestType initiate_ad_no_clcw();

    FDURequestType initiate_ad_clcw();

    FDURequestType initiate_ad_unlock();

    FDURequestType initiate_ad_vr(uint8_t vr);

    FDURequestType terminate_ad_service();

    FDURequestType resume_ad_service();

    COPDirectiveResponse set_vs(uint8_t vs);

    COPDirectiveResponse set_fop_width(uint8_t width);

    COPDirectiveResponse set_t1_initial(uint16_t t1_init);

    COPDirectiveResponse set_transmission_limit(uint8_t vr);

    COPDirectiveResponse set_timeout_type(bool vr);

    COPDirectiveResponse invalid_directive();

    /* Response from lower procedures*/
    void ad_accept();

    void ad_reject();

    void bc_accept();

    void bc_reject();

    COPDirectiveResponse bd_accept();

    void bd_reject();

    COPDirectiveResponse transfer_fdu();

public:
    FrameOperationProcedure(VirtualChannel *vchan, etl::list<PacketTC *, max_received_tx_tc_in_wait_queue> *waitQueue,
                            etl::list<PacketTC *, max_received_tx_tc_in_sent_queue> *sentQueue,
                            const uint8_t repetition_cop_ctrl)
            : waitQueue(waitQueue), sentQueue(sentQueue), state(FOPState::INITIAL), transmitterFrameSeqNumber(0),
              vchan(vchan), adOut(FlagState::READY), bdOut(FlagState::READY), bcOut(FlagState::READY),
              expectedAcknowledgementSeqNumber(0), tiInitial(fop_timer_initial), transmissionLimit(repetition_cop_ctrl),
              transmissionCount(1), fopSlidingWindow(fop_sliding_window_initial), timeoutType(0),
              suspendState(FOPState::INITIAL) {};
};

#endif // CCSDS_FOP_H
