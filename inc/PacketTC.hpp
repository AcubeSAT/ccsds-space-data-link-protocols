#ifndef CCSDS_TC_PACKET_H
#define CCSDS_TC_PACKET_H

#include <CCSDS_Definitions.hpp>
#include <algorithm>
#include <cstring>
#include <cstdint>
#include <etl/memory.h>
#include "Packet.hpp"

class PacketTC;

/**
     * @brief The packet's service type
     * @see p. 2.2.2 from TC SPACE DATA LINK PROTOCOL
 */
enum ServiceType {
    TYPE_A = 0, TYPE_B = 1
};

enum FDURequestType: uint8_t{
    REQUEST_PENDING = 0,
    REQUEST_POSITIVE_CONFIRM = 1,
    REQUEST_NEGATIVE_CONFIRM = 2,
};


struct TransferFrameHeaderTC : public TransferFrameHeader {
public:
	/**
	 * @see p. 4.1.2 from TC SPACE DATA LINK PROTOCOL
	 */
    TransferFrameHeaderTC(uint8_t *pckt) : TransferFrameHeader(pckt) {}

    /**
     * @brief The bypass Flag determines whether the packet will bypass FARM checks
     * @details Bit 2 of the Transfer Frame Primary Header
     * @see p. 4.1.2.3.1 from TC SPACE DATA LINK PROTOCOL
     */
    const bool bypassFlag() const {
        return (packet_header[0] >> 5U) & 0x01;
    }

    /**
     * @brief The control and command Flag determines whether the packet carries control commands (Type-C) or
     * data (Type-D)
     * @details Bit 3 of the Transfer Frame Primary Header
     * @see p. 4.1.2.3.2 from TC SPACE DATA LINK PROTOCOL
     */
    const bool ctrlAndCmdFlag() const {
        return (packet_header[0] >> 4U) & 0x01;
    }

    /**
     * @brief The length of the transfer frame
     * @details Bits 22–31 of the Transfer Frame Primary Header
     * @see p. 4.1.2.7 from TC SPACE DATA LINK PROTOCOL
	 */
    const uint16_t transferFrameLength() const {
        return (static_cast<uint16_t>(packet_header[2] & 0x03) << 8U) | (static_cast<uint16_t>(packet_header[3]));
    }
};

struct PacketTC:public Packet {
    void setConfSignal(FDURequestType reqSignal) {
        confSignal = reqSignal;
        // TODO Maybe signal the higher procedures here instead of having them manually take care of them
    }

    // This only compares the frame sequence number of two packets both as a way to save time when comparing the
    // data fields and because this is handy when getting rid of duplicate packets. However, this could result in
    // undesired behavior if we're to delete different packets that share a frame sequence number for some reason.
    // This is normally not allowed, but we have to cross-check if it is compatible with FARM checks

    friend bool operator==(const PacketTC &pack1, const PacketTC &pack2) {
        return(pack1.transferFrameSeqNumber == pack2.transferFrameSeqNumber);
    }

    // TODO: Handle CLCWs without any ambiguities

    const uint8_t * packetPlData() const {
        return data;
    }

	// see p. 4.2.1.1.2 from TC SPACE DATA LINK PROTOCOL

	/**
	 * @return Bit 0 of the CLCW (the Control Word Type).
	 * @details This one-bit field shall be set to ‘0’.
	 * @see p. 4.2.1.2 from TC SPACE DATA LINK PROTOCOL
	 */
    const uint8_t controlWordType() const {
		return (data[0] & 0x80 >> 7U);
    }

	/**
	 * @return Bits 3-5 of the CLCW (the Status Field).
	 * @see p. 4.2.1.4 from TC SPACE DATA LINK PROTOCOL
	 *
	 */
    const uint8_t statusField() const {
        return (data[0] & 0x1C) >> 2U;
    }

	/**
	 * @brief Used to indicate the COP that is being used.
	 * @return Bits 6-7 of the CLCW (the COP in Effect parameter)
	 * @see p. 4.2.1.5 from TC SPACE DATA LINK PROTOCOL
	 */
    const uint8_t copInEffect() const {
        return data[0] & 0x03;
    }

	/**
	 * @return Bits 8-13 of the CLCW (the Virtual Channel Identifier of the Virtual Channel
	 * with which this COP report is associated).
	 * @see p. 4.2.1.6 from TC SPACE DATA LINK PROTOCOL
	 */
    const uint8_t vcIdentification() const {
        return (data[1] & 0xFC) >> 2U;
    }

