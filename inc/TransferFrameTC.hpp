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

enum SequenceFlags { SegmentationMiddle = 0x0, SegmentationStart = 0x1, SegmentationEnd = 0x2, NoSegmentation = 0x3 };

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
	bool getBypassFlag() const {
		return (transferFrameHeader[0] >> 5U) & 0x01;
	}

	/**
	 * The control and command Flag determines whether the transferFrameData carries control commands (Type-C) or
	 * data (Type-D)
	 * @details Bit 3 of the Transfer Frame Primary Header
	 * @see p. 4.1.2.3.2 from TC SPACE DATA LINK PROTOCOL
	 */
	bool getCtrlAndCmdFlag() const {
		return (transferFrameHeader[0] >> 4U) & 0x01;
	}

	/**
	 * The length of the transfer frame
	 * @details Bits 22–31 of the Transfer Frame Primary Header
	 * @see p. 4.1.2.7 from TC SPACE DATA LINK PROTOCOL
	 */
	uint16_t getTransferFrameLength() const {
		return (static_cast<uint16_t>(transferFrameHeader[2] & 0x03) << 8U) | (static_cast<uint16_t>(transferFrameHeader[3]));
	}

    /**
     * The sequence number of the frame
     * @details Bits 32-39 of the Transfer Frame Primary Header
     * @see p. 4.1.2.8 from TC SPACE DATA LINK PROTOCOL
     */

    uint8_t  getTransferFrameSequenceNumber() const {
        return transferFrameHeader[4];
    }
};

class TransferFrameTC : public TransferFrame {
public:
	/**
	 * Compares two frames
	 */
	friend bool operator==(const TransferFrameTC& frame1, const TransferFrameTC& frame2) {
		if (frame1.transferFrameLength != frame2.transferFrameLength) {
			return false;
		}
		for (uint16_t i = 0; i < frame1.transferFrameLength; i++) {
			if (frame1.getFrameData()[i] != frame2.getFrameData()[i]) {
				return false;
			}
		}
		return true;
	}

    /** PRIMARY HEADER **/

	/**
	 * @return TC transfer frame primary header (the first 5 octets of the TC transfer frame)
	 * @see p. 4.1.2 from TC SPACE DATA LINK PROTOCOL
	 */
    TransferFrameHeaderTC& getTransferFrameHeader() {
        return hdr;
    }

	uint8_t getTransferFrameVersionNumber() const {
		return hdr.getTransferFrameVersionNumber();
	}
	/**
	 * @return Bits  6–15  of  the  Transfer  Frame  Primary  Header (the  Spacecraft Identifier (SCID)).
	 * @see p. 4.1.2.5 from TC SPACE DATA LINK PROTOCOL
	 */
	uint16_t getSpacecraftId() const {
		return hdr.getSpacecraftId(TC);
	}

    /**
     * @return Bits  16–21  of  the  Transfer  Frame  Primary  Header (the  Virtual Channel Identifier (VCID)).
     * @see p. 4.1.2.6 from TC SPACE DATA LINK PROTOCOL
     */
    uint8_t getVirtualChannelId() const {
        return hdr.getVirtualChannelId(TC);
    };

