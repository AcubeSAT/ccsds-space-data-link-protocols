#ifndef CCSDS_TM_PACKETS_FRAMEACCEPTANCEREPORTING_HPP
#define CCSDS_TM_PACKETS_FRAMEACCEPTANCEREPORTING_HPP

#pragma once

#include <cstdint>
#include <TransferFrameTC.hpp>
#include <etl/list.h>
#include <Alert.hpp>

/**
 * @see p. 6.1.2 from COP-1 CCSDS
 */
enum FARMState {
	OPEN = 1,
	WAIT = 2,
	LOCKOUT = 3,
};

template <uint16_t t> class VirtualChannel;

template <uint16_t t> class MAPChannel;

/**
 * @see p. 6 from COP-1 CCSDS
 */

template <uint16_t t>
class FrameAcceptanceReporting {
	friend class ServiceChannel;

private:
	FARMState state;
	FlagState lockout;
	FlagState wait;
	FlagState retransmit;
	uint16_t receiverFrameSeqNumber;
	uint16_t farmBCount;
	const uint8_t farmSlidingWinWidth;
	const uint8_t farmPositiveWinWidth;
	const uint8_t farmNegativeWidth;

public:
	/* Directives */

	/**
	 * FARM actions according to the table 6-1
	 * @see p. 6.2-6.3 and table 6-1 from COP-1 CCSDS
	 */
	COPDirectiveResponse frameArrives();

	/**
	 * signals when sufficient buffer space becomes available for at least one more maximum-size Frame.
	 * @see p. 6.3.2.3 from COP-1 CCSDS
	 */
	COPDirectiveResponse bufferRelease();

	/**
	 * Buffer for storing packets, BEFORE being processed by FARM.
	 */
	etl::list<TransferFrameTC*, t>* waitQueue;
	/**
	 * Buffer for storing packets, AFTER being processed by FARM.
	 */
	etl::list<TransferFrameTC*, t>* sentQueue;

	/**
	 * The Virtual Channel in which FOP is initialized
	 */
	VirtualChannel<256>* vchan;

	FrameAcceptanceReporting(VirtualChannel<256>* vchan, etl::list<TransferFrameTC*, t>* waitQueue,
	                         etl::list<TransferFrameTC*, t>* sentQueue,
	                         const uint8_t farmSlidingWinWidth, const uint8_t farmPositiveWinWidth,
	                         const uint8_t farmNegativeWinWidth)
	    : waitQueue(waitQueue), sentQueue(sentQueue), vchan(vchan), farmSlidingWinWidth(farmSlidingWinWidth),
	      farmNegativeWidth(farmNegativeWinWidth), farmPositiveWinWidth(farmPositiveWinWidth),
	      receiverFrameSeqNumber(0), farmBCount(0), lockout(FlagState::NOT_READY), wait(FlagState::NOT_READY),
	      retransmit(FlagState::NOT_READY), state(FARMState::OPEN){};
};

#endif // CCSDS_TM_PACKETS_FRAMEACCEPTANCEREPORTING_HPP
