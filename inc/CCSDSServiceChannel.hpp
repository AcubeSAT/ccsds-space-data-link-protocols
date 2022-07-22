#ifndef CCSDS_SERVICECHANNEL_HPP
#define CCSDS_SERVICECHANNEL_HPP

#include <CCSDSChannel.hpp>
#include <Alert.hpp>
#include <optional>
#include <TransferFrameTC.hpp>
#include <utility>
#include <CCSDSLoggerImpl.h>

enum SegmentLengthID{
    SegmentationMiddle = 0x0,
    SegmentationStart = 0x1,
    SegmentaionEnd = 0x2,
    NoSegmentation = 0x3
};

/**
 *  This provides a way to interconnect all different CCSDS Space Data Protocol Services and provides a
 * bidirectional interface between the receiving and transmitting parties
 */

class ServiceChannel {
private:
	/**
	 * The Master Channel essentially stores the configuration of your channel. It partitions the physical
	 * channel into virtual channels, each of which has different parameters in order to easily manage incoming traffic
	 */
	MasterChannel masterChannel;
	/**
	 * PhysicalChannel is used to simply represent parameters of the physical channel like the maximum frame
	 * length
	 * @TODO: Replace defines for maxFrameLength
	 */
	PhysicalChannel physicalChannel;

	// @todo Think about whether we need to keep the size of the packets in a queue. Technically, we can determine this
	// from the header of the frame. Not sure what makes more sense

	uint8_t packetCountTM;

    /**
     * Variable to indicate that a CLCW has been constructed and should be sent
     */
    bool clcwWaiting = false;

    /**
     * Buffer to store the data of the clcw transfer frame
     */
    uint8_t clcwTransferFrameBuffer[TmTransferFrameSize] = {0};

public:
	// Public methods that are called by the scheduler

	/**
	 * Fetch packet in the top of the MC buffer
	 */
	const TransferFrameTM* packetMasterChannel() const {
		return masterChannel.txProcessedPacketListBufferTM.front();
	}

	uint16_t availableMcTxTM() const {
		return masterChannel.txProcessedPacketListBufferTM.available();
	}

	/**
	 * Stores an incoming  TC packet in the ring buffer
	 *
	 * @param packet Data of the packet
	 * @param packetLength TransferFrameTC length
	 * @param gvcid Global Virtual Channel ID
	 * @param mapid MAP ID
	 * @param sduid SDU ID
	 * @param serviceType Service Type - Type-A or Type-B
	 */
	ServiceChannelNotification storeTC(uint8_t* packet, uint16_t packetLength, uint8_t gvcid, uint8_t mapid,
	                                   uint16_t sduid, ServiceType serviceType);

	/**
	 * This service is used for storing incoming TM packets in the master channel. It also includes the
	 * Virtual Channel Generation Service
	 *
	 * @param packet Data of the packet
	 * @param packetLength TransferFrameTC length
	 * @param gvcid Global Virtual Channel ID
	 * @param scid Spacecraft ID
	 */

	ServiceChannelNotification storeTM(uint8_t* packet, uint16_t packetLength, uint8_t gvcid);

	/**
	 * Service that generates a transfer frame by combining the packet data to transfer frame data and initializes the
	 * transfer frame primary header
	 * @param maxTransferFrameDataLength the maximum transfer frame data legnth
	 * @param gvcid the global virtual channel id
	 * @return PACKET_BUFFER_EMPTY Alert if the virtual channel packet buffer is empty
	 * NO_TX_PACKETS_TO_TRANSFER_FRAME Alert if no packets from the packet buffer can be stored to the transfer frame
	 * NO_SERVICE_EVENT Alert if the packets are stored as expected to the transfer frame
	 */
	ServiceChannelNotification vcGenerationService(uint16_t maxTransferFrameDataLength, uint8_t gvcid);

