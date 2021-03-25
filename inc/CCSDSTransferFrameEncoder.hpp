#ifndef CCSDS_CCSDSTRANSFERFRAMEENCODER_HPP
#define CCSDS_CCSDSTRANSFERFRAMEENCODER_HPP

#include "CCSDS_Definitions.hpp"
#include "CCSDSTransferFrame.hpp"
#include "etl/String.hpp"

class CCSDSTransferFrameEncoder : public CCSDSTransferFrame {

public:
    virtual void appendSynchBits() {};
};

#endif // CCSDS_CCSDSTRANSFERFRAMEENCODER_HPP