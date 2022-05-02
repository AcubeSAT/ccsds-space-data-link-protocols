#include <CCSDSChannel.hpp>
#include <Alert.hpp>
#include "CCSDSLoggerImpl.h"
// Virtual Channel

VirtualChannelAlert VirtualChannel::storeVC(PacketTC* packet) {
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

MasterChannelAlert MasterChannel::storeOut(PacketTC* packet) {
	if (txOutFramesBeforeAllFramesGenerationListTC.full()) {
		// Log that buffer is full
		ccsdsLog(Tx, TypeMasterChannelAlert, OUT_FRAMES_LIST_FULL);
		return MasterChannelAlert::OUT_FRAMES_LIST_FULL;
	}
	txOutFramesBeforeAllFramesGenerationListTC.push_back(packet);
	uint8_t vid = packet->globalVirtualChannelId();
	// virtChannels.at(0).fop.
	ccsdsLog(Tx, TypeMasterChannelAlert, NO_MC_ALERT);
	return MasterChannelAlert::NO_MC_ALERT;
}

MasterChannelAlert MasterChannel::storeOut(PacketTM* packet) {
	if (txOutFramesBeforeAllFramesGenerationListTM.full()) {
		// Log that buffer is full
		ccsdsLog(Tx, TypeMasterChannelAlert, OUT_FRAMES_LIST_FULL);
		return MasterChannelAlert::OUT_FRAMES_LIST_FULL;
	}
	txOutFramesBeforeAllFramesGenerationListTM.push_back(packet);
	ccsdsLog(Tx, TypeMasterChannelAlert, NO_MC_ALERT);
	return MasterChannelAlert::NO_MC_ALERT;
}

MasterChannelAlert MasterChannel::storeTransmittedOut(PacketTC* packet) {
	if (txToBeTransmittedFramesAfterAllFramesGenerationListTC.full()) {
		ccsdsLog(Tx, TypeMasterChannelAlert, TO_BE_TRANSMITTED_FRAMES_LIST_FULL);
		return MasterChannelAlert::TO_BE_TRANSMITTED_FRAMES_LIST_FULL;
	}
	txToBeTransmittedFramesAfterAllFramesGenerationListTC.push_back(packet);
	ccsdsLog(Tx, TypeMasterChannelAlert, NO_MC_ALERT);
	return MasterChannelAlert::NO_MC_ALERT;
}

MasterChannelAlert MasterChannel::storeTransmittedOut(PacketTM* packet) {
	if (txToBeTransmittedFramesAfterAllFramesGenerationListTM.full()) {
		ccsdsLog(Tx, TypeMasterChannelAlert, TO_BE_TRANSMITTED_FRAMES_LIST_FULL);
		return MasterChannelAlert::TO_BE_TRANSMITTED_FRAMES_LIST_FULL;
	}
	txToBeTransmittedFramesAfterAllFramesGenerationListTM.push_back(packet);
	ccsdsLog(Tx, TypeMasterChannelAlert, NO_MC_ALERT);
	return MasterChannelAlert::NO_MC_ALERT;
}

MasterChannelAlert MasterChannel::addVC(const uint8_t vcid, const bool segmentHeaderPresent,
                                        const uint16_t maxFrameLength, const uint8_t clcwRate, const bool blocking,
                                        const uint8_t repetitionTypeAFrame, const uint8_t repetitionCopCtrl,
                                        const uint8_t frameCountP, const bool frameErrorControlFieldTMPresent,
                                        const bool operationalControlFieldTMPresent,
                                        SynchronizationFlag synchronization,
                                        etl::flat_map<uint8_t, MAPChannel, MaxMapChannels> mapChan) {
	if (virtChannels.full()) {
		ccsdsLog(Tx, TypeMasterChannelAlert, MAX_AMOUNT_OF_VIRT_CHANNELS);
		return MasterChannelAlert::MAX_AMOUNT_OF_VIRT_CHANNELS;
	}

	virtChannels.emplace(vcid, VirtualChannel(*this, vcid, segmentHeaderPresent, maxFrameLength, clcwRate, blocking,
	                                          repetitionTypeAFrame, repetitionCopCtrl, frameCountP,
	                                          frameErrorControlFieldTMPresent, operationalControlFieldTMPresent,
	                                          synchronization, mapChan));
	return MasterChannelAlert::NO_MC_ALERT;
}

void MasterChannel::removeMasterTx(PacketTC* packet_ptr) {
	etl::list<PacketTC, MaxTxInMasterChannel>::iterator it;
	for (it = txMasterCopyTC.begin(); it != txMasterCopyTC.end(); ++it) {
		if (&it == packet_ptr) {
			txMasterCopyTC.erase(it);
			return;
		}
	}
}

void MasterChannel::removeMasterTx(PacketTM* packet_ptr) {
	etl::list<PacketTM, MaxTxInMasterChannel>::iterator it;
	for (it = txMasterCopyTM.begin(); it != txMasterCopyTM.end(); ++it) {
		if (&it == packet_ptr) {
			txMasterCopyTM.erase(it);
			return;
		}
	}
}

void MasterChannel::removeMasterRx(PacketTC* packet_ptr) {
	etl::list<PacketTC, MaxRxInMasterChannel>::iterator it;
	for (it = rxMasterCopyTC.begin(); it != rxMasterCopyTC.end(); ++it) {
		if (&it == packet_ptr) {
			rxMasterCopyTC.erase(it);
			return;
		}
	}
}

void MasterChannel::removeMasterRx(PacketTM* packet_ptr) {
	etl::list<PacketTM, MaxRxInMasterChannel>::iterator it;
	for (it = rxMasterCopyTM.begin(); it != rxMasterCopyTM.end(); ++it) {
		if (&it == packet_ptr) {
			rxMasterCopyTM.erase(it);
			return;
		}
	}
}