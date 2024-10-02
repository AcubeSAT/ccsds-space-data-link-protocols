#include <CCSDSChannel.hpp>
#include <Alert.hpp>
#include "CCSDSLoggerImpl.h"
#include "MemoryPool.hpp"
// Virtual Channel

VirtualChannelAlert VirtualChannel::storeVC(TransferFrameTC* transferFrameTc) {
	// Limit the amount of transfer frames that can be stored at any given time
	if (unprocessedFrameListBufferTxTC.full()) {
		ccsdsLogNotice(Tx, TypeVirtualChannelAlert, TX_WAIT_QUEUE_FULL);
		return VirtualChannelAlert::TX_WAIT_QUEUE_FULL;
	}
	unprocessedFrameListBufferTxTC.push_back(transferFrameTc);
	ccsdsLogNotice(Tx, TypeVirtualChannelAlert, NO_VC_ALERT);
	return VirtualChannelAlert::NO_VC_ALERT;
}
// Master Channel

// Technically not a transfer frame, but it has identical information
// @todo consider another data structure

MasterChannelAlert MasterChannel::storeOut(TransferFrameTC* transferFrameTc) {
	if (outFramesBeforeAllFramesGenerationListTxTC.full()) {
		// Log that buffer is full
		ccsdsLogNotice(Tx, TypeMasterChannelAlert, OUT_FRAMES_LIST_FULL);
		return MasterChannelAlert::OUT_FRAMES_LIST_FULL;
	}
	outFramesBeforeAllFramesGenerationListTxTC.push_back(transferFrameTc);
	uint8_t vid = transferFrameTc->virtualChannelId();
	// virtChannels.at(0).fop.
	ccsdsLogNotice(Tx, TypeMasterChannelAlert, NO_MC_ALERT);
	return MasterChannelAlert::NO_MC_ALERT;
}

MasterChannelAlert MasterChannel::storeOut(TransferFrameTM* transferFrameTm) {
	if (txOutFramesBeforeAllFramesGenerationListTM.full()) {
		// Log that buffer is full
		ccsdsLogNotice(Tx, TypeMasterChannelAlert, OUT_FRAMES_LIST_FULL);
		return MasterChannelAlert::OUT_FRAMES_LIST_FULL;
	}
	txOutFramesBeforeAllFramesGenerationListTM.push_back(transferFrameTm);
	ccsdsLogNotice(Tx, TypeMasterChannelAlert, NO_MC_ALERT);
	return MasterChannelAlert::NO_MC_ALERT;
}

MasterChannelAlert MasterChannel::storeTransmittedOut(TransferFrameTC* transferFrameTc) {
	if (toBeTransmittedFramesAfterAllFramesGenerationListTxTC.full()) {
		ccsdsLogNotice(Tx, TypeMasterChannelAlert, TO_BE_TRANSMITTED_FRAMES_LIST_FULL);
		return MasterChannelAlert::TO_BE_TRANSMITTED_FRAMES_LIST_FULL;
	}
    toBeTransmittedFramesAfterAllFramesGenerationListTxTC.push_back(transferFrameTc);
	ccsdsLogNotice(Tx, TypeMasterChannelAlert, NO_MC_ALERT);
	return MasterChannelAlert::NO_MC_ALERT;
}

MasterChannelAlert MasterChannel::storeTransmittedOut(TransferFrameTM* transferFrameTm) {
	if (toBeTransmittedFramesAfterAllFramesGenerationListTxTM.full()) {
		ccsdsLogNotice(Tx, TypeMasterChannelAlert, TO_BE_TRANSMITTED_FRAMES_LIST_FULL);
		return MasterChannelAlert::TO_BE_TRANSMITTED_FRAMES_LIST_FULL;
	}
	toBeTransmittedFramesAfterAllFramesGenerationListTxTM.push_back(transferFrameTm);
	ccsdsLogNotice(Tx, TypeMasterChannelAlert, NO_MC_ALERT);
	return MasterChannelAlert::NO_MC_ALERT;
}

MasterChannelAlert MasterChannel::addVC(const uint8_t vcid, const bool segmentHeaderPresent, const uint16_t maxFrameLength, const bool blockingTM,
                                        const bool segmentationTM, const bool blockingTC,
                                        const uint8_t repetitionTypeAFrame, const uint8_t repetitionTypeBFrame,
                                        const bool frameErrorControlFieldPresent, const bool secondaryHeaderTMPresent,
                                        const uint8_t secondaryHeaderTMLength,
                                        const bool operationalControlFieldTMPresent,
                                        SynchronizationFlag synchronization, const uint8_t farmSlidingWinWidth,
                                        const uint8_t farmPositiveWinWidth, const uint8_t farmNegativeWinWidth, const uint8_t vcRepetitions,
                                        const etl::flat_map<uint8_t, MAPChannel, MaxMapChannels> mapChan) {

	if (virtualChannels.full()) {
		ccsdsLogNotice(Tx, TypeMasterChannelAlert, MAX_AMOUNT_OF_VIRT_CHANNELS);
		return MasterChannelAlert::MAX_AMOUNT_OF_VIRT_CHANNELS;
	}

	virtualChannels.emplace(vcid, VirtualChannel(*this, vcid, segmentHeaderPresent, maxFrameLength, blockingTM, segmentationTM,
                                                 blockingTC, repetitionTypeAFrame,
	                                             repetitionTypeBFrame, secondaryHeaderTMPresent, secondaryHeaderTMLength,
	                                             operationalControlFieldTMPresent, frameErrorControlFieldPresent,
	                                             synchronization, farmSlidingWinWidth, farmPositiveWinWidth,
	                                             farmNegativeWinWidth, vcRepetitions, mapChan));
	return MasterChannelAlert::NO_MC_ALERT;
}

