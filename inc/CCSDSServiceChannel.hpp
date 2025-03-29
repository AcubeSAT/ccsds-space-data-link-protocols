/**
 * @file CCSDSServiceChannel.hpp
 * @details Provides a collection of data processing functions / services, contained within the "service channels"
 *          Those classes constitute the interface through which the data link user can interact.
 */

#pragma once

#include "etl/optional.h"
#include "etl/expected.h"
#include "etl/utility.h"
#include "CCSDSChannel.hpp"
#include "Alert.hpp"
#include "CCSDSChannelConfiguration.hpp"
#include "CCSDSSecurityAssociation.hpp"
#include "FrameAcceptanceReporting.hpp"

namespace CCSDSDataLinkLayer {
	/**
	 * @brief Contains services common to both space and ground segments.
	 */
	class BaseServiceChannel {
	public:
	    // Debugging services
	    /**
         * @brief Auxiliary service that accepts TM transfer frames and print their fields. Offered for debugging purposes.
         * @param ocfPresent, eccPresent              Indicates to the function whether those fields exist.
         * @param verbosePrimaryHeader, verboseOCF    If true, subfield names will also appear for each field, but more space is taken.
	     */
	    static void printTransferFrameTM(const TransferFrameTM &TransferFrameTM,
	                                     bool ocfPresent,
	                                     bool eccPresent,
	                                     bool verbosePrimaryHeader,
	                                     bool verboseOCF,
	                                     uint16_t transferFrameDataFieldLength);

	    /**
         * @brief Auxiliary service that accepts TM transfer frames and print their fields. Offered for debugging purposes
         * @param segHeaderPresent, eccFieldPresent, verbosePrimaryHeader Indicates to the function whether those fields exist.
         * @param verbosePrimaryHeader  If true, the primary header fields will also be printed.
	     */
	    static void printTransferFrameTC(const TransferFrameTC &TransferFrameTC,
	    	                      const SecurityAssociation& securityAssociation,
	                              bool segHeaderPresent,
	                              bool eccFieldPresent,
	                              bool verbosePrimaryHeader,
	                              uint16_t transferFrameDataFieldLength);


	    // Other services
	    /**
         * @brief Reset anti replay attack sequence numbers, used for frame authentication by the security association (SA).
         * @details A reset may be performed for the following reasons:
         * - Testing of the SA
         * - Sequence number overflow
         * - Sender-Receiver sequence number difference exceeded sequenceNumberWindow
	     */
	    static void resetSequenceCountersSA(SecurityAssociation& securityAssociation) {
		    securityAssociation.resetSequenceNumber();
	    }
	};

#ifdef SPACE_SEGMENT
class ServiceChannelSpaceSegment {
	public:
		/** ================================================
		 *   @name TC TransferFrame - Receiving End (TC Rx)
		 *  ================================================
		 *  @{
		 */

	    // All Frames Reception
	    /**
         * The  All  Frames  Generation  Function  shall  be  used  to  perform  error  control
         * encoding defined by this Recommendation, along with other standard checks. Serves as an entry point
         * for frames.
         * @see p. 4.2.7 from TCstatic  Space Data Link Protocol
	     */
	    static etl::expected<void, ServiceChannelNotification> allFramesReceptionRequestTC(
	    	const PhysicalChannel& physicalChannel,
		    MasterChannelSpaceSegmentVariant& mcChanVariant,
		    VirtualChannelSearchFunctionType vChanSearchFunction,
		    MapChannelSearchFunctionType mapChanSearchFunction,
		    uint8_t *frameData, uint16_t frameLength);

	    // Master Channel Demultiplexing

	    // Virtual Channel Reception
	    /**
         * The Virtual Channel Reception Function shall perform the Frame Acceptance and
         * Reporting Mechanism (FARM), which is a sub-procedure of the Communications Operation
         * Procedure (COP).
         * @see  p. 4.4.5 from TC Space Data Link Protocol
	     */
	    static etl::pair<ServiceChannelNotification, uint8_t> vcReceptionTC(
	    	FrameAcceptanceReporting& farm,
		    MasterChannelSpaceSegmentVariant& mcChanVariant,
		    VirtualChannelSpaceSegmentVariant& vcChanVariant);

