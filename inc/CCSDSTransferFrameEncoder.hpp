#ifndef CCSDS_TM_PACKETS_CCSDSTRANSFERFRAMEENCODER_HPP
#define CCSDS_TM_PACKETS_CCSDSTRANSFERFRAMEENCODER_HPP

#include "CCSDS_Definitions.hpp"
#include "etl/String.hpp"

#define SECONDARY_HEADER_PRESENT 1
#define SECONDARY_HEADER_SIZE 64 // The size of the secondary header in octets, if present

#define OPERATIONAL_CONTROL_FIELD 0 // The presence of the Operational Control Field in the Transfer Frame trailer

class CCSDSTransferFrameEncoder {
private:
	/**
	 * Store the transfer frame primary header
	 */
	String<PRIMARY_HEADER_SIZE> primaryHeader;
#if SECONDARY_HEADER_PRESENT
	/**
	 * Store the transfer frame secondary header
	 */
	String<SECONDARY_HEADER_SIZE> secondaryHeader;
	/**
	 *
	 * @return
	 */
	void createSecondaryHeader();
#endif
	/**
	 *
	 * @note As per CCSDS 132.0-B-2 recommendation, the transfer frame version number is `00`
	 *
	 * @return
	 */
	void createPrimaryHeader();

public:
	/**
	 *
	 * @return
	 */
	String<TRANSFER_FRAME_SIZE> generateTransferFrame();
};


#endif //CCSDS_TM_PACKETS_CCSDSTRANSFERFRAMEENCODER_HPP
