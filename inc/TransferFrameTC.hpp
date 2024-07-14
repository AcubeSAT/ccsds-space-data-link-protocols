#pragma once

#include <CCSDS_Definitions.hpp>
#include <algorithm>
#include <cstring>
#include <cstdint>
#include <etl/memory.h>
#include "TransferFrame.hpp"

class TransferFrameTC;

/**
 *  The transferFrameData's service type
 * @see p. 2.2.2 from TC SPACE DATA LINK PROTOCOL
 */
enum class ServiceType {
	TYPE_AD = 0x0,
	TYPE_AC = 0x1,
	TYPE_BD = 0x2,
	TYPE_BC = 0x3,
};

enum FDURequestType : uint8_t {
	REQUEST_PENDING = 0,
	REQUEST_POSITIVE_CONFIRM = 1,
	REQUEST_NEGATIVE_CONFIRM = 2,
};

struct TransferFrameHeaderTC : public TransferFrameHeader {
public:
	/**
	 * @see p. 4.1.2 from TC SPACE DATA LINK PROTOCOL
	 */
	TransferFrameHeaderTC(uint8_t* frameData) : TransferFrameHeader(frameData) {}

	/**
	 * The bypass Flag determines whether the transferFrameData will bypass FARM checks
	 * @details Bit 2 of the Transfer Frame Primary Header
	 * @see p. 4.1.2.3.1 from TC SPACE DATA LINK PROTOCOL
	 */
	bool bypassFlag() const {
		return (transferFrameHeader[0] >> 5U) & 0x01;
	}

	/**
	 * The control and command Flag determines whether the transferFrameData carries control commands (Type-C) or
	 * data (Type-D)
	 * @details Bit 3 of the Transfer Frame Primary Header
	 * @see p. 4.1.2.3.2 from TC SPACE DATA LINK PROTOCOL
	 */
	bool ctrlAndCmdFlag() const {
		return (transferFrameHeader[0] >> 4U) & 0x01;
	}

	/**
	 * The length of the transfer frame
	 * @details Bits 22–31 of the Transfer Frame Primary Header
	 * @see p. 4.1.2.7 from TC SPACE DATA LINK PROTOCOL
	 */
	uint16_t transferFrameLength() const {
		return (static_cast<uint16_t>(transferFrameHeader[2] & 0x03) << 8U) | (static_cast<uint16_t>(transferFrameHeader[3]));
	}
};

class TransferFrameTC : public TransferFrame {
public:
	void setConfSignal(FDURequestType reqSignal) {
		confSignal = reqSignal;
		// TODO Maybe signal the higher procedures here instead of having them manually take care of them
	}

	/**
	 * Compares two frames
	 */
	friend bool operator==(const TransferFrameTC& frame1, const TransferFrameTC& frame2) {
		if (frame1.transferFrameLength != frame2.transferFrameLength) {
			return false;
		}
		for (uint16_t i = 0; i < frame1.transferFrameLength; i++) {
			if (frame1.frameData()[i] != frame2.frameData()[i]) {
				return false;
			}
		}
		return true;
	}

	// TODO: Handle CLCWs without any ambiguities

    // seems redundant?
	const uint8_t* packetPlData() const {
		return transferFrameData;
	}

	// see p. 4.2.1.1.2 from TC SPACE DATA LINK PROTOCOL

	/**
	 * @return Bit 0 of the CLCW (the Control Word Type).
	 * @details This one-bit field shall be set to ‘0’.
	 * @see p. 4.2.1.2 from TC SPACE DATA LINK PROTOCOL
	 */
	uint8_t controlWordType() const {
		return (transferFrameData[0] & 0x80 >> 7U);
	}

	/**
	 * @return Bits 3-5 of the CLCW (the Status Field).
	 * @see p. 4.2.1.4 from TC SPACE DATA LINK PROTOCOL
	 *
	 */
	uint8_t statusField() const {
		return (transferFrameData[0] & 0x1C) >> 2U;
	}

	/**
	 * Used to indicate the COP that is being used.
	 * @return Bits 6-7 of the CLCW (the COP in Effect parameter)
	 * @see p. 4.2.1.5 from TC SPACE DATA LINK PROTOCOL
	 */
	uint8_t copInEffect() const {
		return transferFrameData[0] & 0x03;
	}

	/**
	 * @return Bits 8-13 of the CLCW (the Virtual Channel Identifier of the Virtual Channel
	 * with which this COP report is associated).
	 * @see p. 4.2.1.6 from TC SPACE DATA LINK PROTOCOL
	 */
	uint8_t vcIdentification() const {
		return (transferFrameData[1] & 0xFC) >> 2U;
	}