	/**
	 * @brief The No RF Available Flag shall provide a logical indication of the ‘ready’ status
	 * of the radio frequency (RF) elements within the space link provided by the Physical Layer.
	 * @return Bit 16 of the CLCW (the No RF Available Flag).
	 * @see p. 4.2.1.8.2 from TC SPACE DATA LINK PROTOCOL
	 */
    const uint8_t noRfAvail() const {
        return (data[2] & 0x80) >> 7U;
    }

	/**
	 * @return Bit 17 of the CLCW (the No Bit Lock Flag).
	 * @see p. 4.2.1.8.3 from TC SPACE DATA LINK PROTOCOL
	 */
    const uint8_t noBitLock() const {
        return (data[2] & 0x20) >> 5U;
    }

	/**
	 * @return Bit 18 of the CLCW (the Lockout Flag).
	 * @see p. 4.2.1.8.4 from TC SPACE DATA LINK PROTOCOL
	 */
    const uint8_t lockout() const {
        return (data[2] & 0x20) >> 5U;
    }

	/**
	 * @return Bit 19 of the CLCW (the Wait Flag)
	 * @see p. 4.1.2.8.5 from TC SPACE DATA LINK PROTOCOL
	 */
    const uint8_t wait() const {
        return (data[2] & 0x10) >> 4U;
    }

	/**
	 * @return Bit 20 of the CLCW (the Retransmit Flag).
	 * @see p. 4.2.1.8.6 from TC SPACE DATA LINK PROTOCOL
	 */
    const uint8_t retransmit() const {
        return (data[2] & 0x08) >> 3U;
    }

	/**
	 * @return Bits 21-22 of the CLCW (the FARM-B Counter).
	 * @see p. 4.2.1.9 from TC SPACE DATA LINK PROTOCOL
	 */
    const uint8_t farmBCounter() const {
        return (data[2] & 0x06) >> 1U;
    }

	/**
	 * @return Bits 24-31 of the CLCW (the Report Value).
	 * @see p. 4.2.11.1 from TC SPACE DATA LINK PROTOCOL
	 */
    const uint8_t reportValue() const {
        return data[3];
    }


    /**
     * @brief Set the number of repetitions that is determined by the virtual channel
     */
    void setRepetitions(const uint8_t repetitions) {
        reps = repetitions;
    }

    /**
     * @brief Determines whether the packet is marked for retransmission while in the sent queue
     */
    const bool getToBeRetransmitted() const {
        return toBeRetransmitted;
    }

    void setToBeRetransmitted(bool f) {
        toBeRetransmitted = f;
    }

	/**
	 * @return TC transfer frame primary header (the first 5 octets of the TC transfer frame)
	 * @see p. 4.1.2 from TC SPACE DATA LINK PROTOCOL
	 */
    const TransferFrameHeaderTC transferFrameHeader() const {
        return hdr;
    }

	/**
	 * @return Bits 22–31 of the Transfer Frame Primary Header (the Frame Length)
	 * @see p. 4.1.2.7 from TC SPACE DATA LINK PROTOCOL
	 */
    const uint16_t getPacketLength() const {
        return packetLength;
    }

	/**
	 * @see p. 4.1.3.2.2 from TC SPACE DATA LINK PROTOCOL
	 */
    const uint8_t segmentationHeader() const {
        return segHdr;
    }

	/**
	 * @return the Global Virtual Channel Identifier (GVCID)
	 * @see p. 2.1.3 from TC SPACE DATA LINK PROTOCOL
	 */
    const uint8_t globalVirtualChannelId() const {
        return gvcid;
    }

	/**
	 * @return Bits  16–21  of  the  Transfer  Frame  Primary  Header (the  Virtual Channel Identifier (VCID)).
	 * @see p. 4.1.2.6 from TC SPACE DATA LINK PROTOCOL
	 */
    const uint8_t virtualChannelId() const {
        return gvcid & 0x3F;
    };

    const uint8_t mapId() const {
        return mapid;
    }

	/**
	 * @return Bits  6–15  of  the  Transfer  Frame  Primary  Header (the  Spacecraft Identifier (SCID)).
	 * @see p. 4.1.2.5 from TC SPACE DATA LINK PROTOCOL
	 */
    const uint16_t spacecraftId() const {
        return sduid;
    }

	/**
	 * @return Bits 32–39 of the Transfer Frame Primary Header (the Frame Sequence Number, N(S)).
	 * @see p. 4.1.2.8 from TC SPACE DATA LINK PROTOCOL.
	 */
    const uint8_t transferFrameSequenceNumber() const {
        return transferFrameSeqNumber;
    }

