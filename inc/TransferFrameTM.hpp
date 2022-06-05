#ifndef CCSDS_TM_PACKETS_TRANSFERFRAMETM_HPP
#define CCSDS_TM_PACKETS_TRANSFERFRAMETM_HPP
#include "TransferFrame.hpp"
#include "Alert.hpp"
#include "CCSDS_Definitions.hpp"
#include "optional"

struct TransferFrameHeaderTM : public TransferFrameHeader {
public:
	/**
	 * @see p. 4.1.2 from TM SPACE DATA LINK PROTOCOL
	 */
	TransferFrameHeaderTM(uint8_t* pckt) : TransferFrameHeader(pckt) {}

	/**
	 * @brief The Operational Control Field Flag indicates the presence or absence of the Operational Control Field
	 * @details Bit  15  of  the  Transfer  Frame  Primary  Header
	 * @see p. 4.1.2.4 from TM SPACE DATA LINK PROTOCOL
	 */
	bool operationalControlFieldFlag() const {
		return (packetHeader[1]) & 0x01;
	}

	/**
	 * @brief Provides  a  running  count  of  the  Transfer  Frames  which  have  been  transmitted  through  the
	 * same  Master  Channel.
	 * @details Bits  16–23  of  the  Transfer  Frame  Primary  Header
	 * @see p. 4.1.2.5 from TM SPACE DATA LINK PROTOCOL
	 */
	uint8_t masterChannelFrameCount() const {
		return packetHeader[2];
	}

	/**
	 * @brief contain  a  sequential  binary  count (modulo-256) of each Transfer Frame transmitted within a
	 * specific Virtual Channel.
	 * @details Bits  24–31  of  the  Transfer  Frame  Primary  Header
	 * @see p. 4.1.2.6 from TM SPACE DATA LINK PROTOCOL
	 */
	uint8_t virtualChannelFrameCount() const {
		return packetHeader[3];
	}

	/**
	 * @brief Indicates the presence of the secondary header.
	 * @details Bit  32  of  the Transfer  Frame  Primary  Header
	 * @see p. 4.1.2.7.2 from TM SPACE DATA LINK PROTOCOL
	 */

	bool transferFrameSecondaryHeaderFlag() const {
		return (packetHeader[4] & 0x80) >> 7U;
	}

	/**
	 * @brief Signals the type of data which are inserted into the Transfer Frame Data Field (VCA_SDU or Packets).
	 * @details Bit 33 of the Transfer Frame Primary Header
	 * @see p. 4.1.2.7.3 from TM SPACE DATA LINK PROTOCOL
	 */

	bool synchronizationFlag() const {
		return (packetHeader[4] & 0x40) >> 6U;
	}

	/**
	 * @brief If the Synchronization Flag is set to ‘0’,t he TransferFrame Order Flag is reserved for
	            future use by the CCSDS and shall be set to ‘0’. If the Synchronization Flag is
	            set to ‘1’, the use of the TransferFrame Order Flag is undefined.
	 * @details Bit 34 of the Transfer Frame Primary Header
	 * @see p. 4.1.2.7.4 from TM SPACE DATA LINK PROTOCOL
	 */
	bool packetOrderFlag() const {
		return (packetHeader[4] & 0x20) >> 5U;
	}
	/**
	 * @brief If the Synchronization Flag is set to ‘0’, the First Header Pointer shall contain
	 *		the position of the first octet of the first TransferFrame that starts in the Transfer Frame Data Field.
	 *		Otherwise it is undefined.
	 * @details Bits 37–47 of the Transfer Frame Primary Header
	 * @see p. 4.1.2.7.6 from TM SPACE DATA LINK PROTOCOL
	 */
	uint16_t firstHeaderPointer() const {
		return (static_cast<uint16_t>((packetHeader[4]) & 0x07)) << 8U | (static_cast<uint16_t>((packetHeader[5])));
	}

	/**
	 * @brief Contains the 	a)Transfer Frame Secondary Header Flag (1 bit)
	 *							b) Synchronization Flag (1 bit)
	 *							c) TransferFrame Order Flag (1 bit)
	 *							d) Segment Length Identifier (2 bits)
	 *							e) First Header Pointer (11 bits)
	 * @details Bits  32–47  of  the  Transfer  Frame  Primary  Header.
	 * @see p. 4.1.2.7 from TM SPACE DATA LINK PROTOCOL
	 */
	uint16_t transferFrameDataFieldStatus() const {
		return (static_cast<uint16_t>((packetHeader[4])) << 8U | (static_cast<uint16_t>((packetHeader[5]))));
	}
};

