#include "Logger.hpp"
#include "CCSDSChannel.hpp"
#include "CCSDS_Log.h"
#include <iostream>
int main() {
	LOG_DEBUG << "CCSDS Services test application";

	// Set up Service Channel
	PhysicalChannel phy_channel_fop = PhysicalChannel(1024, false, 12, 1024, 220000, 20);

	LOG_DEBUG << "CCSDS Services test application";
	ccsds_log(Tx, TypeVirtualChannelAlert, MAP_CHANNEL_FRAME_BUFFER_FULL);
	ccsds_log(Rx, TypeServiceChannelNotif, RX_IN_BUFFER_FULL);
	ccsds_log(Rx, TypeServiceChannelNotif, NO_SERVICE_EVENT);
	ccsds_log(Rx, TypeServiceChannelNotif, RX_INVALID_LENGTH);

	ccsds_log(Tx, TypeServiceChannelNotif, MAP_CHANNEL_FRAME_BUFFER_FULL);
	ccsds_log(Tx, TypeServiceChannelNotif, NO_SERVICE_EVENT);
	ccsds_log(Tx, TypeServiceChannelNotif, MASTER_CHANNEL_FRAME_BUFFER_FULL);
	ccsds_log(Tx, TypeServiceChannelNotif, NO_SERVICE_EVENT);

	ccsds_log(Tx, TypeServiceChannelNotif, NO_TX_PACKETS_TO_PROCESS);
	ccsds_log(Tx, TypeServiceChannelNotif, VC_MC_FRAME_BUFFER_FULL);
	ccsds_log(Tx, TypeServiceChannelNotif, PACKET_EXCEEDS_MAX_SIZE);
	ccsds_log(Tx, TypeServiceChannelNotif, NO_TX_PACKETS_TO_PROCESS);
	ccsds_log(Tx, TypeServiceChannelNotif, TX_MC_FRAME_BUFFER_FULL);
	ccsds_log(Tx, TypeServiceChannelNotif, FOP_REQUEST_REJECTED);

	ccsds_log(Tx, TypeServiceChannelNotif, NO_SERVICE_EVENT);
	ccsds_log(Rx, TypeServiceChannelNotif, NO_RX_PACKETS_TO_PROCESS);
	ccsds_log(Rx, TypeServiceChannelNotif, RX_OUT_BUFFER_FULL);
	ccsds_log(Rx, TypeServiceChannelNotif, VC_RX_WAIT_QUEUE_FULL);
	ccsds_log(Rx, TypeServiceChannelNotif, RX_INVALID_TFVN);
	ccsds_log(Rx, TypeServiceChannelNotif, RX_INVALID_SCID);
	ccsds_log(Rx, TypeServiceChannelNotif, NO_SERVICE_EVENT);



//	std::cout<<Tx_VirtualChannel_store_VirtualChannelAlert_NO_VC_ALERT<<std::endl;
}