	    // SDLS Processing
		/**
	     * @brief Processes TC frames that belong in a security association and discards them if they do not pass checks.
	     * @param mapChanFunction: Used to find the correct map channel in case the frame has a segmentation header present.
		 */
		static etl::expected<void, ServiceChannelNotification> processSDLSSecurityTC(
			SecurityAssociation& securityAssociation,
			MasterChannelSpaceSegmentVariant &mcChanVariant,
			VirtualChannelSpaceSegmentVariant &vcChanVariant,
			MapChannelSearchFunctionType mapChanFunction,
			DefsAndUtils::ServiceType serviceType);

	    // Packet Extraction
	    /**
         * The VC Packet Extraction Function shall be used to extract variable-length
         * Packets from Frame Data Units on a Virtual Channel
         * @see 4.4.1 from TC Data Link Protocol
         *
         * @param vid Virtual channel ID
         * @param mapid MAP channel ID. This parameter is ignored if a segmentation header does not exist for the given virtual channel
         *              or TYPE_BC packets are asked to be extracted (serviceType = TYPE_BC)
         * @param serviceType The frames type packets will be extracted from
         * @param packetDest Provided packetDest data destination
         *
         * @returns A service channel notification and the packet's length. A 'NO_SERVICE_EVENT' indicates that a packet
         *          was successfully copied to the destination buffer, while 'PROCESSING_SEGMENTED_PACKET' means construction
         *          of a segmented packet (between multiple frames) is in process. Every other notification is an error. The
         *          returned packet's length should be considered valid only in case of a 'NO_SERVICE_EVENT'.
         *
         * @note This function assumes that the user's destination buffer is at least as large as MaxPacketSize. Should an
         *       unexpected packet with size larger than MaxPacketSize arrive, the packet copy will be partial (up to
         *       MaxPacketSize), but the user will not be alerted (a 'NO_SERVICE_EVENT' is returned).
	     */
	    etl::expected<uint16_t , ServiceChannelNotification>
	    packetExtractionRxTC(uint8_t vid, uint8_t mapid, ServiceType serviceType, uint8_t *packetDest);

		/**
		 * @}
		 */

		/** ==============================================
		 *   @name TM TransferFrame - Sending End (TM Tx)
		 *  ==============================================
		 *  @{
		 */

	    // Packet Processing and Virtual Channel Generation

	    /**
         * Serves as the main entry point from the upper layers.
         * Stores the raw packets  along with their length so they can be later inserted into transfer frames,
         * and be transmitted.
         *
         * @param packetSource pointer to the packet
         * @param packetLength length of the packet
	     */
	    static etl::expected<void, ServiceChannelNotification> storePacketTM(
	    	VirtualChannelSpaceSegmentVariant &vcChanVariant,
	    	const uint8_t *packetSource,
	    	uint16_t packetLength);

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
		static etl::expected<void, ServiceChannelNotification> segmentationTM(
			MasterChannelSpaceSegmentVariant &mcChanVariant,
			VirtualChannelSpaceSegmentVariant &vcChanVariant,
			TransferFrameTM *prevFrame,
			uint16_t transferFrameDataFieldLength,
			uint16_t packetLength);

		/**
	     * Auxiliary function for blocking of packets stored in the stored packet buffer
	     *
	     * @param prevFrame                      Half full frame waiting in the master channel (nullptr if it does
	     *                                       not exist or is full)
	     * @param transferFrameDataFieldLength   The length of the data field of the TM Transfer frame (where packets are
	     *                                       stored)
	     * @param packetLength                   The length of the next packet in the stored TM packet buffer
	     * @return                               A Service Channel Notification
		 */
	    static etl::expected<void, ServiceChannelNotification> blockingTM(
			MasterChannelSpaceSegmentVariant &mcChanVariant,
			VirtualChannelSpaceSegmentVariant &vcChanVariant,
	    	TransferFrameTM *prevFrame,
	    	uint16_t transferFrameDataFieldLength,
	    	uint16_t packetLength);

