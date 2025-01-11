#pragma once

#include <cstdint>
#include <TransferFrameTC.hpp>
#include <etl/list.h>
#include <optional>
#include <Alert.hpp>
#include <MemoryPool.hpp>
#include <CCSDS_Definitions.hpp>
#include <CLCW.hpp>

/**
 * Directive request signal
 * @see p. 3.2.2.2.2 & 4.1 from COP-1 CCSDS
 */
enum DirectiveRequestType {
    INITIATE_AD_SERVICE_WITHOUT_CLCW_CHECK = 1,
    INITIATE_AD_SERVICE_WITH_CLCW_CHECK = 2,
    INITIATE_AD_SERVICE_WITH_UNLOCK = 3,
    INITIATE_AD_SERVICE_WITH_SET_VR = 4,
    TERMINATE_AD_SERVICE = 5,
    RESUME_AD_SERVICE = 6,
    SET_NEW_VS = 7,
    SET_FOP_SLIDING_WINDOW_WIDTH = 8,
    SET_T1_INITIAL = 9,
    SET_TRANSMISSION_LIMIT = 10,
    SET_TIMEOUT_TYPE
};

struct DirectiveRequestSignal {
    uint8_t requestIdentifier;
    DirectiveRequestType directiveType;
    uint8_t directiveQualifier = 0;
};

/**
 * Directive notification signal
 * @see p. 3.2.2.2.3 & 4.2 from COP-1 CCSDS
 */
enum DirectiveNotificationType {
    ACCEPT_RESPONSE_TO_DIRECTIVE,
    REJECT_RESPONSE_TO_DIRECTIVE,
    POSITIVE_CONFIRM_RESPONSE_TO_DIRECTIVE,
    NEGATIVE_CONFIRM_RESPONSE_TO_DIRECTIVE
};

struct DirectiveNotificationSignal {
    uint8_t requestIdentifier;
    DirectiveNotificationType directiveNotificationType;
};

/**
 * Asynchronous notification signal
 * @see p. 3.2.2.2.4 & 4.3 from COP-1 CCSDS
 */
enum AsynchronousNotificationType {
    ALERT,
    SUSPEND
};

enum AlertEvent {
	ALRT_SYNCH = 0,
	ALRT_CLCW = 1,
	ALRT_LIMIT = 2,
	ALRT_TERM = 3,
	ALRT_LLIF = 4,
	ALRT_NNR = 5,
	ALRT_LOCKOUT = 6,
    ALRT_NONE = 7
};

struct AsynchronousNotificationSignal {
    AsynchronousNotificationType asynchronousNotificationType;
    AlertEvent alertEvent = AlertEvent::ALRT_NONE;
};

/**
 * FDU Transfer signal
 * @see p. 3.2.2.3 from COP-1 CCSDS
 */
struct DfuTransferSignal {
    uint8_t requestIdentifier;
    ServiceType serviceType;
    TransferFrameTC* frame;
};

/**
 * Transfer notification signal
 * @see p. 3.2.2.3.3 & 4.4 from COP-1 CCSDS
 */
enum TransferNotificationType {
    ACCEPT_RESPONSE_TO_TRANSFER_FDU, // for AD & BD frames
    REJECT_RESPONSE_TO_TRANSFER_FDU, // for AD & BD frames
    POSITIVE_CONFIRM_RESPONSE_TO_TRANSFER_FDU, // for AD frames only
    NEGATIVE_CONFIRM_RESPONSE_TO_TRANSFER_FDUm // for AD frames only
};

struct TransferNotificationSignal {
    uint8_t requestIdentifier;
    TransferNotificationType transferNotificationType;
};


/**
 *  Transmit & abort request for frame signal
 *  @see p. 3.2.3 from COP-1 CCSDS
 */
enum LowerLayerRequestType {
    LOW_LAYER_TRANSMIT,
    LOW_LAYER_ABORT  // lower layers should abort all AD and BC frame transmission
};

struct LowerLayerRequestSignal {
    LowerLayerRequestType lowerLayerRequestType;
    TransferFrameTC* frame;
};

/**
 *  Transmit request response signal
 *  @see p. 3.2.3 from COP-1 CCSDS
 */
enum FopTransmitRequestResponseSignal {
     AD_ACCEPT,
     AD_REJECT,
     BC_ACCEPT,
     BC_REJECT,
     BD_ACCEPT,
     BD_REJECT
};

 /**
  * A struct containing output signal(s) from FOP-1, as well the event code
  */
