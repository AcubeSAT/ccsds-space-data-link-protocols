#ifndef CCSDS_TM_PACKETS_PACKETTM_HPP
#define CCSDS_TM_PACKETS_PACKETTM_HPP
#include "Packet.hpp"

struct TransferFrameHeaderTM : public TransferFrameHeader {
public:
	/**
	 * @see p. 4.1.2 from TM SPACE DATA LINK PROTOCOL
	 */
    TransferFrameHeaderTM(uint8_t *pckt) : TransferFrameHeader(pckt) {}

    /**
     * @brief The Operational Control Field Flag indicates the presence or absence of the Operational Control Field
     * @details Bit  15  of  the  Transfer  Frame  Primary  Header
     * @see p. 4.1.2.4 from TM SPACE DATA LINK PROTOCOL
	 */
    const bool operational_control_field_flag() const {
        return (packet_header[1]) & 0x01;
    }

    /**
     * @brief Provides  a  running  count  of  the  Transfer  Frames  which  have  been  transmitted  through  the
     * same  Master  Channel.
     * @details Bits  16–23  of  the  Transfer  Frame  Primary  Header
     * @see p. 4.1.2.5 from TM SPACE DATA LINK PROTOCOL
	 */
    const uint8_t master_channel_frame_count() const {
        return packet_header[2];
    }

    /**
     * @brief contain  a  sequential  binary  count (modulo-256) of each Transfer Frame transmitted within a
     * specific Virtual Channel.
     * @details Bits  24–31  of  the  Transfer  Frame  Primary  Header
     * @see p. 4.1.2.6 from TM SPACE DATA LINK PROTOCOL
     */
    const uint8_t virtual_channel_frame_count() const {
        return packet_header[3];
    }

    /**
     * @brief Indicates the presence of the secondary header.
     * @details Bit  32  of  the Transfer  Frame  Primary  Header
     * @see p. 4.1.2.7.2 from TM SPACE DATA LINK PROTOCOL
     */

    const bool transfer_frame_secondary_header_flag() const {
        return (packet_header[4] & 0x80) >> 7U;
    }

    /**
     * @brief Signals the type of data which are inserted into the Transfer Frame Data Field (VCA_SDU or Packets).
     * @details Bit 33 of the Transfer Frame Primary Header
     * @see p. 4.1.2.7.3 from TM SPACE DATA LINK PROTOCOL
     */

    const bool synchronization_flag() const {
        return (packet_header[4] & 0x40) >> 6U;
    }

	/**
     * @brief If the Synchronization Flag is set to ‘0’,t he Packet Order Flag is reserved for
				future use by the CCSDS and shall be set to ‘0’. If the Synchronization Flag is
				set to ‘1’, the use of the Packet Order Flag is undefined.
	 * @details Bit 34 of the Transfer Frame Primary Header
	 * @see p. 4.1.2.7.4 from TM SPACE DATA LINK PROTOCOL
	 */
    const bool packet_order_flag() const {
        return (packet_header[4] & 0x20) >> 5U;
    }
	/**
	 * @brief If the Synchronization Flag is set to ‘0’, the First Header Pointer shall contain
	 *		the position of the first octet of the first Packet that starts in the Transfer Frame Data Field.
	 *		Otherwise it is undefined.
	 * @details Bits 37–47 of the Transfer Frame Primary Header
	 * @see p. 4.1.2.7.6 from TM SPACE DATA LINK PROTOCOL
	 */
    const uint16_t first_header_pointer() const {
        return (static_cast<uint16_t>((packet_header[4]) & 0x07)) << 8U | (static_cast<uint16_t>((packet_header[5])));
    }

	/**
	 * @brief Contains the 	a)Transfer Frame Secondary Header Flag (1 bit)
	 *							b) Synchronization Flag (1 bit)
	 *							c) Packet Order Flag (1 bit)
	 *							d) Segment Length Identifier (2 bits)
	 *							e) First Header Pointer (11 bits)
	 * @details Bits  32–47  of  the  Transfer  Frame  Primary  Header.
	 * @see p. 4.1.2.7 from TM SPACE DATA LINK PROTOCOL
	 */
    const uint16_t transfer_frame_data_field_status() const {
        return (static_cast<uint16_t>((packet_header[4])) << 8U | (static_cast<uint16_t>((packet_header[5]))));
    }

};

struct PacketTM:public Packet {

	/**
	 * @note this function is the same with uint8_t *packet_data() const.
	 * it is duplicated
	 */

	const uint8_t *packet_pl_data() const {
		return data;
	}

	/**
	 * @see p. 4.1.2 from TM SPACE DATA LINK PROTOCOL
	 */
	const TransferFrameHeaderTM transfer_frame_header() const {
		return hdr;
	}
	/**
	 * @brief Bits  0–1  of  the  Transfer  Frame  Secondary  Header.
	 * 			(shall be set to ‘00’)
	 * @see p. 4.1.2.2.2 from TM SPACE DATA LINK PROTOCOL
	 */
	const uint8_t transfer_frame_version_number() const {
		return transferFrameVersionNumber;
	}

