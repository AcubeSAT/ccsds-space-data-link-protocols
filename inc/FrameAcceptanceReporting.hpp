#pragma once

#include <cstdint>
#include "etl/queue.h"
#include "etl/list.h"
#include "etl/optional.h"
#include "etl/circular_buffer.h"
#include "TransferFrameTC.hpp"
#include "MemoryPool.hpp"
#include "CountdownTimer.hpp"
#include "CLCW.hpp"
#include "Alert.hpp"
#include "CCSDS_Definitions.hpp"

namespace CCSDSDataLinkLayer {
#ifdef SPACE_SEGMENT
/**
 * @see p. 6.1.2 from COP-1 CCSDS
 */
    enum class FARMState : uint8_t {
        OPEN = 1,
        WAIT = 2,
        LOCKOUT = 3,
    };

    enum class Window : uint8_t {
        POSITIVE_WINDOW = 1,
        NEGATIVE_WINDOW = 2,
        OUTSIDE_WINDOWS = 3
    };

    class BaseVirtualChannel;

    class BaseMAPChannel;

/**
 * The frame acceptance reporting mechanism (FARM-1) is the spacecraft segment of COP-1, a process responsible
 * for TC frame acknowledgment and keeping the frame sequence order intact. Frames
 * that this mechanism will be applied to are called 'TYPE-AD frames' (sequence controlled service). Specifically, if it
 * detects an unexpected frame, it will go to 'lockout mode'. If the TC Rx chains buffers are full, it will go
 * to 'wait' mode. In both cases, any subsequent TYPE-AD frame is discarded, and a report value called 'CLCW', will
 * be sent to FOP-1 (COP-1's ground segment). The CLCW delivery is handled by the TM Data Link Protocol. It is also possible to accept 'TYPE-BD'
 * frames, which bypass FARM-1 checks and overwrite their respective buffers if they are full (expedited service).
 * Finally, FARM-1 consumes 'TYPE-BC' frames, which are messages sent by FOP-1, aimed at synchronizing the 2 segments.
 * FARM-1 is implemented as a state machine.
 */
    class FrameAcceptanceReporting {
	    friend class BaseServiceChannel;
	    friend class ServiceChannelSpaceSegment;

        friend class MasterChannelSpaceSegment;
	    friend class VirtualChannelSpaceSegment;

//	public:
//	    FrameAcceptanceReporting(const FrameAcceptanceReporting& f)
//		   : state(f.state), lockout(f.lockout), wait(f.wait), retransmit(f.retransmit), farmBCount(f.farmBCount),
//	         receiverFrameSeqNumber(f.receiverFrameSeqNumber), farmSlidingWinWidth(f.farmSlidingWinWidth),
//	         farmPositiveWinWidth(f.farmPositiveWinWidth), farmNegativeWidth(f.farmNegativeWidth),
//	         clcwReportInterval(f.clcwReportInterval), timer(f.timer), vid(f.vid),
//	         errorControlFieldPresent(f.errorControlFieldPresent), lowerLayerBuffer(f.lowerLayerBuffer),
//	         higherLayerBufferTypeAD(f.higherLayerBufferTypeAD), higherLayerBufferTypeBD(f.higherLayerBufferTypeBD),
//	         frameMasterCopyBuffer(f.frameMasterCopyBuffer), memoryPool(f.memoryPool), clcwBuffer(f.clcwBuffer) {}

    private:
        /** FARM-1 Variables **/

        /**
         * Holds the state of the state machine
         * @see p. 6.1.2 of COP-1 CCSDS
         */
        FARMState state;
        /**
         * Indicates if state machine is in lockout mode
         * @see p. 6.1.3 of COP-1 CCSDS
         */
        bool lockout;
        /**
         * Indicates if state machine is in wait mode
         * @see p. 6.1.4 of COP-1 CCSDS
         */
        bool wait;
        /**
         * Indicates whether a frame retransmission should be performed by FOP-1
         * @see p. 6.1.5 of COP-1 CCSDS
         */
        bool retransmit;
        /**
         * Goes up by one each time a TYPE-BC pr TYPE-BD frame is received
         * @see p. 6.1.6 of COP-1 CCSDS
         */
        uint8_t farmBCount;
        /**
         * The expected sequence number of the next TYPE-AD frame
         * @see p. 6.1.7 of COP-1 CCSDS
         */
        uint8_t receiverFrameSeqNumber;

        /**
         * These variables are used to determine when the state machine should enter lockout
         * mode
         * @see p. 6.1.8 of COP-1 CCSDS
         */
        const uint8_t farmSlidingWinWidth;
        const uint8_t farmPositiveWinWidth;
        const uint8_t farmNegativeWidth;

        /**
         * The amount of time (in milliseconds) that must elapse before another CLCW report is
         * pushed. It is not required by the protocol to have a constant CLCW data rate, therefore a
         * simple countdown timer can be used (send a CLCW once it has elapsed).
         *
         * // TODO For now, the x86 countdown timer from fop is used.
         *         Create a countdown timer implementation using freertos.
         */
        const uint16_t clcwReportInterval;
        CountdownTimer timer;

        /** Implementation specific variables **/

        /**
         * virtual channel parameters passed upon construction
         */
        uint8_t vid;
        bool errorControlFieldPresent;