struct FopOutputSignals {
    uint8_t eventCode;
    std::optional<DirectiveRequestSignal> directiveNotificationSignal;
    std::optional<AsynchronousNotificationSignal> asynchronousNotificationSignal;
    std::optional<FopTransmitRequestResponseSignal> fopTransmitRequestResponseSignal;
};

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

class VirtualChannel;

/**
 * The frame operation procedure (FOP-1) is a sub-segment of COP-1, a process responsible
 * for TC frame acknowledgment and assurance of their arrival with the correct order. Frames
 * that this service will be applied to are called 'TYPE-AD frames'. It is also possible to bypass
 * this service, in which case we have 'TYPE-BD frames'. For communication with COP-1's reception side
 * subsegment (FARM-1), 'TYPE-BC frames' are generated within FOP-1.
 *
 * For proper operation, the TC Data Link user must provide FOP-1 with:
 * 1. CLCWs: Those are carried by TM transfer frames (@see TM Data Link Protocol), and constitute FARM-1's
 *    way of communicating with FOP-1. They are provided with the method pushClcw()
 * 2. directives: Those are commands that initialize the process or change certain parameters.
 *
 * Furthermore, signals are returned to the user via vcGeneration, the data processing function where FOP-1 runs within.
 * These can indicate the successful reception of frames (can be collected for monitoring reasons) or alerts, which
 * indicate an unrecoverable problem with the data link, and demand action from higher level protocols.
 *
 */
class FrameOperationProcedure {
    friend class ServiceChannel;
    friend class MasterChannel;

public:
    VirtualChannel* vchan;

private:
    /** FOP-1 VARIABLES **/

    /**
     * This  variable  represents  the  state  of  FOP-1  for  the  specific  Virtual  Channel.
     * @see p. 5.1.2 from COP-1 CCSDS
     */
    FOPState state;

    /**
     * It contains the value of the Frame Sequence Number to be put in the Transfer Frame Primary Header of
     * the  next  Type-AD Transfer Frame to be transmitted.
     * @see p. 5.1.3 from COP-1 CCSDS
     */
    uint8_t transmitterFrameSeqNumber;

    /**
     * Type-AD transfer frames stored in list, before being processed by the FOP service.
     * @see p. 5.1.4 from COP-1 CCSDS
     */
    etl::list<TransferFrameTC*, MaxReceivedTxTcInWaitQueue>* waitQueueFOP;
    /**
     * Type-AD transfer frames stored in list, after being processed by the FOP service, as well as generated Type-BC
     * frames.
     * @see p. 5.1.7 from COP-1 CCSDS
     */
    etl::list<TransferFrameTC*, MaxReceivedTxTcInFOPSentQueue>* sentQueueFOP;

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
    uint8_t fopSlidingWindowWidth;
    /**
     * It specifies the action to be performed when both the Timer expires and the Transmission
     * Count (see 5.1.10.4) has reached the Transmission_Limit.
     * @see p. 5.1.10.3 from COP-1 CCSDS
     */
    bool timeoutType;

    /**
     * It records the state that FOP-1 was in when the AD Service was suspended (as described in 5.1.10).
     * This is the state to which FOP-1 will return should the AD Service be resumed.
     * @see p. 5.1.11 from COP-1 CCSDS
     */
    FOPState suspendState;


    /** Implementation Specific variables ** /

    /**
     * Queues for storing incoming signals and clcws
     */
    etl::queue<DirectiveNotificationType, DirectiveRequestSignalQueueSize> directiveRequestSignalQueue;
    etl::queue<DfuTransferSignal, TransferfduSignalQueueSize> transferFduSignalQueue;
    etl::queue<FopTransmitRequestResponseSignal, FopTransmitRequestResponseSignalQueueSize> fopTransmitRequestResponseSignalQueue;
    etl::queue<CLCW, clcwQueueSize> clcwQueue;

    /**
     * frame master copy and memory pool: No frame master frame copies will be stored inside FOP-1, to avoid
     * extra memory overhead and the time it would require to copy them. However, FOP-1 needs to delete frames
     * once their reception is confirmed and purge frames if an error has occurred, as well as create
     * TYPE-BC frames. Therefore, the Tx TC chain's master copy buffer and memory pool are stored here as a reference.
     * FOP-1 will take the responsibility of master copy deletion.
     */
    etl::list<TransferFrameTC, MaxTxInMasterChannel>& frameMasterCopyBuffer;
    MemoryPool& memoryPool;

