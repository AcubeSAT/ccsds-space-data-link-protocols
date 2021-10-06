
#ifndef CCSDS_TM_PACKETS_PACKETTM_HPP
#define CCSDS_TM_PACKETS_PACKETTM_HPP
#include "Packet.hpp"
struct TransferFrameHeaderTM : public TransferFrameHeader {
public:
    TransferFrameHeaderTM(uint8_t *pckt) : TransferFrameHeader(pckt) {}

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
        return (packet_header[4] & 0x80) >> 7U;
    }

    /**
     * @brief Signals the type of data which are inserted into the Transfer Frame Data Field (VCA_SDU or Packets).
     */

    const bool synchronization_flag() const {
        return (packet_header[4] & 0x40) >> 6U;
    }

    const bool packet_order_flag() const {
        return (packet_header[4] & 0x20) >> 5U;
    }

    const uint16_t first_header_pointer() const {
        return (static_cast<uint16_t>((packet_header[4]) & 0x07)) << 8U | (static_cast<uint16_t>((packet_header[5])));
    }


};


struct PacketTM {

	// This only compares the frame sequence number of two packets both as a way to save time when comparing the
	// data fields and because this is handy when getting rid of duplicate packets. However this could result in
	// undesired behavior if we're to delete different packets that share a frame sequence number for some reason.
	// This is normally not allowed but we have to cross-check if it is compatible with FARM checks


	// TODO: Handle CLCWs without any ambiguities

	const uint8_t *packet_pl_data() const {
		return data;
	}


	const TransferFrameHeaderTM transfer_frame_header() const {
		return hdr;
	}

	const uint8_t transfer_frame_version_number() const {
		return transferFrameVersionNumber;
	}


	const uint16_t spacecraft_id() const {
		return scid;
	}

	const uint8_t virtual_channel_id() const {
		return vcid;
	}

	const uint8_t master_channel_frame_count() const {
		return masterChannelFrameCount;
	}

	const uint8_t virtual_channel_frame_count() const {
		return virtualChannelFrameCount;
	}

	const uint16_t transfer_frame_data_field_status() const {
		return transferFrameDataFieldStatus;
	}

	const uint16_t packet_length() const {
		return packetLength;
	}

	uint8_t *packet_data() const {
		return packet;
	}

	const uint8_t *secondary_header() const {
		return secondaryHeader;
	}

	const uint16_t first_header_pointer() const {
		return firstHeaderPointer;
	}

	uint8_t *operational_control_field() const {
		return operationalControlField;
	}

	// Setters are not strictly needed in this case. The are just offered as a utility functions for the VC/MAP
	// generation services when segmenting or blocking transfer frames.

	void set_packet_data(uint8_t *packt_data) {
		packet = packt_data;
	}

	void set_packet_length(uint16_t packt_len) {
		packetLength = packt_len;
	}

	/**
     * @brief Appends the CRC code (given that the corresponding Error Correction field is present in the given
     * virtual channel)
	 */
	void append_crc_tm();

	uint16_t calculate_crc_tm(uint8_t *packet, uint16_t len);


	PacketTM(uint8_t *packet, uint16_t packet_length, uint8_t virtualChannelFrameCount, uint8_t scid,
	         uint16_t vcid, uint8_t masterChannelFrameCount, uint8_t* secondary_header, uint16_t transferFrameDataFieldStatus
	         , uint8_t transferFrameVersionNumber)
	    : packet(packet), hdr(packet), packetLength(packet_length), scid(scid), vcid(vcid),
	      transferFrameDataFieldStatus(transferFrameDataFieldStatus), transferFrameVersionNumber(transferFrameVersionNumber),
	      secondaryHeader(secondary_header), virtualChannelFrameCount(virtualChannelFrameCount),
	      masterChannelFrameCount(masterChannelFrameCount), firstHeaderPointer(firstHeaderPointer) {}

	PacketTM(uint8_t *packet, uint16_t packet_length);

private:
	uint8_t *packet;
	TransferFrameHeaderTM hdr;
	uint16_t packetLength;
	uint8_t masterChannelFrameCount;
	uint8_t virtualChannelFrameCount;
	uint8_t scid;
	uint16_t vcid;
	uint16_t transferFrameDataFieldStatus;
	uint8_t transferFrameVersionNumber;
	uint8_t * secondaryHeader;
	uint8_t *data;
	uint16_t firstHeaderPointer;
	uint8_t *operationalControlField;
};




#endif // CCSDS_TM_PACKETS_PACKETTM_HPP
