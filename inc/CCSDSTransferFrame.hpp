#ifndef CCSDS_TM_PACKETS_CCSDSTRANSFERFRAME_HPP
#define CCSDS_TM_PACKETS_CCSDSTRANSFERFRAME_HPP

#include "CCSDS_Definitions.hpp"
#include "etl/String.hpp"

class CCSDSTransferFrame {
private:
	/**
	 * @note As per CCSDS 132.0-B-2 recommendation, the transfer frame version number is `00`
	 */
	void createPrimaryHeader(uint8_t virtChannelID, bool secondaryHeaderPresent, bool ocfFlag);

protected:
	// Field information containers
	/**
	 * Store the transfer frame primary header
	 */
	String<PRIMARY_HEADER_SIZE> primaryHeader;

	/**
	 * Store the transfer frame secondary header
	 */
#if SECONDARY_HEADER_SIZE > 0U
	String<SECONDARY_HEADER_SIZE> secondaryHeader;
#endif

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
	 *
	 * @param datFieldSize
	 * @param virtChannelID
	 * @param secHeaderSize
	 */
	CCSDSTransferFrame(uint8_t datFieldSize, uint8_t vChannelID, uint8_t secHeaderSize = 0)
	    : secondaryHeaderSize(secHeaderSize), dataFieldSize(datFieldSize), primaryHeader(),
#if SECONDARY_HEADER_SIZE > 0U
	      secondaryHeader(),
#endif
	      dataField(), operationalControlField(), errorControlField(), masterChannelFrameCount(0),
	      virtualChannelFrameCount(0), virtualChannelID(vChannelID) {
		createPrimaryHeader(vChannelID, secHeaderSize > 0U, not operationalControlField.empty());
	}

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
	void increaseMasterChannelFrameCount(){};

	/**
	 *
	 */
	void decreaseMasterChannelFrameCount(){};

	/**
	 *
	 */
	void increaseVirtualChannelFrameCount(){};

	/**
	 *
	 */
	void decreaseVirtualChannelFrameCount(){};

	/**
	 *
	 */
	String<PRIMARY_HEADER_SIZE> getPrimaryHeader() {
		return primaryHeader;
	};

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

#endif // CCSDS_TM_PACKETS_CCSDSTRANSFERFRAME_HPP