	/**
	 * The No RF Available Flag shall provide a logical indication of the ‘ready’ status
	 * of the radio frequency (RF) elements within the space link provided by the Physical Layer.
	 * @return Bit 16 of the CLCW (the No RF Available Flag).
	 * @see p. 4.2.1.8.2 from TC SPACE DATA LINK PROTOCOL
	 */
	uint8_t noRfAvail() const {
		return (transferFrameData[2] & 0x80) >> 7U;
	}

	/**
	 * @return Bit 17 of the CLCW (the No Bit Lock Flag).
	 * @see p. 4.2.1.8.3 from TC SPACE DATA LINK PROTOCOL
	 */
	uint8_t noBitLock() const {
		return (transferFrameData[2] & 0x20) >> 5U;
	}

	/**
	 * @return Bit 18 of the CLCW (the Lockout Flag).
	 * @see p. 4.2.1.8.4 from TC SPACE DATA LINK PROTOCOL
	 */
	uint8_t lockout() const {
		return (transferFrameData[2] & 0x20) >> 5U;
	}

	/**
	 * @return Bit 19 of the CLCW (the Wait Flag)
	 * @see p. 4.1.2.8.5 from TC SPACE DATA LINK PROTOCOL
	 */
	uint8_t wait() const {
		return (transferFrameData[2] & 0x10) >> 4U;
	}

	/**
	 * @return Bit 20 of the CLCW (the Retransmit Flag).
	 * @see p. 4.2.1.8.6 from TC SPACE DATA LINK PROTOCOL
	 */
	uint8_t retransmit() const {
		return (transferFrameData[2] & 0x08) >> 3U;
	}

	/**
	 * @return Bits 21-22 of the CLCW (the FARM-B Counter).
	 * @see p. 4.2.1.9 from TC SPACE DATA LINK PROTOCOL
	 */
	uint8_t farmBCounter() const {
		return (transferFrameData[2] & 0x06) >> 1U;
	}

	/**
	 * @return Bits 24-31 of the CLCW (the Report Value).
	 * @see p. 4.2.11.1 from TC SPACE DATA LINK PROTOCOL
	 */
	uint8_t reportValue() const {
		return transferFrameData[3];
	}

	/**
	 * Set the number of repetitions that is determined by the virtual channel
	 */
	void setRepetitions(const uint8_t repetitions) {
		reps = repetitions;
	}

	/**
	 * Determines whether the transferFrameData is marked for retransmission while in the sent queue
	 */
	bool isToBeRetransmitted() const {
		return toBeRetransmitted;
	}

	void setToBeRetransmitted(bool f) {
		toBeRetransmitted = f;
	}

	/**
	 * @return TC transfer frame primary header (the first 5 octets of the TC transfer frame)
	 * @see p. 4.1.2 from TC SPACE DATA LINK PROTOCOL
	 */
	TransferFrameHeaderTC transferFrameHeader() const {
		return hdr;
	}

	/**
	 * @return Bits 22–31 of the Transfer Frame Primary Header (the Frame Length)
	 * @see p. 4.1.2.7 from TC SPACE DATA LINK PROTOCOL
	 */
	uint16_t getFrameLength() const {
		return transferFrameLength;
	}

	/**
	 * @see p. 4.1.3.2.2 from TC SPACE DATA LINK PROTOCOL
	 */
	// TODO: Use std::optional
	uint8_t segmentationHeader() const {
		return transferFrameData[5];
	}

	/**
	 * @return Bits  16–21  of  the  Transfer  Frame  Primary  Header (the  Virtual Channel Identifier (VCID)).
	 * @see p. 4.1.2.6 from TC SPACE DATA LINK PROTOCOL
	 */
	uint8_t virtualChannelId() const {
		return (transferFrameData[2] & 0xFC) >> 2;
	};

	// Assumes MAP Id exists
	// TODO: Replace with std::optional
	uint8_t mapId() const {
		if (segmentationHeaderPresent) {
			return transferFrameData[5] & 0x3F;
		}
		return 0;
	}

	uint8_t getTransferFrameVersionNumber() const {
		return (transferFrameData[0] >> 6) & 0x3;
	}
	/**
	 * @return Bits  6–15  of  the  Transfer  Frame  Primary  Header (the  Spacecraft Identifier (SCID)).
	 * @see p. 4.1.2.5 from TC SPACE DATA LINK PROTOCOL
	 */
	uint16_t spacecraftId() const {
		return SpacecraftIdentifier;
	}

