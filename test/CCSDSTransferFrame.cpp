#include <catch2/catch.hpp>
#include <CCSDSTransferFrame.hpp>

TEST_CASE("CCSDS Transfer Frame") {
	CCSDSTransferFrame transferFrame = CCSDSTransferFrame(5, 4);

	String<PRIMARY_HEADER_SIZE> primHeader = transferFrame.getPrimaryHeader();

	CHECK(primHeader.size() == PRIMARY_HEADER_SIZE);

	CHECK(primHeader.at(0) == 0x23U);
	CHECK(primHeader.at(1) == 0x78U);

	CHECK(primHeader.at(4) == 0x18U);
	CHECK(primHeader.at(5) == 0x00U);
}