	/**
	 * @see p. 2.2.2 from TC SPACE DATA LINK PROTOCOL
	 */
	ServiceType getServiceType() const {
		bool bypass = hdr.getBypassFlag();
		bool ctrl = hdr.getCtrlAndCmdFlag();

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

    void setServiceType(ServiceType service_type) {
        serviceType = service_type;
        transferFrameData[0] = ((static_cast<uint8_t>(service_type) & 0x3) << 4) | (transferFrameData[0] & 0xCF);
    }

    void setFrameLength(uint16_t frameLength) {
        transferFrameLength = frameLength;
        transferFrameData[2] = ((getVirtualChannelId() & 0x3F) << 2) | static_cast<uint8_t>((transferFrameLength & 0x300) >> 8);
        transferFrameData[3] = static_cast<uint8_t>(transferFrameLength & 0xFF);
    }

    /**
     * @return Bits 32–39 of the Transfer Frame Primary Header (the Frame Sequence Number, N(S)).
     * @see p. 4.1.2.8 from TC SPACE DATA LINK PROTOCOL.
     */
    uint8_t getTransferFrameSequenceNumber() const {
        return hdr.getTransferFrameSequenceNumber();
    }

    void setTransferFrameSequenceNumber(uint8_t frame_seq_number) {
        transferFrameData[4] = frame_seq_number;
    }

    /** SEGMENTATION HEADER **/

    /**
     * @see p. 4.1.3.2.2 from TC SPACE DATA LINK PROTOCOL
     */
    // TODO: Use std::optional
    uint8_t segmentationHeader() const {
        return transferFrameData[5];
    }

    /**
     *  Sets the sequence flag, assuming that the segment header exists
     */
    void setSequenceFlags(SequenceFlags seqFlags){
        transferFrameData[5] = ((static_cast<uint8_t>(seqFlags) & 0x3) << 6) | (transferFrameData[5] & 0x3F);
    }

    // Assumes MAP Id exists
    // TODO: Replace with std::optional
    uint8_t getMapId() const {
        if (segmentationHeaderPresent) {
            return transferFrameData[5] & 0x3F;
        }
        return 0;
    }


    /** AUXILIARY VARIABLES **/

    void setConfSignal(FDURequestType reqSignal) {
        confSignal = reqSignal;
        // TODO Maybe signal the higher procedures here instead of having them manually take care of them
    }

    /**
     * Set the number of repetitions that is determined by the virtual channel
     */
    void setRepetitions(const uint8_t repetitions) {
        reps = repetitions;
    }

    /**
     * Determines whether the transfer frame is marked for retransmission while in the sent queue
     */
    bool isToBeRetransmitted() const {
        return toBeRetransmitted;
    }

    void setToBeRetransmitted(bool f) {
        toBeRetransmitted = f;
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

	void setAcknowledgement(bool acknowledgement) {
		ack = acknowledgement;
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

	TransferFrameTC(uint8_t *frameData, ServiceType serviceType, uint8_t vid, uint16_t frameLength, bool segHdrPresent,
                    uint8_t sequenceFlag = 0x3, uint8_t mapId = 0, uint16_t firstEmptyOctet = 0, FrameType t = TC)
	    : TransferFrame(t, frameLength, frameData, firstEmptyOctet), hdr(frameData), serviceType(serviceType), ack(false),
          toBeRetransmitted(false), segmentationHeaderPresent(segHdrPresent), transmit(false), processedByFOP(false) {
		uint8_t bypassFlag = (serviceType == ServiceType::TYPE_AD) ? 0 : 1;
		uint8_t ctrlCmdFlag = (serviceType == ServiceType::TYPE_BC) ? 1 : 0;
        frameData[0] = ((TransferFrameVersionNumber & 0x3) << 6) | (bypassFlag << 5) | (ctrlCmdFlag << 4) | 0 | static_cast<uint8_t>((SpacecraftIdentifier & 0x300) >> 8);
        frameData[1] = static_cast<uint8_t>(SpacecraftIdentifier & 0xFF);
        frameData[2] = ((vid & 0x3F) << 2) | static_cast<uint8_t>((frameLength & 0x300) >> 8);
        frameData[3] = static_cast<uint8_t>(frameLength & 0xFF);

        if (segHdrPresent){
            frameData[5] = (sequenceFlag << 6) | (mapId & 0x3F);
        }
	}

	TransferFrameTC(uint8_t* frameData, uint16_t frameLength, uint16_t firstEmptyOctet = 0)
	    : TransferFrame(FrameType::TC, frameLength, frameData, firstEmptyOctet), hdr(frameData), transmit(false){};

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
