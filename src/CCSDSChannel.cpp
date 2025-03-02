#include "CCSDSChannel.hpp"
#include "Alert.hpp"
#include "CCSDSLoggerImpl.h"
#include "MemoryPool.hpp"

namespace CCSDSDataLinkLayer {

    MasterChannelAlert
    MasterChannel::addVC(const uint8_t vcid, const bool segmentHeaderPresent, const uint16_t maxFrameLength,
                         const bool blockingTM,
                         const bool segmentationTM, const bool blockingTC,
                         const bool frameErrorControlFieldPresent, const bool secondaryHeaderTMPresent,
                         const uint8_t secondaryHeaderTMLength,
                         const bool operationalControlFieldTMPresent,
                         SynchronizationFlag synchronization, const uint8_t farmSlidingWinWidth,
                         const uint8_t farmPositiveWinWidth, const uint8_t farmNegativeWinWidth,
                         const uint8_t vcRepetitions,
                         const etl::flat_map<uint8_t, MAPChannel, MaxMapChannels> mapChan) {

        if (virtualChannels.full()) {
            ccsdsLogNotice(TxRx::Tx, NotificationType::TypeMasterChannelAlert, MasterChannelAlert::MAX_AMOUNT_OF_VIRT_CHANNELS);
            return MasterChannelAlert::MAX_AMOUNT_OF_VIRT_CHANNELS;
        }

        virtualChannelClcwQueues.emplace(vcid, etl::queue<CLCW, 1>());

        virtualChannels.emplace(vcid,
                                VirtualChannel(vcid, segmentHeaderPresent, maxFrameLength, blockingTM, segmentationTM,
                                               blockingTC, secondaryHeaderTMPresent, secondaryHeaderTMLength,
                                               operationalControlFieldTMPresent, frameErrorControlFieldPresent,
                                               synchronization, farmSlidingWinWidth, farmPositiveWinWidth,
                                               farmNegativeWinWidth, vcRepetitions, mapChan, virtualChannelClcwQueues,
                                               masterCopyRxTC, masterCopyTxTC, masterChannelPoolTxTC,
                                               masterChannelPoolRxTC));


        return MasterChannelAlert::NO_MC_ALERT;
    }

    MasterChannelAlert
    MasterChannel::addVC(const uint8_t vcid, const bool segmentHeaderPresent, const uint16_t maxFrameLengthTC,
                         const bool blockingTM,
                         const bool segmentationTM, const bool blockingTC, const bool frameErrorControlFieldPresent,
                         const bool secondaryHeaderTMPresent,
                         const uint8_t secondaryHeaderTMLength,
                         const bool operationalControlFieldTMPresent,
                         SynchronizationFlag synchronization, const uint8_t farmSlidingWinWidth,
                         const uint8_t farmPositiveWinWidth, const uint8_t farmNegativeWinWidth,
                         const uint8_t vcRepetitions) {
        if (virtualChannels.full()) {
            ccsdsLogNotice(TxRx::Tx, NotificationType::TypeMasterChannelAlert, MasterChannelAlert::MAX_AMOUNT_OF_VIRT_CHANNELS);
            return MasterChannelAlert::MAX_AMOUNT_OF_VIRT_CHANNELS;
        }

        virtualChannelClcwQueues.emplace(vcid, etl::queue<CLCW, 1>());

        virtualChannels.emplace(vcid,
                                VirtualChannel(vcid, segmentHeaderPresent, maxFrameLengthTC, blockingTM,
                                               segmentationTM, blockingTC, secondaryHeaderTMPresent,
                                               secondaryHeaderTMLength,
                                               frameErrorControlFieldPresent, operationalControlFieldTMPresent,
                                               synchronization, farmSlidingWinWidth, farmPositiveWinWidth,
                                               farmNegativeWinWidth, vcRepetitions,
                                               etl::flat_map<uint8_t, MAPChannel, MaxMapChannels>(),
                                               virtualChannelClcwQueues, masterCopyRxTC, masterCopyTxTC,
                                               masterChannelPoolTxTC,
                                               masterChannelPoolRxTC));

        return MasterChannelAlert::NO_MC_ALERT;
    }

    void MasterChannel::removeMasterTxTC(TransferFrameTC *frame_ptr) {
        etl::list<TransferFrameTC, MaxTxInMasterChannel>::iterator it;
        for (it = masterCopyTxTC.begin(); it != masterCopyTxTC.end(); ++it) {
            if (&it == frame_ptr) {
                masterCopyTxTC.erase(it);
                return;
            }
        }
    }

    void MasterChannel::removeMasterTxTM(TransferFrameTM *frame_ptr) {
        etl::list<TransferFrameTM, MaxTxInMasterChannel>::iterator it;
        for (it = masterCopyTxTM.begin(); it != masterCopyTxTM.end(); ++it) {
            if (&it == frame_ptr) {
                masterCopyTxTM.erase(it);
                return;
            }
        }
    }

    void MasterChannel::removeMasterRxTC(TransferFrameTC *frame_ptr) {
        etl::list<TransferFrameTC, MaxRxInMasterChannel>::iterator it;
        for (it = masterCopyRxTC.begin(); it != masterCopyRxTC.end(); ++it) {
            if (&it == frame_ptr) {
                masterCopyRxTC.erase(it);
                return;
            }
        }
    }

// RxTM chain
//    void MasterChannel::removeMasterRxTM(TransferFrameTM *frame_ptr) {
//        etl::list<TransferFrameTM, MaxRxInMasterChannel>::iterator it;
//        for (it = masterCopyRxTM.begin(); it != masterCopyRxTM.end(); ++it) {
//            if (&it == frame_ptr) {
//                masterCopyRxTM.erase(it);
//                return;
//            }
//        }
//    }
} // namespace CCSDSDataLinkLayer