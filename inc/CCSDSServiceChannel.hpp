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
			PhysicalChannel& physicalChannel,
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
         * Serves as the main entry point from the upper layers, by storing
         * raw packets along with their length so they can be later inserted into transfer frames,
         * and transmitted.
         *
         * @param packetSource pointer to the packet
         * @param packetLength length of the packet
	     */
	    static etl::expected<void, ServiceChannelNotification> storePacketTM(
	    	VirtualChannelSpaceSegmentVariant &vcChanVariant,
	    	const uint8_t *packetSource,
	    	uint16_t packetLength);

	private:
		/**
	     * @brief Auxiliary function for blocking of packets stored in packet queue
	     *
	     * @param finishedOperationsFlag Signifies to vcGeneration that no more processing can take place and that it
	     *                               should exit.
	     * @param segmentationData If a packet is too large to fit in a frame, the frame pointer and the packet
	     *                         length is returned to this parameter, so that segmentation may handle this case.
	     */
		static etl::expected<void, ServiceChannelNotification> blockingTM(
			const PhysicalChannel &physicalChannel,
			MasterChannelSpaceSegmentVariant &mcChanVariant,
			VirtualChannelSpaceSegmentVariant &vcChanVariant,
			bool &finishedOperationsFlag,
			etl::optional<etl::pair<TransferFrameTM *, uint16_t> > &segmentationData
		);

	    /**
         * @brief Auxiliary function for segmentation of packets stored in packet queue
         *
         * @param frameTm      Pointer to half full frame given by blockingTC.
         * @param packetLength The length of the packet that is too large to fit in the frame.
	     */
		static etl::expected<void, ServiceChannelNotification>
		segmentationTM(
			const PhysicalChannel &physicalChannel,
			MasterChannelSpaceSegmentVariant &mcChanVariant,
			VirtualChannelSpaceSegmentVariant &vcChanVariant,
			TransferFrameTM *frameTm,
			uint16_t packetLength
		);

	    /**
         * @brief Auxiliary function that generates a space packet and pushes it to the corresponding
         * virtual channel queue.
         *
         * @param remainingDataFieldSpace This parameter determines the length of the generated packet. If it is
         *                                greater or equal than the minimum space packet length (
         *                                SpacePacketPrimaryHeaderLength + 1), then the resulting
         *                                length is remainingDataFieldSpace. Otherwise, the minimum length packet
         *                                will be created, which will need to be segmented across two frames.
         *                                @see	p. 4.2.2.5 from CCSDS TM SPACE DATA LINK PROTOCOL
         *
	     */
		static void generateIdleSpacePacket(VirtualChannelSpaceSegmentVariant &vChanVariant,
								uint16_t remainingDataFieldSpace);

	public:
	    /**
         * Service that generates a transfer frame by combining packets via blocking and segmentation and initializing
         * the transfer frame primary header @see p. 4.2.2 and 4.2.3 of TM Space Data Link protocol.
         *
         * @return void, when if there were packets to process and all the necessary frames could be created.
         *         INVALID_CHANNELS_COMBINATION, if an invalid channel hierarchy is used as input.
         *         PACKET_QUEUE_EMPTY, if there were no packets to process. An idle frame is generated.
         *         NOT_ENOUGH_SPACE_IN_MASTER_COPY_OR_MEMORY_POOL or FRAME_LIST_FULL in case of insufficient space.
         *
	     */
		static etl::expected<void, ServiceChannelNotification> vcGenerationServiceTM(
			const PhysicalChannel &physicalChannel,
			MasterChannelSpaceSegmentVariant &mcChanVariant,
			VirtualChannelSpaceSegmentVariant &vcChanVariant
		);

		// Virtual channel multiplexing
	private:
		/**
	     * @brief In the scenario that there frames for the multiplexer to process, OID frames shall be generated to
	     *        keep the frame rate constant.
	     * @details OID (Only Idle Data) frames do not carry any information, with its data field being filled with
	     *          pseudorandom noise
	     * @param vChanVariant OID frames will appear as being generated by this virtual channel.
	     */
		static etl::expected<void, ServiceChannelNotification> generateOidFrame(
			const PhysicalChannel &physicalChannel,
			MasterChannelSpaceSegmentVariant &mcChanVariant,
			VirtualChannelSpaceSegmentVariant &vChanVariant);

	public:
		/**
		 * @brief This service is responsible extracting frames pointers from the virtual channel
		 *        instances and moving them to the appropriate master channel.
		 *
		 * @details  The multiplexer will pop one frame per virtual channel. The popping order is defined
		 *           by the order the virtual channels where placed in the span. In the absence
	     *           of frames, an OID (only idle data) frame will be generated. The user can choose
	     *           which virtual channel this OID frame will "belong" to, but it is recommended
	     *           to use one where the operational control field is present, to ensure that CLCW
	     *           report rate is not influenced by the TM frame rate.
	     *
	     * @note In case a given virtual channel (in the span or for OID frame generation) does not belong
	     *       to the given master channel, it will be ignored, but frames from valid
	     *       virtual channels will still get multiplexed.
	     *
		 * @param vcChanVariants The virtual channels whose frames will be multiplexed in the master
		 *                       channel.
		 *
		 */
		static etl::expected<void, ServiceChannelNotification> virtualChannelMultiplexerTM(
			PhysicalChannel& physical_channel,
			MasterChannelSpaceSegmentVariant mcChanVariant,
			etl::span<VirtualChannelSpaceSegmentVariant&>& vcChanVariants,
			VirtualChannelSpaceSegmentVariant& vcChanVariantForOidGeneration);

	    // Master Channel Generation
	    /**
         * @brief The Master Channel Generation Service shall be used to insert Transfer Frame
         * Secondary Header and/or Operational Control Field service data units into Transfer Frames
         * of a Master Channel.
         *
         * @param clcwContainers The clcw containers of every virtual channel that belongs to the given
         *                       master channel.
         * @note Should there be no clcw available during the call of this function, frames with an operational
         *       control field will be marked as 'under processing', and operations with
         *
         * @see p. 4.2.5 from TM Space Data Link Protocol (CCSDS 132.0-B-3)
	     */
	    static etl::expected<void, ServiceChannelNotification> mcGenerationRequestTM(
	    	MasterChannelSpaceSegmentVariant &mcChanVariant,
	    	etl::span<etl::optional<CLCW>>& clcwContainers);

	    // All Frames Generation
	    /**
         * The  All  Frames  Generation  Function  shall  be  used  to  perform  error  control
         * encoding defined by this Recommendation and to deliver Transfer Frames at an appropriate
         * rate to the Channel Coding Sublayer.
         *
         * @param frameDestination User provided buffer to copy the fully processed transfer frame.
         * @see p. 4.2.7 from TM Space Data Link Protocol
         *  TODO do not forget to have a mechanism for sending frames at an appropriate rate (unless lower layers can handle it by transmitting empty codewords)
         *  TODO consider a no-copy approach where a pointer to the frame data is returned and then the user has to delete it
         *       explicitly for the memory pool and master copy buffer (if copying proves to be a bottleneck)
	     */
	    static etl::expected<void, ServiceChannelNotification> allFramesGenerationRequestTM(
	    	PhysicalChannel& physicalChannel,
	    	MasterChannelSpaceSegmentVariant& mcChanVariant,
	    	uint8_t *frameDestination);
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