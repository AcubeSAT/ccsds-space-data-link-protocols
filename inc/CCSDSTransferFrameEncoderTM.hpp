#ifndef CCSDS_CCSDSTRANSFERFRAMEENCODERTM_HPP
#define CCSDS_CCSDSTRANSFERFRAMEENCODERTM_HPP

#include "CCSDS_Definitions.hpp"
#include "CCSDSTransferFrameTM.hpp"
#include "etl/String.hpp"

#define SYNCH_BITS_SIZE 8U

class CCSDSTransferFrameEncoderTM : public CCSDSTransferFrameTM {

private:
	/**
	 * @brief Store the encoded sequence of transfer frames.
	 * @details The ASM synchronization bits are also included in the sequence
	 */
	String<TM_MAX_PACKET_SIZE> encodedFrame;

    /**
     * @brief Hold the virtual channel ID included in the packet
     */
    uint8_t virtChannelID;

public:
	/**
	 * @brief Generate a transfer frame sequence with the data provided
	 */
	void encodeFrame(CCSDSTransferFrameTM& transferFrame,
	                 String<(TM_MAX_PACKET_SIZE / (TM_TRANSFER_FRAME_SIZE + SYNCH_BITS_SIZE)) * TM_FRAME_DATA_FIELD_SIZE>& data,
	                 const uint32_t* packetSizes = nullptr);

	/**
	 * @brief When called it appends the ASM synchronization bits to the `encodedFrame` string
	 */
	void appendSynchBits();

	/**
	 * @brief Get the encoded transfer frame sequence
	 */
	String<TM_MAX_PACKET_SIZE> getEncodedPacket() {
		return encodedFrame;
	}
};

#endif // CCSDS_CCSDSTRANSFERFRAMEENCODERTM_HPP
