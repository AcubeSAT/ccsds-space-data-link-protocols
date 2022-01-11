#include <CCSDSChannel.hpp>
#include <Alert.hpp>
#include "CCSDS_Log.h"
// Virtual Channel

VirtualChannelAlert VirtualChannel::storeVC(PacketTC *packet) {
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

MasterChannelAlert MasterChannel::storeOut(PacketTC *packet) {
    if (txOutFramesBeforeAllFramesGenerationList.full()) {
        // Log that buffer is full
		ccsdsLog(Tx, TypeMasterChannelAlert, OUT_FRAMES_LIST_FULL);
        return MasterChannelAlert::OUT_FRAMES_LIST_FULL;
    }
	txOutFramesBeforeAllFramesGenerationList.push_back(packet);
    uint8_t vid = packet->globalVirtualChannelId();
    // virtChannels.at(0).fop.
	ccsdsLog(Tx, TypeMasterChannelAlert, NO_MC_ALERT);
    return MasterChannelAlert::NO_MC_ALERT;
}

MasterChannelAlert MasterChannel::storeTransmittedOut(PacketTC *packet) {
    if (txToBeTransmittedFramesAfterAllFramesGenerationList.full()) {
		ccsdsLog(Tx, TypeMasterChannelAlert, TO_BE_TRANSMITTED_FRAMES_LIST_FULL);
        return MasterChannelAlert::TO_BE_TRANSMITTED_FRAMES_LIST_FULL;
    }
	txToBeTransmittedFramesAfterAllFramesGenerationList.push_back(packet);
	ccsdsLog(Tx, TypeMasterChannelAlert, NO_MC_ALERT);
    return MasterChannelAlert::NO_MC_ALERT;
}

MasterChannelAlert MasterChannel::addVC(const uint8_t vcid, const bool segmentHeaderPresent,
                                         const uint16_t maxFrameLength, const uint8_t clcwRate, const bool blocking,
                                         const uint8_t repetitionTypeAFrame, const uint8_t repetitionCopCtrl,
                                         const uint8_t frameCountP,
                                         etl::flat_map<uint8_t, MAPChannel, MAX_MAP_CHANNELS> mapChan) {
    if (virtChannels.full()) {
		ccsdsLog(Tx, TypeMasterChannelAlert, MAX_AMOUNT_OF_VIRT_CHANNELS);
        return MasterChannelAlert::MAX_AMOUNT_OF_VIRT_CHANNELS;
    }

    virtChannels.emplace(vcid, VirtualChannel(*this, vcid, segmentHeaderPresent, maxFrameLength, clcwRate,
                                              blocking,
	                                          repetitionTypeAFrame, repetitionCopCtrl, frameCountP, mapChan));
	return MasterChannelAlert::NO_MC_ALERT;
}
