#include <CCSDSChannel.hpp>
#include "CCSDS_Log.h"
// Virtual Channel

VirtualChannelAlert VirtualChannel::store(PacketTC *packet) {
    // Limit the amount of packets that can be stored at any given time
    if (txUnprocessedPacketList.full()) {
		ccsds_log(Tx_VirtualChannel_store_VirtualChannelAlert_TX_WAIT_QUEUE_FULL,true);
        return VirtualChannelAlert::TX_WAIT_QUEUE_FULL;
    }
    txUnprocessedPacketList.push_back(packet);
	ccsds_log(Tx_VirtualChannel_store_VirtualChannelAlert_NO_VC_ALERT,true);
    return VirtualChannelAlert::NO_VC_ALERT;
}

// Master Channel

// Technically not a packet, but it has identical information
// @todo consider another data structure

MasterChannelAlert MasterChannel::store_out(PacketTC *packet) {
    if (txOutFramesListTC.full()) {
        // Log that buffer is full
		ccsds_log(Tx_MasterChannel_store_out_MasterChannelAlert_OUT_FRAMES_LIST_FULL,true);
        return MasterChannelAlert::OUT_FRAMES_LIST_FULL;
    }
    txOutFramesListTC.push_back(packet);
    uint8_t vid = packet->global_virtual_channel_id();
    // virtChannels.at(0).fop.
	ccsds_log(Tx_MasterChannel_store_out_MasterChannelAlert_NO_MC_ALERT,true);
    return MasterChannelAlert::NO_MC_ALERT;
}

MasterChannelAlert MasterChannel::store_out(PacketTM *packet) {
	if (txOutFramesListTM.full()) {
		// Log that buffer is full
		ccsds_log(Tx_MasterChannel_store_out_MasterChannelAlert_OUT_FRAMES_LIST_FULL,true);
		return MasterChannelAlert::OUT_FRAMES_LIST_FULL;
	}
	txOutFramesListTM.push_back(packet);
	ccsds_log(Tx_MasterChannel_store_out_MasterChannelAlert_NO_MC_ALERT,true);
	return MasterChannelAlert::NO_MC_ALERT;
}

MasterChannelAlert MasterChannel::store_transmitted_out(PacketTC *packet) {
    if (txToBeTransmittedFramesListTC.full()) {
		ccsds_log(Tx_MasterChannel_store_transmitted_out_MasterChannelAlert_TO_BE_TRANSMITTED_FRAMES_LIST_FULL, 1);
        return MasterChannelAlert::TO_BE_TRANSMITTED_FRAMES_LIST_FULL;
    }
    txToBeTransmittedFramesListTC.push_back(packet);
	ccsds_log(Tx_MasterChannel_store_transmitted_out_MasterChannelAlert_NO_MC_ALERT,1);
    return MasterChannelAlert::NO_MC_ALERT;
}

MasterChannelAlert MasterChannel::store_transmitted_out(PacketTM *packet) {
	if (txToBeTransmittedFramesListTM.full()) {
		ccsds_log(Tx_MasterChannel_store_transmitted_out_MasterChannelAlert_TO_BE_TRANSMITTED_FRAMES_LIST_FULL, 1);
		return MasterChannelAlert::TO_BE_TRANSMITTED_FRAMES_LIST_FULL;
	}
	txToBeTransmittedFramesListTM.push_back(packet);
	ccsds_log(Tx_MasterChannel_store_transmitted_out_MasterChannelAlert_NO_MC_ALERT,1);
	return MasterChannelAlert::NO_MC_ALERT;
}

MasterChannelAlert MasterChannel::add_vc(const uint8_t vcid, const bool segment_header_present,
                                         const uint16_t max_frame_length, const uint8_t clcw_rate, const bool blocking,
                                         const uint8_t repetition_type_a_frame, const uint8_t repetition_cop_ctrl,
                                         const uint8_t frame_count,
                                         etl::flat_map<uint8_t, MAPChannel, max_map_channels> map_chan) {
    if (virtChannels.full()) {
        return MasterChannelAlert::MAX_AMOUNT_OF_VIRT_CHANNELS;
    }

    virtChannels.emplace(vcid, VirtualChannel(*this, vcid, segment_header_present, max_frame_length, clcw_rate,
                                              blocking, repetition_type_a_frame, repetition_cop_ctrl, frame_count,
                                              map_chan));
}
