#ifndef CCSDS_CHANNEL_HPP
#define CCSDS_CHANNEL_HPP

#pragma once

#include <cstdint>
#include <etl/map.h>
#include <etl/flat_map.h>
#include <etl/list.h>
#include <etl/queue.h>

#include <CCSDS_Definitions.hpp>
#include <FrameOperationProcedure.hpp>
#include <FrameAcceptanceReporting.hpp>
#include <TransferFrameTC.hpp>
#include <TransferFrameTM.hpp>
#include <iostream>
#include "MemoryPool.hpp"
#include "CLCW.hpp"

class MasterChannel;

/**
 * @see Table 5-1 from TC SPACE DATA LINK PROTOCOL
 */
struct PhysicalChannel {
private:
	/**
	 * Maximum length of a single transfer frame
	 */
	const uint16_t maxFrameLength;

	/**
	 * Determines whether Error Control field is present
	 */
	const bool errorControlFieldPresent;

	/**
	 * Sets the maximum number of transfer frames that can be transferred in a single data unit
	 */
	const uint16_t maxFramePdu;

	/**
	 * Maximum length of a data unit
	 */
	const uint16_t maxPDULength;

	/**
	 * Maximum bit rate (bits per second)
	 */
	const uint32_t bitrate;

	/**
	 * Maximum number of retransmissions for a data unit
	 */
	const uint16_t repetitions;

public:
	PhysicalChannel(const uint16_t maxFrameLength, const bool errorControlPresent, const uint16_t maxFramesPdu,
	                const uint16_t maxPduLength, const uint32_t bitrate, const uint16_t repetitions)
	    : maxFrameLength(maxFrameLength), errorControlFieldPresent(errorControlPresent), maxFramePdu(maxFramesPdu),
	      maxPDULength(maxPduLength), bitrate(bitrate), repetitions(repetitions) {}

	uint16_t getMaxFrameLength() const {
		return maxFrameLength;
	}

	/**
	 * Determines whether Error Control field is present
	 */
	bool getErrorControlFieldPresent() const {
		return errorControlFieldPresent;
	}

	/**
	 * Sets the maximum number of transfer frames that can be transferred in a single data unit
	 */
	uint16_t getMaxFramePdu() const {
		return maxFramePdu;
	};

	/**
	 * Maximum length of a data unit
	 */
	uint16_t getMaxPDULength() const {
		return maxPDULength;
	};

	/**
	 * Maximum bit rate (bits per second)
	 */
	uint32_t getBitrate() const {
		return bitrate;
	};

	/**
	 * Maximum number of retransmissions for a data unit
	 */
	uint16_t getRepetitions() const {
		return repetitions;
	}
};

/**
 * @see Table 5-4 from TC SPACE DATA LINK PROTOCOL
 */
class MAPChannel {
	friend class ServiceChannel;

private:
	/**
	 * MAP Channel Identifier
	 */
	const uint8_t MAPID; // 6 bits

public:
	void storeMAPChannel(TransferFrame packet);

	/**
	 * Returns availableBufferTC space in the MAP Channel buffer
	 */
	uint16_t availableBufferTC() const {
		return unprocessedPacketListBufferTC.available();
	}

	/**
	 * Returns availableBufferTM space in the MAP Channel buffer
	 */
	uint16_t availableBufferTM() const {
		return unprocessedPacketListBufferTM.available();
	}

	MAPChannel(const uint8_t mapid, bool blockingTC, bool segmentationTC)
	    : MAPID(mapid), blockingTC(blockingTC), segmentationTC(segmentationTC) {
		uint8_t d = unprocessedPacketListBufferTC.size();
		unprocessedPacketListBufferTC.full();
	};

protected:
	bool blockingTC;
	bool segmentationTC;

	/**
	 * Store unprocessed received TCs
	 */
	etl::list<TransferFrameTC*, MaxReceivedTcInMapChannel> unprocessedPacketListBufferTC;
	/**
	 * Store unprocessed received TMs
	 */
	etl::list<TransferFrameTC*, MaxReceivedTmInMapChannel> unprocessedPacketListBufferTM;
	/**
	 * Store frames before being extracted
	 */
	etl::list<TransferFrameTC*, MaxReceivedRxTcInMAPBuffer> rxInFramesAfterVCReception;
};

/**
 * @see Table 5-3 from TC SPACE DATA LINK PROTOCOL
 */
