#include <cstdint>
#include "CLCW.hpp"

const uint32_t CLCW::getClcw() {
	return clcw;
}

const bool CLCW::getControlWordType() {
	return clcw >> 31U;
}

const uint8_t CLCW::getClcwVersion() {
	return (clcw >> 29U) & 0x3;
}

const uint8_t CLCW::getStatusField() {
	return (clcw >> 26U) & 0x7;
}

const uint8_t CLCW::getCopInEffect() {
	return (clcw >> 24U) & 0x3;
}

const uint8_t CLCW::getVcId() {
	return (clcw >> 18U) & 0x3F;
}

const bool CLCW::getNoRfAvailable() {
	return (clcw >> 15U) & 0x1;
}

const bool CLCW::getNoBitLock() {
	return (clcw >> 14U) & 0x1;
}

const bool CLCW::getLockout() {
	return (clcw >> 13U) & 0x1;
}

const bool CLCW::getWait() {
	return (clcw >> 12U) & 0x1;
}

const bool CLCW::getRetransmit() {
	return (clcw >> 11U) & 0x1;
}

const uint8_t CLCW::getFarmBCounter() {
	return (clcw >> 9U) & 0x3;
}

const uint8_t CLCW::getReportValue() {
	return clcw & 0xFF;
}
