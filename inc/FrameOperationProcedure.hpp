#pragma once

#include <cstdint>
#include <TransferFrameTC.hpp>
#include <etl/list.h>
#include <Alert.hpp>
#include <CCSDSChannel.hpp>

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

template <uint16_t MAP_T, uint16_t VC_T, class MC>
class VirtualChannel;

template <uint16_t MAP_T>
class MAPChannel;

template <uint16_t MAP_T, uint16_t VC_T, uint16_t MC_T>
class MasterChannelChannel;

template <uint16_t MAP_T, uint16_t VC_T, class MC>
class FrameOperationProcedure {

    template <uint16_t, uint16_t, uint16_t> friend class ServiceChannel;

	template <uint16_t, uint16_t, uint16_t> friend class MasterChannel;

public:
	/**
	 * TC Packets stored in list, before being processed by the FOP service
	 * @see p. 5.1.4 from COP-1 CCSDS
	 */
	etl::list<TransferFrameTC*, VC_T>* waitQueueFOP;
	/**
	 * TC Packets stored in list, after being processed by the FOP service
	 * @see p. 5.1.7 from COP-1 CCSDS
	 */
	etl::list<TransferFrameTC*, VC_T>* sentQueueFOP;

	/**
	 * TC Packets stored in list, before being processed by the FOP service
	 * @see p. 5.1.4 from COP-1 CCSDS
	 */
	etl::list<TransferFrameTC*, VC_T>* waitQueueFARM;
	/**
	 * TC Packets stored in list, after being processed by the FOP service
	 * @see p. 5.1.7 from COP-1 CCSDS
	 */
	etl::list<TransferFrameTC*, VC_T>* sentQueueFARM;

	VirtualChannel<MAP_T, VC_T, MC>* vchan;

private:
	/**
	 * This  variable  represents  the  state  of  FOP-1  for  the  specific  Virtual  Channel.
	 * @see p. 5.1.2 from COP-1 CCSDS
	 */
	FOPState state;
	/**
	 * It records the state that FOP-1 was in when the AD Service was suspended (as described in 5.1.10).
	 * This is the state to which FOP-1 will return should the AD Service be resumed.
	 * @see p. 5.1.11 from COP-1 CCSDS
	 */
	FOPState suspendState;

	/**
	 * It contains the value of the Frame Sequence Number to be put in the Transfer Frame Primary Header of
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
	 * Timer
	 * @see p. 5.1.9 from COP-1 CCSDS
	 */
	uint16_t tiInitial;
	/**
	 * The  Transmission Limit  holds  a  value  which  represents  the  maximum  number  of  times  the  first
	 * Transfer  Frame  on  the  Sent_Queue  may  be  transmitted
	 * @see p. 5.1.10.2 from COP-1 CCSDS
	 */
	uint16_t transmissionLimit;
	/**
	 * The  Transmission Count  variable  is  used  to  count  the  number  of  transmissions  of  the  first
	 * Transfer  Frame  on  the  Sent_Queue
	 * @see p. 5.1.10.4 from COP-1 CCSDS
	 */
	uint16_t transmissionCount;
	/**
	 * The FOP Sliding Window is a mechanism which limits the number of Transfer Frames which can  be
	 * transmitted  ahead  of  the  last  acknowledged  Transfer  Frame
	 * @see p. 5.1.12 from COP-1 CCSDS
	 */
	uint8_t fopSlidingWindow;
	/**
	 * It specifies the action to be performed when both the Timer expires and the Transmission
	 * Count (see 5.1.10.4) has reached the Transmission_Limit.
	 * @see p. 5.1.10.3 from COP-1 CCSDS
	 */
	bool timeoutType;
	/**
	 * Purge the sent queue of the virtual channel and generate a response
	 * @see p. 5.2.2 from COP-1 CCSDS
	 */
	FOPNotification purgeSentQueue();

	/**
	 * Purge the wait queue of the virtual channel and generate a response
	 * @see p. 5.2.3 from COP-1 CCSDS
	 */
	FOPNotification purgeWaitQueue();

	/**
	 * Prepares a Type-AD Frame for transmission
	 * @see p. 5.2.4 from COP-1 CCSDS
	 */
	FOPNotification transmitAdFrame();

	/**
	 * Prepares a Type-BC Frame for transmission
	 * @see p. 5.2.5 from COP-1 CCSDS
	 */
	FOPNotification transmitBcFrame(TransferFrameTC* bc_frame);

	/**
	 * Prepares a Type-BD Frame for transmission
	 * @see p. 5.2.6 from COP-1 CCSDS
	 */
	FOPNotification transmitBdFrame(TransferFrameTC* bd_frame);

	/**
	 * Marks AD Frames stored in the sent queue to be retransmitted
	 * @see p. 5.2.7 from COP-1 CCSDS
	 */
	void initiateAdRetransmission();

	/**
	 * Marks BC Frames stored in the sent queue to be retransmitted
	 * @see p. 5.2.7 from COP-1 CCSDS
	 */
	void initiateBcRetransmission();

	/**
	 * Remove acknowledged frames from sent queue
	 * @see p. 5.2.8 from COP-1 CCSDS
	 */
	void removeAcknowledgedFrames();

	/**
	 * Search for directives in the sent queue and transmit any eligible frames
	 * @see p. 5.2.9 from COP-1 CCSDS
	 */
	void lookForDirective();

