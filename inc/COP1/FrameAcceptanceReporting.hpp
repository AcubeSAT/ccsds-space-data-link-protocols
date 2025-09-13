/**
 * @file FrameAcceptanceReporting.hpp
 * @brief Space segment component of COP-1
 */

#pragma once
#include <cstdint>
#include "etl/queue.h"
#include "etl/optional.h"
#include "CountdownTimer.hpp"
#include "CLCW.hpp"
#include "DataLinkNotifications.hpp"
#include "CcsdsDefinitions.hpp"
#include "VirtualChannel.hpp"

namespace CCSDSDataLinkLayer {
#ifdef INCLUDE_SPACE_SEGMENT_CODE
    class SpaceSegmentTcServices;
    class SpaceSegmentTcDataHandling;

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
    public:
        FrameAcceptanceReporting(const Defs::Vcid vcid,
                                 const uint8_t farmSlidingWinWidth,
                                 const uint16_t clcwReportInterval,
                                 const uint8_t fopTransmissionLimit);

        /**
         * Applies the FARM-1 state table.
         * @see table 6-1 from COP-1 CCSDS
         *
         * @returns The occurred event code. An event code of 0 means no event was
         *          detected.
         */
        etl::pair<FARMNotification, uint8_t> applyFarmStateTable();

        void resetFARM();

        /**
         * Generated CLCWs are placed here. Since we only care about the most recent state of farm, this is a length
         * one buffer.
         */
        etl::optional<CLCW> clcwBuffer;
        Mutex clcwBufferMutex;

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
         */
        const uint16_t clcwReportInterval;
        CountdownTimer timer;

        VirtualChannelSsTc& vcChan;

        /** FARM-1 actions **/

        /**
         * Accepts a frame and passes it to the higher layer buffers. If the frame
         * service type is BD and the BD buffers are full, the oldest BD frame is overwritten.
         *
         * @see p. 6.2.2 of COP-1 CCSDS
         */
        FARMNotification accept(TransferFrameTC *frame);

        /**
         * Deletes frame master copy and octets.
         *
         * @see p. 6.2.3 of COP-1 CCSDS
         */
        FARMNotification discard(TransferFrameTC *frame);

        /**
         * Creates a CLCW report based on the current state machine variable values and
         * pushes it to the CLCW buffer. Since only the most recent state is of interest,
         * the precious CLCW (if it exists) is overwritten.
         *
         * @see p. 6.2.4 of COP-1 CCSDS
         */
        FARMNotification report();

        /** Implementation specific methods **/

        /**
         * Returns whether the frame sequence number N(S) is within the positive-negative window or outside
         * of those windows. This is used for events E3,E4,E5.
         *
         * @see figure 6-1 of COP-1 CCSDS
         */
        [[nodiscard]] Window getWindow(uint8_t frameSeqNumber) const;
    };
#endif // INCLUDE_SPACE_SEGMENT_CODE
} // namespace CCSDSDataLinkLayer
