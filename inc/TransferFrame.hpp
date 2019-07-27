#ifndef CCSDS_TM_PACKETS_TRANSFERFRAME_HPP
#define CCSDS_TM_PACKETS_TRANSFERFRAME_HPP

#include "CCSDS_Definitions.hpp"
#include "etl/String.hpp"

class TransferFrame {
private:

public:
	/**
	 *
	 * @return
	 */
	String<TRANSFER_FRAME_SIZE> getTransferFrame();
};


#endif //CCSDS_TM_PACKETS_TRANSFERFRAME_HPP
