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
	TransferFrameHeaderTM(uint8_t* frameData) : TransferFrameHeader(frameData) {}

	/**
	 * The Operational Control Field Flag indicates the presence or absence of the Operational Control Field
	 * @details Bit  15  of  the  Transfer  Frame  Primary  Header
	 * @see p. 4.1.2.4 from TM SPACE DATA LINK PROTOCOL
	 */
	bool operationalControlFieldFlag() const {
		return (transferFrameHeader[1]) & 0x01;
	}

	/**
	 * Provides  a  running  count  of  the  Transfer  Frames  which  have  been  transmitted  through  the
	 * same  Master  Channel.
	 * @details Bits  16–23  of  the  Transfer  Frame  Primary  Header
	 * @see p. 4.1.2.5 from TM SPACE DATA LINK PROTOCOL
	 */
	uint8_t masterChannelFrameCount() const {
		return transferFrameHeader[2];
	}

	/**
	 * contain  a  sequential  binary  count (modulo-256) of each Transfer Frame transmitted within a
	 * specific Virtual Channel.
	 * @details Bits  24–31  of  the  Transfer  Frame  Primary  Header
	 * @see p. 4.1.2.6 from TM SPACE DATA LINK PROTOCOL
	 */
	uint8_t virtualChannelFrameCount() const {
		return transferFrameHeader[3];
	}

	/**
	 * Indicates the presence of the secondary header.
	 * @details Bit  32  of  the Transfer  Frame  Primary  Header
	 * @see p. 4.1.2.7.2 from TM SPACE DATA LINK PROTOCOL
	 */

	bool transferFrameSecondaryHeaderFlag() const {
		return (transferFrameHeader[4] & 0x80) >> 7U;
	}

	/**
	 * Signals the type of data which are inserted into the Transfer Frame Data Field (VCA_SDU or Packets).
	 * @details Bit 33 of the Transfer Frame Primary Header
	 * @see p. 4.1.2.7.3 from TM SPACE DATA LINK PROTOCOL
	 */

	bool synchronizationFlag() const {
		return (transferFrameHeader[4] & 0x40) >> 6U;
	}

	/**
	 * If the Synchronization Flag is set to ‘0’,t he TransferFrame Order Flag is reserved for
	            future use by the CCSDS and shall be set to ‘0’. If the Synchronization Flag is
	            set to ‘1’, the use of the TransferFrame Order Flag is undefined.
	 * @details Bit 34 of the Transfer Frame Primary Header
	 * @see p. 4.1.2.7.4 from TM SPACE DATA LINK PROTOCOL
	 */
	bool packetOrderFlag() const {
		return (transferFrameHeader[4] & 0x20) >> 5U;
	}

	/**
	 * Segment Length Id indicates the order of the segmented packets
	 * Bits 35 and 36 of the Transfer Frame Primary Header.
	 */
	uint8_t segmentLengthId() const {
		return (transferFrameHeader[4] >> 3) & 0x3;
	}

	/**
	 * If the Synchronization Flag is set to ‘0’, the First Header Pointer shall contain
	 *		the position of the first octet of the first TransferFrame that starts in the Transfer Frame Data Field.
	 *		Otherwise it is undefined.
	 * @details Bits 37–47 of the Transfer Frame Primary Header
	 * @see p. 4.1.2.7.6 from TM SPACE DATA LINK PROTOCOL
	 */
	uint16_t getFirstHeaderPointer() const {
		return (static_cast<uint16_t>((transferFrameHeader[4]) & 0x07)) << 8U | (static_cast<uint16_t>((transferFrameHeader[5])));
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
		return (static_cast<uint16_t>((transferFrameHeader[4])) << 8U | (static_cast<uint16_t>((transferFrameHeader[5]))));
	}
};

struct TransferFrameTM : public TransferFrame {
	/**
	 * The Virtual Channel Identifier provides the identification of the Virtual Channel.
	 * @details Bits 12–14 of the Transfer Frame Primary Header.
	 * @see p. 4.1.2.3 from TM SPACE DATA LINK PROTOCOL
	 */
	uint8_t virtualChannelId() const {
		return (transferFrameData[1] & 0xE) >> 1U;
	}

	/**
	 * This  8-bit  field  shall  contain  a  sequential  binary  count  (modulo-256)  of  each
	 * Transfer Frame transmitted within a specific Master Channel.
	 * @details Bits  16–23  of  the  Transfer  Frame  Primary  Header.
	 * @see p. 4.1.2.5 from TM SPACE DATA LINK PROTOCOL
	 */
	uint8_t getMasterChannelFrameCount() const {
		return transferFrameData[2];
	}

	/**
	 * This  8-bit  field  shall  contain  a  sequential  binary  count  (modulo-256)  of  each
	 * Transfer Frame transmitted within a specific Virtual Channel.
	 * @details Bits  24-31  of  the  Transfer  Frame  Primary  Header.
	 * @see p. 4.1.2.6 from TM SPACE DATA LINK PROTOCOL
	 */
	uint8_t getVirtualChannelFrameCount() const {
		return transferFrameData[3];
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
		return static_cast<uint16_t>((transferFrameData[4]) << 8) | transferFrameData[5];
	}

	uint16_t getFrameLength() const {
		return transferFrameLength;
	}

	bool operationalControlFieldExists() const {
		return transferFrameData[1] & 0x1;
	}
	uint8_t getSegmentLengthId() const {
		return (transferFrameData[4] >> 3) & 0x3;
	}