class VirtualChannel {
	friend class ServiceChannel;

	friend class MasterChannel;

	friend class FrameOperationProcedure;

	//@TODO: Those variables shouldn't be public
public:
	/**
	 * Virtual Channel Identifier
	 */
	const uint8_t VCID; // 6 bits

	/**
	 * Global Virtual Channel Identifier
	 */
	const uint16_t GVCID; // 16 bits (assumes TFVN is set to 0)

	/**
	 * Determines whether the Segment Header is present (enables MAP services)
	 */
	const bool segmentHeaderPresent;

	/**
	 * Maximum length of a single transfer frame
	 */
	const uint16_t maxFrameLengthTC;

	/**
	 * Determines whether smaller data units can be combined into a single transfer frame
	 */
	const bool blockingTC;

	/**
	 * Determines the maximum number of times Type A frames will be re-transmitted
	 */
	const uint8_t repetitionTypeAFrame;

	/**
	 * Determines the maximum number of times Type B frames will be re-transmitted
	 */
	const uint8_t repetitionTypeBFrame;

	/**
	 * Determines the number of times a frame will be repeated in transmission in the Physical Layer
	 */
	const uint8_t vcRepetitions;

	/**
	 * Determines the number of TM Transfer Frames transmitted
	 */
	uint8_t frameCountTM;

	/**
	 * Returns availableVCBufferTC space in the VC TC buffer
	 */
	uint16_t availableBufferTC() const {
		return txUnprocessedPacketListBufferTC.available();
	}

	/**
	 * Returns availableVCBufferTC space in the VC TC buffer
	 */
	uint16_t rxInAvailableTM() const {
		return rxInFramesAfterMCReception.available();
	}

	/**
	 * Defines whether the OCF service is present
	 */
	const bool operationalControlFieldTMPresent;

	/**
	 * Defines whether the ECF service is present
	 */
	const bool frameErrorControlFieldPresent;

	/**
	 * Defines whether octet or forward-ordered synchronization is used
	 */
	const SynchronizationFlag synchronization;

	/**
	 * Indicates whether secondary header is present in this VC
	 */
	const bool secondaryHeaderTMPresent;

	/**
	 * Indicates the length of the secondary header for this VC. If secondary header is disabled for this VC,
	 * it is ignored
	 */
	const uint8_t secondaryHeaderTMLength;

	/**
	 *
	 *  MAP channels of the virtual channel
	 */
	etl::flat_map<uint8_t, MAPChannel, MaxMapChannels> mapChannels;

	uint16_t availableInPacketLengthBufferTmTx() {
		return packetLengthBufferTmTx.available();
	}

	uint16_t availableInPacketBufferTmTx() {
		return packetBufferTmTx.available();
	}

	VirtualChannel(std::reference_wrapper<MasterChannel> masterChannel, const uint8_t vcid,
	               const bool segmentHeaderPresent, const uint16_t maxFrameLength, const bool blockingTC,
	               const uint8_t repetitionTypeAFrame, const uint8_t repetitionCopCtrl,
	               const bool secondaryHeaderTMPresent, const uint8_t secondaryHeaderTMLength,
	               const bool operationalControlFieldTMPresent, bool frameErrorControlFieldPresent,
	               const SynchronizationFlag synchronization, const uint8_t farmSlidingWinWidth,
	               const uint8_t farmPositiveWinWidth, const uint8_t farmNegativeWinWidth, const uint8_t vcRepetitions,
	               etl::flat_map<uint8_t, MAPChannel, MaxMapChannels> mapChan)
	    : masterChannel(masterChannel), VCID(vcid & 0x3FU), GVCID((MCID << 0x06U) + VCID),
	      secondaryHeaderTMPresent(secondaryHeaderTMPresent), secondaryHeaderTMLength(secondaryHeaderTMLength),
	      segmentHeaderPresent(segmentHeaderPresent), maxFrameLengthTC(maxFrameLength), blockingTC(blockingTC),
	      repetitionTypeAFrame(repetitionTypeAFrame), vcRepetitions(vcRepetitions), repetitionTypeBFrame(repetitionCopCtrl), waitQueueTxTC(),
	      sentQueueTxTC(), waitQueueRxTC(), sentQueueRxTC(),
	      frameErrorControlFieldPresent(frameErrorControlFieldPresent),
	      operationalControlFieldTMPresent(operationalControlFieldTMPresent), synchronization(synchronization),
	      currentlyProcessedCLCW(0), frameCountTM(0),
	      fop(FrameOperationProcedure(this, &waitQueueTxTC, &sentQueueTxTC, repetitionCopCtrl)),
	      farm(FrameAcceptanceReporting(this, &waitQueueRxTC, &sentQueueRxTC, farmSlidingWinWidth, farmPositiveWinWidth,
	                                    farmNegativeWinWidth)) {
		mapChannels = mapChan;
	}

