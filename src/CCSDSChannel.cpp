#include <CCSDSChannel.hpp>
#include <Alert.hpp>
#include "CCSDSLoggerImpl.h"
#include "MemoryPool.hpp"
// Virtual Channel

template <uint16_t t>
VirtualChannelAlert VirtualChannel<t>::storeVC(TransferFrameTC* packet) {
	// Limit the amount of packets that can be stored at any given time
	if (txUnprocessedPacketListBufferTC.full()) {
		ccsdsLog(Tx, TypeVirtualChannelAlert, TX_WAIT_QUEUE_FULL);
		return VirtualChannelAlert::TX_WAIT_QUEUE_FULL;
	}
	txUnprocessedPacketListBufferTC.push_back(packet);
	ccsdsLog(Tx, TypeVirtualChannelAlert, NO_VC_ALERT);
	return VirtualChannelAlert::NO_VC_ALERT;
}
// Master Channel

// Technically not a packet, but it has identical information
// @todo consider another data structure

template <uint16_t t>
MasterChannelAlert MasterChannel<t>::storeOut(TransferFrameTC* packet) {
	if (txOutFramesBeforeAllFramesGenerationListTC.full()) {
		// Log that buffer is full
		ccsdsLog(Tx, TypeMasterChannelAlert, OUT_FRAMES_LIST_FULL);
		return MasterChannelAlert::OUT_FRAMES_LIST_FULL;
	}
	txOutFramesBeforeAllFramesGenerationListTC.push_back(packet);
	uint8_t vid = packet->virtualChannelId();
	// virtChannels.at(0).fop.
	ccsdsLog(Tx, TypeMasterChannelAlert, NO_MC_ALERT);
	return MasterChannelAlert::NO_MC_ALERT;
}

template <uint16_t t>
MasterChannelAlert MasterChannel<t>::storeOut(TransferFrameTM* packet) {
	if (txOutFramesBeforeAllFramesGenerationListTM.full()) {
		// Log that buffer is full
		ccsdsLog(Tx, TypeMasterChannelAlert, OUT_FRAMES_LIST_FULL);
		return MasterChannelAlert::OUT_FRAMES_LIST_FULL;
	}
	txOutFramesBeforeAllFramesGenerationListTM.push_back(packet);
	ccsdsLog(Tx, TypeMasterChannelAlert, NO_MC_ALERT);
	return MasterChannelAlert::NO_MC_ALERT;
}

template <uint16_t t>
MasterChannelAlert MasterChannel<t>::storeTransmittedOut(TransferFrameTC* packet) {
	if (txToBeTransmittedFramesAfterAllFramesGenerationListTC.full()) {
		ccsdsLog(Tx, TypeMasterChannelAlert, TO_BE_TRANSMITTED_FRAMES_LIST_FULL);
		return MasterChannelAlert::TO_BE_TRANSMITTED_FRAMES_LIST_FULL;
	}
	txToBeTransmittedFramesAfterAllFramesGenerationListTC.push_back(packet);
	ccsdsLog(Tx, TypeMasterChannelAlert, NO_MC_ALERT);
	return MasterChannelAlert::NO_MC_ALERT;
}

template <uint16_t t>
MasterChannelAlert MasterChannel<t>::storeTransmittedOut(TransferFrameTM* packet) {
	if (txToBeTransmittedFramesAfterAllFramesGenerationListTM.full()) {
		ccsdsLog(Tx, TypeMasterChannelAlert, TO_BE_TRANSMITTED_FRAMES_LIST_FULL);
		return MasterChannelAlert::TO_BE_TRANSMITTED_FRAMES_LIST_FULL;
	}
	txToBeTransmittedFramesAfterAllFramesGenerationListTM.push_back(packet);
	ccsdsLog(Tx, TypeMasterChannelAlert, NO_MC_ALERT);
	return MasterChannelAlert::NO_MC_ALERT;
}

