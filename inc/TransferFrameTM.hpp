#pragma once

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
	 * The Operational Control Field Flag indicates the presence or absence of the Operational Control Field
	 * @details Bit  15  of  the  Transfer  Frame  Primary  Header
	 * @see p. 4.1.2.4 from TM SPACE DATA LINK PROTOCOL
	 */
	bool operationalControlFieldFlag() const {
		return (packetHeader[1]) & 0x01;
	}

	/**
	 * Provides  a  running  count  of  the  Transfer  Frames  which  have  been  transmitted  through  the
	 * same  Master  Channel.
	 * @details Bits  16–23  of  the  Transfer  Frame  Primary  Header
	 * @see p. 4.1.2.5 from TM SPACE DATA LINK PROTOCOL
	 */
	uint8_t masterChannelFrameCount() const {
		return packetHeader[2];
	}

	/**
	 * contain  a  sequential  binary  count (modulo-256) of each Transfer Frame transmitted within a
	 * specific Virtual Channel.
	 * @details Bits  24–31  of  the  Transfer  Frame  Primary  Header
	 * @see p. 4.1.2.6 from TM SPACE DATA LINK PROTOCOL
	 */
	uint8_t virtualChannelFrameCount() const {
		return packetHeader[3];
	}

	/**
	 * Indicates the presence of the secondary header.
	 * @details Bit  32  of  the Transfer  Frame  Primary  Header
	 * @see p. 4.1.2.7.2 from TM SPACE DATA LINK PROTOCOL
	 */

	bool transferFrameSecondaryHeaderFlag() const {
		return (packetHeader[4] & 0x80) >> 7U;
	}

	/**
	 * Signals the type of data which are inserted into the Transfer Frame Data Field (VCA_SDU or Packets).
	 * @details Bit 33 of the Transfer Frame Primary Header
	 * @see p. 4.1.2.7.3 from TM SPACE DATA LINK PROTOCOL
	 */

	bool synchronizationFlag() const {
		return (packetHeader[4] & 0x40) >> 6U;
	}

	/**
	 * If the Synchronization Flag is set to ‘0’,t he TransferFrame Order Flag is reserved for
	            future use by the CCSDS and shall be set to ‘0’. If the Synchronization Flag is
	            set to ‘1’, the use of the TransferFrame Order Flag is undefined.
	 * @details Bit 34 of the Transfer Frame Primary Header
	 * @see p. 4.1.2.7.4 from TM SPACE DATA LINK PROTOCOL
	 */
	bool packetOrderFlag() const {
		return (packetHeader[4] & 0x20) >> 5U;
	}

	/**
	 * Segment Length Id indicates the order of the segmented packets
	 * Bits 35 and 36 of the Transfer Frame Primary Header.
	 */
	uint8_t segmentLengthId() const {
		return (packetHeader[4] >> 3) & 0x3;
	}

	/**
	 * If the Synchronization Flag is set to ‘0’, the First Header Pointer shall contain
	 *		the position of the first octet of the first TransferFrame that starts in the Transfer Frame Data Field.
	 *		Otherwise it is undefined.
	 * @details Bits 37–47 of the Transfer Frame Primary Header
	 * @see p. 4.1.2.7.6 from TM SPACE DATA LINK PROTOCOL
	 */
	uint16_t firstHeaderPointer() const {
		return (static_cast<uint16_t>((packetHeader[4]) & 0x07)) << 8U | (static_cast<uint16_t>((packetHeader[5])));
	}

	/**
	 * Contains the 	a)Transfer Frame Secondary Header Flag (1 bit)
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
	 * The Virtual Channel Identifier provides the identification of the Virtual Channel.
	 * @details Bits 12–14 of the Transfer Frame Primary Header.
	 * @see p. 4.1.2.3 from TM SPACE DATA LINK PROTOCOL
	 */
	uint8_t virtualChannelId() const {
		return (packet[1] & 0xE) >> 1U;
	}

	/**
	 * This  8-bit  field  shall  contain  a  sequential  binary  count  (modulo-256)  of  each
	 * Transfer Frame transmitted within a specific Master Channel.
	 * @details Bits  16–23  of  the  Transfer  Frame  Primary  Header.
	 * @see p. 4.1.2.5 from TM SPACE DATA LINK PROTOCOL
	 */
	uint8_t getMasterChannelFrameCount() const {
		return packet[2];
	}

	/**
	 * This  8-bit  field  shall  contain  a  sequential  binary  count  (modulo-256)  of  each
	 * Transfer Frame transmitted within a specific Virtual Channel.
	 * @details Bits  24-31  of  the  Transfer  Frame  Primary  Header.
	 * @see p. 4.1.2.6 from TM SPACE DATA LINK PROTOCOL
	 */
	uint8_t getVirtualChannelFrameCount() const {
		return packet[3];
	}

	/**
	 *
	 *  Contains the 	a)Transfer Frame Secondary Header Flag (1 bit)
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

	bool operationalControlFieldExists() const {
		return packet[1] & 0x1;
	}
	uint8_t segmentLengthId() const {
		return (packet[4] >> 3) & 0x3;
	}

	/**
	 * @see p. 4.1.5 from TM SPACE DATA LINK PROTOCOL
	 */
	std::optional<uint32_t> getOperationalControlField() const {
		uint32_t operationalControlField;
		uint8_t* operationalControlFieldPointer;
		if (!operationalControlFieldExists()) {
			return {};
		}
		operationalControlFieldPointer = packet + frameLength - 4 - 2 * eccFieldExists;
		operationalControlField = (operationalControlFieldPointer[0] << 24U) |
		                          (operationalControlFieldPointer[1] << 16U) |
		                          (operationalControlFieldPointer[2] << 8U) | operationalControlFieldPointer[3];
		return operationalControlField;
	}

	void setMasterChannelFrameCount(uint8_t masterChannelFrameCount) {
		packet[2] = masterChannelFrameCount;
	}

	TransferFrameTM(uint8_t* packet, uint16_t packetLength, uint8_t virtualChannelFrameCount, uint16_t vcid,
	                bool eccFieldExists, bool transferFrameSecondaryHeaderPresent, uint8_t segmentationLengthId,
	                SynchronizationFlag syncFlag, PacketType type = TM)
	    : TransferFrame(type, packetLength, packet), hdr(packet), scid(scid), eccFieldExists(eccFieldExists),
	      firstHeaderPointer(firstHeaderPointer) {
		// Transfer Frame Version Number + Spacecraft Id
		packet[0] = SpacecraftIdentifier & 0xE0 >> 4;
		// Spacecraft  Id + Virtual Channel ID + Operational Control Field
		packet[1] = ((SpacecraftIdentifier & 0x0F) << 4) | ((vcid & 0x7) << 1);
		// Master Channel Frame Count is set by the MC Generation Service
		packet[2] = 0;
		packet[3] = virtualChannelFrameCount;
		// Data field status. TransferFrame Order Flag and Segment Length ID are unused
		packet[4] = (transferFrameSecondaryHeaderPresent << 7) | (static_cast<uint8_t>(syncFlag << 6)) |
		            (segmentationLengthId << 3) | (firstHeaderPointer & 0x700 >> 8);
		packet[5] = firstHeaderPointer & 0xFF;
	}

	/**
	 * Constructor with operational control field
	 */
	TransferFrameTM(uint8_t* packet, uint16_t packetLength, uint8_t virtualChannelFrameCount, uint16_t vcid,
	                bool eccFieldExists, bool transferFrameSecondaryHeaderPresent, uint8_t segmentationLengthId,
	                SynchronizationFlag syncFlag, uint32_t operationalControlField, PacketType type = TM)
	    : TransferFrame(type, packetLength, packet), hdr(packet), scid(scid), eccFieldExists(eccFieldExists),
	      firstHeaderPointer(firstHeaderPointer) {
		// Transfer Frame Version Number + Spacecraft Id
		packet[0] = SpacecraftIdentifier & 0xE0 >> 4;
		// Spacecraft  Id + Virtual Channel ID + Operational Control Field
		packet[1] = ((SpacecraftIdentifier & 0x0F) << 4) | ((vcid & 0x7) << 1) | 1;
		// Master Channel Frame Count is set by the MC Generation Service
		packet[2] = 0;
		packet[3] = virtualChannelFrameCount;
		// Data field status. TransferFrame Order Flag and Segment Length ID are unused
		packet[4] = (transferFrameSecondaryHeaderPresent << 7) | (static_cast<uint8_t>(syncFlag << 6)) |
		            (segmentationLengthId << 3) | (firstHeaderPointer & 0x700 >> 8);
		packet[5] = firstHeaderPointer & 0xFF;
		uint8_t* ocfPointer = packet + frameLength - 4 - 2 * eccFieldExists;
		ocfPointer[0] = operationalControlField >> 24;
		ocfPointer[1] = (operationalControlField >> 16) & 0xFF;
		ocfPointer[2] = (operationalControlField >> 8) & 0xFF;
		ocfPointer[3] = operationalControlField & 0xFF;
	}

	TransferFrameTM(uint8_t* packet, uint16_t packet_length, bool eccFieldExists)
	    : TransferFrame(PacketType::TM, packet_length, packet), hdr(packet), eccFieldExists(eccFieldExists) {}

private:
	TransferFrameHeaderTM hdr;
	uint8_t scid;
	bool eccFieldExists;
	uint16_t firstHeaderPointer;
};
