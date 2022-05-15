#include <CCSDSChannel.hpp>
#include <Alert.hpp>
#include "CCSDSLoggerImpl.h"
#include "MemoryPool.hpp"
// Virtual Channel

VirtualChannelAlert VirtualChannel::storeVC(TransferFrameTC* packet) {
	// Limit the amount of packets that can be stored at any given time
	if (txUnprocessedPacketListBufferTC.full()) {
		ccsdsLog(Tx, TypeVirtualChannelAlert, TX_WAIT_QUEUE_FULL);
		return VirtualChannelAlert::TX_WAIT_QUEUE_FULL;
	}
	txUnprocessedPacketListBufferTC.push_back(packet);
	ccsdsLog(Tx, TypeVirtualChannelAlert, NO_VC_ALERT);
	return VirtualChannelAlert::NO_VC_ALERT;
}

etl::queue<std::pair<uint8_t *, uint16_t>, PacketBufferTmSize> VirtualChannel::getPacketPtrBufferTm() {
    return packetPtrBufferTmTx;
}

etl::queue<uint8_t, PacketBufferTmSize> VirtualChannel::getPacketBufferTm() {
    return packetBufferTmTx;
}

void VirtualChannel::setPacketPtrBufferTm(etl::queue<std::pair<uint8_t *, uint16_t>, PacketBufferTmSize> &packetPtrBuffer) {
    this->packetPtrBufferTmTx = packetPtrBuffer;
}

void VirtualChannel::setPacketBufferTm(etl::queue<uint8_t, PacketBufferTmSize> &packetBuffer) {
    this->packetBufferTmTx = packetBuffer;
}

void VirtualChannel::storePacektTm(uint8_t *packet, uint16_t packetLength) {
    packet = virtualChannelPool.allocatePacket(packet, packetLength);
    if (packet != nullptr) {
        std::pair<uint8_t *, uint16_t> packetPtr;
        packetPtr.first = packet;
        packetPtr.second = packetLength;
        packetPtrBufferTmTx.push(packetPtr);
        for (uint16_t i = 0; i < packetLength; i++) {
            packetBufferTmTx.push(packet[i]);
        }
    }
}
/*
void VirtualChannel::vcGenerationService(MasterChannel* mc) {
    mc->vcGenerationService(this, 1000);
    uint8_t * transferFramePacket = mc->getMasterTransferFramePtrBuffer().front();
    frameCountTM++;
    mc->frameCount++;
    uint8_t masterChannelFrameCount = mc->frameCountTM;
    uint8_t transferFrameVersionNumber = 0;
    bool secondaryHeaderTMPresent = mc->getSecondaryHeaderTMPresent();
    TransferFrameTM transferFrameTm = TransferFrameTM(transferFramePacket, 1000, TM);
}*/

void VirtualChannel::vcGenerationService(uint16_t maxTransferFrameDataLength){
    uint16_t transferFrameDataLength = 0;
    uint16_t packetLength = packetPtrBufferTmTx.front().second;
    uint8_t* transferFrameData = packetPtrBufferTmTx.front().first;
    while (transferFrameDataLength + packetLength <= maxTransferFrameDataLength) {
        transferFrameDataLength += packetLength;
        for (uint16_t i = 0; i < packetLength; i++) {
            packetBufferTmTx.pop();
        }
        packetPtrBufferTmTx.pop();
        packetLength = packetPtrBufferTmTx.front().second;
    }
    if(transferFrameDataLength != 0 ){
        transferFrameData = master_channel().masterChannelPool.allocatePacket(transferFrameData, transferFrameDataLength);
        frameCountTM++;
        master_channel().frameCount++;
        TransferFrameTM transferFrameTm = TransferFrameTM(transferFrameData, transferFrameDataLength, TM);
        master_channel().rxMasterCopyTM.push_back(transferFrameTm);
        master_channel().masterTransferFramePtrBufferTm.push(transferFrameData);
    }
}

// Master Channel

// Technically not a packet, but it has identical information
// @todo consider another data structure