	    /**
         * Auxiliary function for generating idle space packets in the scenario that there are not enough packets to
         * fill the transfer frame data field, or blocking and segmentation permissions do not allow for their placement.
         * @see Space Packet Protocol for details on the idle space packet
         *
         * @param vid                   Virtual channel ID
         * @param lastPacketPlacedIdle  An indicator on whether the last packet placed in a frame was idle.
         * @return                      A service channel notification and an indication on whether an idle packet was generated or not
	     */
	    etl::expected<bool, ServiceChannelNotification>
	    generateIdleSpacePacket(uint8_t vid, TransferFrameTM *lastProcessedFrame, uint16_t transferFrameDataFieldLength,
	                            bool lastPacketPlacedIdle);

	    /**
         * Service that generates a transfer frame by combining packets via blocking and segmentation and initializing
         * the transfer frame primary header @see p. 4.2.2 and 4.2.3 of TM Space Data Link protocol. If the virtual channel
         * supports the presence of the operational control field in frames, it is filled with a clcw. In case there is no clcw
         * to be placed to a newly generated frame, it is withheld until one becomes available.
         *
         * @param transferFrameDataFieldLength the transfer frame data field length
         * @param gvcid the global virtual channel id
         * @return PACKET_BUFFER_EMPTY Alert if the virtual channel packet buffer is empty
         * NO_TX_PACKETS_TO_TRANSFER_FRAME Alert if no packets from the packet buffer can be stored to the transfer frame
         * NO_SERVICE_EVENT Alert if the packets are stored as expected to the transfer frame
	     */
	    etl::expected<void, ServiceChannelNotification> vcGenerationServiceTxTM(uint16_t transferFrameDataFieldLength, uint8_t vid);

	    // Master Channel Generation
	    /**
         * The Master Channel Generation Service shall be used to insert Transfer Frame
         * Secondary Header and/or Operational Control Field service data units into Transfer Frames
         * of a Master Channel.
         * @see p. 4.2.5 from TM Space Data Link Protocol (CCSDS 132.0-B-3)
	     */
	    etl::expected<void, ServiceChannelNotification> mcGenerationRequestTxTM();

	    // All Frames Generation
	    /**
         * The  All  Frames  Generation  Function  shall  be  used  to  perform  error  control
         * encoding defined by this Recommendation and to deliver Transfer Frames at an appropriate
         * rate to the Channel Coding Sublayer.
         * @see p. 4.2.7 from TM Space Data Link Protocol
         * @TODO do not forget to have a mechanism for sending frames at an appropriate rate (unless lower layers can handle it by transmitting empty codewords)
	     */
	    etl::expected<void, ServiceChannelNotification> allFramesGenerationRequestTxTM(uint8_t *frameDataTarget);
	};

	/**
	 * @}
	 */
#endif // SPACE_SEGMENT

#ifdef GROUND_SEGMENT
    class ServiceChannelGroundSegment {
	public:

	    /** ==============================================
	     *   @name TC TransferFrame - Sending End (TC Tx)
	     *  ==============================================
	     *  @{
	     */

	    // MAP/VC Packet Processing and Frame Initialization
	    /**
         * @brief Auxiliary function to implement the segmentation of packets stored in the packet buffer.
         * @param maxTransferFrameDataFieldLength   The max length the data field of the transfer frame is allowed to
         *                                           take (the segment header is included in this length, if it exists).
         * @param packetLength                   The length of the next packet in the packetBufferTxTM.
         * @param vid                            Virtual Channel ID.
         * @param mapid                          MAP Channel ID. This is ignored if the virtual channel does not contain
         *                                       MAP channels (segmentHeaderTCPresent == false) or if the service type
         *                                       is BC.
         * @param serviceType                    Whether the service is of type AD or BC.
         * @return A Service Channel Notification indicating whether an error has occurred.
	     */
	    etl::expected<void, ServiceChannelNotification> segmentationTC(uint16_t maxTransferFrameDataFieldLength, uint16_t packetLength,
	                                                                   uint8_t vid, uint8_t mapid, ServiceType serviceType);

