#pragma once

#include <cstdint>
#include "etl/queue.h"
#include "etl/list.h"
#include "etl/optional.h"
#include "TransferFrameTC.hpp"
#include "Alert.hpp"
#include "CCSDSChannelConfiguration.hpp"
#include "MemoryPool.hpp"
#include "CCSDSDefinitionsAndUtilities.hpp"
#include "CLCW.hpp"
#include "CountdownTimer.hpp"

namespace CCSDSDataLinkLayer {
#ifdef GROUND_SEGMENT
    /**
     * The frame operation procedure (FOP-1) is the ground segment of COP-1, a process responsible
     * for TC frame acknowledgment and keeping the frame sequence order intact. Frames
     * that this service will be applied to are called 'TYPE-AD frames' (sequence controlled service). It is also possible to bypass
     * it, by transmitting 'TYPE-BD frames' (expedited service). For communication with COP-1's reception side
     * subsegment (FARM-1), 'TYPE-BC frames' are generated within FOP-1. FOP-1 (and FARM-1) are state machines.
     *
     * For proper operation, the TC Data Link user must provide FOP-1 with:
     * 1. CLCWs: Those are carried by TM transfer frames (@see TM Data Link Protocol), and constitute FARM-1's
     *    method of communicating with FOP-1, thus having a closed loop system. They are provided with the method pushClcw()
     * 2. directives: Those are commands that initialize the process or change certain parameters. They are provided with the
     *    method pushDirectiveRequestSignal()
     *
     * Furthermore, signals are returned to the user via vcGeneration, the data processing function where FOP-1 is executed.
     * These can indicate the successful or unsuccessful execution of directives and alerts, which
     * indicate an unrecoverable problem with the data link, and demand action from higher level protocols.
     *
     */
    class FrameOperationProcedure {
        friend class BaseServiceChannel;
        friend class ServiceChannelGroundSegment;

        friend class MasterChannelGroundSegment;

    private:
        /** FOP-1 VARIABLES **/

        /**
         * This  variable  represents  the  state  of  FOP-1  for  the  specific  Virtual  Channel.
         * @see p. 5.1.2 from COP-1 CCSDS
         */
        DefsAndUtils::FOPState state;
        /**
         * It contains the value of the Frame Sequence Number to be put in the Transfer Frame Primary Header of
         * the  next  Type-AD Transfer Frame to be transmitted.
         * @see p. 5.1.3 from COP-1 CCSDS
         */
        uint8_t transmitterFrameSeqNumber;
        /**
         * Type-AD transfer frames stored in list, before being processed by the FOP service. It has a capacity of one.
         * @see p. 5.1.4 from COP-1 CCSDS
         */
        etl::list<TransferFrameTC *, 1> waitQueueFOP;
        /**
         * Type-AD transfer frames stored in list, after being processed by the FOP service, as well as generated Type-BC
         * frames.
         * @see p. 5.1.7 from COP-1 CCSDS
         * // TODO magic num
         */
        etl::list<TransferFrameTC *, 10> sentQueueFOP;
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
         * Countdown timer initial value, in milliseconds
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
         * Transfer  Frame  on  the  sent queue
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
        DefsAndUtils::SuspendVariableState suspendState;

        /** Implementation Specific variables **/

        /**
         * virtual channel parameters passed upon construction
         */
        const uint8_t vid;
        const bool errorControlFieldPresent;

        CountdownTimer timer;
        /**
         * Queues for storing incoming signals and clcws
         */
        etl::queue<DefsAndUtils::DirectiveRequestSignal, DefsAndUtils::DirectiveRequestSignalQueueSize> directiveRequestSignalQueue;
        etl::queue<DefsAndUtils::FduTransferSignal, DefsAndUtils::TransferfduSignalQueueSize> transferFduSignalQueue;
        etl::queue<DefsAndUtils::LowerLayerResponseSignal, DefsAndUtils::LowerLayerResponseSignalQueueSize> lowerLayerResponseSignalQueue;
        etl::queue<CLCW, 1> clcwQueue;