	VirtualChannel(const VirtualChannel& v)
	    : VCID(v.VCID), GVCID(v.GVCID), segmentHeaderPresent(v.segmentHeaderPresent),
	      maxFrameLengthTC(v.maxFrameLengthTC), repetitionTypeAFrame(v.repetitionTypeAFrame),
	      repetitionTypeBFrame(v.repetitionTypeBFrame), vcRepetitions(v.vcRepetitions), frameCountTM(v.frameCountTM), waitQueueTxTC(v.waitQueueTxTC),
	      sentQueueTxTC(v.sentQueueTxTC), waitQueueRxTC(v.waitQueueRxTC), sentQueueRxTC(v.waitQueueRxTC),
	      txUnprocessedPacketListBufferTC(v.txUnprocessedPacketListBufferTC), fop(v.fop), farm(v.farm),
	      masterChannel(v.masterChannel), blockingTC(v.blockingTC), synchronization(v.synchronization),
	      secondaryHeaderTMPresent(v.secondaryHeaderTMPresent), secondaryHeaderTMLength(v.secondaryHeaderTMLength),
	      frameErrorControlFieldPresent(v.frameErrorControlFieldPresent),
	      operationalControlFieldTMPresent(v.operationalControlFieldTMPresent), mapChannels(v.mapChannels),
	      currentlyProcessedCLCW(0) {
		fop.vchan = this;
		fop.sentQueueFOP = &sentQueueTxTC;
		fop.waitQueueFOP = &waitQueueTxTC;
		fop.sentQueueFARM = &sentQueueRxTC;
		fop.waitQueueFARM = &sentQueueRxTC;
		farm.waitQueue = &waitQueueRxTC;
		farm.sentQueue = &sentQueueRxTC;
	}

	VirtualChannelAlert storeVC(TransferFrameTC* packet);

	/**
	 * @bried Add MAP channel to virtual channel
	 */
	VirtualChannelAlert add_map(const uint8_t mapid);

	MasterChannel& master_channel() {
		return masterChannel;
	}

private:
	/**
	 * TM packets after being processed by the MasterChannelReception Service
	 */
	etl::list<TransferFrameTM*, MaxReceivedRxTmInVirtBuffer> rxInFramesAfterMCReception;

	/**
	 * Buffer to store incoming packets BEFORE being processed by COP
	 */
	etl::list<TransferFrameTC*, MaxReceivedTxTcInWaitQueue> waitQueueTxTC;

	/**
	 * Buffer to store incoming packets AFTER being processed by COP
	 */
	etl::list<TransferFrameTC*, MaxReceivedTxTcInWaitQueue> waitQueueRxTC;

	/**
	 * Buffer to store outcoming packets AFTER being processed by COP
	 */
	etl::list<TransferFrameTC*, MaxReceivedTxTcInFOPSentQueue> sentQueueTxTC;

	/**
	 * Buffer to storeOut outcoming packets AFTER being processed by COP
	 */
	etl::list<TransferFrameTC*, MaxReceivedRxTcInFARMSentQueue> sentQueueRxTC;

	/**
	 * Buffer to store incoming packets AFTER being processed by FARM
	 */
	etl::list<TransferFrameTC*, MaxReceivedRxTcInVirtualChannelBuffer> rxInFramesAfterVCReception;

	/**
	 * Buffer to storeOut unprocessed packets that are directly processed in the virtual instead of MAP channel
	 */
	etl::list<TransferFrameTC*, MaxReceivedUnprocessedTxTcInVirtBuffer> txUnprocessedPacketListBufferTC;

	/**
	 * Holds the FOP state of the virtual channel
	 */
	FrameOperationProcedure fop;

	/**
	 * Buffer holding the master copy of the CLCW that is currently being processed
	 */
	CLCW currentlyProcessedCLCW;

	/**
	 * Holds the FARM state of the virtual channel
	 */
	FrameAcceptanceReporting farm;

