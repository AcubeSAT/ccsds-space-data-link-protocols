#ifndef CCSDS_TM_PACKETS_CCSDSTRANSFERFRAMEENCODER_HPP
#define CCSDS_TM_PACKETS_CCSDSTRANSFERFRAMEENCODER_HPP

#include "CCSDS_Definitions.hpp"
#include "CCSDSTransferFrame.hpp"
#include "etl/String.hpp"

#define SYNCH_BITS_SIZE 8U

class CCSDSTransferFrameEncoder : public CCSDSTransferFrame {
private:
	/**
	 * @brief Store the encoded sequence of transfer frames.
	 * @details The ASM synchronization bits are also included in the sequence
	 */
	String<MAX_PACKET_SIZE> encodedFrame;


public:
	/**
	 * @brief Generate a transfer frame sequence with the data provided
	 */
	void encodeFrame(CCSDSTransferFrame& transferFrame,
	                 String<(MAX_PACKET_SIZE / (TRANSFER_FRAME_SIZE + SYNCH_BITS_SIZE)) * FRAME_DATA_FIELD_SIZE>& data);

	/**
	 * @brief When called it appends the ASM synchronization bits to the `encodedFrame` string
	 */
	void appendSynchBits();

	/**
	 * @brief Get the encoded transfer frame sequence
	 */
	String<MAX_PACKET_SIZE> getEncodedPacket() { return encodedFrame; }

};

#endif // CCSDS_TM_PACKETS_CCSDSTRANSFERFRAMEENCODER_HPP
