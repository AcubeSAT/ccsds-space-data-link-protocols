#include <cstdint>
#include "CCSDSChannel.hpp"
#include "TransferFrameTM.hpp"


struct CLCW{
    uint32_t clcw;
    CLCW(TransferFrameTM *transferFrame, bool frameErrorControlFieldExists);
};