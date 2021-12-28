#ifndef CCSDS_TM_PACKETS_PACKETTM_HPP
#define CCSDS_TM_PACKETS_PACKETTM_HPP
#include "Packet.hpp"

struct TransferFrameHeaderTM : public TransferFrameHeader {
public:
    TransferFrameHeaderTM(uint8_t *pckt) : TransferFrameHeader(pckt) {}

    /**
     * @brief The Operational Control Field Flag indicates the presence or absence of the Operational Control Field
     */
     bool operational_control_field_flag() const {
        return (packet_header[1]) & 0x01;
    }

    /**
     * @brief Provides  a  running  count  of  the  Transfer  Frames  which  have  been  transmitted  through  the
     * same  Master  Channel.
     */
     uint8_t master_channel_frame_count() const {
        return packet_header[2];
    }

    /**
     * @brief contain  a  sequential  binary  count (modulo-256) of each Transfer Frame transmitted within a
     * specific Virtual Channel.
     */
     uint8_t virtual_channel_frame_count() const {
        return packet_header[3];
    }

    /**
     * @brief Indicates the presence of the secondary header.
     */

     bool transfer_frame_secondary_header_flag() const {
        return (packet_header[4] & 0x80) >> 7U;
    }

    /**
     * @brief Signals the type of data which are inserted into the Transfer Frame Data Field (VCA_SDU or Packets).
     */

     bool synchronization_flag() const {
        return (packet_header[4] & 0x40) >> 6U;
    }

     bool packet_order_flag() const {
        return (packet_header[4] & 0x20) >> 5U;
    }

     uint16_t first_header_pointer() const {
        return (static_cast<uint16_t>((packet_header[4]) & 0x07)) << 8U | (static_cast<uint16_t>((packet_header[5])));
    }

     uint16_t transfer_frame_data_field_status() const {
        return (static_cast<uint16_t>((packet_header[4])) << 8U | (static_cast<uint16_t>((packet_header[5]))));
    }

};

struct PacketTM:public Packet {

    const uint8_t *packet_pl_data() const {
		return data;
	}


	const TransferFrameHeaderTM transfer_frame_header() const {
		return hdr;
	}

    uint8_t transfer_frame_version_number() const {
		return transferFrameVersionNumber;
	}


    uint16_t spacecraft_id() const {
		return scid;
	}

    uint8_t virtual_channel_id() const {
		return vcid;
	}

    uint8_t master_channel_frame_count() const {
		return masterChannelFrameCount;
	}

    uint8_t virtual_channel_frame_count() const {
		return virtualChannelFrameCount;
	}

    uint16_t transfer_frame_data_field_status() const {
		return transferFrameDataFieldStatus;
	}

    uint16_t packet_length() const {
		return packetLength;
	}

	uint8_t *packet_data() const {
		return packet;
	}

	const uint8_t *secondary_header() const {
		return secondaryHeader;
	}

    uint16_t first_header_pointer() const {
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

	PacketTM(uint8_t *packet, uint16_t packet_length, uint8_t virtualChannelFrameCount, uint8_t scid,
	         uint16_t vcid, uint8_t masterChannelFrameCount, uint8_t* secondary_header,
	         uint16_t transferFrameDataFieldStatus,uint16_t firstHeaderPointer, PacketType t=TM)//added firstHeaderPointer as an argument
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