    /** FOP-1 ACTIONS **/

    /**
     * Purge the sent queue of the virtual channel and generate a response
     * @see p. 5.2.2 from COP-1 CCSDS
     */
    bool purgeSentQueue();

    /**
     * Purge the wait queue of the virtual channel and generate a response
     * @see p. 5.2.3 from COP-1 CCSDS
     */
    bool purgeWaitQueue();

    /**
     * Prepares a Type-AD Frame for transmission
     * @see p. 5.2.4 from COP-1 CCSDS
     */
    bool transmitAdFrame();

    /**
     * Prepares a Type-BC Frame for transmission
     * @see p. 5.2.5 from COP-1 CCSDS
     */
    bool transmitBcFrame(TransferFrameTC* bc_frame);

    /**
     * Prepares a Type-BD Frame for transmission
     * @see p. 5.2.6 from COP-1 CCSDS
     */
    bool transmitBdFrame(TransferFrameTC* bd_frame);

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
     * Search for a FDU that can be transmitted in the sent_queue. If none are found also search in
     * the wait_queue
     * @see p. 5.2.10 from COP-1 CCSDS
     */
    bool lookForFdu();

    /**
     * initializes FOP service
     * @see p. 5.2.14 from COP-1 CCSDS
     */
    void initialize();

    /**
     * @see p. 5.2.15 from COP-1 CCSDS
     */
    void alert(AlertEvent event);

    /**
     * @see p. 5.2.17 from COP-1 CCSDS
     */
     void resume();


    /** Implementation specific FOP-1 methods **/

    /**
     * Detect and process an event. Return notifications for upper or lower layers.
     * Meant to be used by the vcGeneration Service
     * @TODO figure out a reasonable event processing order. one like this seems fine:
     * 1. directive request event come first: the data link user must have priority
     * 2. clcw requests: some of them require urgent actions, like stopping lower layer transmission
     * 3. timer expiration events
     * 4. transfer fdu and lower layer responses: essentially requests having to do with frame transfers
     */
      FopOutputSignals applyFopStateTable();

     /**
      * Attempt to pass a frame data unit to FOP-1.
      * Meant to be used by the vcGeneration Service.
      * A return signal of ACCEPT_RESPONSE_TO_TRANSFER_FDU indicates that the frame
      * is now handled by FOP-1. At a later time, applyFopStateTable() will return a
      * POSITIVE_CONFIRM_RESPONSE_TO_TRANSFER_FDU or a NEGATIVE_CONFIRM_RESPONSE_TO_TRANSFER_FDU
      * (with the corresponding signal Id) to indicate vcGeneration service (and potentially the
      * TC data link user) that the frame was obtained by the receiving side successfully.
      *
      * NOTE: Removal of the frame master copy is handled by FOP-1.
      */
      TransferNotificationSignal pushTransferFduSignal(DfuTransferSignal signal);


      /**
       * Respond to FOP-1's request for passing a frame to lower layers
       * Meant to be used by the vcGeneration service. A return value of true
       * indicates the action was successful (there was enough space in the signal queue)
       */
       bool pushFopTransmitRequestResponseSignal(FopTransmitRequestResponseSignal signal);

     /**
      * Pass a directive request to FOP-1. This method is offers a way for the TC Data Link users to send commands
      * to FOP-1
      *
      * @param signal: A DirectiveRequestSignal, specifying the type of directive and a signal ID.
      * @returns An immediate response (DirectiveNotificationSignal) on whether the request is accept or rejected.
      *          What an acceptance or rejection mean is explained below:
      *          - accepted (ACCEPT_RESPONSE_TO_DIRECTIVE) -> The directive is successfully stored in the queue. However,
      *          it's execution will be confirmed at a later time either by receiving a POSITIVE_CONFIRM_RESPONSE_TO_DIRECTIVE
      *          or a NEGATIVE_CONFIRM_RESPONSE_TO_DIRECTIVE. This confirmation will be returned by applyFopStateTable()
      *          (and by extension, vcGenerationService), along with the signal ID, so the user can reaxt appropriately.
      *
      *          - rejected (REJECT_RESPONSE_TO_DIRECTIVE) -> The directive is rejected because either the signal queue
      *          is full, or for there is a FOP related reason (@see FOP-1 State Table)
      *
      * @TODO make a wrapper function in  service channel.
      *       the wrapper function should additionaly have a vid param, since a FOP-1 object will exist for every
      *       virtual channel
      */
      DirectiveNotificationSignal pushDirectiveRequestSignal(DirectiveRequestSignal signal);