	/**
	 * @return Bits 32–39 of the Transfer Frame Primary Header (the Frame Sequence Number, N(S)).
	 * @see p. 4.1.2.8 from TC SPACE DATA LINK PROTOCOL.
	 */
	uint8_t transferFrameSequenceNumber() const {
		return transferFrameData[4];
	}

	/**
	 * @see p. 2.2.2 from TC SPACE DATA LINK PROTOCOL
	 */
	ServiceType getServiceType() const {
		bool bypass = (transferFrameData[0] >> 6) & 0x1;
		bool ctrl = (transferFrameData[0] >> 5) & 0x1;

		if (bypass && ctrl) {
			return ServiceType::TYPE_BC;
		} else if (bypass && !ctrl) {
			return ServiceType::TYPE_BD;
		} else if (!bypass && !ctrl) {
			return ServiceType::TYPE_AD;
		}
		// Reserved type not normally used as per the standard
		return ServiceType::TYPE_AC;
	}

	bool acknowledged() const {
		return ack;
	}

	/**
	 * @see p. 2.4.2 from TC SPACE DATA LINK PROTOCOL
	 */
	uint8_t repetitions() const {
		return reps;
	}

	uint8_t* frameData() const {
		return transferFrameData;
	}

	// Setters are not strictly needed in this case. They are just offered as a utility functions for the VC/MAP
	// generation services when segmenting or blocking transfer frames.
	void setSegmentationHeader(uint8_t segmentation_hdr) {
        transferFrameData[5] = segmentation_hdr;
	}

	void setFrameData(uint8_t* frame_data) {
        transferFrameData = frame_data;
	}

	void setFrameLength() {
        transferFrameData[2] = ((virtualChannelId() & 0x3F) << 2) | (transferFrameLength & 0x300 >> 8);
        transferFrameData[3] = transferFrameLength & 0xFF;
	}

	uint16_t frameLength() {
		return (static_cast<uint16_t>(transferFrameData[2] & 0x3) << 8) | transferFrameData[3];
	}

	void setServiceType(ServiceType service_type) {
		serviceType = service_type;
	}

	void setAcknowledgement(bool acknowledgement) {
		ack = acknowledgement;
	}

	void setTransferFrameSequenceNumber(uint8_t frame_seq_number) {
        transferFrameData[4] = frame_seq_number;
	}

	/**
	 * Indicates that the frame has been passed to the physical layer and supposedly transmitted
	 */
	void setToTransmitted() {
		transmit = true;
	}

	/**
	 * Indicates whether transfer frame has been transmitted
	 */
	bool isTransmitted() {
		return transmit;
	}

	/**
	 * Indicates that the frame has gone through the FOP checks
	 */
	void setToProcessedByFOP() {
		processedByFOP = true;
	}

	/**
	 * Indicates whether the frame has gone through the FOP checks
	 */
	bool getProcessedByFOP() {
		return processedByFOP;
	}

	TransferFrameTC(uint8_t* frameData, uint16_t frameLength, uint8_t gvcid, ServiceType serviceType, bool segHdrPresent,
                    FrameType t = TC)
	    : TransferFrame(t, frameLength, frameData), hdr(frameData), serviceType(serviceType), ack(false),
          toBeRetransmitted(false), segmentationHeaderPresent(segHdrPresent), transmit(false), processedByFOP(false) {
		uint8_t bypassFlag = (serviceType == ServiceType::TYPE_AD) ? 0 : 1;
		uint8_t ctrlCmdFlag = (serviceType == ServiceType::TYPE_BC) ? 1 : 0;
        frameData[0] = (bypassFlag << 6) | (ctrlCmdFlag << 5) | ((SpacecraftIdentifier & 0x300) >> 8);
        frameData[1] = SpacecraftIdentifier & 0xFF;
        frameData[2] = ((gvcid & 0x3F) << 2) | (frameLength & 0x300 >> 8);
        frameData[3] = frameLength & 0xFF;
	}

	TransferFrameTC(uint8_t* frameData, uint16_t frameLength, FrameType t = TC)
	    : TransferFrame(FrameType::TC, frameLength, frameData), hdr(frameData), transmit(false){};
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

		return crc;
	}

private:
	bool toBeRetransmitted;
	// This is used by COP to signal the higher procedures
	FDURequestType confSignal;
	TransferFrameHeaderTC hdr;
	ServiceType serviceType;
	bool segmentationHeaderPresent;
	bool ack;
	bool transmit;
	bool processedByFOP;
	uint8_t reps;
};