	/**
	 * @return Bits 0–1 of the Transfer Frame Primary Header (the (binary encoded) Transfer Frame Version Number).
	 * @see p. 4.1.2.2 from TC SPACE DATA LINK PROTOCOL
	 */
    const uint8_t getTransferFrameVersionNumber() const {
        return transferFrameVersionNumber;
    }

	/**
	 * @see p. 2.2.2 from TC SPACE DATA LINK PROTOCOL
	 */
    const ServiceType getServiceType() const {
        return serviceType;
    }

    const bool acknowledged() const {
        return ack;
    }

	/**
	 * @see p. 2.4.2 from TC SPACE DATA LINK PROTOCOL
	 */
    const uint8_t repetitions() const {
        return reps;
    }

    uint8_t * packetData() const {
        return packet;
    }

    // Setters are not strictly needed in this case. They are just offered as a utility functions for the VC/MAP
    // generation services when segmenting or blocking transfer frames.
    void setSegmentationHeader(uint8_t seg_hdr) {
        segHdr = seg_hdr;
    }

    void setPacketData(uint8_t *packt_data) {
        packet = packt_data;
    }

    void setPacketLength(uint16_t packt_len) {
        packetLength = packt_len;
    }

    void setServiceType(ServiceType serv_type) {
        serviceType = serv_type;
    }

    void setAcknowledgement(bool acknowledgement) {
        ack = acknowledgement;
    }

    void setTransferFrameSequenceNumber(uint8_t frame_seq_number) {
        transferFrameSeqNumber = frame_seq_number;
    }

    PacketTC(uint8_t *packet, uint16_t packet_length, uint8_t seg_hdr, uint8_t gvcid, uint8_t mapid, uint16_t sduid,
             ServiceType service_type, bool seg_hdr_present, PacketType t=TC):  Packet(t,packet_length,packet),
	         hdr(packet), segHdr(seg_hdr), gvcid(gvcid), mapid(mapid), sduid(sduid), serviceType(service_type),
	         transferFrameSeqNumber(0), ack(0), toBeRetransmitted(0), transferFrameVersionNumber(0)
	          {
		data=&packet[5 + 1 * seg_hdr_present];
	}

    PacketTC(uint8_t *packet, uint16_t packet_length, PacketType t=TC);

private:
    bool toBeRetransmitted;
    // This is used by COP to signal the higher procedures
    FDURequestType confSignal;
    TransferFrameHeaderTC hdr;
    uint8_t segHdr;
    uint8_t gvcid;
    uint8_t mapid;
    uint16_t sduid;
    uint8_t transferFrameVersionNumber;
    ServiceType serviceType;
    uint8_t transferFrameSeqNumber;
    bool ack;
    uint8_t reps;
};

/**
     * @brief see p. 4.2 from TC SPACE DATA LINK PROTOCOL
 */
class CLCW {
public:
    void setConfSignal(FDURequestType reqSignal) {
        confSignal = reqSignal;
        // TODO Maybe signal the higher procedures here instead of having them manually take care of them
    }

    /**
     * @brief Field status is mission-specific and not used in the CCSDS data link protocol
	 * @return Bits 3-5 of the CLCW (the Status Field).
	 * @see p. 4.2.1.4 from TC SPACE DATA LINK PROTOCOL
	 *
	 */

    const uint8_t getFieldStatus() const {
        return fieldStatus;
    }

	/**
	 * @brief Used to indicate the COP that is being used.
	 * @return Bits 6-7 of the CLCW (the COP in Effect parameter)
	 * @see p. 4.2.1.5 from TC SPACE DATA LINK PROTOCOL
	 */
    const uint8_t getCopInEffect() const {
        return copInEffect;
    }

	/**
	 * @return Bits 8-13 of the CLCW (the Virtual Channel Identifier of the Virtual Channel
	 * with which this COP report is associated).
	 * @see p. 4.2.1.6 from TC SPACE DATA LINK PROTOCOL
	 */
    const uint8_t vcid() const {
        return vcId;
    }

	/**
	 * @brief The No RF Available Flag shall provide a logical indication of the ‘ready’ status
	 * of the radio frequency (RF) elements within the space link provided by the Physical Layer.
	 * @return Bit 16 of the CLCW (the No RF Available Flag).
	 * @see p. 4.2.1.8.2 from TC SPACE DATA LINK PROTOCOL
	 */
    const bool getNoRFAvail() const {
        return noRFAvail;
    }

