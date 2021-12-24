#ifndef CCSDS_TM_PACKETS_FARMEACCEPTANCEREPORTING_HPP
#define CCSDS_TM_PACKETS_FARMEACCEPTANCEREPORTING_HPP

#pragma once

#include <cstdint>
#include <PacketTC.hpp>
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

class VirtualChannel;

class MAPChannel;

/**
 * @see p. 6 from COP-1 CCSDS
 */
class FarmAcceptanceReporting {
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
	 * @brief FARM actions according to the table 6-1
	 * @see p. 6.2-6.3 and table 6-1 from COP-1 CCSDS
	 */
    COPDirectiveResponse frameArrives();

	/**
	 * @brief signals when sufficient buffer space becomes available for at least one more maximum-size Frame.
	 * @see p. 6.3.2.3 from COP-1 CCSDS
	 */
    COPDirectiveResponse bufferRelease();

	/**
	 * @brief Buffer for storing packets, BEFORE being processed by FARM.
	 */
    etl::list<PacketTC *, MAX_RECEIVED_RX_TC_IN_WAIT_QUEUE> *waitQueue;
	/**
	 * @brief Buffer for storing packets, AFTER being processed by FARM.
	 */
    etl::list<PacketTC *, MAX_RECEIVED_RX_TC_IN_SENT_QUEUE> *sentQueue;

	/**
	 * @brief The Virtual Channel in which FOP is initialized
	 */
    VirtualChannel *vchan;

    FarmAcceptanceReporting(VirtualChannel *vchan, etl::list<PacketTC *, MAX_RECEIVED_RX_TC_IN_WAIT_QUEUE> *waitQueue,
                            etl::list<PacketTC *, MAX_RECEIVED_RX_TC_IN_SENT_QUEUE> *sentQueue,
                            const uint8_t farm_sliding_win_width, const uint8_t farm_positive_win_width,
                            const uint8_t farm_negative_win_width)
            : waitQueue(waitQueue), sentQueue(sentQueue), vchan(vchan), farmSlidingWinWidth(farm_sliding_win_width),
              farmNegativeWidth(farm_negative_win_width), farmPositiveWinWidth(farm_positive_win_width),
              receiverFrameSeqNumber(0), farmBCount(0), lockout(FlagState::NOT_READY), wait(FlagState::NOT_READY),
              retransmit(FlagState::NOT_READY) {};
};

#endif // CCSDS_TM_PACKETS_FARMEACCEPTANCEREPORTING_HPP
