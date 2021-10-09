#include "Logger.hpp"
#include "CCSDSChannel.hpp"

int main() {
	LOG_DEBUG << "CCSDS Services test application";

	// Set up Service Channel
	PhysicalChannel phy_channel_fop = PhysicalChannel(1024, false, 12, 1024, 220000, 20);

	LOG_DEBUG << "CCSDS Services test application";

}
