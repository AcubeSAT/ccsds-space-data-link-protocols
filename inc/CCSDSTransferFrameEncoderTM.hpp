#ifndef CCSDS_TRANSFERFRAMEENCODERTM_HPP
#define CCSDS_TRANSFERFRAMEENCODERTM_HPP

#include "CCSDS_Definitions.hpp"
#include "CCSDSTransferFrameTM.hpp"
#include "CCSDSTransferFrameEncoder.hpp"
#include "etl/String.hpp"


class CCSDSTransferFrameEncoderTM : public CCSDSTransferFrameTM, CCSDSTransferFrameEncoder {

private:
    /**
     * @brief Store the encoded sequence of transfer frames.
     * @details The ASM synchronization bits are also included in the sequence
     */
    String<TM_MAX_PACKET_SIZE> encodedFrame;

public:
    /**
     * @brief Generate a transfer frame sequence with the data provided
     */
    void encodeFrame(CCSDSTransferFrameTM &transferFrame,
                     String<(TM_MAX_PACKET_SIZE / (tm_transfer_frame_size + tc_synch_bits_size)) *
                            tm_frame_data_field_size> &data,
                     const uint32_t *packetSizes = nullptr);

    /**
     * @brief Get the encoded transfer frame sequence
     */
    String<TM_MAX_PACKET_SIZE> getEncodedPacket() {
        return encodedFrame;
    }

    /**
	 * @brief When called it appends the ASM synchronization bits to the `encodedFrame` string
	 */
    void appendSynchBits() override;
};

#endif // CCSDS_TRANSFERFRAMEENCODERTM_HPP