	/**
	 * The Master Channel the Virtual Channel belongs in
	 */
	std::reference_wrapper<MasterChannel> masterChannel;

	/**
	 *  Queue that stores the pointers of the packets that will eventually be concatenated to transfer frame data.
	 */
	etl::queue<uint16_t, PacketBufferTmSize> packetLengthBufferTmTx;

	/**
	 *  Queue that stores the packet data that will eventually be concatenated to transfer frame data
	 */
	etl::queue<uint8_t, PacketBufferTmSize> packetBufferTmTx;
};

struct MasterChannel {
	friend class ServiceChannel;

	friend class FrameOperationProcedure;

	friend class FrameAcceptanceReporting;

	/**
	 * Virtual channels of the master channel
	 */
	// TODO: Type aliases because this is getting out of hand
	etl::flat_map<uint8_t, VirtualChannel, MaxVirtualChannels> virtualChannels;
	uint8_t frameCount{};

	MasterChannel()
	    : virtualChannels(), txOutFramesBeforeAllFramesGenerationListTC(),
	      txToBeTransmittedFramesAfterAllFramesGenerationListTC(), currFrameCountTM(0) {}

	MasterChannel(const MasterChannel& m)
	    : virtualChannels(m.virtualChannels), frameCount(m.frameCount),
	      txOutFramesBeforeAllFramesGenerationListTC(m.txOutFramesBeforeAllFramesGenerationListTC),
	      txToBeTransmittedFramesAfterAllFramesGenerationListTC(
	          m.txToBeTransmittedFramesAfterAllFramesGenerationListTC),
	      rxMasterCopyTC(m.rxMasterCopyTC), rxMasterCopyTM(m.rxMasterCopyTM), currFrameCountTM(m.currFrameCountTM) {
		for (auto& vc : virtualChannels) {
			vc.second.masterChannel = *this;
		}
	}

	/**
	 *
	 * @param packet TM
	 *  stores TM packet in order to be processed by the All Frames Generation Service
	 */
	MasterChannelAlert storeOut(TransferFrameTM* packet);

	/**
	 *
	 * @param packet TM
	 *  stores TM packet after it has been processed by the All Frames Generation Service
	 */
	MasterChannelAlert storeTransmittedOut(TransferFrameTM* packet);

	/**
	 * Keeps track of last master channel frame count. If lost frames in a master channel are detected, then a warning
	 * is logged. However, this isn't considered a reason for raising an error as per CCSDS TM Data Link.
	 * Upon initialization of the channel, a MC count of 0 is expected.
	 */
	uint8_t currFrameCountTM;

	/**
	 *
	 * @param packet TC
	 *  stores TC packet in order to be processed by the All Frames Generation Service
	 */
	MasterChannelAlert storeOut(TransferFrameTC* packet);

	/**
	 *
	 * @param packet TC
	 *  stores TC packet after it has been processed by the All Frames Generation Service
	 */
	MasterChannelAlert storeTransmittedOut(TransferFrameTC* packet);

	/**
	 * Returns the last stored Transfer Frame in txMasterCopyTc
	 */
	TransferFrameTC getLastTxMasterCopyTcFrame();

	/**
	 * Add virtual channel to master channel
	 */
	MasterChannelAlert addVC(const uint8_t vcid, const uint16_t maxFrameLength, const bool blocking,
	                         const uint8_t repetitionTypeAFrame, const uint8_t repetitionCopCtrl,
	                         const bool frameErrorControlFieldPresent, const bool secondaryHeaderTMPresent,
	                         const uint8_t secondaryHeaderTMLength, const bool operationalControlFieldTMPresent,
	                         SynchronizationFlag synchronization, const uint8_t farmSlidingWinWidth,
	                         const uint8_t farmPositiveWinWidth, const uint8_t farmNegativeWinWidth,
	                         const uint8_t vcRepetitions, etl::flat_map<uint8_t, MAPChannel, MaxMapChannels> mapChan);