        /**
         * Queues for storing output signals
         * // TODO magic number
         */
        etl::queue<DefsAndUtils::DirectiveNotificationSignal, 1> directiveNotificationSignalQueue;
        etl::queue<DefsAndUtils::TransferNotificationSignal, 10 + 1> transferNotificationSignalQueue;
        etl::queue<DefsAndUtils::AsynchronousNotificationSignal, 1> asynchronousNotificationSignalQueue;
        etl::queue<DefsAndUtils::FopToLowerLayerRequestSignal, 10 + 1> fopToLowerLayerRequestSignalQueue;

        /**
         * There are 3 directives that will not receive confirmation immediately upon processing:
         * Initiate AD service (with CLCW check)
         * Initiate AD service (with unlock)
         * Initiate AD service (with set V(R))
         * The first makes FOP wait for a CLCW, so that FOP is synchronized by farm.
         * The last 2 generate and transmit a type BC frame, so that FARM is synchronized by FOP.
         *
         * Their identifiers are stored in these variables.
         */
        etl::optional<uint8_t> initiateWithClcwCheckId;
        etl::optional<uint8_t> initiateWithBcFrameId;

        /** FOP-1 ACTIONS **/

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
         * Prepares a Type-AD Frame for transmission. Type-AD frames are popped from the wait queue
         * and placed in the sent queue. They will be removed once there is confirmation of their reception.
         * @see p. 5.2.4 from COP-1 CCSDS
         */
        FOPNotification transmitAdFrame(TransferFrameTC *adFrame);

        /**
         * Prepares a Type-BC Frame for transmission. Type-BC frames are generated in this function
         * and placed in the sent queue when 2 specific directive types are sent:
         * INITIATE_AD_SERVICE_WITH_UNLOCK
         * INITIATE_AD_SERVICE_WITH_SET_VR
         * Those frames are removed from the sent queue once the lower layers accept them
         * @see p. 5.2.5 from COP-1 CCSDS
         */
        FOPNotification transmitBcFrame(const ChannelConfig::MasterChannelGroundSegmentVariant& masterChannelVariant,
            const DefsAndUtils::DirectiveRequestSignal &directiveSignal);

        /**
         * Prepares a Type-BD Frame for transmission. Type-BD frames essentially bypass FOP-1 services.
         * They are not placed in the wait or sent queue. In case they are accepted to the lower layers,
         * a POSITIVE_RESPONSE_TO_TRANSFER_FDU will be returned to higher layers, but not a
         * POSITIVE(NEGATIVE)_CONFIRM_RESPONSE_TO_TRANSFER_FDU. In case the lower layers cannot accept the frame,
         * a NEGATIVE_CONFIRM_RESPONSE_TO_TRANSFER_FDU will be returned, and the frame will be immediately discarded.
         *
         * @see p. 5.2.6 from COP-1 CCSDS
         */
        FOPNotification transmitBdFrame(TransferFrameTC *bdFrame);

        /**
         * Marks AD (or BC) Frames stored in the sent queue to be retransmitted
         * @see p. 5.2.7 from COP-1 CCSDS
         */
        FOPNotification initiateRetransmission(DefsAndUtils::ServiceType serviceType);

        /**
         * Remove acknowledged TYPE-AD frames from sent queue (TYPE-BD frames are instead cleared upon successful
         * CLCW reception, @see E1 in FOP-1 table). The frames for ready removal are those that have a transfer frame sequence
         * number smaller than
         * @param reportValue: The report value field of a CLCW. It is equal to the next frame sequence number FARM-1 expects
         *                     to get in the next transmission (@see p. 4.2.1.11 from TC Data Link). Therefore this is equal to
         *                     the frame sequence number of the oldest unacknowledged frame, aka the expectedAcknowledgementSeqNumber
         *                     (@see p. 5.1.8 from COP-1)
         * @see p. 5.2.8 from COP-1 CCSDS
         */
        FOPNotification removeAcknowledgedFramesFromSentQueue(uint8_t reportValue);

        /**
         * Search for directives in the sent queue and transmit any eligible frames
         * @see p. 5.2.9 from COP-1 CCSDS
         */
        FOPNotification lookForDirective();