MasterChannelAlert MasterChannel::storeOut(TransferFrameTC* packet) {
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

MasterChannelAlert MasterChannel::storeOut(TransferFrameTM* packet) {
	if (txOutFramesBeforeAllFramesGenerationListTM.full()) {
		// Log that buffer is full
		ccsdsLog(Tx, TypeMasterChannelAlert, OUT_FRAMES_LIST_FULL);
		return MasterChannelAlert::OUT_FRAMES_LIST_FULL;
	}
	txOutFramesBeforeAllFramesGenerationListTM.push_back(packet);
	ccsdsLog(Tx, TypeMasterChannelAlert, NO_MC_ALERT);
	return MasterChannelAlert::NO_MC_ALERT;
}

MasterChannelAlert MasterChannel::storeTransmittedOut(TransferFrameTC* packet) {
	if (txToBeTransmittedFramesAfterAllFramesGenerationListTC.full()) {
		ccsdsLog(Tx, TypeMasterChannelAlert, TO_BE_TRANSMITTED_FRAMES_LIST_FULL);
		return MasterChannelAlert::TO_BE_TRANSMITTED_FRAMES_LIST_FULL;
	}
	txToBeTransmittedFramesAfterAllFramesGenerationListTC.push_back(packet);
	ccsdsLog(Tx, TypeMasterChannelAlert, NO_MC_ALERT);
	return MasterChannelAlert::NO_MC_ALERT;
}

MasterChannelAlert MasterChannel::storeTransmittedOut(TransferFrameTM* packet) {
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
                                        const bool frameErrorControlFieldTMPresent,
                                        const bool operationalControlFieldTMPresent,
                                        SynchronizationFlag synchronization,
                                        etl::flat_map<uint8_t, MAPChannel, MaxMapChannels> mapChan) {
	if (virtChannels.full()) {
		ccsdsLog(Tx, TypeMasterChannelAlert, MAX_AMOUNT_OF_VIRT_CHANNELS);
		return MasterChannelAlert::MAX_AMOUNT_OF_VIRT_CHANNELS;
	}

	virtChannels.emplace(vcid, VirtualChannel(*this, vcid, segmentHeaderPresent, maxFrameLength, clcwRate, blocking,
	                                          repetitionTypeAFrame, repetitionCopCtrl,
	                                          frameErrorControlFieldTMPresent, operationalControlFieldTMPresent,
	                                          synchronization, mapChan));
	return MasterChannelAlert::NO_MC_ALERT;
}

void MasterChannel::removeMasterTx(TransferFrameTC* packet_ptr) {
	etl::list<TransferFrameTC, MaxTxInMasterChannel>::iterator it;
	for (it = txMasterCopyTC.begin(); it != txMasterCopyTC.end(); ++it) {
		if (&it == packet_ptr) {
			txMasterCopyTC.erase(it);
			return;
		}
	}
}

void MasterChannel::removeMasterTx(TransferFrameTM* packet_ptr) {
	etl::list<TransferFrameTM, MaxTxInMasterChannel>::iterator it;
	for (it = txMasterCopyTM.begin(); it != txMasterCopyTM.end(); ++it) {
		if (&it == packet_ptr) {
			txMasterCopyTM.erase(it);
			return;
		}
	}
}

void MasterChannel::removeMasterRx(TransferFrameTC* packet_ptr) {
	etl::list<TransferFrameTC, MaxRxInMasterChannel>::iterator it;
	for (it = rxMasterCopyTC.begin(); it != rxMasterCopyTC.end(); ++it) {
		if (&it == packet_ptr) {
			rxMasterCopyTC.erase(it);
			return;
		}
	}
}

void MasterChannel::removeMasterRx(TransferFrameTM* packet_ptr) {
	etl::list<TransferFrameTM, MaxRxInMasterChannel>::iterator it;
	for (it = rxMasterCopyTM.begin(); it != rxMasterCopyTM.end(); ++it) {
		if (&it == packet_ptr) {
			rxMasterCopyTM.erase(it);
			return;
		}
	}
}

uint8_t MasterChannel::getSecondaryHeaderTMPresent(){
    return secondaryHeaderTMPresent;
}