	/**
	 * Method that stores a TM packet pointer and the TM  packet data to the packetLengthBufferTmTx and packetBufferTmTx
	 * queues
	 * @param packet pointer to the packet data
	 * @param packetLength length of the packet
	 * @param gvcid the global virtual channel id
	 */
	ServiceChannelNotification storePacketTm(uint8_t* packet, uint16_t packetLength, uint8_t gvcid);

	/**
	 * This service is used for extracting RX TM packet. It signals the end of the TM Rx chain
	 * @param vid           Virtual Channel ID that determines from which vid buffer the frame is processed
	 * @param packet_target A pointer to the packet buffer. The user has to pre-allocate the correct size for the buffer
	 */
	ServiceChannelNotification packetExtractionTM(uint8_t vid, uint8_t* packet_target);

	/**
	 * This service is used for storing incoming TC packets in the master channel
	 * @param packet Raw packet data
	 * @param packetLength The length of the packet
	 */
	ServiceChannelNotification storeTC(uint8_t* packet, uint16_t packetLength);

	/**
	 * Requests to process the last packet stored in the buffer of the specific MAPP channel
	 * (possible more if blocking is enabled). The packets are segmented or blocked together
	 * and then transferred to the buffer of the virtual channel
	 */
	ServiceChannelNotification mappRequest(uint8_t vid, uint8_t mapid);

	uint16_t availableSpaceBufferTxTM() const {
		return masterChannel.txMasterCopyTM.available();
	}

	uint16_t availableSpaceBufferRxTM() const {
		return masterChannel.rxMasterCopyTM.available();
	}

#if maxReceivedUnprocessedTcInVirtBuffer > 0

	/**
	 *  Requests to process the last packet stored in the buffer of the specific virtual channel
	 * (possible more if blocking is enabled). The packets are segmented or blocked together
	 * and then stored in the buffer of the virtual channel
	 */
	ServiceChannelNotif vcpp_request(uint8_t vid);

#endif

	/**
	 * The Master Channel Generation Service shall be used to insert Transfer Frame
	 * Secondary Header and/or Operational Control Field service data units into Transfer Frames
	 * of a Master Channel.
	 @see p. 4.2.5 from TM Space Data Link Protocol (CCSDS 132.0-B-3)
	 */
	ServiceChannelNotification mcGenerationTMRequest();
	/**
	 * The  Virtual  Channel  Generation  Function  shall  perform  the  following  two
	 * procedures in the following order:
	 * 		a) the  Frame  Operation  Procedure  (FOP),  which  is  a  sub-procedure  of  the
	 * 		Communications Operation Procedure (COP); and
	 * 		b) the Frame Generation Procedure in this order.
	 * @see p. 4.3.5 from TC Space Data Link Protocol
	 */
	ServiceChannelNotification vcGenerationRequestTC(uint8_t vid);

	/**
	 * The Virtual Channel Reception Function shall perform the Frame Acceptance and
	 *  Reporting Mechanism (FARM), which is a sub-procedure of the Communications Operation
	 *  Procedure (COP)
	 @see  p. 4.4.5 from TC Space Data Link Protocol
	 */
	ServiceChannelNotification vcReceptionTC(uint8_t vid);

	/**
	 * The  All  Frames  Generation  Function  shall  be  used  to  perform  error  control
	 * encoding defined by this Recommendation and to deliver Transfer Frames at an appropriate
	 * rate to the Channel Coding Sublayer.
	 * @see p. 4.3.8 from TC Space Data Link Protocol
	 */
	ServiceChannelNotification allFramesGenerationTCRequest();

	/**
	 * The  All  Frames  Reception  Function  shall  be  used  to  perform  error  control
	 * encoding defined by this Recommendation and to deliver Transfer Frames at an appropriate
	 * rate to the Channel Coding Sublayer. Also writes the received packet to the provided pointer.
	 * @see p. 4.3.7 from TM Space Data Link Protocol (CCSDS 132.0-B-3)
	 */
	ServiceChannelNotification allFramesReceptionTMRequest(uint8_t* packet, uint16_t packetLength);

