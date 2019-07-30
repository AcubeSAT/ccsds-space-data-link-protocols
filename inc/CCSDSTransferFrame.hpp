#ifndef CCSDS_TM_PACKETS_CCSDSTRANSFERFRAME_HPP
#define CCSDS_TM_PACKETS_CCSDSTRANSFERFRAME_HPP

#include "CCSDS_Definitions.hpp"
#include "etl/String.hpp"


class CCSDSTransferFrame {
protected:
	// Field information containers
	/**
	 * Store the transfer frame primary header
	 */
	String<PRIMARY_HEADER_SIZE> primaryHeader;

	/**
	 * Store the transfer frame secondary header
	 */
	String<SECONDARY_HEADER_MAX_SIZE> secondaryHeader;

	/**
	 *
	 */
	String<FRAME_DATA_FIELD_MAX_SIZE> dataField;

	/**
	 *
	 */
	String<4> operationalControlField;

	/**
	 *
	 */
	String<2> errorControlField;

	// Counters and single variables
	/**
	 *
	 */
	uint8_t secondaryHeaderSize;

	/**
	 *
	 */
	uint8_t dataFieldSize;



public:
	/**
	 *
	 * @param secHeaderSize
	 * @param datFieldSize
	 * @param virtChannelID
	 */
	CCSDSTransferFrame(uint8_t secHeaderSize, uint8_t datFieldSize,
	                   uint8_t vChannelID) : secondaryHeaderSize(secHeaderSize),
	                                            dataFieldSize(datFieldSize),
	                                            primaryHeader(0, PRIMARY_HEADER_SIZE),
	                                            secondaryHeader(0, SECONDARY_HEADER_MAX_SIZE),
	                                            dataField(0, FRAME_DATA_FIELD_MAX_SIZE),
	                                            operationalControlField(0, 4),
	                                            errorControlField(0, 4),
	                                            masterChannelFrameCount(0),
	                                            virtualChannelFrameCount(0), virtualChannelID(vChannelID) {}

    /**
     *
     */
    uint16_t masterChannelID();

	/**
	*
	*/
	uint8_t ocfFlag();

	/**
	 *
	 */
	uint8_t masterChannelFrameCount;

	/**
	 *
	 */
	uint8_t virtualChannelFrameCount;

	/**
	 *
	 */
	uint8_t virtualChannelID;
};


#endif //CCSDS_TM_PACKETS_CCSDSTRANSFERFRAME_HPP