	    /**
         * @brief Auxiliary function to implement the blocking of packets stored in the packet buffer.
         * @param maxTransferFrameDataFieldLength   The max length the data field of the transfer frame is allowed to
         *                                           take (the segment header is included in this length, if it exists).
         * @param packetLength                   The length of the next packet in the packetBufferTxTM.
         * @param vid                            Virtual Channel ID.
         * @param mapid                          MAP Channel ID. This is ignored if the virtual channel does not contain
         *                                       MAP channels (segmentHeaderTCPresent == false) or if the service type
         *                                       is BC.
         * @param serviceType                    Whether the service is of type AD or BC.
         * @return A Service Channel Notification indicating whether an error has occurred.
	     */
	    etl::expected<void, ServiceChannelNotification> blockingTC(uint16_t maxTransferFrameDataFieldLength, uint16_t packetLength,
	                                                               uint8_t vid, uint8_t mapid, ServiceType serviceType);

	    /**
         * @brief Stores a packet's bytes and it's length in the appropriate buffers.
         *
         * @param packet        Pointer to the packet source.
         * @param packetLength  Length of the packet.
         * @param vid           Virtual Channel ID.
         * @param mapid         MAP Channel ID. This is ignored if the virtual channel does not contain MAP channels
         *                      (segmentHeaderTCPresent == false) or if the service type is BC.
         * @param serviceType  Type AD, BD frames
	     */
	    etl::expected<void, ServiceChannelNotification> storePacketTxTC(uint8_t *packet, uint16_t packetLength, uint8_t vid,
	                                                                    ServiceType serviceType, etl::optional<uint8_t> mapid = etl::nullopt);

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
         *                      (segmentHeaderTCPresent == false) or if the service type is BC
         * @param maxTransferFrameDataFieldLength   The max length the data field of the transfer frame is allowed to take
         *                                          (segment header included). Note: Should a segment header exist, it's 1
         *                                          octet length is included in this parameter.
         * @param serviceType               Service type of resulting frame. Only packets from the respective service will
         *                                  be grouped together
	     */
	    etl::expected<void, ServiceChannelNotification>
	    packetProcessingRequestTxTC(uint8_t vid, uint8_t mapid, uint8_t maxTransferFrameDataFieldLength,
	                                ServiceType serviceType);

	    // SDLS Processing
	    /**
         * Apply security services for TC frames
         * @param mapid Is ignored if no MAP channels exist for the given virtual channel
	     */
	    etl::expected<void, ServiceChannelNotification> applySDLSSecurityTxTC(uint8_t vid, uint8_t mapid);

	    // Virtual Channel Generation
	    /**
         * The  Virtual  Channel  Generation  Function  shall  perform  the  following  two
         * procedures in the following order:
         * 		The  Frame  Operation  Procedure  (FOP),  which  is  a  sub-procedure  of  the
         * 		Communications Operation Procedure (COP)
         *
         * @see p. 4.3.5 from TC Space Data Link Protocol
         *
         * @returns - A service channel Notification indicating whether an error occurred within FOP,
         *          or an unexpected value was encountered.
         *          - A struct, which may or may not contain directive notifications, asynchronous notifications
         *            and the event code detected by fop. The last field is offered for diagnostic reasons.
         *          @see p. 4.2 & 4.3 from COP-1 CCSDS
         *
         * @note If an alert is contained within the asynchronous notification, an unrecoverable error occurred within FOP-1,
         *       which demands action from a higher layer. Only Type-BD frame transmission remains undisrupted.
	     */
	    etl::pair<ServiceChannelNotification, FopSignals> vcGenerationRequestTxTC(uint8_t vid);


	    //         - FOP-1 User services and debugging methods

	    etl::expected<void, ServiceChannelNotification>
	    pushDirectiveRequestSignal(uint8_t vid, const DirectiveRequestSignal &directiveRequestSignal);