    /**
     * @brief Indicates whether "bit lock" is achieved
	 * @return Bit 17 of the CLCW (the No Bit Lock Flag).
	 * @see p. 4.2.1.8.3 from TC SPACE DATA LINK PROTOCOL
	 */
    const bool getNoBitLock() const {
        return noBitLock;
    }

    /**
     * @brief Indicates whether FARM is in the Lockout state
     * @return Bit 18 of the CLCW (the Lockout Flag).
	 * @see p. 4.2.1.8.4 from TC SPACE DATA LINK PROTOCOL
     */
    const bool lockout() const {
        return lckout;
    }

    /**
     * @brief Indicates whether the receiver doesn't accept any packets
	 * @return Bit 19 of the CLCW (the Wait Flag)
	 * @see p. 4.1.2.8.5 from TC SPACE DATA LINK PROTOCOL
     */
    const bool wait() const {
        return wt;
    }

    /**
     * @brief Indicates whether there are Type-A Frames that will need to be retransmitted
     * @return Bit 20 of the CLCW (the Retransmit Flag).
	 * @see p. 4.2.1.8.6 from TC SPACE DATA LINK PROTOCOL
     */
    const bool retransmission() const {
        return retransmit;
    }

    /**
     * @brief The two least significant bits of the virtual channel
     * @return Bits 21-22 of the CLCW (the FARM-B Counter).
	 * @see p. 4.2.1.9 from TC SPACE DATA LINK PROTOCOL
     */
    const uint8_t getFarmBCounter() const {
        return farmBCounter;
    }

    /**
     * @brief Next expected transfer frame number
     * @return Bits 24-31 of the CLCW (the Report Value).
	 * @see p. 4.2.11.1 from TC SPACE DATA LINK PROTOCOL
     */
    const uint8_t getReportValue() const {
        return reportValue;
    }

    // @todo Is this a bit risky, ugly and hacky? Yes, like everything in life. However, I still prefer this over
    //  passing etl strings around but that's probably just me.

    /**
     * @brief Control Link Words used for FARM reports (see Figure 4-6)
     */
    CLCW(uint8_t *data) {
        clcw_data[0] = data[0];
        clcw_data[1] = data[1];
        clcw_data[2] = data[2];
        clcw_data[3] = data[3];

        fieldStatus = (clcw_data[0] & 0x1C) >> 2U;
        copInEffect = clcw_data[0] & 0x03;
        vcId = (clcw_data[1] & 0xFC) >> 2U;
        noRFAvail = (clcw_data[2] & 0x80) >> 7U;
        noBitLock = (clcw_data[2] & 0x40) >> 6U;
        lckout = (clcw_data[2] & 0x20) >> 5U;
        wt = (clcw_data[2] & 0x10) >> 4U;
        retransmit = (clcw_data[2] & 0x08) >> 3U;
        farmBCounter = (clcw_data[2] & 0x06) >> 1U;
        reportValue = clcw_data[3];
    }

    /**
     * @brief Control Link Words used for FARM reports
     */
    CLCW(const uint8_t field_status, const uint8_t cop_in_effect, const uint8_t virtual_channel, const bool no_rf_avail,
         const bool no_bit_lock, const bool lockout, const bool wait, const bool retransmission,
         const uint8_t farm_b_counter, const uint8_t report_value) {
        clcw_data[0] = (field_status << 2U) | cop_in_effect;
        clcw_data[1] = (virtual_channel << 2U);
        clcw_data[2] = (no_rf_avail << 7U) | (no_bit_lock << 6U) | (lockout << 5U) | (wait << 4U) |
                       (retransmission << 3U) | (farm_b_counter << 1U);
        clcw_data[3] = report_value;

        fieldStatus = field_status;
        noRFAvail = no_rf_avail;
        vcId = virtual_channel;
        copInEffect = cop_in_effect;
        lckout = lockout;
        noBitLock = no_bit_lock;
        wt = wait;
        retransmit = retransmission;
        farmBCounter = farm_b_counter;
        reportValue = report_value;
    }

private:
    uint8_t clcw_data[4];
    // This is used by COP to signal the higher procedures
    FDURequestType confSignal;
    uint8_t fieldStatus;
    uint8_t copInEffect;
    uint8_t vcId;
    bool noRFAvail;
    bool noBitLock;
    bool lckout;
    bool retransmit;
    bool wt;
    uint8_t farmBCounter;
    uint8_t reportValue;
};

#endif // CCSDS_PACKET_H
