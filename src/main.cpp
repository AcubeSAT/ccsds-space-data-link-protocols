#include "Logger.hpp"
#include "CCSDSChannel.hpp"
#include "CCSDSLogger.h"

int main() {
	LOG_DEBUG << "CCSDS Services test application";

	// Set up Service Channel
	PhysicalChannel phy_channel_fop = PhysicalChannel(1024, false, 12, 1024, 220000, 20);
	ccsdsLog(Tx, TypeVirtualChannelAlert, MAP_CHANNEL_FRAME_BUFFER_FULL);
	ccsdsLog(Rx, TypeServiceChannelNotif, RX_IN_BUFFER_FULL);
	ccsdsLog(Rx, TypeServiceChannelNotif, NO_SERVICE_EVENT);
	ccsdsLog(Rx, TypeServiceChannelNotif, RX_INVALID_LENGTH);
	ccsdsLog(Tx, TypeFOPNotif, NO_FOP_EVENT);

	ccsdsLog(Tx, TypeServiceChannelNotif, MAP_CHANNEL_FRAME_BUFFER_FULL);
	ccsdsLog(Tx, TypeServiceChannelNotif, NO_SERVICE_EVENT);
	ccsdsLog(Tx, TypeServiceChannelNotif, MASTER_CHANNEL_FRAME_BUFFER_FULL);
	ccsdsLog(Tx, TypeServiceChannelNotif, NO_SERVICE_EVENT);

	ccsdsLog(Tx, TypeServiceChannelNotif, NO_TX_PACKETS_TO_PROCESS);
	ccsdsLog(Tx, TypeServiceChannelNotif, VC_MC_FRAME_BUFFER_FULL);
	ccsdsLog(Tx, TypeServiceChannelNotif, PACKET_EXCEEDS_MAX_SIZE);
	ccsdsLog(Tx, TypeServiceChannelNotif, NO_TX_PACKETS_TO_PROCESS);
	ccsdsLog(Tx, TypeServiceChannelNotif, TX_MC_FRAME_BUFFER_FULL);
	ccsdsLog(Tx, TypeServiceChannelNotif, FOP_REQUEST_REJECTED);

	ccsdsLog(Tx, TypeServiceChannelNotif, NO_SERVICE_EVENT);
	ccsdsLog(Rx, TypeServiceChannelNotif, NO_RX_PACKETS_TO_PROCESS);
	ccsdsLog(Rx, TypeServiceChannelNotif, RX_OUT_BUFFER_FULL);
	ccsdsLog(Rx, TypeServiceChannelNotif, VC_RX_WAIT_QUEUE_FULL);
	ccsdsLog(Rx, TypeServiceChannelNotif, RX_INVALID_TFVN);
	ccsdsLog(Rx, TypeServiceChannelNotif, RX_INVALID_SCID);
	ccsdsLog(Rx, TypeServiceChannelNotif, NO_SERVICE_EVENT);

	LOG_DEBUG << "CCSDS Services test application";

	//	std::cout<<Tx_VirtualChannel_store_VirtualChannelAlert_NO_VC_ALERT<<std::endl;
}