	/**
	 * stores TC Packets, that have being processed by the FOP service, to the
	 * txOutFramesBeforeAllFramesGenerationListTC list, in order to be processed by All Frames Generation Service
	 */
	COPDirectiveResponse pushSentQueue();
	/**
	 * Search for a FDU that can be transmitted in the sent_queue. If none are found also search in
	 * the wait_queue
	 * @see p. 5.2.10 from COP-1 CCSDS
	 */
	COPDirectiveResponse lookForFdu();

	/**
	 * initializes FOP service
	 * @see p. 5.2.14 from COP-1 CCSDS
	 */
	void initialize();

	/**
	 * @see p. 5.2.15 from COP-1 CCSDS
	 */
	void alert(AlertEvent event);

	/* CLCW arrival*/

	/**
	 * Process event where a valid CLCW arrives
	 * @see Table 5-1 from COP-1 CCSDS (E1 - E14)
	 */
	COPDirectiveResponse validClcwArrival();

	// TODO: Check for invalid CLCW
	/**
	 * Process invalid CLCW arrival
	 * @see Table 5-1 from COP-1 CCSDS (E15)
	 */
	void invalidClcwArrival();

	/**
	 * acknowledges TC Packets with frame_seq_num, that have being processed by the FOP service and that have
	 * @param frame_seq_num
	 */
	void acknowledgeFrame(uint8_t frame_seq_num);

	/* Directives (@see Table 5-1 from COP-1 CCSDS)*/

	/**
	 * E23
	 * @see Table 5-1 from COP-1 CCSDS
	 */
	FDURequestType initiateAdNoClcw();
	/**
	 * E24
	 * @see Table 5-1 from COP-1 CCSDS
	 */
	FDURequestType initiateAdClcw();
	/**
	 * E25-E26
	 * @see Table 5-1 from COP-1 CCSDS
	 */
	FDURequestType initiateAdUnlock();
	/**
	 * E27-E28
	 * @see Table 5-1 from COP-1 CCSDS
	 */
	FDURequestType initiateAdVr(uint8_t vr);
	/**
	 * E29-E30
	 * @see Table 5-1 from COP-1 CCSDS
	 */
	FDURequestType terminateAdService();
	/**
	 * E30-E34
	 * @see Table 5-1 from COP-1 CCSDS
	 */
	FDURequestType resumeAdService();
	/**
	 * E35
	 * @see Table 5-1 from COP-1 CCSDS
	 */
	COPDirectiveResponse setVs(uint8_t vs);
	/**
	 * E36
	 * @see Table 5-1 from COP-1 CCSDS
	 */
	COPDirectiveResponse setFopWidth(uint8_t width);
	/**
	 * E37
	 * @see Table 5-1 from COP-1 CCSDS
	 */
	COPDirectiveResponse setT1Initial(uint16_t t1_init);
	/**
	 * E38
	 * @see Table 5-1 from COP-1 CCSDS
	 */
	COPDirectiveResponse setTransmissionLimit(uint8_t vr);
	/**
	 * E39
	 * @see Table 5-1 from COP-1 CCSDS
	 */
	COPDirectiveResponse setTimeoutType(bool vr);
	/**
	 * E40
	 * @see Table 5-1 from COP-1 CCSDS
	 */
	COPDirectiveResponse invalidDirective();

	/** Response from lower procedures*/

	/**
	 * E41
	 * @see Table 5-1 Page 5 - 22 from COP-1 CCSDS
	 */
	void adAccept();
	/**
	 * E42
	 * @see Table 5-1 Page 5 - 22 from COP-1 CCSDS
	 */
	void adReject();
	/**
	 * E43
	 * @see Table 5-1 Page 5 - 22 from COP-1 CCSDS
	 */
	void bcAccept();
	/**
	 * E44
	 * @see Table 5-1 Page 5 - 22 from COP-1 CCSDS
	 */
	void bcReject();
	/**
	 * E45
	 * @see Table 5-1 Page 5 - 22 from COP-1 CCSDS
	 */
	COPDirectiveResponse bdAccept();
	/**
	 * E46
	 * @see Table 5-1 Page 5 - 22 from COP-1 CCSDS
	 */
	void bdReject();

	/**
	 * E19 - E22 Table 5-1 COP-1 CCSDS
	 * @see p. 3.2.2.3.2 from COP-1 CCSDS
	 */
	COPDirectiveResponse transferFdu();

	/**
	 * Function that acknowledges all the unacknowledged frames in the txMasterCopyTC buffer before the received report
	 * value from the CLCW
	 */
	void acknowledgePreviousFrames(uint8_t frameSequenceNumber);

public:
	FrameOperationProcedure(VirtualChannel<MAP_T, VC_T, MC>* vchan, etl::list<TransferFrameTC*, VC_T>* waitQueue,
	                        etl::list<TransferFrameTC*, MaxReceivedTxTcInFOPSentQueue>* sentQueue,
	                        const uint8_t repetitionCopCtrl)
	    : waitQueueFOP(waitQueue), sentQueueFOP(sentQueue), vchan(vchan), state(FOPState::INITIAL),
	      suspendState(FOPState::INITIAL), transmitterFrameSeqNumber(0), adOut(FlagState::READY),
	      bdOut(FlagState::READY), bcOut(FlagState::READY), expectedAcknowledgementSeqNumber(0),
	      tiInitial(FopTimerInitial), transmissionLimit(repetitionCopCtrl), transmissionCount(1),
	      fopSlidingWindow(FopSlidingWindowInitial), timeoutType(false){};
};