#ifndef CCSDS_TM_PACKETS_CCSDSTRANSFERFRAMEENCODER_HPP
#define CCSDS_TM_PACKETS_CCSDSTRANSFERFRAMEENCODER_HPP

#include "CCSDS_Definitions.hpp"
#include "CCSDSTransferFrame.hpp"
#include "etl/String.hpp"

class CCSDSTransferFrameEncoder : public CCSDSTransferFrame {
private:
	/**
	 *
	 */
	String<MAX_PACKET_SIZE> encodedFrame;


public:
	/**
	 *
	 */
	void encodeFrame(CCSDSTransferFrame& transferFrame,
	                 String<(MAX_PACKET_SIZE / TRANSFER_FRAME_SIZE) * FRAME_DATA_FIELD_SIZE>& data);

	/**
	 *
	 */
	String<MAX_PACKET_SIZE> getEncodedPacket() { return encodedFrame; }

};

#endif // CCSDS_TM_PACKETS_CCSDSTRANSFERFRAMEENCODER_HPP
