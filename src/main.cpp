#include "Logger.hpp"
#include "CCSDSChannel.hpp"
#include "CCSDSLogger.h"

int main() {
	LOG_DEBUG << "CCSDS Services test application";

	// Set up Service Channel
	PhysicalChannel phy_channel_fop = PhysicalChannel(1024, false, 12, 1024, 220000, 20);
	ccsdsLogNotice(Tx, TypeVirtualChannelAlert, MAP_CHANNEL_FRAME_BUFFER_FULL);
	ccsdsLogNotice(Rx, TypeServiceChannelNotif, RX_IN_BUFFER_FULL);
	ccsdsLogNotice(Rx, TypeServiceChannelNotif, NO_SERVICE_EVENT);
	ccsdsLogNotice(Rx, TypeServiceChannelNotif, RX_INVALID_LENGTH);
	ccsdsLogNotice(Tx, TypeFOPNotif, NO_FOP_EVENT);

	ccsdsLogNotice(Tx, TypeServiceChannelNotif, MAP_CHANNEL_FRAME_BUFFER_FULL);
	ccsdsLogNotice(Tx, TypeServiceChannelNotif, NO_SERVICE_EVENT);
	ccsdsLogNotice(Tx, TypeServiceChannelNotif, MASTER_CHANNEL_FRAME_BUFFER_FULL);
	ccsdsLogNotice(Tx, TypeServiceChannelNotif, NO_SERVICE_EVENT);

	ccsdsLogNotice(Tx, TypeServiceChannelNotif, NO_TX_PACKETS_TO_PROCESS);
	ccsdsLogNotice(Tx, TypeServiceChannelNotif, VC_MC_FRAME_BUFFER_FULL);
	ccsdsLogNotice(Tx, TypeServiceChannelNotif, PACKET_EXCEEDS_MAX_SIZE);
	ccsdsLogNotice(Tx, TypeServiceChannelNotif, NO_TX_PACKETS_TO_PROCESS);
	ccsdsLogNotice(Tx, TypeServiceChannelNotif, TX_MC_FRAME_BUFFER_FULL);
	ccsdsLogNotice(Tx, TypeServiceChannelNotif, FOP_REQUEST_REJECTED);

	ccsdsLogNotice(Tx, TypeServiceChannelNotif, NO_SERVICE_EVENT);
	ccsdsLogNotice(Rx, TypeServiceChannelNotif, NO_RX_PACKETS_TO_PROCESS);
	ccsdsLogNotice(Rx, TypeServiceChannelNotif, RX_OUT_BUFFER_FULL);
	ccsdsLogNotice(Rx, TypeServiceChannelNotif, VC_RX_WAIT_QUEUE_FULL);
	ccsdsLogNotice(Rx, TypeServiceChannelNotif, RX_INVALID_TFVN);
	ccsdsLogNotice(Rx, TypeServiceChannelNotif, RX_INVALID_SCID);
	ccsdsLogNotice(Rx, TypeServiceChannelNotif, NO_SERVICE_EVENT);

	LOG_DEBUG << "CCSDS Services test application";

	//	std::cout<<Tx_VirtualChannel_store_VirtualChannelAlert_NO_VC_ALERT<<std::endl;
}
