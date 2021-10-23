#include "Logger.hpp"
#include "CCSDSChannel.hpp"
#include "CCSDS_Log.h"
#include <iostream>
int main() {
	LOG_DEBUG << "CCSDS Services test application";

	// Set up Service Channel
	PhysicalChannel phy_channel_fop = PhysicalChannel(1024, false, 12, 1024, 220000, 20);

	LOG_DEBUG << "CCSDS Services test application";
	ccsds_log(Tx_VirtualChannel_store_VirtualChannelAlert_NO_VC_ALERT, 1);
	std::cout<<Tx_VirtualChannel_store_VirtualChannelAlert_NO_VC_ALERT<<std::endl;
}
