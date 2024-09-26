#pragma once

#include <CCSDSChannel.hpp>
#include <Alert.hpp>
#include <optional>
#include <TransferFrameTC.hpp>
#include <utility>
#include <CCSDSLoggerImpl.h>

enum SegmentLengthID { SegmentationMiddle = 0x0, SegmentationStart = 0x1, SegmentationEnd = 0x2, NoSegmentation = 0x3 };

/**
 *  This provides a way to interconnect all different CCSDS Space Data Protocol Services and provides a
 *  bidirectional interface between the receiving and transmitting parties
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
	 * TODO: Replace defines for maxFrameLength
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
    /**
     * Get a reference to the master channel
     */
     MasterChannel& getMasterChannel(){
         return masterChannel;
     }

	// Public methods that are called by the scheduler

    // TC TransferFrame - Sending End (TC Tx)

    //     - Utility and Debugging

    /**
     * Returns the first frame in the unprocessedFrameListBufferMcCopyTxTC buffer
     */
    TransferFrameTC frontUnprocessedFrameMcCopyTxTC();

    /**
     * Returns the last frame in the unprocessedFrameListBufferMcCopyTxTC buffer
     */
    // TODO This is probably not needed, we already have backUnprocessedFrameMcCopyTxTC
    TransferFrameTC getLastMasterCopyTcFrame();

    /**
     * Available space in master channel buffer
     */
    uint16_t availableFramesAfterAllFramesGenerationTxTC() const {
        return masterChannel.toBeTransmittedFramesAfterAllFramesGenerationListTxTC.available();
    }

    /**
     * Available space in TC virtual channel buffer
     */
    uint16_t availableUnprocessedFramesTxTC(const uint8_t vid) const {
        if (masterChannel.virtualChannels.find(vid) == masterChannel.virtualChannels.end()) {
            ccsdsLogNotice(Tx, TypeServiceChannelNotif, INVALID_VC_ID);
            return ServiceChannelNotification::INVALID_VC_ID;
        }
        return masterChannel.virtualChannels.at(vid).availableBufferTC();
    }

    /**
     * Read first TC transfer frame of the virtual channel buffer (unprocessedFrameListBufferTxTC)
     */
    std::pair<ServiceChannelNotification, const TransferFrameTC*> frontUnprocessedFrameTxTC(uint8_t vid) const;

    /**
     * Return the last stored transfer frame from masterCopyTxTC
     */
    std::pair<ServiceChannelNotification, const TransferFrameTC*> backUnprocessedFrameMcCopyTxTC() const;

    std::pair<ServiceChannelNotification, const TransferFrameTC*> frontFrameAfterAllFramesGenerationTxTC() const;

    /**
     * @return The front TC TransferFrame from outFramesBeforeAllFramesGenerationListTxTC
     */
    std::optional<TransferFrameTC> frontFrameBeforeAllFramesGenerationTxTC();

    /**
     * @return The buffer unprocessedFrameLIstBufferTxTc
     */
    const etl::list<TransferFrameTC*, MaxReceivedUnprocessedTxTcInVirtBuffer>& getUnprocessedFramesListBuffer(uint16_t vid);

    //     - MAP/VC Packet Processing and Frame Initialization
    /**
     * Auxiliary function to implement the segmentation of packets stored in
     * the packet buffer
     *
     * @param prevFrame              Half full frame waiting in the master channel (nullptr if it does
     *                              not exist or is full)
     * @param maxTransferFrameDataFieldLength   The max length the data field of the transfer frame is allowed to take
     *                                          (segment header included)
     * @param packetLength                   The length of the next transfer frame data in the packetBufferTxTM
     * @param vid                            Virtual Channel ID
     * @param mapid                          MAP Channel ID. This is ignored if the virtual channel does not contain MAP channels
     *                                      (segmentHeaderPresent = false) or if the service type is BC
     * @param serviceType                    Type AD or BC frames
     * @return A Service Channel Notification
     */
    ServiceChannelNotification segmentationTC(TransferFrameTC* prevFrame, uint16_t maxTransferFrameDataFieldLength, uint16_t packetLength,
                                              uint8_t vid, uint8_t mapid, ServiceType serviceType);

    /**
     * Auxiliary function for blocking of packets stored in the stored packet buffer
     *
     * @param prevFrame                      Half full frame waiting in the master channel (nullptr if it does
     *                                       not exist or is full)
     * @param maxTransferFrameDataFieldLength   The max length the data field of the transfer frame is allowed to take
     *                                          (segment header included)
     * @param packetLength                   The length of the next packet in the stored TC packet buffer
     * @param vcid                           Virtual Channel ID
     * @param mapid                          MAP Channel ID. This is ignored if the virtual channel does not contain MAP channels
     *                                        (segmentHeaderPresent = false) or if the service type is BC
     * @param serviceType                    Type AD or BC frames
     * @return                               A Service Channel Notification
     */
    ServiceChannelNotification blockingTC(TransferFrameTC* prevFrame, uint16_t maxTransferFrameDataFieldLength, uint16_t packetLength,
                                          uint8_t vid, uint8_t mapid, ServiceType serviceType);


    /**
     * Method that stores a packet pointer and the packet to the relevant buffers. Serves as an entry point for upper layers.
     * queues
     *
     * @param packet        Pointer to the packet
     * @param packetLength  Length of the packet
     * @param vid           Virtual channel id
     * @param mapid         MAP channel id. This is ignored if the virtual channel does not contain MAP channels
     *                      (segmentHeaderPresent = false) or if the service type is BC
     * @param serviceType  Service Type
     */
    ServiceChannelNotification storePacketTxTC(uint8_t *packet, uint16_t packetLength, uint8_t vid, uint8_t mapid,
                                               ServiceType serviceType);

    /**
     * Requests to process the last packet stored in the buffer of the specific MAP/VC channel
     * (possible more if blocking is enabled). The packets are segmented or blocked together
     * from data of the memory pool, a transfer frame is created with some primary header fields initialized
     * (as well as the segment header if that is required) and then transferred to the buffer of the virtual channel.
     * This method implements the following protocol functions (@see TC Space Data Link Protocol):
     * a) MAP Packet Processing (@see p. 4.3.1) and VC Packet Processing (@see p. 4.3.4)
     * b) The Frame Initialization Procedure (@see p. 4.3.5.2) of the Virtual Channel Generation function (@see p. 4.3.5)
     *
     * @param vid           Virtual Channel ID
     * @param mapid         MAP channel id. This is ignored if the virtual channel does not contain MAP channels
     *                      (segmentHeaderPresent == false) or if the service type is BC
     * @param maxTransferFrameDataFieldLength   The max length the data field of the transfer frame is allowed to take
     *                                          (segment header included)
     * @param serviceType               Service type of resulting frame. Only packets from the respective service will
     *                                  be grouped together
     */
    ServiceChannelNotification packetProcessingRequestTxTC(uint8_t vid, uint8_t mapid, uint8_t maxTransferFrameDataFieldLength,
                                                           ServiceType serviceType);

    //     - Virtual Channel Generation
    /**
     * The  Virtual  Channel  Generation  Function  shall  perform  the  following  two
     * procedures in the following order:
     * 		a) the  Frame  Operation  Procedure  (FOP),  which  is  a  sub-procedure  of  the
     * 		Communications Operation Procedure (COP); and
     * 		b) the Frame Generation Procedure in this order.
     * @see p. 4.3.5 from TC Space Data Link Protocol
     */
    ServiceChannelNotification vcGenerationRequestTxTC(uint8_t vid);


    //         -- FOP Directives

    ServiceChannelNotification frameTransmission(uint8_t* frameTarget);

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
     *
     * @param vid Virtual Channel ID
     */
    uint8_t transmitterFrameSeqNumber(uint8_t vid) const;

    /**
     * Returns the expected acknowledgement frame sequence number, NN(R). This is essentially the frame sequence
     * number of the oldest unacknowledged frame
     *
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
    ServiceChannelNotification allFramesGenerationRequestTxTC();

    // TC TransferFrame - Receiving End (TC Rx)

    //     - Utility and Debugging
    /**
     * Read first TC transfer frame of the TC MAP channel buffer (unprocessedFrameListBufferTC)
     */
    std::pair<ServiceChannelNotification, const TransferFrameTC*> txOutFrameTC(uint8_t vid, uint8_t mapid) const;

    /**
	 * Available number of outcoming TC RX transfer frames in master channel buffer
	 */
    uint16_t availableFramesBeforeAllFramesReceptionRxTC() const {
        // The commented out buffer is probably redundant
        // return masterChannel.toBeTransmittedFramesAfterAllFramesReceptionListRxTC.available();
        return masterChannel.inFramesBeforeAllFramesReceptionListRxTC.available();
    }

    /**
     * Available space for TC transfer frames at waitQueueRxTC buffer
     */
    uint16_t getAvailableWaitQueueRxTC(uint8_t vid) const {
        if (masterChannel.virtualChannels.find(vid) == masterChannel.virtualChannels.end()) {
            ccsdsLogNotice(Tx, TypeServiceChannelNotif, INVALID_VC_ID);
            return ServiceChannelNotification::INVALID_VC_ID;
        }
        return masterChannel.virtualChannels.at(vid).waitQueueRxTC.available();
    }

    /**
     * Available space for TC transfer frames waiting to be processed from the VC Generation Service
     */
    uint16_t getAvailableInFramesAfterVCReceptionRxTC(uint8_t vid) const {
        if (masterChannel.virtualChannels.find(vid) == masterChannel.virtualChannels.end()) {
            ccsdsLogNotice(Tx, TypeServiceChannelNotif, INVALID_VC_ID);
            return ServiceChannelNotification::INVALID_VC_ID;
        }
        return masterChannel.virtualChannels.at(vid).inFramesAfterVCReceptionRxTC.available();
    }

    /**
     * Available space for TC transfer frames at inFramesAfterVCReceptionRxTC buffer
     */
    uint16_t getAvailableInFramesAfterVCReceptionRxTC(uint8_t vid, uint8_t mapid) const {
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
        return masterChannel.virtualChannels.at(vid).mapChannels.at(mapid).inFramesAfterVCReceptionRxTC.available();
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
     * This service is used for storing incoming TC transfer frames in the master channel.
     *
     * @param frameData Raw transfer frame data
     * @param frameLength The length of the transfer frame
     */
    ServiceChannelNotification storeFrameRxTC(uint8_t* frameData, uint16_t frameLength);

    /**
     * The  All  Frames  Generation  Function  shall  be  used  to  perform  error  control
     * encoding defined by this Recommendation and to deliver Transfer Frames at an appropriate
     * rate to the Channel Coding Sublayer.
     * @see p. 4.2.7 from TC Space Data Link Protocol
     */
    ServiceChannelNotification allFramesReceptionRequestRxTC();


    //     - Master Channel Demultiplexing

    //     - Virtual Channel Reception
    /**
     * The Virtual Channel Reception Function shall perform the Frame Acceptance and
     * Reporting Mechanism (FARM), which is a sub-procedure of the Communications Operation
     * Procedure (COP).
     * @see  p. 4.4.5 from TC Space Data Link Protocol
     */
    ServiceChannelNotification vcReceptionRxTC(uint8_t vid);

    //         -- Farm Directives
    /**
     * @brief A function that generates a CLCW and stores it to a clcw buffer.
     */
    ServiceChannelNotification clcwReportTime(uint8_t vid);

    //     - Virtual Channel Extraction
    /**
     * The VC Packet Extraction Function shall be used to extract variable-length
     * Packets from Frame Data Units on a Virtual Channel in case a segmentation header
     * isn't used.
     * @see 4.4.1 from TC Data Link Protocol
     *
     * @param vid Virtual channel ID
     * @param packetTarget Provided packetTarget data destination
     *
     * @warning This function assumes that the transfer frame data size is checked and correct
     */
    ServiceChannelNotification packetExtractionTC(uint8_t vid, uint8_t* packetTarget);

    //     - MAP Packet Extraction
/**
	 * The MAP Packet Extraction Function shall be used to extract variable-length
	 * Packets from Frame Data Units on a MAP Channel.
	 * @see 4.4.1 from TC Data Link Protocol
	 *
	 * @param vid Virtual channel ID
	 * @param mapid MAP channel ID
	 * @param packet Provided packet data destination
	 */
    ServiceChannelNotification packetExtractionTC(uint8_t vid, uint8_t mapid, uint8_t* packet);


    // TM TransferFrame - Sending End (TM Tx)

    //     - Utility and Debugging
    uint16_t availableFramesAfterVcGenerationTxTM() const {
        return masterChannel.masterCopyTxTM.available();
    }
    /**
     * Returns the available space in the packetLengthBufferTxTM buffer
     */
    uint16_t availablePacketLengthBufferTxTM(uint8_t gvcid);

    /**
     * Returns the available space in the packetBufferTxTM buffer
     */
    uint16_t availablePacketBufferTxTM(uint8_t gvcid);

    /**
     * Return the last stored TM transfer frame from masterCopyTxTM
     */
    std::pair<ServiceChannelNotification, const TransferFrameTM*> backFrameAfterVcGenerationTxTM() const;

    /**
	 * Return the last processed transfer frame from all frames generation
	 */
    std::pair<ServiceChannelNotification, const TransferFrameTM*> frontFrameAfterAllFramesGenerationTxTM() const;

    /**
     * Fetch packet in the top of the MC buffer
     */
    const TransferFrameTM* frontFrameAfterVcGenerationTxTM() const {
        return masterChannel.framesAfterVcGenerationServiceTxTM.front();
    }

    //     - Packet Processing and Virtual Channel Generation

    /**
     * Serves as the main entry point from the upper layers.
     * Stores the raw packets  along with their length so they can be later inserted into transfer frames,
     * and be transmitted.
     *
     * @param packet pointer to the packet
     * @param packetLength length of the packet
     * @param vid the virtual channel id
     */
    ServiceChannelNotification storePacketTxTM(uint8_t* packet, uint16_t packetLength, uint8_t vid);

    /**
     * Function used by the vcGenerationServiceTxTM function to implement the segmentation of packets stored in
     * packetBufferTxTM
     *
     * @param prevFrame                    Half full frame waiting in the master channel (nullptr if it does
     *                                     not exist or is full)
     * @param transferFrameDataFieldLength The length of the data field of the TM Transfer frame, taken by the
     *                                     vcGenerationServiceTxTM parameter
     * @param packetLength                 The length of the next transfer frame data in the packetBufferTxTM
     * @return                             A Service Channel Notification as it is the case with vcGenerationServiceTxTM
     */
    ServiceChannelNotification segmentationTM(TransferFrameTM* prevFrame, uint16_t transferFrameDataFieldLength,
                                              uint16_t packetLength,uint8_t gvcid);

    /**
     * Auxiliary function for blocking of packets stored in the stored packet buffer
     *
     * @param prevFrame                      Half full frame waiting in the master channel (nullptr if it does
     *                                       not exist or is full)
     * @param transferFrameFieldLength       The length of the data field of the TM Transfer frame (where packets are
     *                                       stored)
     * @param packetLength                   The length of the next packet in the stored TM packet buffer
     * @param vcid                           Virtual Channel ID
     * @return                               A Service Channel Notification
     */
    ServiceChannelNotification blockingTM(TransferFrameTM* prevFrame, uint16_t transferFrameDataFieldLength, uint16_t packetLength, uint8_t vid);

    /**
     * Service that generates a transfer frame by combining the packets via blocking and segmentation and initializing
     * the transfer frame primary header @see p. 4.2.2 and 4.2.3 of TM Space Data Link protocol
     *
     * @param transferFrameDataFieldLength the maximum transfer frame data field length
     * @param gvcid the global virtual channel id
     * @return PACKET_BUFFER_EMPTY Alert if the virtual channel packet buffer is empty
     * NO_TX_PACKETS_TO_TRANSFER_FRAME Alert if no packets from the packet buffer can be stored to the transfer frame
     * NO_SERVICE_EVENT Alert if the packets are stored as expected to the transfer frame
     */
    ServiceChannelNotification vcGenerationServiceTxTM(uint16_t transferFrameDataFieldLength, uint8_t vid);

    //     - Master Channel Generation
    /**
     * The Master Channel Generation Service shall be used to insert Transfer Frame
     * Secondary Header and/or Operational Control Field service data units into Transfer Frames
     * of a Master Channel.
     * @see p. 4.2.5 from TM Space Data Link Protocol (CCSDS 132.0-B-3)
     */
    ServiceChannelNotification mcGenerationRequestTxTM();

    //     - All Frames Generation
    /**
     * The  All  Frames  Generation  Function  shall  be  used  to  perform  error  control
     * encoding defined by this Recommendation and to deliver Transfer Frames at an appropriate
     * rate to the Channel Coding Sublayer.
     * @see p. 4.2.7 from TM Space Data Link Protocol
     */
    ServiceChannelNotification allFramesGenerationRequestTxTM(uint8_t* frameDataTarget,
                                                              uint16_t frameLength = TmTransferFrameSize);


    // TM TransferFrame - Receiving End (TM Rx)

    //     - Utility and Debugging
    /**
     * Available number of incoming TM transfer frames in virtual channel buffer
     */
    uint16_t availableFramesVcCopyRxTM(uint8_t vid) const {
        if (masterChannel.virtualChannels.find(vid) == masterChannel.virtualChannels.end()) {
            ccsdsLogNotice(Tx, TypeServiceChannelNotif, INVALID_VC_ID);
            return ServiceChannelNotification::INVALID_VC_ID;
        }
        return masterChannel.virtualChannels.at(vid).availableFramesVcCopyRxTM();
    }

    uint16_t availableFramesMcCopyRxTM() const {
        return masterChannel.masterCopyRxTM.available();
    }

    uint8_t getFrameCountTM(uint8_t vid);

    uint8_t getFrameCountTM();

    //     - All Frames Reception
    /**
     * The  All  Frames  Reception  Function  shall  be  used  to  perform  error  control
     * encoding defined by this Recommendation and to deliver Transfer Frames at an appropriate
     * rate to the Channel Coding Sublayer. Also writes the received transfer frame data to the provided pointer.
     * @see p. 4.3.7 from TM Space Data Link Protocol (CCSDS 132.0-B-3)
     */
    ServiceChannelNotification allFramesReceptionRequestRxTM(uint8_t* frameData, uint16_t frameLength);

    //     - Virtual Channel Reception

    //     - Packet Extraction
    /**
     * This service is used for extracting RX TM packets. It signals the end of the TM Rx chain
     *
     * @param vid           Virtual Channel ID that determines from which vid buffer the frame is processed
     * @param packetTarget A pointer to the packet buffer. The user has to pre-allocate the correct size for the buffer
     */
    ServiceChannelNotification packetExtractionRxTM(uint8_t vid, uint8_t* packetTarget);

    // Not sure about the purpose of that one
	/**
	 * Processes the packet at the head of the buffer
	 */
	void process();


	// This is honestly a bit confusing
	ServiceChannel(const MasterChannel& masterChannel, const PhysicalChannel& physicalChannel)
	    : masterChannel(masterChannel), physicalChannel(physicalChannel) {}
	//Default constructor
	ServiceChannel() : masterChannel(), physicalChannel(){};
};