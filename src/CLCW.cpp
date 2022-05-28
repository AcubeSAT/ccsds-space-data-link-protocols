#include <cstdint>
#include "CLCW.hpp"


CLCW::CLCW(uint32_t operationalControlField) {
    clcw = operationalControlField;
}

CLCW::CLCW(bool controlWordType, uint8_t clcwVersion, uint8_t statusField, uint8_t copInEffect, uint8_t vcId,
           uint8_t spare, bool noRfAvailable, bool noBitLock, bool lockout, bool wait, bool retransmit,
           uint8_t farmBCounter, bool spare2, uint8_t reportValue) {
    clcw = 0;
    clcw = controlWordType << 31U | clcwVersion << 29U | statusField << 26U | copInEffect << 24U |
           vcId << 18U | spare << 16U | noRfAvailable << 15U | noBitLock << 14U | lockout << 13U | wait << 12U |
           retransmit << 11U | farmBCounter << 9U | spare2 << 8U | reportValue;
}

uint32_t CLCW::getClcw() {
    return clcw;
}

bool CLCW::getControlWordType() {
    return clcw >> 31U;
}

uint8_t CLCW::getClcwVersion() {
    return (clcw >> 29U) & 0x3;
}

uint8_t CLCW::getStatusField() {
    return (clcw >> 26U) & 0x7;
}

uint8_t CLCW::getCopInEffect() {
    return (clcw >> 24U) & 0x3;
}

uint8_t CLCW::getVcId() {
    return (clcw >> 18U) & 0x3F;
}

uint8_t CLCW::getSpare() {
    return (clcw >> 16U) & 0x3;
}

bool CLCW::getNoRfAvailable() {
    return (clcw >> 15U) & 0x1;
}

bool CLCW::getNoBitLock() {
    return (clcw >> 14U) & 0x1;
}

bool CLCW::getLockout() {
    return (clcw >> 13U) & 0x1;
}

bool CLCW::getWait() {
    return (clcw >> 12U) & 0x1;
}

bool CLCW::getRetransmit() {
    return (clcw >> 11U) & 0x1;
}

uint8_t CLCW::getFarmBCounter() {
    return (clcw >> 9U) & 0x3;
}

bool CLCW::getSpare2() {
    return (clcw >> 8U) & 0x1;
}

uint8_t CLCW::getReportValue() {
    return clcw & 0xFF;
}


