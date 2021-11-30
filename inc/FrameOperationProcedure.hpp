#ifndef CCSDS_FOP_H
#define CCSDS_FOP_H

#pragma once

#include <cstdint>
#include <PacketTC.hpp>
#include <etl/list.h>
#include <Alert.hpp>

/**
 *@see p. 5.1.2 from COP-1 CCSDS
 */
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

/**
 * @see p.4.3.2 from COP-1 CCSDS
 */
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
class NasterChannel;
class MAPChannel;

class FrameOperationProcedure {
    friend class ServiceChannel;
    friend class MasterChannel;

public:
	/**
	 * @brief TC Packets stored in list, before being processed by the FOP service
	 * @see p. 5.1.4 from COP-1 CCSDS
	 */
    etl::list<PacketTC*, MAX_RECEIVED_TX_TC_IN_WAIT_QUEUE> *waitQueue;
	/**
	 * @brief TC Packets stored in list, after being processed by the FOP service
	 * @see p. 5.1.7 from COP-1 CCSDS
	 */
    etl::list<PacketTC*, MAX_RECEIVED_TX_TC_IN_SENT_QUEUE> *sentQueue;
    VirtualChannel *vchan;

private:
	/**
	 * @brief This  variable  represents  the  state  of  FOP-1  for  the  specific  Virtual  Channel.
	 * @see p. 5.1.2 from COP-1 CCSDS
	 */
    FOPState state;
	/**
	 * @brief It records the state that FOP-1 was in when the AD Service was suspended (as described in 5.1.10).
	 * This is the state to which FOP-1 will return should the AD Service be resumed.
	 * @see p. 5.1.11 from COP-1 CCSDS
	 */
    FOPState suspendState;

	/**
	 * @brief It contains the value of the Frame Sequence Number to be put in the Transfer Frame Primary Header of
	 * the  next  Type-AD Transfer Frame to be transmitted.
	 * @see p. 5.1.3 from COP-1 CCSDS
	 */
    uint8_t transmitterFrameSeqNumber;
	/**
	 * @see p. 5.1.6 from COP-1 CCSDS
	 */
    bool adOut;
	/**
	 * @see p. 5.1.6 from COP-1 CCSDS
	 */
    bool bdOut;
	/**
	 * @see p. 5.1.6 from COP-1 CCSDS
	 */
    bool bcOut;
	/**
	 * @see p. 5.1.8 from COP-1 CCSDS
	 */
    uint8_t expectedAcknowledgementSeqNumber;
	/**
	 * @brief Timer
	 * @see p. 5.1.9 from COP-1 CCSDS
	 */
    uint16_t tiInitial;
	/**
	 * @brief The  Transmission Limit  holds  a  value  which  represents  the  maximum  number  of  times  the  first
	 * Transfer  Frame  on  the  Sent_Queue  may  be  transmitted
	 * @see p. 5.1.10.2 from COP-1 CCSDS
	 */
    uint16_t transmissionLimit;
	/**
	 * @brief The  Transmission Count  variable  is  used  to  count  the  number  of  transmissions  of  the  first
	 * Transfer  Frame  on  the  Sent_Queue
	 * @see p. 5.1.10.4 from COP-1 CCSDS
	 */
    uint16_t transmissionCount;
	/**
	 * @brief The FOP Sliding Window is a mechanism which limits the number of Transfer Frames which can  be
	 * transmitted  ahead  of  the  last  acknowledged  Transfer  Frame
	 * @see p. 5.1.12 from COP-1 CCSDS
	 */
    uint8_t fopSlidingWindow;
	/**
	 * @brief It specifies the action to be performed when both the Timer expires and the Transmission
	 * Count (see 5.1.10.4) has reached the Transmission_Limit.
	 * @see p. 5.1.10.3 from COP-1 CCSDS
	 */
    bool timeoutType;
    /**
     * @brief Purge the sent queue of the virtual channel and generate a response
     * @see p. 5.2.2 from COP-1 CCSDS
     */
	FOPNotification purge_sent_queue();

    /**
     * @brief Purge the wait queue of the virtual channel and generate a response
     * @see p. 5.2.3 from COP-1 CCSDS
     */
	FOPNotification purge_wait_queue();

    /**
     * @brief Prepares a Type-AD Frame for transmission
     * @see p. 5.2.4 from COP-1 CCSDS
     */
	FOPNotification transmit_ad_frame();

    /**
     * @brief Prepares a Type-BC Frame for transmission
     * @see p. 5.2.5 from COP-1 CCSDS
     */
	FOPNotification transmit_bc_frame(PacketTC*bc_frame);

    /**
     * @brief Prepares a Type-BD Frame for transmission
     * @see p. 5.2.6 from COP-1 CCSDS
     */
	FOPNotification transmit_bd_frame(PacketTC*bd_frame);

    /**
     * @brief Marks AD Frames stored in the sent queue to be retransmitted
     * @see p. 5.2.7 from COP-1 CCSDS
     */
    void initiate_ad_retransmission();