        /**
         * FARM-1 examines frames from this buffer
         */
        etl::list<TransferFrameTC *, MaxReceivedRxTcInFARMSentQueue> &lowerLayerBuffer;

        /**
         * References to buffers FARM-1 will place the accepted frames to.
         */
        etl::circular_buffer<TransferFrameTC *, MaxReceivedRxTcInVirtualChannelBuffer> &higherLayerBufferTypeBD;
        etl::list<TransferFrameTC *, MaxReceivedRxTcInVirtualChannelBuffer> &higherLayerBufferTypeAD;

        /**
         * In order to be able to:
         * - Reject TYPE-AD frames
         * - Overwrite TYPE-BD frames
         * - Delete TYPE-BC frames
         *
         * FARM-1 needs to have a reference to the frame master copy buffer and the frame octet memory pool
         */
        etl::list<TransferFrameTC, MaxTxInMasterChannel> &frameMasterCopyBuffer;
        MemoryPool &memoryPool;

        /**
         * A buffer of space 1, to store CLCW reports.
         */
        etl::queue<CLCW, 1>* clcwBuffer;
	    void registerClcwOutputBuffer(etl::queue<CLCW, 1>* buf) {
		    clcwBuffer = buf;
	    }

        /** FARM-1 actions **/

        /**
         * Accepts a frame and passes it to the higher layer buffers. If the frame
         * service type is BD and the BD buffers are full, the oldest BD frame is overwritten.
         *
         * @see p. 6.2.2 of COP-1 CCSDS
         */
        FARMNotification accept(TransferFrameTC *frame, ServiceType serviceType);

        /**
         * Deletes frame master copy and octets.
         *
         * @see p. 6.2.3 of COP-1 CCSDS
         */
        void discard(TransferFrameTC *frame);

        /**
         * Creates a CLCW report based on the current state machine variable values and
         * pushes it to the CLCW buffer. Since only the most recent state is of interest,
         * the precious CLCW (if it exists) is overwritten.
         *
         * @see p. 6.2.4 of COP-1 CCSDS
         */
        void report();

        /** Implementation specific methods **/

        /**
         * Returns whether the frame sequence number N(S) is within the positive-negative window or outside
         * of those windows. This is used for events E3,E4,E5.
         *
         * @see figure 6-1 of COP-1 CCSDS
         */
        [[nodiscard]] Window getWindow(uint8_t frameSeqNumber) const;

        /**
         * Applies the FARM-1 state table.
         * @see table 6-1 from COP-1 CCSDS
         *
         * @returns The occurred event code. An event code of 0 means no event was
         *          detected.
         */
        std::pair<FARMNotification, uint8_t> applyFarmStateTable();

        /**
         * Due to the COP-1 arithmetic being mod 256, it is possible in the inequality:
         * lowerBound <= value <= upperBound
         * for upperBound to be numerically smaller than lower bound (wraparound). In order to make this concept
         * more clear, diagrams exist in the wiki, under section: 'Circular arithmetic'.
         *
         * @returns True, if the given value is within the window or it's edges. Otherwise, false is returned.
         */
        static bool withinWindow(uint8_t value, uint8_t lowerBound, uint8_t upperBound) {
            if (upperBound < lowerBound) { // wraparound
                // The window region consists of 2 subregions: [lowerBound, 255] and [0, upperBound]
                return ((value >= lowerBound) && (value <= 255)) || ((value >= 0) && (value <= upperBound));
            } else {  // normal comparison
                return (value >= lowerBound) && (value <= upperBound);
            }
        }

    public:
        FrameAcceptanceReporting(uint8_t vid,
                                 bool errorControlFieldPresent,
                                 etl::list<TransferFrameTC *, MaxReceivedRxTcInFARMSentQueue> &lowerLayerBuffer,
                                 etl::circular_buffer<TransferFrameTC *, MaxReceivedRxTcInVirtualChannelBuffer> &higherLayerBufferTypeBD,
                                 etl::list<TransferFrameTC *, MaxReceivedRxTcInVirtualChannelBuffer> &higherLayerBufferTypeAD,
                                 etl::list<TransferFrameTC, MaxTxInMasterChannel> &frameMasterCopyBuffer,
                                 MemoryPool &memoryPool,
                                 uint8_t farmSlidingWinWidth = FarmSlidingWinLength,
                                 uint8_t farmPositiveWinWidth = FarmPositiveWinLength,
                                 uint8_t farmNegativeWinWidth = FarmNegativeWinLength,
                                 uint16_t clcwReportInterval = ClcwReportInterval)
                : vid(vid), errorControlFieldPresent(errorControlFieldPresent), lowerLayerBuffer(lowerLayerBuffer),
                  higherLayerBufferTypeBD(higherLayerBufferTypeBD),
                  higherLayerBufferTypeAD(higherLayerBufferTypeAD), frameMasterCopyBuffer(frameMasterCopyBuffer),
                  memoryPool(memoryPool),
                  farmSlidingWinWidth(farmSlidingWinWidth),
                  farmPositiveWinWidth(farmPositiveWinWidth),
                  farmNegativeWidth(farmNegativeWinWidth), receiverFrameSeqNumber(0), farmBCount(0),
                  lockout(false),
                  wait(false), retransmit(false), state(FARMState::OPEN), timer(CountdownTimer()),
                  clcwReportInterval(clcwReportInterval) {};
    };
#endif // SPACE_SEGMENT
} // namespace CCSDSDataLinkLayer