	/**
	 * The MAP Packet Extraction Function shall be used to extract variable-length
	 * Packets from Frame Data Units on a MAP Channel
	 * @see 4.4.1 from TC Data Link Protocol
	 * @param vid Virtual channel ID
	 * @param mapid MAP channel ID
	 * @param packet Provided packet data
	 */
	ServiceChannelNotification packetExtractionTC(uint8_t vid, uint8_t mapid, uint8_t* packet);

	/**
	 * The VC Packet Extraction Function shall be used to extract variable-length
	 * Packets from Frame Data Units on a Virtual Channel in case a segmentation header
	 * isn't used
	 * @see 4.4.1 from TC Data Link Protocol
	 * @param vid Virtual channel ID
	 * @param packet Provided packet data
	 *
	 * @warning This function assumes that the packet size is checked and correct
	 */
	ServiceChannelNotification packetExtractionTC(uint8_t vid, uint8_t* packet);

	/**
	 * The  All  Frames  Generation  Function  shall  be  used  to  perform  error  control
	 * encoding defined by this Recommendation and to deliver Transfer Frames at an appropriate
	 * rate to the Channel Coding Sublayer.
	 * @see p. 4.2.7 from TC Space Data Link Protocol
	 */
	ServiceChannelNotification allFramesGenerationTMRequest(uint8_t* packet_data,
	                                                        uint16_t packet_length = TmTransferFrameSize);

	ServiceChannelNotification allFramesReceptionTCRequest();
	/**
	 * @return The front TC TransferFrame from txOutFramesBeforeAllFramesGenerationListTC
	 */
	std::optional<TransferFrameTC> getTxProcessedPacket();

	// COP Directives

	ServiceChannelNotification transmitFrame(uint8_t* pack);

	ServiceChannelNotification transmitAdFrame(uint8_t vid);

	ServiceChannelNotification pushSentQueue(uint8_t vid);

	uint8_t getFrameCountTM(uint8_t vid);

	uint8_t getFrameCountTM();

	// TODO: Properly handle Notifications
	void acknowledgeFrame(uint8_t vid, uint8_t frameSeqNumber);

	void clearAcknowledgedFrames(uint8_t vid);

	void initiateAdNoClcw(uint8_t vid);

	void initiateAdClcw(uint8_t vid);

	void initiateAdUnlock(uint8_t vid);

	void initiateAdVr(uint8_t vid, uint8_t vr);

	void terminateAdService(uint8_t vid);

	void resumeAdService(uint8_t vid);

	void setVs(uint8_t vid, uint8_t vs);

	void setFopWidth(uint8_t vid, uint8_t width);

	void setT1Initial(uint8_t vid, uint16_t t1_init);

	void setTransmissionLimit(uint8_t vid, uint8_t vr);

	void setTimeoutType(uint8_t vid, bool vr);

	void invalidDirective(uint8_t vid);

    /**
     * Returns the available space in the packetLengthBufferTmTx buffer
     */
    uint16_t availableInPacketLengthBufferTmTx(uint8_t gvcid);

    /**
     * Returns the available space in the packetBufferTmTx buffer
     */
    uint16_t availableInPacketBufferTmTx(uint8_t gvcid);

    /**
     * Function used by the vcGenerationService function to implement the blocking of packets stored in packetBufferTmTx
     * @param transferFrameDataLength The length of the data field of the TM Transfer frame, taken by the vcGenerationService parameter
     * @param packetLength The length of the next packet in the packetBufferTmTx
     * @return A Service Channel Notification as it is the case with vcGenerationService
     */
    ServiceChannelNotification blockingTm(uint16_t  transferFrameDataLength, uint16_t packetLength, uint8_t gvcid);

    /**
     * Function used by the vcGenerationService function to implement the segmentation of packets stored in packetBufferTmTx
     * @param numberOfTransferFrames The number of transfer frames that the packet will segmented into
     * @param transferFrameDataLength The length of the data field of the TM Transfer frame, taken by the vcGenerationService parameter
     * @param packetLength The length of the next packet in the packetBufferTmTx
     * @return A Service Channel Notification as it is the case with vcGenerationService
     */
    ServiceChannelNotification segmentationTm(uint8_t numberOfTransferFrames, uint16_t packetLength, uint16_t transferFrameDataLength, uint8_t gvcid);

