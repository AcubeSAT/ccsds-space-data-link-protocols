#include "CLCW.hpp"
#include "catch2/catch_all.hpp"

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

        CHECK(((rawClcw >> 31U) & 0x01) == controlWordType);
        CHECK(((rawClcw >> 29U) & 0x03) == clcwVersion);
        CHECK(((rawClcw >> 26U) & 0x07) == statusField);
        CHECK(((rawClcw >> 24U) & 0x03) == copInEffect);
        CHECK(((rawClcw >> 18U) & 0x3F) == vcid);
        CHECK(((rawClcw >> 16U) & 0x03) == spare);
        CHECK(((rawClcw >> 15U) & 0x01) == noRfAvailable);
        CHECK(((rawClcw >> 14U) & 0x01) == noBitLock);
        CHECK(((rawClcw >> 13U) & 0x01) == lockout);
        CHECK(((rawClcw >> 12U) & 0x01) == wait);
        CHECK(((rawClcw >> 11U) & 0x01) == retransmit);
        CHECK(((rawClcw >> 9U) & 0x03) == farmBCounter);
        CHECK(((rawClcw >> 8U) & 0x01) == spare2);
        CHECK((rawClcw & 0xFF) == reportValue);
    }

    SECTION("Getters") {
        CHECK(clcw.getControlWordType() == controlWordType);
        CHECK(clcw.getClcwVersion() == clcwVersion);
        CHECK(clcw.getStatusField() == statusField);
        CHECK(clcw.getCopInEffect() == copInEffect);
        CHECK(clcw.getVcId() == vcid);
        CHECK(clcw.getNoRfAvailable() == noRfAvailable);
        CHECK(clcw.getNoBitLock() == noBitLock);
        CHECK(clcw.getLockout() == lockout);
        CHECK(clcw.getWait() == wait);
        CHECK(clcw.getRetransmit() == retransmit);
        CHECK(clcw.getFarmBCounter() == farmBCounter);
        CHECK(clcw.getReportValue() == reportValue);
    }
}