    /**
     * Push CLCWs for FOP-1 to inspect. A return value of true
     * indicates the action was successful (there was enough space in the CLCWs queue)
     *
     * @TODO make a service channel wrapper function, that moves the clcws to the correct virtual channel
     */
    bool pushClcw(CLCW clcw);

public:
    FrameOperationProcedure(const uint16_t fopTimerInitial, const uint8_t transmissionLimit, const uint8_t foSlidingWindowWidth,
                            etl::list<TransferFrameTC, MaxTxInMasterChannel>& frameMasterCopyBuffer, MemoryPool& memoryPool)
            : vchan(vchan), state(FOPState::INITIAL),
              suspendState(FOPState::INITIAL), transmitterFrameSeqNumber(0), adOut(FlagState::NOT_READY),
              bdOut(FlagState::NOT_READY), bcOut(FlagState::NOT_READY), expectedAcknowledgementSeqNumber(0),
              tiInitial(fopTimerInitial), transmissionLimit(transmissionLimit), transmissionCount(1),
              fopSlidingWindowWidth(foSlidingWindowWidth), timeoutType(false), frameMasterCopyBuffer(frameMasterCopyBuffer),
              memoryPool(memoryPool){};
};

//class VirtualChannel;
//class MAPChannel;
//
//class FrameOperationProcedure {
//	friend class ServiceChannel;
//	friend class MasterChannel;
//
//public:
//	/**
//	 * TC transfer frames stored in list, before being processed by the FOP service
//	 * @see p. 5.1.4 from COP-1 CCSDS
//	 */
//	etl::list<TransferFrameTC*, MaxReceivedTxTcInWaitQueue>* waitQueueFOP;
//	/**
//	 * TC transfer frames stored in list, after being processed by the FOP service
//	 * @see p. 5.1.7 from COP-1 CCSDS
//	 */
//	etl::list<TransferFrameTC*, MaxReceivedTxTcInFOPSentQueue>* sentQueueFOP;
//
//	/**
//	 * TC transfer frames stored in list, before being processed by the FOP service
//	 * @see p. 5.1.4 from COP-1 CCSDS
//	 */
//	etl::list<TransferFrameTC*, MaxReceivedTxTcInWaitQueue>* waitQueueFARM;
//	/**
//	 * TC transfer frames stored in list, after being processed by the FOP service
//	 * @see p. 5.1.7 from COP-1 CCSDS
//	 */
//	etl::list<TransferFrameTC*, MaxReceivedTxTcInFOPSentQueue>* sentQueueFARM;
//
//	VirtualChannel* vchan;
//
//private:
//	/**
//	 * This  variable  represents  the  state  of  FOP-1  for  the  specific  Virtual  Channel.
//	 * @see p. 5.1.2 from COP-1 CCSDS
//	 */
//	FOPState state;
//	/**
//	 * It records the state that FOP-1 was in when the AD Service was suspended (as described in 5.1.10).
//	 * This is the state to which FOP-1 will return should the AD Service be resumed.
//	 * @see p. 5.1.11 from COP-1 CCSDS
//	 */
//	FOPState suspendState;
//
//	/**
//	 * It contains the value of the Frame Sequence Number to be put in the Transfer Frame Primary Header of
//	 * the  next  Type-AD Transfer Frame to be transmitted.
//	 * @see p. 5.1.3 from COP-1 CCSDS
//	 */
//	uint8_t transmitterFrameSeqNumber;
//	/**
//	 * @see p. 5.1.6 from COP-1 CCSDS
//	 */
//	bool adOut;
//	/**
//	 * @see p. 5.1.6 from COP-1 CCSDS
//	 */
//	bool bdOut;
//	/**
//	 * @see p. 5.1.6 from COP-1 CCSDS
//	 */
//	bool bcOut;
//	/**
//	 * @see p. 5.1.8 from COP-1 CCSDS
//	 */
//	uint8_t expectedAcknowledgementSeqNumber;
//	/**
//	 * Timer
//	 * @see p. 5.1.9 from COP-1 CCSDS
//	 */
//	uint16_t tiInitial;
//	/**
//	 * The  Transmission Limit  holds  a  value  which  represents  the  maximum  number  of  times  the  first
//	 * Transfer  Frame  on  the  Sent_Queue  may  be  transmitted
//	 * @see p. 5.1.10.2 from COP-1 CCSDS
//	 */
//	uint16_t transmissionLimit;
//	/**
//	 * The  Transmission Count  variable  is  used  to  count  the  number  of  transmissions  of  the  first
//	 * Transfer  Frame  on  the  Sent_Queue
//	 * @see p. 5.1.10.4 from COP-1 CCSDS
//	 */
//	uint16_t transmissionCount;
//	/**
//	 * The FOP Sliding Window is a mechanism which limits the number of Transfer Frames which can  be
//	 * transmitted  ahead  of  the  last  acknowledged  Transfer  Frame
//	 * @see p. 5.1.12 from COP-1 CCSDS
//	 */
//	uint8_t fopSlidingWindowWidth;
//	/**
//	 * It specifies the action to be performed when both the Timer expires and the Transmission
//	 * Count (see 5.1.10.4) has reached the Transmission_Limit.
//	 * @see p. 5.1.10.3 from COP-1 CCSDS
//	 */
//	bool timeoutType;
//	/**
//	 * Purge the sent queue of the virtual channel and generate a response
//	 * @see p. 5.2.2 from COP-1 CCSDS
//	 */
//	FOPNotification purgeSentQueue();
//
//	/**
//	 * Purge the wait queue of the virtual channel and generate a response
//	 * @see p. 5.2.3 from COP-1 CCSDS
//	 */
//	FOPNotification purgeWaitQueue();
//
//	/**
//	 * Prepares a Type-AD Frame for transmission
//	 * @see p. 5.2.4 from COP-1 CCSDS
//	 */
//	FOPNotification transmitAdFrame();
//
//	/**
//	 * Prepares a Type-BC Frame for transmission
//	 * @see p. 5.2.5 from COP-1 CCSDS
//	 */
//	FOPNotification transmitBcFrame(TransferFrameTC* bc_frame);
//
//	/**
//	 * Prepares a Type-BD Frame for transmission
//	 * @see p. 5.2.6 from COP-1 CCSDS
//	 */
//	FOPNotification transmitBdFrame(TransferFrameTC* bd_frame);
//
//	/**
//	 * Marks AD Frames stored in the sent queue to be retransmitted
//	 * @see p. 5.2.7 from COP-1 CCSDS
//	 */
//	void initiateAdRetransmission();
//
//	/**
//	 * Marks BC Frames stored in the sent queue to be retransmitted
//	 * @see p. 5.2.7 from COP-1 CCSDS
//	 */
//	void initiateBcRetransmission();
//
//	/**
//	 * Remove acknowledged frames from sent queue
//	 * @see p. 5.2.8 from COP-1 CCSDS
//	 */
//	void removeAcknowledgedFrames();
//
//	/**
//	 * Search for directives in the sent queue and transmit any eligible frames
//	 * @see p. 5.2.9 from COP-1 CCSDS
//	 */
//	void lookForDirective();
//
//	/**
//	 * stores TC transfer frames, that have being processed by the FOP service, to the
//	 * outFramesBeforeAllFramesGenerationListTxTC list, in order to be processed by All Frames Generation Service
//	 */
//	COPDirectiveResponse pushSentQueue();
//	/**
//	 * Search for a FDU that can be transmitted in the sent_queue. If none are found also search in
//	 * the wait_queue
//	 * @see p. 5.2.10 from COP-1 CCSDS
//	 */
//	COPDirectiveResponse lookForFdu();
//
//	/**
//	 * initializes FOP service
//	 * @see p. 5.2.14 from COP-1 CCSDS
//	 */
//	void initialize();
//
//	/**
//	 * @see p. 5.2.15 from COP-1 CCSDS
//	 */
//	void alert(AlertEvent event);
//
//	/* CLCW arrival*/
//
//	/**
//	 * Process event where a valid CLCW arrives
//	 * @see Table 5-1 from COP-1 CCSDS (E1 - E14)
//	 */
//	COPDirectiveResponse validClcwArrival();
//
//	// TODO: Check for invalid CLCW
//	/**
//	 * Process invalid CLCW arrival
//	 * @see Table 5-1 from COP-1 CCSDS (E15)
//	 */
//	void invalidClcwArrival();
//
//	/**
//	 * acknowledges TC transfer frames with frame_seq_num, that have being processed by the FOP service and that have
//	 * @param frame_seq_num
//	 */
//	void acknowledgeFrame(uint8_t frame_seq_num);
//
//	/* Directives (@see Table 5-1 from COP-1 CCSDS)*/
//
//	/**
//	 * E23
//	 * @see Table 5-1 from COP-1 CCSDS
//	 */
//	FDURequestType initiateAdNoClcw();
//	/**
//	 * E24
//	 * @see Table 5-1 from COP-1 CCSDS
//	 */
//	FDURequestType initiateAdClcw();
//	/**
//	 * E25-E26
//	 * @see Table 5-1 from COP-1 CCSDS
//	 */
//	FDURequestType initiateAdUnlock();
//	/**
//	 * E27-E28
//	 * @see Table 5-1 from COP-1 CCSDS
//	 */
//	FDURequestType initiateAdVr(uint8_t vr);
//	/**
//	 * E29-E30
//	 * @see Table 5-1 from COP-1 CCSDS
//	 */
//	FDURequestType terminateAdService();
//	/**
//	 * E30-E34
//	 * @see Table 5-1 from COP-1 CCSDS
//	 */
//	FDURequestType resumeAdService();
//	/**
//	 * E35
//	 * @see Table 5-1 from COP-1 CCSDS
//	 */
//	COPDirectiveResponse setVs(uint8_t vs);
//	/**
//	 * E36
//	 * @see Table 5-1 from COP-1 CCSDS
//	 */
//	COPDirectiveResponse setFopWidth(uint8_t width);
//	/**
//	 * E37
//	 * @see Table 5-1 from COP-1 CCSDS
//	 */
//	COPDirectiveResponse setT1Initial(uint16_t t1_init);
//	/**
//	 * E38
//	 * @see Table 5-1 from COP-1 CCSDS
//	 */
//	COPDirectiveResponse setTransmissionLimit(uint8_t vr);
//	/**
//	 * E39
//	 * @see Table 5-1 from COP-1 CCSDS
//	 */
//	COPDirectiveResponse setTimeoutType(bool vr);
//	/**
//	 * E40
//	 * @see Table 5-1 from COP-1 CCSDS
//	 */
//	COPDirectiveResponse invalidDirective();
//
//	/** Response from lower procedures*/
//
//	/**
//	 * E41
//	 * @see Table 5-1 Page 5 - 22 from COP-1 CCSDS
//	 */
//	void adAccept();
//	/**
//	 * E42
//	 * @see Table 5-1 Page 5 - 22 from COP-1 CCSDS
//	 */
//	void adReject();
//	/**
//	 * E43
//	 * @see Table 5-1 Page 5 - 22 from COP-1 CCSDS
//	 */
//	void bcAccept();
//	/**
//	 * E44
//	 * @see Table 5-1 Page 5 - 22 from COP-1 CCSDS
//	 */
//	void bcReject();
//	/**
//	 * E45
//	 * @see Table 5-1 Page 5 - 22 from COP-1 CCSDS
//	 */
//	COPDirectiveResponse bdAccept();
//	/**
//	 * E46
//	 * @see Table 5-1 Page 5 - 22 from COP-1 CCSDS
//	 */
//	void bdReject();
//
//	/**
//	 * E19 - E22 Table 5-1 COP-1 CCSDS
//	 * @see p. 3.2.2.3.2 from COP-1 CCSDS
//	 */
//	COPDirectiveResponse transferFdu();
//
//	/**
//	 * Function that acknowledges all the unacknowledged frames in the masterCopyTxTC buffer before the received report
//	 * value from the CLCW
//	 */
//	void acknowledgePreviousFrames(uint8_t frameSequenceNumber);
//
//public:
//	FrameOperationProcedure(VirtualChannel* vchan, etl::list<TransferFrameTC*, MaxReceivedTxTcInWaitQueue>* waitQueue,
//	                        etl::list<TransferFrameTC*, MaxReceivedTxTcInFOPSentQueue>* sentQueue,
//	                        const uint8_t repetitionCopCtrl)
//	    : waitQueueFOP(waitQueue), sentQueueFOP(sentQueue), vchan(vchan), state(FOPState::INITIAL),
//	      suspendState(FOPState::INITIAL), transmitterFrameSeqNumber(0), adOut(FlagState::READY),
//	      bdOut(FlagState::READY), bcOut(FlagState::READY), expectedAcknowledgementSeqNumber(0),
//	      tiInitial(FopTimerInitial), transmissionLimit(repetitionCopCtrl), transmissionCount(1),
//	      fopSlidingWindowWidth(FopSlidingWindowInitial), timeoutType(false){};
//};