    void setSegmentLengthId(uint8_t segmentLengthId) {
        transferFrameData[4] = (transferFrameData[4] & 0xE7) | (segmentLengthId << 3);
    }

    uint16_t getFirstHeaderPointer() const {
        return hdr.getFirstHeaderPointer();
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
		operationalControlFieldPointer = transferFrameData + transferFrameLength - 4 - 2 * eccFieldExists;
		operationalControlField = (operationalControlFieldPointer[0] << 24U) |
		                          (operationalControlFieldPointer[1] << 16U) |
		                          (operationalControlFieldPointer[2] << 8U) | operationalControlFieldPointer[3];
		return operationalControlField;
	}

	void setMasterChannelFrameCount(uint8_t masterChannelFrameCount) {
        transferFrameData[2] = masterChannelFrameCount;
	}

	TransferFrameTM(uint8_t* frameData, uint16_t frameLength, uint8_t virtualChannelFrameCount, uint16_t vcid,
                    bool eccFieldExists, bool transferFrameSecondaryHeaderPresent, uint8_t segmentationLengthId,
                    SynchronizationFlag syncFlag, uint16_t  firstHeaderPointer, uint16_t  firstEmptyOctet = 0, FrameType type = TM)
	    : TransferFrame(type, frameLength, frameData, firstEmptyOctet), hdr(frameData), scid(scid), eccFieldExists(eccFieldExists) {
		// Transfer Frame Version Number + Spacecraft Id
		frameData[0] = SpacecraftIdentifier & 0xE0 >> 4;
		// Spacecraft  Id + Virtual Channel ID + Operational Control Field
		frameData[1] = ((SpacecraftIdentifier & 0x0F) << 4) | ((vcid & 0x7) << 1);
		// Master Channel Frame Count is set by the MC Generation Service
		frameData[2] = 0;
        frameData[3] = virtualChannelFrameCount;
		// Data field status. TransferFrame Order Flag and Segment Length ID are unused
		frameData[4] = (transferFrameSecondaryHeaderPresent << 7) | (static_cast<uint8_t>(syncFlag << 6)) |
                       (segmentationLengthId << 3) | ((firstHeaderPointer & 0x700) >> 8);
        frameData[5] = firstHeaderPointer & 0xFF;
	}

	/**
	 * Constructor with operational control field
	 */
	TransferFrameTM(uint8_t* frameData, uint16_t frameLength, uint8_t virtualChannelFrameCount, uint16_t vcid,
                    bool eccFieldExists, bool transferFrameSecondaryHeaderPresent, uint8_t segmentationLengthId,
                    SynchronizationFlag syncFlag, uint16_t firstHeaderPointer, uint32_t operationalControlField,uint16_t  firstEmptyOctet = 0, FrameType type = TM)
	    : TransferFrame(type, frameLength, frameData, firstEmptyOctet), hdr(frameData), scid(scid), eccFieldExists(eccFieldExists) {
		// Transfer Frame Version Number + Spacecraft Id
		frameData[0] = SpacecraftIdentifier & 0xE0 >> 4;
		// Spacecraft  Id + Virtual Channel ID + Operational Control Field
		frameData[1] = ((SpacecraftIdentifier & 0x0F) << 4) | ((vcid & 0x7) << 1) | 1;
		// Master Channel Frame Count is set by the MC Generation Service
		frameData[2] = 0;
        frameData[3] = virtualChannelFrameCount;
		// Data field status. TransferFrame Order Flag and Segment Length ID are unused
		frameData[4] = (transferFrameSecondaryHeaderPresent << 7) | (static_cast<uint8_t>(syncFlag << 6)) |
                       (segmentationLengthId << 3) | ((firstHeaderPointer & 0x700) >> 8);
        frameData[5] = firstHeaderPointer & 0xFF;
		uint8_t* ocfPointer = frameData + transferFrameLength - 4 - 2 * eccFieldExists;
		ocfPointer[0] = operationalControlField >> 24;
		ocfPointer[1] = (operationalControlField >> 16) & 0xFF;
		ocfPointer[2] = (operationalControlField >> 8) & 0xFF;
		ocfPointer[3] = operationalControlField & 0xFF;
	}

	TransferFrameTM(uint8_t* frameData, uint16_t frameLength, bool eccFieldExists, uint16_t firstEmptyOctet = 0)
	    : TransferFrame(FrameType::TM, frameLength, frameData, firstEmptyOctet), hdr(frameData),
          eccFieldExists(eccFieldExists) {}
	/**
	 * Calculates the CRC code
	 * @see p. 4.1.4.2 from TC SPACE DATA LINK PROTOCOL
	 */
	uint16_t calculateCRC(const uint8_t* data, uint16_t len) override {

		uint16_t crc = 0xFFFF;

		// calculate remainder of binary polynomial division
		for (uint16_t i = 0; i < len; i++) {
			crc = crc_16_ccitt_table[(data[i] ^ (crc >> 8)) & 0xFF] ^ (crc << 8);
		}

		for (uint16_t i = 0; i < TmTransferFrameSize - len - 4*operationalControlFieldExists() - 2; i++) {
			crc = crc_16_ccitt_table[(idle_data[i] ^ (crc >> 8)) & 0xFF] ^ (crc << 8);
		}

		return crc;
	}


private:
	TransferFrameHeaderTM hdr;
	uint8_t scid;
	bool eccFieldExists;
};
