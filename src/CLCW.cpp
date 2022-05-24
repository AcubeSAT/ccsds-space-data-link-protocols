#include <cstdint>
#include "CLCW.hpp"


CLCW::CLCW(TransferFrameTM *transferFrame, bool frameErrorControlFieldExists) {
    clcw = 0;
    if(transferFrame->operationalControlFieldExists() && transferFrame->getOperationalControlField()[0] == 0){
        if(frameErrorControlFieldExists){
            uint8_t* operationalControlField = transferFrame->getOperationalControlField() - 2;
            clcw = clcw | operationalControlField[0] | operationalControlField[1] |operationalControlField[2] | operationalControlField[3];
        }
        uint8_t* operationalControlField = transferFrame->getOperationalControlField();
        clcw = clcw | operationalControlField[0] | operationalControlField[1] |operationalControlField[2] | operationalControlField[3];
    }
}