template <uint16_t t>
MasterChannelAlert MasterChannel<t>::addVC(const uint8_t vcid, const uint16_t maxFrameLength, const bool blocking,
                                        const uint8_t repetitionTypeAFrame, const uint8_t repetitionCopCtrl,
                                        const bool frameErrorControlFieldTMPresent, const bool secondaryHeaderTMPresent,
                                        const uint8_t secondaryHeaderTMLength,
                                        const bool operationalControlFieldTMPresent,
                                        SynchronizationFlag synchronization, const uint8_t farmSlidingWinWidth,
                                        const uint8_t farmPositiveWinWidth, const uint8_t farmNegativeWinWidth,
                                        etl::flat_map<uint8_t, MAPChannel<128>, MaxMapChannels> mapChan) {
	if (virtualChannels.full()) {
		ccsdsLog(Tx, TypeMasterChannelAlert, MAX_AMOUNT_OF_VIRT_CHANNELS);
		return MasterChannelAlert::MAX_AMOUNT_OF_VIRT_CHANNELS;
	}

	virtualChannels.emplace(vcid,
	                        VirtualChannel<256>(*this, vcid, true, maxFrameLength, blocking, repetitionTypeAFrame,
	                                       repetitionCopCtrl, frameErrorControlFieldTMPresent, secondaryHeaderTMPresent,
	                                       secondaryHeaderTMLength, operationalControlFieldTMPresent, synchronization,
	                                       farmSlidingWinWidth, farmPositiveWinWidth, farmNegativeWinWidth, mapChan));
	return MasterChannelAlert::NO_MC_ALERT;
}

template <uint16_t t>
MasterChannelAlert MasterChannel<t>::addVC(const uint8_t vcid, const uint16_t maxFrameLength, const bool blocking,
                                        const uint8_t repetitionTypeAFrame, const uint8_t repetitionCopCtrl,
                                        const bool frameErrorControlFieldTMPresent, const bool secondaryHeaderTMPresent,
                                        const uint8_t secondaryHeaderTMLength,
                                        const bool operationalControlFieldTMPresent,
                                        SynchronizationFlag synchronization, const uint8_t farmSlidingWinWidth,
                                        const uint8_t farmPositiveWinWidth, const uint8_t farmNegativeWinWidth) {
	if (virtualChannels.full()) {
		ccsdsLog(Tx, TypeMasterChannelAlert, MAX_AMOUNT_OF_VIRT_CHANNELS);
		return MasterChannelAlert::MAX_AMOUNT_OF_VIRT_CHANNELS;
	}

	virtualChannels.emplace(vcid,
	                        VirtualChannel<256>(*this, vcid, false, maxFrameLength, blocking, repetitionTypeAFrame,
	                                       repetitionCopCtrl, frameErrorControlFieldTMPresent, secondaryHeaderTMPresent,
	                                       secondaryHeaderTMLength, operationalControlFieldTMPresent, synchronization,
	                                       farmSlidingWinWidth, farmPositiveWinWidth, farmNegativeWinWidth,
	                                       etl::flat_map<uint8_t, MAPChannel<128>, MaxMapChannels>()));
	return MasterChannelAlert::NO_MC_ALERT;
}

template <uint16_t t>
void MasterChannel<t>::removeMasterTx(TransferFrameTC* packet_ptr) {
	etl::list<TransferFrameTC, MaxTxInMasterChannel>::iterator it;
	for (it = txMasterCopyTC.begin(); it != txMasterCopyTC.end(); ++it) {
		if (&it == packet_ptr) {
			txMasterCopyTC.erase(it);
			return;
		}
	}
}

template <uint16_t t>
void MasterChannel<t>::removeMasterTx(TransferFrameTM* packet_ptr) {
	etl::list<TransferFrameTM, MaxTxInMasterChannel>::iterator it;
	for (it = txMasterCopyTM.begin(); it != txMasterCopyTM.end(); ++it) {
		if (&it == packet_ptr) {
			txMasterCopyTM.erase(it);
			return;
		}
	}
}

template <uint16_t t>
void MasterChannel<t>::removeMasterRx(TransferFrameTC* packet_ptr) {
	etl::list<TransferFrameTC, MaxRxInMasterChannel>::iterator it;
	for (it = rxMasterCopyTC.begin(); it != rxMasterCopyTC.end(); ++it) {
		if (&it == packet_ptr) {
			rxMasterCopyTC.erase(it);
			return;
		}
	}
}

template <uint16_t t>
void MasterChannel<t>::removeMasterRx(TransferFrameTM* packet_ptr) {
	etl::list<TransferFrameTM, MaxRxInMasterChannel>::iterator it;
	for (it = rxMasterCopyTM.begin(); it != rxMasterCopyTM.end(); ++it) {
		if (&it == packet_ptr) {
			rxMasterCopyTM.erase(it);
			return;
		}
	}
}

template class MasterChannel<256>;
template class VirtualChannel<256>;