	/**
	 * @brief The Spacecraft Identifier shall provide the identification of the spacecraft which
	 *		is associated with the data contained in the Transfer Frame.
	 * @details Bits  2–11  of  the  Transfer  Frame  Primary  Header.
	 * @see p. 4.1.2.2.3 from TM SPACE DATA LINK PROTOCOL
	 */
	const uint16_t spacecraft_id() const {
		return scid;
	}

	/**
	 * @brief The Virtual Channel Identifier provides the identification of the Virtual Channel.
	 * @details Bits 12–14 of the Transfer Frame Primary Header.
	 * @see p. 4.1.2.3 from TM SPACE DATA LINK PROTOCOL
	 */
	const uint8_t virtual_channel_id() const {
		return vcid;
	}

	/**
	 * @brief This  8-bit  field  shall  contain  a  sequential  binary  count  (modulo-256)  of  each
	 * Transfer Frame transmitted within a specific Master Channel.
	 * @details Bits  16–23  of  the  Transfer  Frame  Primary  Header.
	 * @see p. 4.1.2.5 from TM SPACE DATA LINK PROTOCOL
	 */
	const uint8_t master_channel_frame_count() const {
		return masterChannelFrameCount;
	}

	/**
	 * @brief This  8-bit  field  shall  contain  a  sequential  binary  count  (modulo-256)  of  each
	 * Transfer Frame transmitted within a specific Virtual Channel.
	 * @details Bits  24-31  of  the  Transfer  Frame  Primary  Header.
	 * @see p. 4.1.2.6 from TM SPACE DATA LINK PROTOCOL
	 */
	const uint8_t virtual_channel_frame_count() const {
		return virtualChannelFrameCount;
	}

	/**
	     *
	     * @brief Contains the 	a)Transfer Frame Secondary Header Flag (1 bit)
	     *                      b) Synchronization Flag (1 bit)
	     *                      c) Packet Order Flag (1 bit)
	     *                      d) Segment Length Identifier (2 bits)
	     *                      e) First Header Pointer (11 bits)
	     * @details Bits  32–47  of  the  Transfer  Frame  Primary  Header.
		 * @see p. 4.1.2.7 from TM SPACE DATA LINK PROTOCOL
	 */
	const uint16_t transfer_frame_data_field_status() const {
		return transferFrameDataFieldStatus;
	}

	const uint16_t packet_length() const {
		return packetLength;
	}

	uint8_t *packet_data() const {
		return packet;
	}

	/**
	 * @see p. 4.1.3 from TM DATA LINK PROTOCOL
	 */
	const uint8_t *secondary_header() const {
		return secondaryHeader;
	}

	/**
	 * @brief If  the  Synchronization  Flag  is  set  to  ‘0’,  the  First  Header  Pointer  shall  contain
	 *		the position of the first octet of the first Packet that starts in the Transfer Frame Data Field.
	 *		Otherwise it is undefined.
	 * @details Bits 37–47 of the Transfer Frame Primary Header.
	 * @see p. 4.1.2.7.6 from TM SPACE DATA LINK PROTOCOL
	 */
	const uint16_t first_header_pointer() const {
		return firstHeaderPointer;
	}

	/**
	 * @see p. 4.1.5 from TM SPACE DATA LINK PROTOCOL
	 */
	uint8_t *operational_control_field() const {
		return operationalControlField;
	}

	// Setters are not strictly needed in this case. They are just offered as a utility functions for the VC/MAP
	// generation services when segmenting or blocking transfer frames.

	void set_packet_data(uint8_t *packt_data) {
		packet = packt_data;
	}

	void set_packet_length(uint16_t packt_len) {
		packetLength = packt_len;
	}

	PacketTM(uint8_t *packet, uint16_t packet_length, uint8_t virtualChannelFrameCount, uint8_t scid,
	         uint16_t vcid, uint8_t masterChannelFrameCount, uint8_t* secondary_header,
	         uint16_t transferFrameDataFieldStatus, PacketType t=TM)
	    	:Packet(t, packet_length, packet), hdr(packet), masterChannelFrameCount(masterChannelFrameCount), virtualChannelFrameCount(virtualChannelFrameCount),
	      scid(scid), vcid(vcid),
	      transferFrameDataFieldStatus(transferFrameDataFieldStatus), transferFrameVersionNumber(0),
	      secondaryHeader(secondary_header), firstHeaderPointer(firstHeaderPointer) {}

	PacketTM(uint8_t *packet, uint16_t packet_length, PacketType t=TM);

private:
	TransferFrameHeaderTM hdr;
	uint8_t masterChannelFrameCount;
	uint8_t virtualChannelFrameCount;
	uint8_t scid;
	uint16_t vcid;
	uint16_t transferFrameDataFieldStatus;
	uint8_t transferFrameVersionNumber;
	uint8_t * secondaryHeader;
	uint16_t firstHeaderPointer;
	uint8_t *operationalControlField{};
};

#endif // CCSDS_TM_PACKETS_PACKETTM_HPP