    /**
     * @brief A function that generates a CLCW and stores it to a clcw buffer
     */
    ServiceChannelNotification clcwReportTime(uint8_t vid);
	/**
	 * Get FOP State of the virtual channel
	 */
	FOPState fopState(uint8_t vid) const;

	/**
	 * Returns the value of the timer that is used to determine the time frame for acknowledging transferred
	 * frames
	 */
	uint16_t t1Timer(uint8_t vid) const;

	/**
	 * Indicates the width of the sliding window which is used to proceed to the lockout state in case the
	 * transfer frame number of the received packet deviates too much from the expected one.
	 */
	uint8_t fopSlidingWindowWidth(uint8_t vid) const;

	/**
	 * Returns the timeout action which is to be performed once the maximum transmission limit is reached and
	 * the timer has expired.
	 */
	bool timeoutType(uint8_t vid) const;

	/**
	 * Returns the last frame sequence number, V(S), that will be placed in the header of the next transferred
	 * packet
	 * @param vid Virtual Channel ID
	 */
	uint8_t transmitterFrameSeqNumber(uint8_t vid) const;

	/**
	 * Returns the expected acknowledgement frame sequence number, NN(R). This is essentially the frame sequence
	 * number of the oldest unacknowledged frame
	 * @param vid Virtual Channel ID
	 */
	uint8_t expectedFrameSeqNumber(uint8_t vid) const;

	/**
	 * Processes the packet at the head of the buffer
	 */
	void process();

	/**
	 * Available number of incoming TM frames in virtual channel buffer
	 */

	uint16_t rxInAvailableTM(uint8_t vid) const {
        if (masterChannel.virtualChannels.find(vid) == masterChannel.virtualChannels.end()) {
            ccsdsLogNotice(Tx, TypeServiceChannelNotif, INVALID_VC_ID);
            return ServiceChannelNotification::INVALID_VC_ID;
        }
		return masterChannel.virtualChannels.at(vid).rxInAvailableTM();
	}
	/**
	 * Available number of outcoming TX frames in master channel buffer
	 */
	uint16_t txOutAvailable() const {
		return masterChannel.txToBeTransmittedFramesAfterAllFramesGenerationListTC.available();
	}

	/**
	 * Available number of outcoming TC RX frames in master channel buffer
	 */
	uint16_t rxOutAvailableTC() const {
		return masterChannel.rxToBeTransmittedFramesAfterAllFramesReceptionListTC.available();
	}

	/**
	 * Available space in TC virtual channel buffer
	 */
	uint16_t txAvailableTC(const uint8_t vid) const {
        if (masterChannel.virtualChannels.find(vid) == masterChannel.virtualChannels.end()) {
            ccsdsLogNotice(Tx, TypeServiceChannelNotif, INVALID_VC_ID);
            return ServiceChannelNotification::INVALID_VC_ID;
        }
		return masterChannel.virtualChannels.at(vid).availableBufferTC();
	}

	/**
	 * Available space in TC MAP channel buffer
	 */
	uint16_t txAvailableTC(const uint8_t vid, const uint8_t mapid) const {
        if (masterChannel.virtualChannels.find(vid) == masterChannel.virtualChannels.end()){
            ccsdsLogNotice(Tx, TypeServiceChannelNotif, INVALID_VC_ID);
            return ServiceChannelNotification::INVALID_VC_ID;
        }
        const VirtualChannel &virtualChannel = masterChannel.virtualChannels.at(vid);
		if (!virtualChannel.segmentHeaderPresent){
			return ServiceChannelNotification::INVALID_MAP_ID;
		}
        if (virtualChannel.segmentHeaderPresent && (virtualChannel.mapChannels.find(mapid) == virtualChannel.mapChannels.end())) {
            ccsdsLogNotice(Tx, TypeServiceChannelNotif, INVALID_MAP_ID);
            return ServiceChannelNotification::INVALID_MAP_ID;
        }
		return virtualChannel.mapChannels.at(mapid).availableBufferTC();
	}