    /**
     * @brief Marks BC Frames stored in the sent queue to be retransmitted
     * @see p. 5.2.7 from COP-1 CCSDS
     */
    void initiate_bc_retransmission();

    /**
     * @brief Remove acknowledged frames from sent queue
     * @see p. 5.2.8 from COP-1 CCSDS
     */
    void remove_acknowledged_frames();

    /**
     * @brief Search for directives in the sent queue and transmit any eligible frames
     * @see p. 5.2.9 from COP-1 CCSDS
     */
    void look_for_directive();

	/**
	 *
	 * @return 
	 */
    COPDirectiveResponse push_sent_queue();
    /**
     * @brief Search for a FDU that can be transmitted in the sent_queue. If none are found also search in
     * the wait_queue
     * @see p. 5.2.10 from COP-1 CCSDS
     */
    COPDirectiveResponse look_for_fdu();

	/**
	 * @see p. 5.2.14 from COP-1 CCSDS
	 */
    void initialize();

	/**
	 * @see p. 5.2.15 from COP-1 CCSDS
	 */
    void alert(AlertEvent event);

    /* CLCW arrival*/

    /**
     * @brief Process event where a valid CLCW arrives
     * @see Table 5-1 from COP-1 CCSDS (E1 - E14)
     */
    COPDirectiveResponse valid_clcw_arrival();

    // TODO: Check for invalid CLCW
    /**
     * @brief Process invalid CLCW arrival
     * @see Table 5-1 from COP-1 CCSDS (E15)
     */
    void invalid_clcw_arrival();

    void acknowledge_frame(uint8_t frame_seq_num);

    /* Directives */
	/**
	 * @see Table 5-1 from COP-1 CCSDS
	 */

	//E23 (Table 5-1 from COP-1 CCSDS)
    FDURequestType initiate_ad_no_clcw();
	//E24 (Table 5-1 from COP-1 CCSDS)
    FDURequestType initiate_ad_clcw();
	//E25-E26 (Table 5-1 from COP-1 CCSDS)
    FDURequestType initiate_ad_unlock();
	//E27-E28 (Table 5-1 from COP-1 CCSDS)
    FDURequestType initiate_ad_vr(uint8_t vr);
	//E29 (Table 5-1 from COP-1 CCSDS)
    FDURequestType terminate_ad_service();
	//E30 -E34 (Table 5-1 from COP-1 CCSDS)
    FDURequestType resume_ad_service();
	//E35 (Table 5-1 from COP-1 CCSDS)
    COPDirectiveResponse set_vs(uint8_t vs);
	//E36 (Table 5-1 from COP-1 CCSDS)
    COPDirectiveResponse set_fop_width(uint8_t width);
	//E37 (Table 5-1 from COP-1 CCSDS)
    COPDirectiveResponse set_t1_initial(uint16_t t1_init);
	//E38 (Table 5-1 from COP-1 CCSDS)
    COPDirectiveResponse set_transmission_limit(uint8_t vr);
	//E39 (Table 5-1 from COP-1 CCSDS)
    COPDirectiveResponse set_timeout_type(bool vr);
	//E40 (Table 5-1 from COP-1 CCSDS)
    COPDirectiveResponse invalid_directive();

    /** Response from lower procedures*/
	//E41 (Table 5-1 Page 5 - 22 COP-1 CCSDS)
    void ad_accept();
	//E42 (Table 5-1 Page 5 - 22 COP-1 CCSDS)
    void ad_reject();
	//E43 (Table 5-1 Page 5 - 22 COP-1 CCSDS)
    void bc_accept();
	//E44 (Table 5-1 Page 5 - 22 COP-1 CCSDS)
    void bc_reject();
	//E45 (Table 5-1 Page 5 - 22 COP-1 CCSDS)
    COPDirectiveResponse bd_accept();
	//E46 (Table 5-1 Page 5 - 22 COP-1 CCSDS)
    void bd_reject();

	/**
	 * E19 - E22 Table 5-1 COP-1 CCSDS
	 * @see p. 3.2.2.3.2 from COP-1 CCSDS
	 */
    COPDirectiveResponse transfer_fdu();

public:
    FrameOperationProcedure(VirtualChannel *vchan, etl::list<PacketTC*, MAX_RECEIVED_TX_TC_IN_WAIT_QUEUE> *waitQueue,
                            etl::list<PacketTC*, MAX_RECEIVED_TX_TC_IN_SENT_QUEUE> *sentQueue,
                            const uint8_t repetition_cop_ctrl)
            : waitQueue(waitQueue), sentQueue(sentQueue), vchan(vchan), state(FOPState::INITIAL),
              suspendState(FOPState::INITIAL), transmitterFrameSeqNumber(0), adOut(FlagState::READY), bdOut(FlagState::READY),
              bcOut(FlagState::READY), expectedAcknowledgementSeqNumber(0), tiInitial(FOP_TIMER_INITIAL),
              transmissionLimit(repetition_cop_ctrl), transmissionCount(1), fopSlidingWindow(FOP_SLIDING_WINDOW_INITIAL),
              timeoutType(false) {};
};

#endif // CCSDS_FOP_H