	    /**
         *  Push a CLCW to FOP-1's single capacity queue for inspection. The old CLCW (if it exists) is overwritten.
	     */
	    etl::expected<void, ServiceChannelNotification> pushClcwToFop(uint8_t vid, CLCW clcw);

	    /**
         * Get FOP State of the virtual channel
	     */
	    [[nodiscard]] FOPState getFopState(uint8_t vid) const;

	    /**
         * Returns the value of the timer that is used to determine the time frame for acknowledging transferred
         * frames
	     */
	    [[nodiscard]] uint16_t getT1Timer(uint8_t vid) const;

	    /**
         * Indicates the width of the sliding window which is used to proceed to the lockout state in case the
         * transfer frame number of the received packet deviates too much from the expected one.
	     */
	    [[nodiscard]] uint8_t getFopSlidingWindowWidth(uint8_t vid) const;

	    /**
         * Returns the timeout action which is to be performed once the maximum transmission limit is reached and
         * the timer has expired.
	     */
	    [[nodiscard]] bool getTimeoutType(uint8_t vid) const;

	    /**
         * Returns the last frame sequence number, V(S), that will be placed in the header of the next transferred
         * packet
         *
         * @param vid Virtual Channel ID
	     */
	    [[nodiscard]] uint8_t getTransmitterFrameSeqNumber(uint8_t vid) const;

	    /**
         * Returns the expected acknowledgement frame sequence number, NN(R). This is essentially the frame sequence
         * number of the oldest unacknowledged frame
         *
         * @param vid Virtual Channel ID
	     */
	    [[nodiscard]] uint8_t getExpectedFrameSeqNumber(uint8_t vid) const;

	    // All frames generation
	    /**
         * The  All  Frames  Generation  Function  shall  be  used  to  perform  error  control
         * encoding defined by this Recommendation and to deliver Transfer Frames at an appropriate
         * rate to the Channel Coding Sublayer.
         * @see p. 4.3.8 from TC Space Data Link Protocol
         * @param frameTarget: Location to copy the frame to. The buffer should be at least as large as the maximum
         *  TC transfer frame length
         *
         * @returns The number of octets copied (since TC transfer frames have variable length)
	     */
	    etl::expected<uint16_t, ServiceChannelNotification> allFramesGenerationRequestTxTC(uint8_t *frameTarget);

    	/**
    	 * @}
    	 */
	    // TM TransferFrame - Receiving End (TM Rx)
	    //
	    //        //     - Utility and Debugging
	    //
	    //        [[nodiscard]] uint8_t getVirtualChannelFrameCountTM(uint8_t vid);
	    //
	    //        [[nodiscard]] uint8_t getMasterChannelFrameCountTM() const;
	    //
	    //        //     - All Frames Reception
	    //        /**
	    //         * The  All  Frames  Reception  Function  shall  be  used  to  perform  error  control
	    //         * encoding defined by this Recommendation and to deliver Transfer Frames at an appropriate
	    //         * rate to the Channel Coding Sublayer. Also writes the received transfer frame data to the provided pointer.
	    //         * @see p. 4.3.7 from TM Space Data Link Protocol (CCSDS 132.0-B-3)
	    //         */
	    //        ServiceChannelNotification allFramesReceptionRequestRxTM(uint8_t *frameData, uint16_t frameLength);
	    //
	    //        //     - Virtual Channel Reception
	    //
	    //        //     - Packet Extraction
	    //        /**
	    //         * This service is used for extracting RX TM packets. It signals the end of the TM Rx chain
	    //         *
	    //         * @param vid           Virtual Channel ID that determines from which vid buffer the frame is processed
	    //         * @param packetTarget A pointer to the packet buffer. The user has to pre-allocate the correct size for the buffer
	    //         */
	    //        ServiceChannelNotification packetExtractionRxTM(uint8_t vid, uint8_t *packetTarget);
	};
#endif // GROUND_SEGMENT
} // namespace CCSDSDataLinkLayer