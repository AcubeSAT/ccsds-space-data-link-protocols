#include <catch2/catch.hpp>
#include <CCSDSTransferFrame.hpp>

TEST_CASE("CCSDS Transfer Frame") {
	CCSDSTransferFrame transferFrame = CCSDSTransferFrame(4);

	String<PRIMARY_HEADER_SIZE> primHeader = transferFrame.getPrimaryHeader();

	CHECK(primHeader.size() == PRIMARY_HEADER_SIZE); // Check the size of the primary header

	// Validate the contents of the primary header String
	CHECK(primHeader.at(0) == 0x23U);
	CHECK(primHeader.at(1) == 0x78U);

	CHECK(primHeader.at(4) == 0x18U);
	CHECK(primHeader.at(5) == 0x00U);

	// Check individual attributes
	CHECK(not transferFrame.secondaryHeaderExists());
	CHECK(transferFrame.masterChannelID() == (SPACECRAFT_IDENTIFIER & 0x03FFU));
	CHECK(transferFrame.masterChannelFrameCount() == 0); // Master channel frame count
	CHECK(transferFrame.virtualChannelFrameCount() == 0);
	CHECK(transferFrame.virtualChannelID() == 4);
	CHECK(not transferFrame.ocfFlag());
}
