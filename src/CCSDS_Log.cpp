#include <CCSDS_Log.h>
#include <iostream>
#include <iomanip>
#include "Logger.hpp"

std::ostream& operator<<(std::ostream& out, const LogID value)
{
	static std::map<LogID, std::string> strings;
	if (strings.size() == 0){
#define INSERT_ELEMENT(p) strings[p] = #p
		INSERT_ELEMENT(Tx_VirtualChannel_store_VirtualChannelAlert_NO_VC_ALERT);
		INSERT_ELEMENT(Tx_VirtualChannel_store_VirtualChannelAlert_TX_WAIT_QUEUE_FULL);
		INSERT_ELEMENT(Tx_MasterChannel_store_out_MasterChannelAlert_NO_MC_ALERT);
		INSERT_ELEMENT(Tx_MasterChannel_store_out_MasterChannelAlert_OUT_FRAMES_LIST_FULL);
		INSERT_ELEMENT(Tx_MasterChannel_store_transmitted_out_MasterChannelAlert_NO_MC_ALERT);
		INSERT_ELEMENT(Tx_MasterChannel_store_transmitted_out_MasterChannelAlert_TO_BE_TRANSMITTED_FRAMES_LIST_FULL);
		INSERT_ELEMENT(Tx_ServiceChannel_store_ServiceChannelNotif_NO_SERVICE_EVENT);
		INSERT_ELEMENT(Tx_ServiceChannel_store_ServiceChannelNotif_MAP_CHANNEL_FRAME_BUFFER_FULL);
		INSERT_ELEMENT(Rx_ServiceChannel_store_ServiceChannelNotif_NO_SERVICE_EVENT);
		INSERT_ELEMENT(Rx_ServiceChannel_store_ServiceChannelNotif_RX_IN_MC_FULL);
		INSERT_ELEMENT(Rx_ServiceChannel_store_ServiceChannelNotif_MAP_CHANNEL_RX_IN_BUFFER_FULL);
		INSERT_ELEMENT(Rx_ServiceChannel_store_ServiceChannelNotif_MAP_CHANNEL_RX_INVALID_LENGTH);
		INSERT_ELEMENT(Tx_ServiceChannel_vc_generation_request_ServiceChannelNotif_NO_TX_PACKETS_TO_PROCESS);
		INSERT_ELEMENT(Tx_ServiceChannel_vc_generation_request_ServiceChannelNotif_TX_MC_FRAME_BUFFER_FULL);
		INSERT_ELEMENT(Tx_ServiceChannel_vc_generation_request_ServiceChannelNotif_FOP_REQUEST_REJECTED);
		INSERT_ELEMENT(Tx_ServiceChannel_vc_generation_request_ServiceChannelNotif_NO_SERVICE_EVENT);
		INSERT_ELEMENT(Tx_ServiceChannel_mapp_request_ServiceChannelNotif_NO_TX_PACKETS_TO_PROCESS);
		INSERT_ELEMENT(Tx_ServiceChannel_mapp_request_ServiceChannelNotif_VC_MC_FRAME_BUFFER_FULL);
		INSERT_ELEMENT(Tx_ServiceChannel_mapp_request_ServiceChannelNotif_PACKET_EXCEEDS_MAX_SIZE);
		INSERT_ELEMENT(Tx_ServiceChannel_mapp_request_ServiceChannelNotif_NO_SERVICE_EVENT);
		INSERT_ELEMENT(Rx_ServiceChannel_all_frames_reception_request_ServiceChannelNotif_NO_RX_PACKETS_TO_PROCESS);
		INSERT_ELEMENT(Rx_ServiceChannel_all_frames_reception_request_ServiceChannelNotif_RX_OUT_BUFFER_FULL);
		INSERT_ELEMENT(Rx_ServiceChannel_all_frames_reception_request_ServiceChannelNotif_VC_RX_WAIT_QUEUE_FULL);
		INSERT_ELEMENT(Rx_ServiceChannel_all_frames_reception_request_ServiceChannelNotif_RX_INVALID_TFVN);
		INSERT_ELEMENT(Rx_ServiceChannel_all_frames_reception_request_ServiceChannelNotif_RX_INVALID_SCID);
		INSERT_ELEMENT(Rx_ServiceChannel_all_frames_reception_request_ServiceChannelNotif_NO_SERVICE_EVENT);
#undef INSERT_ELEMENT
	}

	return out  << strings[value];
}

void ccsds_log(const LogID id, const bool verbose) {
	std::ostringstream ss;
	uint16_t id_int;
	switch (verbose) {
		case 0:
			ss << id << std::endl;
			LOG_NOTICE << ss.str();

	case 1:
		    id_int = (uint16_t) id;
			ss << id_int << std::endl;
			LOG_NOTICE << ss.str();
};
}

