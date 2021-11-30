#include <CCSDSChannel.hpp>
#include <Alert.hpp>
#include "CCSDS_Log.h"
// Virtual Channel

VirtualChannelAlert VirtualChannel::storeVC(PacketTC *packet) {
    // Limit the amount of packets that can be stored at any given time
    if (txUnprocessedPacketListBufferTC.full()) {
		ccsds_log(Tx, TypeVirtualChannelAlert, TX_WAIT_QUEUE_FULL);
        return VirtualChannelAlert::TX_WAIT_QUEUE_FULL;
    }
	txUnprocessedPacketListBufferTC.push_back(packet);
	ccsds_log(Tx, TypeVirtualChannelAlert, NO_VC_ALERT);
    return VirtualChannelAlert::NO_VC_ALERT;
}

// Master Channel

// Technically not a packet, but it has identical information
// @todo consider another data structure

MasterChannelAlert MasterChannel::  store_out(PacketTC *packet) {
    if (txOutFramesBeforeAllFramesGenerationList.full()) {
        // Log that buffer is full
		ccsds_log(Tx, TypeMasterChannelAlert, OUT_FRAMES_LIST_FULL);
        return MasterChannelAlert::OUT_FRAMES_LIST_FULL;
    }
	txOutFramesBeforeAllFramesGenerationList.push_back(packet);
    uint8_t vid = packet->global_virtual_channel_id();
    // virtChannels.at(0).fop.
	ccsds_log(Tx, TypeMasterChannelAlert, NO_MC_ALERT);
    return MasterChannelAlert::NO_MC_ALERT;
}

MasterChannelAlert MasterChannel::store_transmitted_out(PacketTC *packet) {
    if (txToBeTransmittedFramesAfterAllFramesGenerationList.full()) {
		ccsds_log(Tx, TypeMasterChannelAlert, TO_BE_TRANSMITTED_FRAMES_LIST_FULL);
        return MasterChannelAlert::TO_BE_TRANSMITTED_FRAMES_LIST_FULL;
    }
	txToBeTransmittedFramesAfterAllFramesGenerationList.push_back(packet);
	ccsds_log(Tx, TypeMasterChannelAlert, NO_MC_ALERT);
    return MasterChannelAlert::NO_MC_ALERT;
}

MasterChannelAlert MasterChannel::add_vc(const uint8_t vcid, const bool segment_header_present,
                                         const uint16_t max_frame_length, const uint8_t clcw_rate, const bool blocking,
                                         const uint8_t repetition_type_a_frame, const uint8_t repetition_cop_ctrl,
                                         const uint8_t frame_count,
                                         etl::flat_map<uint8_t, MAPChannel, MAX_MAP_CHANNELS> map_chan) {
    if (virtChannels.full()) {
		ccsds_log(Tx, TypeMasterChannelAlert, MAX_AMOUNT_OF_VIRT_CHANNELS);
        return MasterChannelAlert::MAX_AMOUNT_OF_VIRT_CHANNELS;
    }

    virtChannels.emplace(vcid, VirtualChannel(*this, vcid, segment_header_present, max_frame_length, clcw_rate,
                                              blocking, repetition_type_a_frame, repetition_cop_ctrl, frame_count,
                                              map_chan));
}
