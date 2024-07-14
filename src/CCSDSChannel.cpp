#include <CCSDSChannel.hpp>
#include <Alert.hpp>
#include "CCSDSLoggerImpl.h"
#include "MemoryPool.hpp"
// Virtual Channel

VirtualChannelAlert VirtualChannel::storeVC(TransferFrameTC* transferFrameTc) {
	// Limit the amount of transfer frames that can be stored at any given time
	if (txUnprocessedFrameListBufferTC.full()) {
		ccsdsLogNotice(Tx, TypeVirtualChannelAlert, TX_WAIT_QUEUE_FULL);
		return VirtualChannelAlert::TX_WAIT_QUEUE_FULL;
	}
	txUnprocessedFrameListBufferTC.push_back(transferFrameTc);
	ccsdsLogNotice(Tx, TypeVirtualChannelAlert, NO_VC_ALERT);
	return VirtualChannelAlert::NO_VC_ALERT;
}
// Master Channel

// Technically not a transferFrameData, but it has identical information
// @todo consider another data structure

MasterChannelAlert MasterChannel::storeOut(TransferFrameTC* transferFrameTc) {
	if (txOutFramesBeforeAllFramesGenerationListTC.full()) {
		// Log that buffer is full
		ccsdsLogNotice(Tx, TypeMasterChannelAlert, OUT_FRAMES_LIST_FULL);
		return MasterChannelAlert::OUT_FRAMES_LIST_FULL;
	}
	txOutFramesBeforeAllFramesGenerationListTC.push_back(transferFrameTc);
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
	if (txToBeTransmittedFramesAfterAllFramesGenerationListTC.full()) {
		ccsdsLogNotice(Tx, TypeMasterChannelAlert, TO_BE_TRANSMITTED_FRAMES_LIST_FULL);
		return MasterChannelAlert::TO_BE_TRANSMITTED_FRAMES_LIST_FULL;
	}
    txToBeTransmittedFramesAfterAllFramesGenerationListTC.push_back(transferFrameTc);
	ccsdsLogNotice(Tx, TypeMasterChannelAlert, NO_MC_ALERT);
	return MasterChannelAlert::NO_MC_ALERT;
}

MasterChannelAlert MasterChannel::storeTransmittedOut(TransferFrameTM* transferFrameTm) {
	if (txToBeTransmittedFramesAfterAllFramesGenerationListTM.full()) {
		ccsdsLogNotice(Tx, TypeMasterChannelAlert, TO_BE_TRANSMITTED_FRAMES_LIST_FULL);
		return MasterChannelAlert::TO_BE_TRANSMITTED_FRAMES_LIST_FULL;
	}
	txToBeTransmittedFramesAfterAllFramesGenerationListTM.push_back(transferFrameTm);
	ccsdsLogNotice(Tx, TypeMasterChannelAlert, NO_MC_ALERT);
	return MasterChannelAlert::NO_MC_ALERT;
}

MasterChannelAlert MasterChannel::addVC(const uint8_t vcid, const uint16_t maxFrameLength, const bool blocking,
                                        const uint8_t repetitionTypeAFrame, const uint8_t repetitionTypeBFrame,
                                        const bool frameErrorControlFieldPresent, const bool secondaryHeaderTMPresent,
                                        const uint8_t secondaryHeaderTMLength,
                                        const bool operationalControlFieldTMPresent,
                                        SynchronizationFlag synchronization, const uint8_t farmSlidingWinWidth,
                                        const uint8_t farmPositiveWinWidth, const uint8_t farmNegativeWinWidth, const uint8_t vcRepetitions,
                                        etl::flat_map<uint8_t, MAPChannel, MaxMapChannels> mapChan) {
	if (virtualChannels.full()) {
		ccsdsLogNotice(Tx, TypeMasterChannelAlert, MAX_AMOUNT_OF_VIRT_CHANNELS);
		return MasterChannelAlert::MAX_AMOUNT_OF_VIRT_CHANNELS;
	}

	virtualChannels.emplace(vcid, VirtualChannel(*this, vcid, true, maxFrameLength, blocking, repetitionTypeAFrame,
	                                             repetitionTypeBFrame, secondaryHeaderTMPresent, secondaryHeaderTMLength,
	                                             operationalControlFieldTMPresent, frameErrorControlFieldPresent,
	                                             synchronization, farmSlidingWinWidth, farmPositiveWinWidth,
	                                             farmNegativeWinWidth, vcRepetitions, mapChan));
	return MasterChannelAlert::NO_MC_ALERT;
}