MasterChannelAlert MasterChannel::addVC(const uint8_t vcid, const bool segmentHeaderPresent, const uint16_t maxFrameLength, const bool blockingTM,
                                        const bool segmentationTM, const bool blockingTC,
                                        const uint8_t repetitionTypeAFrame, const uint8_t repetitionCopCtrl,
                                        const bool frameErrorControlFieldPresent, const bool secondaryHeaderTMPresent,
                                        const uint8_t secondaryHeaderTMLength,
                                        const bool operationalControlFieldTMPresent,
                                        SynchronizationFlag synchronization, const uint8_t farmSlidingWinWidth,
                                        const uint8_t farmPositiveWinWidth, const uint8_t farmNegativeWinWidth, const uint8_t vcRepetitions) {
	if (virtualChannels.full()) {
		ccsdsLogNotice(Tx, TypeMasterChannelAlert, MAX_AMOUNT_OF_VIRT_CHANNELS);
		return MasterChannelAlert::MAX_AMOUNT_OF_VIRT_CHANNELS;
	}

	virtualChannels.emplace(vcid,
	                        VirtualChannel(*this, vcid, segmentHeaderPresent, maxFrameLength, blockingTM,
                                           segmentationTM, blockingTC, repetitionTypeAFrame,
	                                       repetitionCopCtrl, secondaryHeaderTMPresent, secondaryHeaderTMLength,
	                                       frameErrorControlFieldPresent, operationalControlFieldTMPresent,
	                                       synchronization, farmSlidingWinWidth, farmPositiveWinWidth,
	                                       farmNegativeWinWidth, vcRepetitions, etl::flat_map<uint8_t, MAPChannel, MaxMapChannels>()));
	return MasterChannelAlert::NO_MC_ALERT;
}

void MasterChannel::removeMasterTx(TransferFrameTC* frame_ptr) {
	etl::list<TransferFrameTC, MaxTxInMasterChannel>::iterator it;
	for (it = masterCopyTxTC.begin(); it != masterCopyTxTC.end(); ++it) {
		if (&it == frame_ptr) {
			masterCopyTxTC.erase(it);
			return;
		}
	}
}

void MasterChannel::removeMasterTx(TransferFrameTM* frame_ptr) {
	etl::list<TransferFrameTM, MaxTxInMasterChannel>::iterator it;
	for (it = masterCopyTxTM.begin(); it != masterCopyTxTM.end(); ++it) {
		if (&it == frame_ptr) {
			masterCopyTxTM.erase(it);
			return;
		}
	}
}

void MasterChannel::removeMasterRx(TransferFrameTC* frame_ptr) {
	etl::list<TransferFrameTC, MaxRxInMasterChannel>::iterator it;
	for (it = masterCopyRxTC.begin(); it != masterCopyRxTC.end(); ++it) {
		if (&it == frame_ptr) {
			masterCopyRxTC.erase(it);
			return;
		}
	}
}

void MasterChannel::removeMasterRx(TransferFrameTM* frame_ptr) {
	etl::list<TransferFrameTM, MaxRxInMasterChannel>::iterator it;
	for (it = masterCopyRxTM.begin(); it != masterCopyRxTM.end(); ++it) {
		if (&it == frame_ptr) {
			masterCopyRxTM.erase(it);
			return;
		}
	}
}
void MasterChannel::acknowledgeFrame(uint8_t frameSequenceNumber) {
	for (TransferFrameTC& transferFrame : masterCopyTxTC) {
		if (transferFrame.transferFrameSequenceNumber() == frameSequenceNumber) {
			transferFrame.setAcknowledgement(true);
			return;
		}
	}
}

void MasterChannel::setRetransmitFrame(uint8_t frameSequenceNumber) {
	for (TransferFrameTC transferFrame : masterCopyTxTC) {
		if (transferFrame.transferFrameSequenceNumber() == frameSequenceNumber) {
			transferFrame.setToBeRetransmitted(true);
			return;
		}
	}
}

TransferFrameTC MasterChannel::getLastTxMasterCopyTcFrame() {
	return masterCopyTxTC.back();
}

TransferFrameTC MasterChannel::geFirstTxMasterCopyTcFrame() {
	return masterCopyTxTC.front();
}