
#ifndef CCSDS_TM_PACKETS_PACKETTM_HPP
#define CCSDS_TM_PACKETS_PACKETTM_HPP
struct TransferFrameHeaderTM : public TransferFrameHeader {
public:
	TransferFrameHeaderTM(uint8_t* pckt) : TransferFrameHeader(pckt) {}

	/**
	 * @brief The Operational Control Field Flag indicates the presence or absence of the Operational Control Field
	 */
	const bool operational_control_field_flag() const {
		return (packet_header[1]) & 0x01;
	}

	/**
	 * @brief Provides  a  running  count  of  the  Transfer  Frames  which  have  been  transmitted  through  the
	 * same  Master  Channel.
	 */
	const uint8_t master_channel_frame_count() const {
		return packet_header[2];
	}

	/**
	 * @brief contain  a  sequential  binary  count (modulo-256) of each Transfer Frame transmitted within a
	 * specific Virtual Channel.
	 */
	const uint8_t virtual_channel_frame_count() const {
		return packet_header[3];
	}

	/**
	 * @brief Indicates the presence of the secondary header.
	 */

	const bool transfer_frame_secondary_header_flag() const {
		return (packet_header[4] << 7U & 0x80);
	}

	/**
	 * @brief Signals the type of data which are inserted into the Transfer Frame Data Field (VCA_SDU or Packets).
	 */

	const bool synchronization_flag() const {
		return (packet_header[4] << 6U & 0x80);
	}

	const bool packet_order_flag() const {
		return (packet_header[4] << 5U & 0x80);
	}


	const uint16_t first_header_pointer() const {
		return (static_cast<uint16_t>((packet_header[4]) & 0x07)) << 8U | (static_cast<uint16_t>((packet_header[5])));
	}
};
#endif // CCSDS_TM_PACKETS_PACKETTM_HPP
