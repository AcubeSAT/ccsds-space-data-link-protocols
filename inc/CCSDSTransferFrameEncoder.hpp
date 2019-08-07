#ifndef CCSDS_TM_PACKETS_CCSDSTRANSFERFRAMEENCODER_HPP
#define CCSDS_TM_PACKETS_CCSDSTRANSFERFRAMEENCODER_HPP

#include "CCSDS_Definitions.hpp"
#include "CCSDSTransferFrame.hpp"
#include "etl/String.hpp"

#define SYNCH_BITS_SIZE 8U

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
	                 String<(MAX_PACKET_SIZE / (TRANSFER_FRAME_SIZE + SYNCH_BITS_SIZE)) * FRAME_DATA_FIELD_SIZE>& data);

	/**
	 *
	 */
	void appendSynchBits();

	/**
	 *
	 */
	String<MAX_PACKET_SIZE> getEncodedPacket() { return encodedFrame; }

};

#endif // CCSDS_TM_PACKETS_CCSDSTRANSFERFRAMEENCODER_HPP
