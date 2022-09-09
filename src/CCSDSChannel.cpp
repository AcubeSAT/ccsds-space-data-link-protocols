#include <CCSDSChannel.hpp>
#include <Alert.hpp>
#include "CCSDSLoggerImpl.h"
#include "MemoryPool.hpp"
// Virtual Channel

template <uint16_t T, uint16_t K, class _MC>
VirtualChannelAlert VirtualChannel<T, K, _MC>::storeVC(TransferFrameTC* packet) {
	// Limit the amount of packets that can be stored at any given time
	if (txUnprocessedPacketListBufferTC.full()) {
		ccsdsLogNotice(Tx, TypeVirtualChannelAlert, TX_WAIT_QUEUE_FULL);
		return VirtualChannelAlert::TX_WAIT_QUEUE_FULL;
	}
	txUnprocessedPacketListBufferTC.push_back(packet);
	ccsdsLogNotice(Tx, TypeVirtualChannelAlert, NO_VC_ALERT);
	return VirtualChannelAlert::NO_VC_ALERT;
}
// Master Channel

// Technically not a packet, but it has identical information
// @todo consider another data structure

template <uint16_t T, uint16_t K, uint16_t L>
MasterChannelAlert MasterChannel<T, K, L>::storeOut(TransferFrameTC* packet) {
	if (txOutFramesBeforeAllFramesGenerationListTC.full()) {
		// Log that buffer is full
		ccsdsLogNotice(Tx, TypeMasterChannelAlert, OUT_FRAMES_LIST_FULL);
		return MasterChannelAlert::OUT_FRAMES_LIST_FULL;
	}
	txOutFramesBeforeAllFramesGenerationListTC.push_back(packet);
	uint8_t vid = packet->virtualChannelId();
	// virtChannels.at(0).fop.
	ccsdsLogNotice(Tx, TypeMasterChannelAlert, NO_MC_ALERT);
	return MasterChannelAlert::NO_MC_ALERT;
}

template <uint16_t T, uint16_t K, uint16_t L>
MasterChannelAlert MasterChannel<T, K, L>::storeOut(TransferFrameTM* packet) {
	if (txOutFramesBeforeAllFramesGenerationListTM.full()) {
		// Log that buffer is full
		ccsdsLogNotice(Tx, TypeMasterChannelAlert, OUT_FRAMES_LIST_FULL);
		return MasterChannelAlert::OUT_FRAMES_LIST_FULL;
	}
	txOutFramesBeforeAllFramesGenerationListTM.push_back(packet);
	ccsdsLogNotice(Tx, TypeMasterChannelAlert, NO_MC_ALERT);
	return MasterChannelAlert::NO_MC_ALERT;
}

template <uint16_t T, uint16_t K, uint16_t L>
MasterChannelAlert MasterChannel<T, K, L>::storeTransmittedOut(TransferFrameTC* packet) {
	if (txToBeTransmittedFramesAfterAllFramesGenerationListTC.full()) {
		ccsdsLogNotice(Tx, TypeMasterChannelAlert, TO_BE_TRANSMITTED_FRAMES_LIST_FULL);
		return MasterChannelAlert::TO_BE_TRANSMITTED_FRAMES_LIST_FULL;
	}
	txToBeTransmittedFramesAfterAllFramesGenerationListTC.push_back(packet);
	ccsdsLogNotice(Tx, TypeMasterChannelAlert, NO_MC_ALERT);
	return MasterChannelAlert::NO_MC_ALERT;
}

template <uint16_t T, uint16_t K, uint16_t L>
MasterChannelAlert MasterChannel<T, K, L>::storeTransmittedOut(TransferFrameTM* packet) {
	if (txToBeTransmittedFramesAfterAllFramesGenerationListTM.full()) {
		ccsdsLogNotice(Tx, TypeMasterChannelAlert, TO_BE_TRANSMITTED_FRAMES_LIST_FULL);
		return MasterChannelAlert::TO_BE_TRANSMITTED_FRAMES_LIST_FULL;
	}
	txToBeTransmittedFramesAfterAllFramesGenerationListTM.push_back(packet);
	ccsdsLogNotice(Tx, TypeMasterChannelAlert, NO_MC_ALERT);
	return MasterChannelAlert::NO_MC_ALERT;
}

