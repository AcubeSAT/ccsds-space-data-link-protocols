#pragma once

#include <CCSDSChannel.hpp>
#include <Alert.hpp>
#include <optional>
#include <TransferFrameTC.hpp>
#include <utility>
#include <CCSDSLoggerImpl.h>

enum SegmentLengthID { SegmentationMiddle = 0x0, SegmentationStart = 0x1, SegmentaionEnd = 0x2, NoSegmentation = 0x3 };

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

	/**
	 * Variable to indicate that a CLCW has been constructed and should be sent
	 */
	bool clcwWaitingToBeTransmitted = false;

	/**
	 * Buffer to store the data of the clcw transfer frame
	 */
	uint8_t clcwTransferFrameDataBuffer[TmTransferFrameSize] = {0};

	etl::list<TransferFrameTM, 1> clcwTransferFrameBuffer;

public:
	// Public methods that are called by the scheduler

    // TC TransferFrame - Sending End (TC Tx)

    //     - Utility and Debugging

    /**
     * Returns the first frame in the masterCopyTcTx buffer
     */
    TransferFrameTC getFirstMasterCopyTcFrame();

    /**
     * Returns the last frame in the masterCopyTcTx buffer
     */
    TransferFrameTC getLastMasterCopyTcFrame();

    /**
     * Available space in master channel buffer
     */
    uint16_t txOutAvailable() const {
        return masterChannel.txToBeTransmittedFramesAfterAllFramesGenerationListTC.available();
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
     * Read first TC packet of the virtual channel buffer (txUnprocessedPacketListBufferTC)
     */
    std::pair<ServiceChannelNotification, const TransferFrameTC*> txOutPacketTC(uint8_t vid) const;

    /**
     * Return the last stored packet from txMasterCopyTC
     */
    std::pair<ServiceChannelNotification, const TransferFrameTC*> txOutPacketTC() const;

    std::pair<ServiceChannelNotification, const TransferFrameTC*> txOutProcessedPacketTC() const;

    /**
     * @return The front TC TransferFrame from txOutFramesBeforeAllFramesGenerationListTC
     */
    std::optional<TransferFrameTC> getTxProcessedPacket();

    //     - MAP Packet Processing
    /**
     * Auxiliary function used to implement the segmentation of packets
     * @param numberOfTransferFrames The number of transfer frames that the packet will segmented into
     * @param transferFrameDataLength The length of the data field
     * @param packetLength The length of the next packet in the buffer
     * @return Service Channel Notification
     */
    ServiceChannelNotification segmentationTC(uint8_t numberOfTransferFrames, uint16_t packetLength,
                                              uint16_t transferFrameDataLength, uint8_t vid, uint8_t mapid,
                                              ServiceType service_type);

    /**
     * Auxiliary function for blocking of packets stored in the stored packet buffer
     * @param transferFrameDataLength   The length of the data field of the TM Transfer frame
     * @param packetLength              The length of the next packet in the stored TM packet buffer
     * @param vcid                      Virtual Channel ID
     * @param mapid                     MAP Channel ID
     * @param serviceType              Service Type for differentiation of Type-A and Type-B frames
     * @return                          A Service Channel Notification
     */
    ServiceChannelNotification blockingTC(uint16_t transferFrameDataLength, uint16_t packetLength,
                                          uint8_t vid, uint8_t mapid, ServiceType serviceType);

    /**
     * Method that stores a packet pointer and the packet data to the relevant buffers
     * queues
     * @param packet        Pointer to the packet data
     * @param packetLength  Length of the packet
     * @param vid           Virtual channel id
     * @param mapid         MAP channel id
     * @param service_type  Service Type
     */
    ServiceChannelNotification storePacketTc(uint8_t *packet, uint16_t packetLength, uint8_t vid, uint8_t mapid,
                                             ServiceType service_type);

    /**
     * Requests to process the last packet stored in the buffer of the specific MAPP channel
     * (possible more if blocking is enabled). The packets are segmented or blocked together
     * from data of the memory pool and then transferred to the buffer of the virtual channel
     * @param vid                       Virtual Channel ID
     * @param mapid                     MAP Id
     * @param transferFrameDataLength   Data length of the transfer frame
     * @param serviceType               Service type of resulting frame. Only packets from the respective service will
     *                                  be grouped together
     */
    ServiceChannelNotification mappRequest(uint8_t vid, uint8_t mapid, uint8_t transferFrameDataLength,
                                           ServiceType serviceType);

    //     - Virtual Channel Packet Processing
    //     - Virtual Channel Generation
    /**
     * The  Virtual  Channel  Generation  Function  shall  perform  the  following  two
     * procedures in the following order:
     * 		a) the  Frame  Operation  Procedure  (FOP),  which  is  a  sub-procedure  of  the
     * 		Communications Operation Procedure (COP); and
     * 		b) the Frame Generation Procedure in this order.
     * @see p. 4.3.5 from TC Space Data Link Protocol
     */
    ServiceChannelNotification vcGenerationRequestTC(uint8_t vid);

    //         -- FOP Directives

    ServiceChannelNotification frameTransmission(uint8_t* tframe);

    ServiceChannelNotification transmitAdFrame(uint8_t vid);

    ServiceChannelNotification pushSentQueue(uint8_t vid);

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

    CLCW getClcwInBuffer();

    uint8_t* getClcwTransferFrameDataBuffer();

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


    //     - All frames generation
    /**
     * The  All  Frames  Generation  Function  shall  be  used  to  perform  error  control
     * encoding defined by this Recommendation and to deliver Transfer Frames at an appropriate
     * rate to the Channel Coding Sublayer.
     * @see p. 4.3.8 from TC Space Data Link Protocol
     */
    ServiceChannelNotification allFramesGenerationTCRequest();


    // TC TransferFrame - Receiving End (TC Rx)

    //     - Utility and Debugging
    /**
     * Read first packet of the TC MAP channel buffer (unprocessedPacketListBufferTC)
     */
    std::pair<ServiceChannelNotification, const TransferFrameTC*> txOutPacketTC(uint8_t vid, uint8_t mapid) const;

    /**
	 * Available space in master channel buffer
	 */
    uint16_t rxOutAvailableTC() const {
        return masterChannel.rxToBeTransmittedFramesAfterAllFramesReceptionListTC.available();
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
        if (masterChannel.virtualChannels.find(vid) == masterChannel.virtualChannels.end()) {
            ccsdsLogNotice(Tx, TypeServiceChannelNotif, INVALID_VC_ID);
            return ServiceChannelNotification::INVALID_VC_ID;
        }
        const VirtualChannel& virtualChannel = masterChannel.virtualChannels.at(vid);
        if (!virtualChannel.segmentHeaderPresent) {
            return ServiceChannelNotification::INVALID_MAP_ID;
        }
        if (virtualChannel.segmentHeaderPresent &&
            (virtualChannel.mapChannels.find(mapid) == virtualChannel.mapChannels.end())) {
            ccsdsLogNotice(Tx, TypeServiceChannelNotif, INVALID_MAP_ID);
            return ServiceChannelNotification::INVALID_MAP_ID;
        }
        return masterChannel.virtualChannels.at(vid).mapChannels.at(mapid).rxInFramesAfterVCReception.available();
    }

    /**
     * Available space in TC MAP channel buffer
     */
    uint16_t txAvailableTC(const uint8_t vid, const uint8_t mapid) const {
        if (masterChannel.virtualChannels.find(vid) == masterChannel.virtualChannels.end()) {
            ccsdsLogNotice(Tx, TypeServiceChannelNotif, INVALID_VC_ID);
            return ServiceChannelNotification::INVALID_VC_ID;
        }
        const VirtualChannel& virtualChannel = masterChannel.virtualChannels.at(vid);
        if (!virtualChannel.segmentHeaderPresent) {
            return ServiceChannelNotification::INVALID_MAP_ID;
        }
        if (virtualChannel.segmentHeaderPresent &&
            (virtualChannel.mapChannels.find(mapid) == virtualChannel.mapChannels.end())) {
            ccsdsLogNotice(Tx, TypeServiceChannelNotif, INVALID_MAP_ID);
            return ServiceChannelNotification::INVALID_MAP_ID;
        }
        return virtualChannel.mapChannels.at(mapid).availableBufferTC();
    }

    //     - All Frames Reception
    /**
     * This service is used for storing incoming TC packets in the master channel
     * @param packet Raw packet data
     * @param packetLength The length of the packet
     */
    ServiceChannelNotification storeTC(uint8_t* packet, uint16_t packetLength);

    /**
     * The  All  Frames  Generation  Function  shall  be  used  to  perform  error  control
     * encoding defined by this Recommendation and to deliver Transfer Frames at an appropriate
     * rate to the Channel Coding Sublayer.
     * @see p. 4.2.7 from TC Space Data Link Protocol
     */
    ServiceChannelNotification allFramesReceptionTCRequest();


    //     - Master Channel Demultiplexing

    //     - Virtual Channel Reception
    /**
     * The Virtual Channel Reception Function shall perform the Frame Acceptance and
     *  Reporting Mechanism (FARM), which is a sub-procedure of the Communications Operation
     *  Procedure (COP)
     @see  p. 4.4.5 from TC Space Data Link Protocol
     */
    ServiceChannelNotification vcReceptionTC(uint8_t vid);

    //         -- Farm Directives
    /**
     * @brief A function that generates a CLCW and stores it to a clcw buffer
     */
    ServiceChannelNotification clcwReportTime(uint8_t vid);

    //     - MAP Demultiplexing
    //     - Virtual Channel Extraction
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

    //     - MAP Packet Extraction
    /**
     * The MAP Packet Extraction Function shall be used to extract variable-length
     * Packets from Frame Data Units on a MAP Channel
     * @see 4.4.1 from TC Data Link Protocol
     * @param vid Virtual channel ID
     * @param mapid MAP channel ID
     * @param packet Provided packet data
     */
    ServiceChannelNotification packetExtractionTC(uint8_t vid, uint8_t mapid, uint8_t* packet);

    // TM TransferFrame - Sending End (TM Tx)

    //     - Utility and Debugging
    uint16_t availableSpaceBufferTxTM() const {
        return masterChannel.txMasterCopyTM.available();
    }
    /**
     * Returns the available space in the packetLengthBufferTmTx buffer
     */
    uint16_t availableInPacketLengthBufferTmTx(uint8_t gvcid);

    /**
     * Returns the available space in the packetBufferTmTx buffer
     */
    uint16_t availableInPacketBufferTmTx(uint8_t gvcid);

    /**
     * Return the last stored packet from txMasterCopyTM
     */
    std::pair<ServiceChannelNotification, const TransferFrameTM*> txOutPacketTM() const;

    /**
     * Return the last processed packet
     */
    std::pair<ServiceChannelNotification, const TransferFrameTM*> txOutProcessedPacketTM() const;

    /**
     * Fetch packet in the top of the MC buffer
     */
    const TransferFrameTM* packetMasterChannel() const {
        return masterChannel.txProcessedPacketListBufferTM.front();
    }

    //     - Packet Processing
    /**
     * Serves as the main entry point from the upper layers
     * Stores the raw packet data along with the length of the packet so they can be later turned into transfer frames
     * so they can be later transmitted
     *
     * @param packet pointer to the packet data
     * @param packetLength length of the packet
     * @param vid the virtual channel id
     */
    ServiceChannelNotification storePacketTm(uint8_t* packet, uint16_t packetLength, uint8_t vid);

    //     - Virtual Channel Generation
    /**
	 * Function used by the vcGenerationServiceTM function to implement the segmentation of packets stored in
	 * packetBufferTmTx
	 * @param numberOfTransferFrames The number of transfer frames that the packet will segmented into
	 * @param transferFrameDataLength The length of the data field of the TM Transfer frame, taken by the
	 * vcGenerationServiceTM parameter
	 * @param packetLength The length of the next packet in the packetBufferTmTx
	 * @return A Service Channel Notification as it is the case with vcGenerationServiceTM
	 */
    ServiceChannelNotification segmentationTM(uint8_t numberOfTransferFrames, uint16_t packetLength,
                                              uint16_t transferFrameDataLength, uint8_t gvcid);

    /**
     * Auxiliary function for blocking of packets stored in the stored packet buffer
     * @param transferFrameDataLength   The length of the data field of the TM Transfer frame
     * @param packetLength              The length of the next packet in the stored TM packet buffer
     * @param vcid                      Virtual Channel ID
     * @return                          A Service Channel Notification
     */
    ServiceChannelNotification blockingTM(uint16_t transferFrameDataLength, uint16_t packetLength, uint8_t vid);

    /**
     * Service that generates a transfer frame by combining the packet data to transfer frame data and initializes the
     * transfer frame primary header
     * @param transferFrameDataLength the maximum transfer frame data length
     * @param gvcid the global virtual channel id
     * @return PACKET_BUFFER_EMPTY Alert if the virtual channel packet buffer is empty
     * NO_TX_PACKETS_TO_TRANSFER_FRAME Alert if no packets from the packet buffer can be stored to the transfer frame
     * NO_SERVICE_EVENT Alert if the packets are stored as expected to the transfer frame
     */
    ServiceChannelNotification vcGenerationServiceTM(uint16_t transferFrameDataLength, uint8_t vid);

    //     - Virtual Channel Multiplexing

    //     - Master Channel Generation
    /**
     * The Master Channel Generation Service shall be used to insert Transfer Frame
     * Secondary Header and/or Operational Control Field service data units into Transfer Frames
     * of a Master Channel.
     @see p. 4.2.5 from TM Space Data Link Protocol (CCSDS 132.0-B-3)
     */
    ServiceChannelNotification mcGenerationTMRequest();

    //     - Master Channel Multiplexing

    //     - All Frames Generation
    /**
     * The  All  Frames  Generation  Function  shall  be  used  to  perform  error  control
     * encoding defined by this Recommendation and to deliver Transfer Frames at an appropriate
     * rate to the Channel Coding Sublayer.
     * @see p. 4.2.7 from TC Space Data Link Protocol
     */
    ServiceChannelNotification allFramesGenerationTMRequest(uint8_t* packet_data,
                                                            uint16_t packet_length = TmTransferFrameSize);


    // TM TransferFrame - Receiving End (TM Rx)

    //     - Utility and Debugging
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

    uint16_t availableSpaceBufferRxTM() const {
        return masterChannel.rxMasterCopyTM.available();
    }

    uint8_t getFrameCountTM(uint8_t vid);

    uint8_t getFrameCountTM();

    //     - All Frames Reception
    /**
	 * The  All  Frames  Reception  Function  shall  be  used  to  perform  error  control
	 * encoding defined by this Recommendation and to deliver Transfer Frames at an appropriate
	 * rate to the Channel Coding Sublayer. Also writes the received packet to the provided pointer.
	 * @see p. 4.3.7 from TM Space Data Link Protocol (CCSDS 132.0-B-3)
	 */
    ServiceChannelNotification allFramesReceptionTMRequest(uint8_t* packet, uint16_t packetLength);

    //     - Master Channel Demultiplexing

    //     - Master Channel Reception

    //     - Virtual Channel Demultiplexing

    //     - Virtual Channel Reception

    //     - Packet Extraction
    /**
     * This service is used for extracting RX TM packet. It signals the end of the TM Rx chain
     * @param vid           Virtual Channel ID that determines from which vid buffer the frame is processed
     * @param packet_target A pointer to the packet buffer. The user has to pre-allocate the correct size for the buffer
     */
    ServiceChannelNotification packetExtractionTM(uint8_t vid, uint8_t* packet_target);


    // Not sure about the purpose of that one
	/**
	 * Processes the packet at the head of the buffer
	 */
	void process();


	// This is honestly a bit confusing
	ServiceChannel(MasterChannel masterChannel, PhysicalChannel physicalChannel)
	    : masterChannel(masterChannel), physicalChannel(physicalChannel) {}
	//Default constructor
	ServiceChannel() : masterChannel(), physicalChannel(){};
};