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
#include "NotificationUtilities/Alert.hpp"
#include "StructureGeneration.hpp"
#include "SecurityAssociation.hpp"
#include "COP1/FrameAcceptanceReporting.hpp"

namespace CCSDSDataLinkLayer {
	/**
	 * @brief Contains services common to both space and ground segments.
	 */
	class BaseServiceChannel {
	public:
	    // Debugging services




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

#ifdef INCLUDE_SPACE_SEGMENT_CODE
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
	    packetExtractionTC(uint8_t vcid, uint8_t mapid, ServiceType serviceType, uint8_t *packetDest);

		/**
		 * @}
		 */

		/** ==============================================
		 *   @name TM TransferFrame - Sending End (TM Tx)
		 *  ==============================================
		 *  @{
		 */



	private:

	public:


		// Virtual channel multiplexing
	private:


	public:

	    // Master Channel Generation


	    // All Frames Generation


	/**
	 * @}
	 */
#endif // INCLUDE_SPACE_SEGMENT_CODE

#ifdef INCLUDE_GROUND_SEGMENT_CODE
    class ServiceChannelGroundSegment {
	public:

	    /** ==============================================
	     *   @name TC TransferFrame - Sending End (TC Tx)
	     *  ==============================================
	     *  @{
	     */
	    // MAP/VC Packet Processing and Frame Initialization

    private:


    public:


	    // SDLS Processing


	    // Virtual Channel Generation



	    //         - FOP-1 User services and debugging methods

	    etl::expected<void, ServiceChannelNotification>
	    pushDirectiveRequestSignal(uint8_t vcid, const DirectiveRequestSignal &directiveRequestSignal);

	    /**
         *  Push a CLCW to FOP-1's single capacity queue for inspection. The old CLCW (if it exists) is overwritten.
	     */
	    etl::expected<void, ServiceChannelNotification> pushClcwToFop(uint8_t vcid, CLCW clcw);

	    /**
         * Get FOP State of the virtual channel
	     */
	    [[nodiscard]] FOPState getFopState(uint8_t vcid) const;

	    /**
         * Returns the value of the timer that is used to determine the time frame for acknowledging transferred
         * frames
	     */
	    [[nodiscard]] uint16_t getT1Timer(uint8_t vcid) const;

	    /**
         * Indicates the width of the sliding window which is used to proceed to the lockout state in case the
         * transfer frame number of the received packet deviates too much from the expected one.
	     */
	    [[nodiscard]] uint8_t getFopSlidingWindowWidth(uint8_t vcid) const;

	    /**
         * Returns the timeout action which is to be performed once the maximum transmission limit is reached and
         * the timer has expired.
	     */
	    [[nodiscard]] bool getTimeoutType(uint8_t vcid) const;

	    /**
         * Returns the last frame sequence number, V(S), that will be placed in the header of the next transferred
         * packet
         *
         * @param vid Virtual Channel ID
	     */
	    [[nodiscard]] uint8_t getTransmitterFrameSeqNumber(uint8_t vcid) const;

	    /**
         * Returns the expected acknowledgement frame sequence number, NN(R). This is essentially the frame sequence
         * number of the oldest unacknowledged frame
         *
         * @param vid Virtual Channel ID
	     */
	    [[nodiscard]] uint8_t getExpectedFrameSeqNumber(uint8_t vcid) const;

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
	    //        [[nodiscard]] uint8_t getVirtualChannelFrameCountTM(uint8_t vcid);
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
	    //         * @param vcid           Virtual Channel ID that determines from which vcid buffer the frame is processed
	    //         * @param packetTarget A pointer to the packet buffer. The user has to pre-allocate the correct size for the buffer
	    //         */
	    //        ServiceChannelNotification packetExtractionRxTM(uint8_t vcid, uint8_t *packetTarget);
	};
#endif // INCLUDE_GROUND_SEGMENT_CODE
} // namespace CCSDSDataLinkLayer