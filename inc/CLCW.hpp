#include <cstdint>
#include "CCSDSChannel.hpp"
#include "TransferFrameTM.hpp"


struct CLCW{
    uint32_t clcw;
    CLCW(bool operationalControlFieldExists, uint32_t operationalControlField);
    CLCW(bool controlWordType, uint8_t clcwVersion, uint8_t statusField, uint8_t copInEffect, uint8_t vcId, uint8_t spare,
         bool noRfAvailalbe, bool noBitLock, bool lockout, bool wait, bool retransmit, uint8_t farmBCounter, bool spare2, uint8_t reportValue);

public:
    bool getControlWordType();
    uint8_t  getClcwVersion();
    uint8_t getStatusField();
    uint8_t getCopInEffect();
    uint8_t getVcId();
    uint8_t getSpare();
    bool getRfAvailable();
    bool getBitLock();
    bool getLockout();
    bool getWait();
    bool getRetransmit();
    uint8_t farmBCounter();
    bool getSpare2();
    uint8_t getReportValue();
};