	/**
	 * Add virtual channel to master channel
	 */
	MasterChannelAlert addVC(const uint8_t vcid, const uint16_t maxFrameLength, const bool blocking,
	                         const uint8_t repetitionTypeAFrame, const uint8_t repetitionCopCtrl,
	                         const bool frameErrorControlFieldPresent, const bool secondaryHeaderTMPresent,
	                         const uint8_t secondaryHeaderTMLength, const bool operationalControlFieldTMPresent,
	                         SynchronizationFlag synchronization, const uint8_t farmSlidingWinWidth,
	                         const uint8_t farmPositiveWinWidth, const uint8_t farmNegativeWinWidth, const uint8_t vcRepetitions);

private:
	// Packets stored in frames list, before being processed by the all frames generation service
	etl::list<TransferFrameTC*, MaxReceivedTxTcInMasterBuffer> txOutFramesBeforeAllFramesGenerationListTC;
	// Packets ready to be transmitted having passed through the all frames generation service
	etl::list<TransferFrameTC*, MaxReceivedTxTcOutInMasterBuffer> txToBeTransmittedFramesAfterAllFramesGenerationListTC;

	// TM packets stored in frames list, before being processed by the vc generation service
	etl::list<TransferFrameTM*, MaxReceivedTxTmInVCBuffer> txOutFramesBeforeMCGenerationListTM;
	// Packets ready to be transmitted having passed through the vc generatiooperationalControlFieldTMPresentn service
	etl::list<TransferFrameTM*, MaxReceivedTxTmOutInVCBuffer> txToBeTransmittedFramesAfterMCGenerationListTM;

	// TM packets stored in frames list, before being processed by the vc reception service
	etl::list<TransferFrameTM*, MaxReceivedTxTmInVCBuffer> txOutFramesBeforeMCReceptionListTM;
	// Packets ready to be transmitted having passed through the vc reception service
	etl::list<TransferFrameTM*, MaxReceivedTxTmOutInVCBuffer> txToBeTransmittedFramesAfterMCReceptionListTM;

	// Packets stored in frames list, before being processed by the all frames generation service
	etl::list<TransferFrameTM*, MaxReceivedTxTmInMasterBuffer> txOutFramesBeforeAllFramesGenerationListTM;
	// Packets ready to be transmitted having passed through the all frames generation service
	etl::list<TransferFrameTM*, MaxReceivedTxTmOutInMasterBuffer> txToBeTransmittedFramesAfterAllFramesGenerationListTM;

	// TC packets that are received, before being received by the all frames reception service
	etl::list<TransferFrameTC*, MaxReceivedRxTcInMasterBuffer> rxInFramesBeforeAllFramesReceptionListTC;
	// TM packets that are ready to be transmitted to higher procedures following all frames generation service
	etl::list<TransferFrameTC*, MaxReceivedRxTcOutInMasterBuffer> rxToBeTransmittedFramesAfterAllFramesReceptionListTC;

	// Buffer to store TM packets that are processed by the packet and VC Generation services
	etl::list<TransferFrameTM*, MaxReceivedUnprocessedTxTmInVirtBuffer> txProcessedPacketListBufferTM;

	/**
	 * Buffer holding the master copy of TC TX packets that are currently being processed
	 */
	etl::list<TransferFrameTC, MaxTxInMasterChannel> txMasterCopyTC;

	/**
	 * Removes TC frames from the Tx master buffer
	 */
	void removeMasterTx(TransferFrameTC* packet_ptr);

	/**
	 * Buffer holding the master copy of TM TX packets that are currently being processed
	 */
	etl::list<TransferFrameTM, MaxTxInMasterChannel> txMasterCopyTM;

	/**
	 * Removes TM frames from the Tx master buffer
	 */
	void removeMasterTx(TransferFrameTM* packet_ptr);

	/**
	 * Buffer holding the master copy of TC RX packets that are currently being processed
	 */
	etl::list<TransferFrameTC, MaxRxInMasterChannel> rxMasterCopyTC;

	/**
	 * Removes TM frames from the Rx master buffer
	 */
	void removeMasterRx(TransferFrameTC* packet_ptr);

	/**
	 * Buffer holding the master copy of TM RX packets that are currently being processed
	 */
	etl::list<TransferFrameTM, MaxRxInMasterChannel> rxMasterCopyTM;

	/**
	 * Removes TM frames from the RX master buffer
	 */
	void removeMasterRx(TransferFrameTM* packet_ptr);

	/**
	 * Sets the acknowledgement flag of a transfer frame to true
	 */
	void acknowledgeFrame(uint8_t frameSequenceNumber);

	/**
	 * Sets the toBeRetransmitted flag of a transfer frame to true
	 */
	void setRetransmitFrame(uint8_t frameSequenceNumber);

	MemoryPool masterChannelPool = MemoryPool();
};

#endif // CCSDS_CHANNEL_HPP
