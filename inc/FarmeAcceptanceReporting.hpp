#ifndef CCSDS_TM_PACKETS_FARMEACCEPTANCEREPORTING_HPP
#define CCSDS_TM_PACKETS_FARMEACCEPTANCEREPORTING_HPP

#pragma once

#include <cstdint>
#include <TransferFrameTC.hpp>
#include <etl/list.h>
#include <Alert.hpp>

enum FARMState {
    OPEN = 1,
    WAIT = 2,
    LOCKOUT = 3,
};

class VirtualChannel;

class MAPChannel;

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
    COPDirectiveResponse frame_arrives();

    COPDirectiveResponse buffer_release();

    etl::list<TransferFrameTC*, max_received_rx_tc_in_wait_queue> *waitQueue;
    etl::list<TransferFrameTC*, max_received_rx_tc_in_sent_queue> *sentQueue;
    VirtualChannel *vchan;

    FarmAcceptanceReporting(VirtualChannel *vchan, etl::list<TransferFrameTC*, max_received_rx_tc_in_wait_queue> *waitQueue,
                            etl::list<TransferFrameTC*, max_received_rx_tc_in_sent_queue> *sentQueue,
                            const uint8_t farm_sliding_win_width, const uint8_t farm_positive_win_width,
                            const uint8_t farm_negative_win_width)
            : waitQueue(waitQueue), sentQueue(sentQueue), vchan(vchan), farmSlidingWinWidth(farm_sliding_win_width),
              farmNegativeWidth(farm_negative_win_width), farmPositiveWinWidth(farm_positive_win_width),
              receiverFrameSeqNumber(0), farmBCount(0), lockout(FlagState::NOT_READY), wait(FlagState::NOT_READY),
              retransmit(FlagState::NOT_READY) {};
};

#endif // CCSDS_TM_PACKETS_FARMEACCEPTANCEREPORTING_HPP