template <uint16_t T, uint16_t K, uint16_t L>
MasterChannelAlert MasterChannel<T, K, L>::addVC(const uint8_t vcid, const uint16_t maxFrameLength, const bool blocking,
                                        const uint8_t repetitionTypeAFrame, const uint8_t repetitionTypeBFrame,
                                        const bool frameErrorControlFieldPresent, const bool secondaryHeaderTMPresent,
                                        const uint8_t secondaryHeaderTMLength,
                                        const bool operationalControlFieldTMPresent,
                                        SynchronizationFlag synchronization, const uint8_t farmSlidingWinWidth,
                                        const uint8_t farmPositiveWinWidth, const uint8_t farmNegativeWinWidth, const uint8_t vcRepetitions,
                                        etl::flat_map<uint8_t, MAPChannel<mapchannellengthtemp>, MaxMapChannels> mapChan) {
	if (virtualChannels.full()) {
		ccsdsLogNotice(Tx, TypeMasterChannelAlert, MAX_AMOUNT_OF_VIRT_CHANNELS);
		return MasterChannelAlert::MAX_AMOUNT_OF_VIRT_CHANNELS;
	}

	virtualChannels.emplace(vcid, VirtualChannel<mapchannellengthtemp, vclengthtemp, MasterChannel>(*this, vcid, true, maxFrameLength, blocking, repetitionTypeAFrame,
	                                             repetitionTypeBFrame, secondaryHeaderTMPresent, secondaryHeaderTMLength,
	                                             operationalControlFieldTMPresent, frameErrorControlFieldPresent,
	                                             synchronization, farmSlidingWinWidth, farmPositiveWinWidth,
	                                             farmNegativeWinWidth, vcRepetitions, mapChan));
	return MasterChannelAlert::NO_MC_ALERT;
}

template <uint16_t T, uint16_t K, uint16_t L>
MasterChannelAlert MasterChannel<T, K, L>::addVC(const uint8_t vcid, const uint16_t maxFrameLength, const bool blocking,
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
	                        VirtualChannel<mapchannellengthtemp, vclengthtemp, MasterChannel>(*this, vcid, false, maxFrameLength, blocking, repetitionTypeAFrame,
	                                       repetitionCopCtrl, secondaryHeaderTMPresent, secondaryHeaderTMLength,
	                                       frameErrorControlFieldPresent, operationalControlFieldTMPresent,
	                                       synchronization, farmSlidingWinWidth, farmPositiveWinWidth,
	                                       farmNegativeWinWidth, vcRepetitions, etl::flat_map<uint8_t, MAPChannel<mapchannellengthtemp>, MaxMapChannels>()));
	return MasterChannelAlert::NO_MC_ALERT;
}

template <uint16_t T, uint16_t K, uint16_t L>
void MasterChannel<T, K, L>::removeMasterTx(TransferFrameTC* packet_ptr) {
	etl::list<TransferFrameTC, MaxTxInMasterChannel>::iterator it;
	for (it = txMasterCopyTC.begin(); it != txMasterCopyTC.end(); ++it) {
		if (&it == packet_ptr) {
			txMasterCopyTC.erase(it);
			return;
		}
	}
}

template <uint16_t T, uint16_t K, uint16_t L>
void MasterChannel<T, K, L>::removeMasterTx(TransferFrameTM* packet_ptr) {
	etl::list<TransferFrameTM, MaxTxInMasterChannel>::iterator it;
	for (it = txMasterCopyTM.begin(); it != txMasterCopyTM.end(); ++it) {
		if (&it == packet_ptr) {
			txMasterCopyTM.erase(it);
			return;
		}
	}
}

template <uint16_t T, uint16_t K, uint16_t L>
void MasterChannel<T, K, L>::removeMasterRx(TransferFrameTC* packet_ptr) {
	etl::list<TransferFrameTC, MaxRxInMasterChannel>::iterator it;
	for (it = rxMasterCopyTC.begin(); it != rxMasterCopyTC.end(); ++it) {
		if (&it == packet_ptr) {
			rxMasterCopyTC.erase(it);
			return;
		}
	}
}

template <uint16_t T, uint16_t K, uint16_t L>
void MasterChannel<T, K, L>::removeMasterRx(TransferFrameTM* packet_ptr) {
	etl::list<TransferFrameTM, MaxRxInMasterChannel>::iterator it;
	for (it = rxMasterCopyTM.begin(); it != rxMasterCopyTM.end(); ++it) {
		if (&it == packet_ptr) {
			rxMasterCopyTM.erase(it);
			return;
		}
	}
}

template <uint16_t T, uint16_t K, uint16_t L>
void MasterChannel<T, K, L>::acknowledgeFrame(uint8_t frameSequenceNumber) {
	for (TransferFrameTC& transferFrame : txMasterCopyTC) {
		if (transferFrame.transferFrameSequenceNumber() == frameSequenceNumber) {
			transferFrame.setAcknowledgement(true);
			return;
		}
	}
}

template <uint16_t T, uint16_t K, uint16_t L>
void MasterChannel<T, K, L>::setRetransmitFrame(uint8_t frameSequenceNumber) {
	for (TransferFrameTC transferFrame : txMasterCopyTC) {
		if (transferFrame.transferFrameSequenceNumber() == frameSequenceNumber) {
			transferFrame.setToBeRetransmitted(true);
			return;
		}
	}
}

template <uint16_t T, uint16_t K, uint16_t L>
TransferFrameTC MasterChannel<T, K, L>::getLastTxMasterCopyTcFrame() {
	return txMasterCopyTC.back();
}

template <uint16_t T, uint16_t K, uint16_t L>
TransferFrameTC MasterChannel<T, K, L>::geFirstTxMasterCopyTcFrame() {
	return txMasterCopyTC.front();
}