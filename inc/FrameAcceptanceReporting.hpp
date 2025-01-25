#pragma once

#include <cstdint>
#include <TransferFrameTC.hpp>
#include <etl/list.h>
#include <Alert.hpp>
#include <CCSDS_Definitions.hpp>

/**
 * @see p. 6.1.2 from COP-1 CCSDS
 */
enum FARMState {
	OPEN = 1,
	WAIT = 2,
	LOCKOUT = 3,
};

class VirtualChannel;

class MAPChannel;

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
    friend class ServiceChannel;
    friend class MasterChannel;

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
	FlagState lockout;
    /**
     * Indicates if state machine is in wait mode
     * @see p. 6.1.4 of COP-1 CCSDS
     */
	FlagState wait;
    /**
     * Indicates whether a frame retransmission should be performed by FOP-1
     * @see p. 6.1.5 of COP-1 CCSDS
     */
	FlagState retransmit;
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

    /** Implementation specific variables **/

    /**
     * FARM-1 examines frames from this buffer
     */
    etl::list<TransferFrameTC*, MaxReceivedRxTcInFARMSentQueue>& lowerLayerBuffer;

    /**
     * References to buffers FARM-1 will place the accepted frames to.
     */
    etl::list<TransferFrameTC*, MaxReceivedRxTcInVirtualChannelBuffer>& higherLayerBufferTypeBD;
    etl::list<TransferFrameTC*, MaxReceivedRxTcInVirtualChannelBuffer>& higherLayerBufferTypeAD;

    /**
     * In order to be able to:
     * - Reject TYPE-AD frames
     * - Overwrite TYPE-BD frames
     * - Delete TYPE-BC frames
     *
     * FARM-1 needs to have a reference to the frame master copy buffer and the frame octet memory pool
     */
    etl::list<TransferFrameTC, MaxTxInMasterChannel>& frameMasterCopyBuffer;
    MemoryPool& memoryPool;

    /**
     * A buffer of space 1, to store CLCW reports.
     */
     etl::list<CLCW, 1> clcwBuffer;

    /** FARM-1 actions **/

    /**
     * Accepts a frame and passes it to the higher layer buffers. If the frame
     * service type is BD and the BD buffers are full, the oldest BD frame is overwritten.
     *
     * @see p. 6.2.2 of COP-1 CCSDS
     */
    void accept(TransferFrameTC* frame, ServiceType serviceType);

    /**
     * Deletes TYPE-AD frame master copy and octets.
     *
     * @see p. 6.2.3 of COP-1 CCSDS
     */
    void discard(TransferFrameTC* frame);

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
     * Applies the FARM-1 state table.
     * @see table 6-1 from COP-1 CCSDS
     *
     * @returns The event code detected. An event code of 0 means no event.
     */
    std::pair<FARMNotification, uint8_t> applyFarmStateTable();

public:

	/**
	 * The Virtual Channel in which FOP is initialized
	 */
	VirtualChannel* vchan;

	FrameAcceptanceReporting(VirtualChannel* vchan,
                             etl::list<TransferFrameTC*,MaxReceivedRxTcInFARMSentQueue>& lowerLayerBuffer,
                             etl::list<TransferFrameTC*, MaxReceivedRxTcInVirtualChannelBuffer>& higherLayerBufferTypeBD,
                             etl::list<TransferFrameTC*, MaxReceivedRxTcInVirtualChannelBuffer>& higherLayerBufferTypeAD,
                             etl::list<TransferFrameTC, MaxTxInMasterChannel>& frameMasterCopyBuffer,
                             MemoryPool& memoryPool,
                             uint8_t farmSlidingWinWidth = FarmSlidingWinLength,
                             uint8_t farmPositiveWinWidth = FarmPositiveWinLength,
                             uint8_t farmNegativeWinWidth = FarmNegativeWinLength)
	    : vchan(vchan), lowerLayerBuffer(lowerLayerBuffer), higherLayerBufferTypeBD(higherLayerBufferTypeBD),
        higherLayerBufferTypeAD(higherLayerBufferTypeAD), frameMasterCopyBuffer(frameMasterCopyBuffer), memoryPool(memoryPool),
        farmSlidingWinWidth(farmSlidingWinWidth), farmPositiveWinWidth(farmPositiveWinWidth),
        farmNegativeWidth(farmNegativeWinWidth), receiverFrameSeqNumber(0), farmBCount(0), lockout(FlagState::NOT_READY),
        wait(FlagState::NOT_READY), retransmit(FlagState::NOT_READY), state(FARMState::OPEN) {};
};