
#include <cstdint>

#ifndef CCSDS_TM_PACKETS_PACKET_H
#define CCSDS_TM_PACKETS_PACKET_H

enum TransferFrameType {
    TC, TM
};

struct TransferFrameHeader {
    TransferFrameHeader(uint8_t * transfer_frame) {
		transfer_frame_header = transfer_frame;
    }

    /**
     * @brief The ID of the spacecraft
     */
    const uint16_t spacecraftId(enum TransferFrameType transfer_frame_type) const {
        if (transfer_frame_type == TC) {
            return (static_cast<uint16_t>(transfer_frame_header[0] & 0x03) << 8U) | (static_cast<uint16_t>(transfer_frame_header[1]));
        } else {
            return ((static_cast<uint16_t>(transfer_frame_header[0]) & 0x3F) << 2U) |
                   ((static_cast<uint16_t>(transfer_frame_header[1])) & 0xC0) >> 6U;
        }
    }

    /**
     * @brief The virtual channel ID this channel is transferred in
     */
    const uint8_t vcid(enum TransferFrameType transfer_frame_type) const {
        if (transfer_frame_type == TC) {
            return (transfer_frame_header[2] >> 2U) & 0x3F;
        } else {
            return ((transfer_frame_header[1] & 0x0E)) >> 1U;
        }
    }

protected:
    uint8_t * transfer_frame_header;
};

class TransferFrame {
private:
	TransferFrameType type;

public:
	TransferFrame(TransferFrameType t, uint16_t transfer_frame_length, uint8_t * transfer_frame):
	      type(t), transferFrameLength(transfer_frame_length), transferFrame(transfer_frame){};

	/**
     * @brief Appends the CRC code (given that the corresponding Error Correction field is present in the given
     * virtual channel)
	 */

	void appendCRC();

protected:
	uint16_t transferFrameLength;
	uint8_t * transferFrame;
	uint8_t *data;
	uint16_t calculateCRC(const uint8_t *transferFrame, uint16_t len);

};

#endif // CCSDS_TM_PACKETS_PACKET_H
