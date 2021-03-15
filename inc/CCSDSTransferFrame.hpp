#ifndef CCSDS_TRANSFERFRAME_HPP
#define CCSDS_TRANSFERFRAME_HPP

#include "CCSDS_Definitions.hpp"
#include "etl/String.hpp"

class CCSDSTransferFrame {
private:
    virtual void createPrimaryHeader(){};

    /**
     * @brief Generates the transfer frame secondary header
     *
     * @attention At the moment the secondary header part is not implemented, since it is not required.
     */
#if TM_SECONDARY_HEADER_SIZE > 0U
    virtual void createSecondaryHeader(String<TM_SECONDARY_HEADER_SIZE - 1>& dataField){};
#endif

protected:
    virtual ~CCSDSTransferFrame(){ };

public:
    virtual uint16_t getTransferFrameSize(){};
};

#endif // CCSDS_TRANSFERFRAME_HPP