        /**
         * Search for a FDU that can be transmitted in the sent_queue. If none are found also search in
         * the wait_queue
         * @see p. 5.2.10 from COP-1 CCSDS
         */
        FOPNotification lookForFdu();

        /**
         * initializes FOP service
         * @see p. 5.2.14 from COP-1 CCSDS
         */
        void initialize();

        /**
         * @see p. 5.2.15 from COP-1 CCSDS
         */
        void alert(DefsAndUtils::AlertEvent event);

        /**
         * @see p. 5.2.17 from COP-1 CCSDS
         */
        void resume();

        /**
         * @see p. 5.2.17 from COP-1 CCSDS
         * @note This function literally does nothing. It is added to explicitly
         *       indicate the "ignore" action in the state table.
         */
        static inline void ignore() {
        }

        /** Implementation specific FOP-1 methods (for usage inside vcGeneration service)**/

        /**
         * This is core process of FOP-1. By examining incoming signals, CLCWs and internal variables,
         * an event is detected, then appropriate actions are taken based on that event, and the current state.
         * @see p. 5.3 from COP-1 CCSDS
         *
         * Any output signals can be collected using the pop signal methods. Each time applyFopStateTable() is
         * executed, the output signal queues are cleared.
         *
         * @returns The event code detected. An event code of 0 means no event.
         *
         */
        std::pair<FOPNotification, uint8_t> applyFopStateTable(const ChannelConfig::MasterChannelGroundSegmentVariant& masterChannelVariant);

        /**
         * Respond to FOP-1's request for passing a frame to lower layers.
         */
        FOPNotification pushLowerLayerResponseSignal(DefsAndUtils::LowerLayerResponseSignal signal);

        /** Implementation specific FOP-1 methods (for the the TC Data Link User). Wrapper functions are provided
         * in ServiceChannel.
         */

        /**
         * Pass a directive request to FOP-1. This method is offers a way for the TC Data Link users to send commands
         * to FOP-1
         *
         * @param signal: A DirectiveRequestSignal, specifying the type of directive and a signal ID.
         *          At a later time, a ACCEPT_RESPONSE_TO_DIRECTIVE or a REJECT_RESPONSE_TO_DIRECTIVE signal will be returned.
         *          What an acceptance or rejection mean is explained below:
         *          - ACCEPT_RESPONSE_TO_DIRECTIVE -> The directive is successfully stored in the queue. However,
         *          it's execution will be confirmed at a later time either by receiving a POSITIVE_CONFIRM_RESPONSE_TO_DIRECTIVE
         *          or a NEGATIVE_CONFIRM_RESPONSE_TO_DIRECTIVE.
         *
         *          - REJECT_RESPONSE_TO_DIRECTIVE -> The directive is rejected either because the signal queue
         *          is full, or there is a FOP related reason (@see p. 5.3 from COP-1 CCSDS)
         */
        FOPNotification pushDirectiveRequestSignal(const DefsAndUtils::DirectiveRequestSignal &signal);

        /**
         * Push a CLCW for FOP-1 to inspect. Since only the most recent CLCW is of interest, the old one (if it exists)
         * is overwritten.
         */
        void pushClcw(CLCW clcw);

    public:
        FrameOperationProcedure(const uint8_t vid, const bool errorControlFieldPresent, const uint16_t tiInitial,
                                const uint16_t transmissionLimit,
                                const uint8_t fopSlidingWindowWidth)
            : state(DefsAndUtils::FOPState::INITIAL), transmitterFrameSeqNumber(0), adOut(true),
              bdOut(true), bcOut(true), expectedAcknowledgementSeqNumber(0),
              tiInitial(tiInitial), transmissionLimit(transmissionLimit), transmissionCount(1),
              fopSlidingWindowWidth(fopSlidingWindowWidth), timeoutType(false),
              suspendState(DefsAndUtils::SuspendVariableState::NOT_SUSPENDED),
              vid(vid), errorControlFieldPresent(errorControlFieldPresent), timer(CountdownTimer()) {
        }
    };
#endif // GROUND_SEGMENT
} // namespace CCSDSDataLinkLayer
