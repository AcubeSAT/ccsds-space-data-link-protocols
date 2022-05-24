#include <cstdint>
#include "CLCW.hpp"


CLCW::CLCW(bool operationalControlFieldExists, uint32_t operationalControlField){
    clcw = 0;
    if(operationalControlFieldExists && ((operationalControlField & 0x80000000) >> 31U) == 0){
        clcw =operationalControlField;
    }
}
CLCW::CLCW(bool controlWordType, uint8_t clcwVersion, uint8_t statusField, uint8_t copInEffect, uint8_t vcId,
           uint8_t spare, bool noRfAvailable, bool noBitLock, bool lockout, bool wait, bool retransmit,
           uint8_t farmBCounter, bool spare2, uint8_t reportValue) {
    clcw = 0;
    clcw = controlWordType << 31U | (clcwVersion & 0x3) << 29U | (statusField & 0x7) << 26U | (copInEffect & 0x3) << 24U |
            (vcId & 0x3F) << 18U | (spare & 0x3) << 16U | noRfAvailable << 15U | noBitLock << 14U | lockout << 13U | wait << 12U |
            retransmit << 11U | (farmBCounter & 0x3) << 9U | spare << 8U | reportValue;
}


