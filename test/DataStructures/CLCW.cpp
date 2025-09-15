#include "CLCW.hpp"

#include "catch2/catch_all.hpp"
#include "ExternalContainers.hpp"

using namespace CCSDSDataLinkLayer;

TEST_CASE("CLCW", "[External Containers]") {
    const bool controlWordType = 1;
    const uint8_t clcwVersion = 1;
    const uint8_t statusField = 2;
    const uint8_t copInEffect = 1;
    const Defs::Vcid vcid = 15;
    const uint8_t spare = 1;
    const bool noRfAvailable = 1;
    const bool noBitLock = 0;
    const bool lockout = 1;
    const bool wait = 0;
    const bool retransmit = 1;
    const uint8_t farmBCounter = 3;
    bool spare2 = 0;
    uint8_t reportValue = 137;

    CLCW clcw = CLCW(controlWordType, clcwVersion, statusField, copInEffect, vcid, spare, noRfAvailable, noBitLock,
        lockout, wait, retransmit, farmBCounter, spare2, reportValue);

    SECTION("Ensuring all fields are placed in the correct positions") {
        uint32_t rawClcw = clcw.getRawBytes();

        REQUIRE(((rawClcw >> 31U) & 0x01) == controlWordType);
        REQUIRE(((rawClcw >> 29U) & 0x03) == clcwVersion);
        REQUIRE(((rawClcw >> 26U) & 0x07) == statusField);
        REQUIRE(((rawClcw >> 24U) & 0x03) == copInEffect);
        REQUIRE(((rawClcw >> 18U) & 0x3F) == vcid);
        REQUIRE(((rawClcw >> 16U) & 0x03) == spare);
        REQUIRE(((rawClcw >> 15U) & 0x01) == noRfAvailable);
        REQUIRE(((rawClcw >> 14U) & 0x01) == noBitLock);
        REQUIRE(((rawClcw >> 13U) & 0x01) == lockout);
        REQUIRE(((rawClcw >> 12U) & 0x01) == wait);
        REQUIRE(((rawClcw >> 11U) & 0x01) == retransmit);
        REQUIRE(((rawClcw >> 9U) & 0x03) == farmBCounter);
        REQUIRE(((rawClcw >> 8U) & 0x01) == spare2);
        REQUIRE((rawClcw & 0xFF) == reportValue);
    }

    SECTION("Getters") {
        REQUIRE(clcw.getControlWordType() == controlWordType);
        REQUIRE(clcw.getClcwVersion() == clcwVersion);
        REQUIRE(clcw.getStatusField() == statusField);
        REQUIRE(clcw.getCopInEffect() == copInEffect);
        REQUIRE(clcw.getVcId() == vcid);
        REQUIRE(clcw.getNoRfAvailable() == noRfAvailable);
        REQUIRE(clcw.getNoBitLock() == noBitLock);
        REQUIRE(clcw.getLockout() == lockout);
        REQUIRE(clcw.getWait() == wait);
        REQUIRE(clcw.getRetransmit() == retransmit);
        REQUIRE(clcw.getFarmBCounter() == farmBCounter);
        REQUIRE(clcw.getReportValue() == reportValue);
    }
}
