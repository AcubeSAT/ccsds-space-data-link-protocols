#ifndef CCSDS_TRANSFERFRAMEENCODERTC_HPP
#define CCSDS_TRANSFERFRAMEENCODERTC_HPP

#include "CCSDS_Definitions.hpp"
#include "CCSDSTransferFrameTC.hpp"
#include "CCSDSTransferFrameEncoder.hpp"
#include "etl/String.hpp"


class CCSDSTransferFrameEncoderTC : public CCSDSTransferFrameTC, CCSDSTransferFrameEncoder {

private:
    /**
     * @brief Store the encoded sequence of transfer frames.
     * @details The ASM synchronization bits are also included in the sequence
     */
    String<TC_MAX_TRANSFER_FRAME_SIZE> encodedFrame;

public:
    /**
     * @brief Generate a transfer frame sequence with the data provided
     */
    void encodeFrame(CCSDSTransferFrameTC& transferFrame,
                     String<(TC_MAX_TRANSFER_FRAME_SIZE / (TM_TRANSFER_FRAME_SIZE + TC_SYNCH_BITS_SIZE)) * TM_FRAME_DATA_FIELD_SIZE>& data,
                     const uint32_t* packetSizes = nullptr);

    /**
     * @brief Get the encoded transfer frame sequence
     */
    String<TC_MAX_TRANSFER_FRAME_SIZE> getEncodedPacket() {
        return encodedFrame;
    }

    /**
	 * @brief When called it appends the ASM synchronization bits to the `encodedFrame` string
	 */
    void appendSynchBits() override;
};

#endif // CCSDS_TRANSFERFRAMEENCODERTC_HPP