MasterChannelAlert MasterChannel::addVC(const uint8_t vcid, const uint16_t maxFrameLength, const bool blocking,
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
	                        VirtualChannel(*this, vcid, false, maxFrameLength, blocking, repetitionTypeAFrame,
	                                       repetitionCopCtrl, secondaryHeaderTMPresent, secondaryHeaderTMLength,
	                                       frameErrorControlFieldPresent, operationalControlFieldTMPresent,
	                                       synchronization, farmSlidingWinWidth, farmPositiveWinWidth,
	                                       farmNegativeWinWidth, vcRepetitions, etl::flat_map<uint8_t, MAPChannel, MaxMapChannels>()));
	return MasterChannelAlert::NO_MC_ALERT;
}

void MasterChannel::removeMasterTx(TransferFrameTC* frame_ptr) {
	etl::list<TransferFrameTC, MaxTxInMasterChannel>::iterator it;
	for (it = txMasterCopyTC.begin(); it != txMasterCopyTC.end(); ++it) {
		if (&it == frame_ptr) {
			txMasterCopyTC.erase(it);
			return;
		}
	}
}

void MasterChannel::removeMasterTx(TransferFrameTM* frame_ptr) {
	etl::list<TransferFrameTM, MaxTxInMasterChannel>::iterator it;
	for (it = txMasterCopyTM.begin(); it != txMasterCopyTM.end(); ++it) {
		if (&it == frame_ptr) {
			txMasterCopyTM.erase(it);
			return;
		}
	}
}

void MasterChannel::removeMasterRx(TransferFrameTC* frame_ptr) {
	etl::list<TransferFrameTC, MaxRxInMasterChannel>::iterator it;
	for (it = rxMasterCopyTC.begin(); it != rxMasterCopyTC.end(); ++it) {
		if (&it == frame_ptr) {
			rxMasterCopyTC.erase(it);
			return;
		}
	}
}

void MasterChannel::removeMasterRx(TransferFrameTM* frame_ptr) {
	etl::list<TransferFrameTM, MaxRxInMasterChannel>::iterator it;
	for (it = rxMasterCopyTM.begin(); it != rxMasterCopyTM.end(); ++it) {
		if (&it == frame_ptr) {
			rxMasterCopyTM.erase(it);
			return;
		}
	}
}
void MasterChannel::acknowledgeFrame(uint8_t frameSequenceNumber) {
	for (TransferFrameTC& transferFrame : txMasterCopyTC) {
		if (transferFrame.transferFrameSequenceNumber() == frameSequenceNumber) {
			transferFrame.setAcknowledgement(true);
			return;
		}
	}
}

void MasterChannel::setRetransmitFrame(uint8_t frameSequenceNumber) {
	for (TransferFrameTC transferFrame : txMasterCopyTC) {
		if (transferFrame.transferFrameSequenceNumber() == frameSequenceNumber) {
			transferFrame.setToBeRetransmitted(true);
			return;
		}
	}
}

TransferFrameTC MasterChannel::getLastTxMasterCopyTcFrame() {
	return txMasterCopyTC.back();
}

TransferFrameTC MasterChannel::geFirstTxMasterCopyTcFrame() {
	return txMasterCopyTC.front();
}