	/**
	 * Available space for packets at waitQueueRxTC buffer
	 */
	uint16_t getAvailableWaitQueueRxTC(uint8_t vid) const {
        if (masterChannel.virtualChannels.find(vid) == masterChannel.virtualChannels.end()) {
            ccsdsLogNotice(Tx, TypeServiceChannelNotif, INVALID_VC_ID);
            return ServiceChannelNotification::INVALID_VC_ID;
        }
		return masterChannel.virtualChannels.at(vid).waitQueueRxTC.available();
	}

	/**
	 * Available space for packets waiting to be processed from the VC Generation Service
	 */
	uint16_t getAvailableRxInFramesAfterVCReception(uint8_t vid) const {
        if (masterChannel.virtualChannels.find(vid) == masterChannel.virtualChannels.end()) {
            ccsdsLogNotice(Tx, TypeServiceChannelNotif, INVALID_VC_ID);
            return ServiceChannelNotification::INVALID_VC_ID;
        }
		return masterChannel.virtualChannels.at(vid).rxInFramesAfterVCReception.available();
	}

	/**
	 * Available space for packets at rxInFramesAfterVCReception buffer
	 */
	uint16_t getAvailableRxInFramesAfterVCReception(uint8_t vid, uint8_t mapid) const {
        if (masterChannel.virtualChannels.find(vid) == masterChannel.virtualChannels.end()){
            ccsdsLogNotice(Tx, TypeServiceChannelNotif, INVALID_VC_ID);
            return ServiceChannelNotification::INVALID_VC_ID;
        }
        const VirtualChannel &virtualChannel = masterChannel.virtualChannels.at(vid);
        if (!virtualChannel.segmentHeaderPresent){
            return ServiceChannelNotification::INVALID_MAP_ID;
        }
        if (virtualChannel.segmentHeaderPresent && (virtualChannel.mapChannels.find(mapid) == virtualChannel.mapChannels.end())) {
            ccsdsLogNotice(Tx, TypeServiceChannelNotif, INVALID_MAP_ID);
            return ServiceChannelNotification::INVALID_MAP_ID;
        }
		return masterChannel.virtualChannels.at(vid).mapChannels.at(mapid).rxInFramesAfterVCReception.available();
	}

	/**
	 * Read first packet of the TC MAP channel buffer (unprocessedPacketListBufferTC)
	 */
	std::pair<ServiceChannelNotification, const TransferFrameTC*> txOutPacketTC(uint8_t vid, uint8_t mapid) const;

	/**
	 * Read first TC packet of the virtual channel buffer (txUnprocessedPacketListBufferTC)
	 */
	std::pair<ServiceChannelNotification, const TransferFrameTC*> txOutPacketTC(uint8_t vid) const;

	/**
	 * Return the last stored packet from txMasterCopyTC
	 */
	std::pair<ServiceChannelNotification, const TransferFrameTC*> txOutPacketTC() const;

	/**
	 * Return the last stored packet from txMasterCopyTM
	 */
	std::pair<ServiceChannelNotification, const TransferFrameTM*> txOutPacketTM() const;

	/**
	 * Return the last processed packet
	 */
	std::pair<ServiceChannelNotification, const TransferFrameTM*> txOutProcessedPacketTM() const;
	std::pair<ServiceChannelNotification, const TransferFrameTC*> txOutProcessedPacketTC() const;

	// This is honestly a bit confusing
	ServiceChannel(MasterChannel masterChannel, PhysicalChannel physicalChannel)
	    : masterChannel(masterChannel), physicalChannel(physicalChannel) {}
};

#endif // CCSDS_CCSDSSERVICECHANNEL_HPP