struct TransferFrameTM : public TransferFrame {
	/**
	 * @brief The Virtual Channel Identifier provides the identification of the Virtual Channel.
	 * @details Bits 12–14 of the Transfer Frame Primary Header.
	 * @see p. 4.1.2.3 from TM SPACE DATA LINK PROTOCOL
	 */
	uint8_t virtualChannelId() const {
		return (packet[1] & 0xE) >> 1U;
	}

	/**
	 * @brief This  8-bit  field  shall  contain  a  sequential  binary  count  (modulo-256)  of  each
	 * Transfer Frame transmitted within a specific Master Channel.
	 * @details Bits  16–23  of  the  Transfer  Frame  Primary  Header.
	 * @see p. 4.1.2.5 from TM SPACE DATA LINK PROTOCOL
	 */
	uint8_t getMasterChannelFrameCount() const {
		return packet[2];
	}

	/**
	 * @brief This  8-bit  field  shall  contain  a  sequential  binary  count  (modulo-256)  of  each
	 * Transfer Frame transmitted within a specific Virtual Channel.
	 * @details Bits  24-31  of  the  Transfer  Frame  Primary  Header.
	 * @see p. 4.1.2.6 from TM SPACE DATA LINK PROTOCOL
	 */
	uint8_t getVirtualChannelFrameCount() const {
		return packet[3];
	}


	/**
	 *
	 * @brief Contains the 	a)Transfer Frame Secondary Header Flag (1 bit)
	 *                      b) Synchronization Flag (1 bit)
	 *                      c) TransferFrame Order Flag (1 bit)
	 *                      d) Segment Length Identifier (2 bits)
	 *                      e) First Header Pointer (11 bits)
	 * @details Bits  32–47  of  the  Transfer  Frame  Primary  Header.
	 * @see p. 4.1.2.7 from TM SPACE DATA LINK PROTOCOL
	 */
	uint16_t getTransferFrameDataFieldStatus() const {
		return static_cast<uint16_t>((packet[4]) << 8) | packet[5];
	}

	uint16_t getPacketLength() const {
		return frameLength;
	}

	uint8_t* packetData() const {
		return packet;
	}


	bool operationalControlFieldExists() const{
		return packet[1] & 0x1;
	}


	/**
	 * @see p. 4.1.5 from TM SPACE DATA LINK PROTOCOL
	 */
	std::optional<uint32_t> getOperationalControlField() const {
        uint32_t operationalControlField;
        uint8_t* ocfPtr;
		if(!operationalControlFieldExists()){return {};}
		ocfPtr = packet + frameLength - 4 - 2*eccFieldExists;
        operationalControlField = (ocfPtr[0] << 24U) | (ocfPtr[1] << 16U) | (ocfPtr[2] << 8U) | ocfPtr[3];
        return operationalControlField;
	}

	void setMasterChannelFrameCount(uint8_t mcfc){
		packet[2] = mcfc;
	}

	TransferFrameTM(uint8_t* packet, uint16_t packetLength, uint8_t virtualChannelFrameCount, uint16_t vcid,
	                bool ocfPresent, bool eccFieldExists, bool transferFrameSecondaryHeaderPresent,
	                SynchronizationFlag syncFlag, PacketType t = TM)
	    : TransferFrame(t, packetLength, packet), hdr(packet), scid(scid),
	      eccFieldExists(eccFieldExists), firstHeaderPointer(firstHeaderPointer) {
		// TFVN + SC Id
		packet[0] = SpacecraftIdentifier & 0xE0 >> 4;
		// SC Id + VC ID + OCF
		packet[1] = ((SpacecraftIdentifier & 0x0F) << 4) | ((vcid & 0x7) << 1) | ocfPresent;
		// MC Frame Count is set by the MC Generation Service
		packet[2] = 0;
		packet[3] = virtualChannelFrameCount;
		// Data field status. TransferFrame Order Flag and Segment Length ID are unused
		packet[4] = (transferFrameSecondaryHeaderPresent << 7) | (syncFlag << 6) & (firstHeaderPointer & 0x700 >> 8);
		packet[5] = firstHeaderPointer & 0xFF;
	}

    TransferFrameTM(uint8_t* packet, uint16_t packet_length, bool eccFieldExists)
        : TransferFrame(PacketType::TM, packet_length, packet), hdr(packet), eccFieldExists(eccFieldExists) {}

private:
	TransferFrameHeaderTM hdr;
	uint8_t scid;
	bool eccFieldExists;
	uint16_t firstHeaderPointer;
};

#endif // CCSDS_TM_PACKETS_TRANSFERFRAMETM_HPP
