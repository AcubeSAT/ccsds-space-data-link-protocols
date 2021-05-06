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
    String<tc_max_header_size> encodedFrame;

public:
    /**
     * @brief Generate a transfer frame sequence with the data provided
     */
    void encodeFrame(CCSDSTransferFrameTC &transferFrame,
                     String<(tc_max_header_size / (tm_transfer_frame_size + tc_synch_bits_size)) *
                            tm_frame_data_field_size> &data,
                     const uint32_t *packetSizes = nullptr);

    /**
     * @brief Get the encoded transfer frame sequence
     */
    String<tc_max_header_size> getEncodedPacket() {
        return encodedFrame;
    }

    /**
	 * @brief When called it appends the ASM synchronization bits to the `encodedFrame` string
	 */
    void appendSynchBits() override;
};

#endif // CCSDS_TRANSFERFRAMEENCODERTC_HPP
