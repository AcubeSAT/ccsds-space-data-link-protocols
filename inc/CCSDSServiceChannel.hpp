/**
 * @file CCSDSServiceChannel.hpp
 * @details  This provides a way to interconnect all different CCSDS Space Data Protocol Services and a
 *           bidirectional interface between the receiving and transmitting parties.
 */

#pragma once

#include <utility>
#include "etl/optional.h"
#include "etl/expected.h"
#include "etl/utility.h"
#include "CCSDSChannel.hpp"
#include "Alert.hpp"
#include "TransferFrameTC.hpp"
#include "CCSDSLoggerImpl.h"
#include "CCSDSSecurityAssociation.hpp"

namespace CCSDSDataLinkLayer {
	class BaseServiceChannel {
	public:
	    explicit BaseServiceChannel(const PhysicalChannel& physicalChannel, const SecurityAssociation& securityAssociation)
	        : physicalChannel(physicalChannel), securityAssociation(securityAssociation) {}

	    // Debugging services
	    /**
         * Auxiliary service that accepts TM transfer frames and print their fields. Offered for debugging puproses
         * @param verbosePrimaryHeader, verboseOCF     If true, subfield names will also appear for each field, but more space is taken
         * @param vid                                 Used to detect the existence of certain fields, that depend on certain virtual/MAP channel flags
	     */
	    void printTransferFrameTM(TransferFrameTM &TransferFrameTM, bool verbosePrimaryHeader, bool verboseOCF,
	                                      uint8_t vid,
	                                      uint16_t transferFrameDataFieldLength);

	    /**
         * Auxiliary service that accepts TM transfer frames and print their fields. Offered for debugging puproses
         * @param verbose     If true, names will also be printed for each field
         * @param vid, mapid  Used to detect the existence of certain fields, that depend on certain virtual/MAP channel flags
         *                    mapid will be ignored if MAP channels do not exist in the given virtual channel
	     */
	    void printTransferFrameTC(TransferFrameTC &TransferFrameTC, bool verbosePrimaryHeader, uint8_t vid,
	                                      uint8_t mapid, uint16_t transferFrameDataFieldLength);


	    // Other services
	    /**
         * Reset anti replay attack sequence numbers, used for frame authentication by the security association (SA).
         * A reset may be performed for the following reasons:
         * - Testing of the SA
         * - Sequence number overflow
         * - Sender-Receiver sequence number difference exceeded sequenceNumberWindow
	     */
	    void resetSequenceCountersSA() {
		    securityAssociation.resetSequenceNumber();
	    }

	protected:
	    /**
         * @brief PhysicalChannel is used to simply represent parameters of the physical channel like the maximum frame
         * length.
         * TODO: Replace defines for maxFrameLength
	     */
	    PhysicalChannel physicalChannel;

	    /**
		 * @brief The security association is used for providing authentication and enryption services.
	     */
	    SecurityAssociation securityAssociation;
	};

#ifdef SPACE_SEGMENT
class ServiceChannelSpaceSegment : public BaseServiceChannel {
	public:
	    ServiceChannelSpaceSegment(const PhysicalChannel& physicalChannel,
	                               const MasterChannelSpaceSegment& masterChannelSpaceSegment,
	                               const SecurityAssociation& securityAssociation)
	        : BaseServiceChannel(physicalChannel, securityAssociation),
	          masterChannel(masterChannelSpaceSegment) {}

	    // TC TransferFrame - Receiving End (TC Rx)

	    //     - Utility and Debugging

	    /**
         * Returns the total length of a space packet (as defined in CCSDS Space Packet Protocol)
	     */
	    static uint16_t getSpacePacketLength(const uint8_t *packetSource) {
		    // plus one is added because the field actually contains the data field length, reduced by one
		    return (static_cast<uint16_t>(packetSource[PacketDataLengthFieldPosition - 1]) << 8) |
		           (static_cast<uint16_t>(packetSource[PacketDataLengthFieldPosition])) + PacketPrimaryHeaderLength + 1;
	    }

	    //     - All Frames Reception
	    /**
         * The  All  Frames  Generation  Function  shall  be  used  to  perform  error  control
         * encoding defined by this Recommendation, along with other standard checks. Serves as an entry point
         * for frames.
         * @see p. 4.2.7 from TC Space Data Link Protocol
	     */
	    etl::expected<void, ServiceChannelNotification> allFramesReceptionRequestRxTC(uint8_t *frameData, uint16_t frameLength);

	    //     - Master Channel Demultiplexing

	    //     - Virtual Channel Reception
	    /**
         * The Virtual Channel Reception Function shall perform the Frame Acceptance and
         * Reporting Mechanism (FARM), which is a sub-procedure of the Communications Operation
         * Procedure (COP).
         * @see  p. 4.4.5 from TC Space Data Link Protocol
	     */
	    etl::pair<ServiceChannelNotification, uint8_t> vcReceptionRxTC(uint8_t vid);

	    //     - FARM-1 utility and debugging functions
	    /**
         * Directly inject a clcw to the corresponding master channel data structure (this process is normally handled
         * inside FARM-1). Offered for situations where the TM-Tx chain needs to be tested, but the TC-Rx chain is not available.
	     */
	    etl::expected<void, ServiceChannelNotification> injectClcw(uint32_t clcw, uint8_t vid);

	    //    - SDLS Processing
	    /**
         * Processes TC frames that belong in a security association and discards them if they do not pass checks.
         * @param mapid Is ignored if no MAP channels exist for the given virtual channel
	     */
	    etl::expected<void, ServiceChannelNotification> processSDLSSecurityRxTC(uint8_t vid, uint8_t mapid, ServiceType serviceType);

	    //     - Packet Extraction
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

	    // TM TransferFrame - Sending End (TM Tx)

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
	    etl::expected<void, ServiceChannelNotification> storePacketTxTM(uint8_t *packet, uint16_t packetLength, uint8_t vid);

	    /**
         * Function used by the vcGenerationServiceTxTM function to implement the segmentation of packets stored in
         * packetBufferTxTM
         *
         * @param prevFrame                    Half full frame waiting in the master channel (nullptr if it does
         *                                     not exist or is full)
         * @param transferFrameDataFieldLength The length of the data field of the TM Transfer frame, taken by the
         *                                     vcGenerationServiceTxTM parameter
         * @param packetLength                 The length of the next transfer frame data in the packetBufferTxTM
         * @param idlePacketFlag                 Indicates whether the next packet is an idle space packet or not
         * @return                             A Service Channel Notification as it is the case with vcGenerationServiceTxTM
	     */
	    etl::expected<void, ServiceChannelNotification> segmentationTM(TransferFrameTM *prevFrame, uint16_t transferFrameDataFieldLength,
	                                                                   uint16_t packetLength, uint8_t vid);

	    /**
         * Auxiliary function for blocking of packets stored in the stored packet buffer
         *
         * @param prevFrame                      Half full frame waiting in the master channel (nullptr if it does
         *                                       not exist or is full)
         * @param transferFrameFieldLength       The length of the data field of the TM Transfer frame (where packets are
         *                                       stored)
         * @param packetLength                   The length of the next packet in the stored TM packet buffer
         * @param vcid                           Virtual Channel ID
         * @param idlePacketFlag                 Indicates whether the next packet is an idle space packet or not
         * @return                               A Service Channel Notification
	     */
	    etl::expected<void, ServiceChannelNotification> blockingTM(TransferFrameTM *prevFrame, uint16_t transferFrameDataFieldLength,
	                                                               uint16_t packetLength, uint8_t vid);

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

	    //     - Master Channel Generation
	    /**
         * The Master Channel Generation Service shall be used to insert Transfer Frame
         * Secondary Header and/or Operational Control Field service data units into Transfer Frames
         * of a Master Channel.
         * @see p. 4.2.5 from TM Space Data Link Protocol (CCSDS 132.0-B-3)
	     */
	    etl::expected<void, ServiceChannelNotification> mcGenerationRequestTxTM();

	    //     - All Frames Generation
	    /**
         * The  All  Frames  Generation  Function  shall  be  used  to  perform  error  control
         * encoding defined by this Recommendation and to deliver Transfer Frames at an appropriate
         * rate to the Channel Coding Sublayer.
         * @see p. 4.2.7 from TM Space Data Link Protocol
         * @TODO do not forget to have a mechanism for sending frames at an appropriate rate (unless lower layers can handle it by transmitting empty codewords)
	     */
	    etl::expected<void, ServiceChannelNotification> allFramesGenerationRequestTxTM(uint8_t *frameDataTarget);

	private:
	    /**
         * @brief The Master Channel essentially stores the configuration of your channel. It partitions the physical
         * channel into virtual channels, each of which has different parameters in order to easily manage incoming traffic.
	     */
	    MasterChannelSpaceSegment masterChannel;

#ifdef ENABLE_BUFFER_ACCESS
	public:
	    MasterChannelSpaceSegment& getMasterChannel() {
		    return masterChannel;
	    }
#endif // ENABLE_BUFFER_ACCESS
	};
#endif // SPACE_SEGMENT

#ifdef GROUND_SEGMENT
    class ServiceChannelGroundSegment :public BaseServiceChannel {
	public:
	    ServiceChannelGroundSegment(const PhysicalChannel& physicalChannel,
	                               const MasterChannelGroundSegment& masterChannelGroundSegment,
	                               const SecurityAssociation& securityAssociation)
	        : BaseServiceChannel(physicalChannel, securityAssociation),
	          masterChannel(masterChannelGroundSegment) {}


	    // TC TransferFrame - Sending End (TC Tx)

	    //     - MAP/VC Packet Processing and Frame Initialization
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

	    //    - SDLS Processing
	    /**
         * Apply security services for TC frames
         * @param mapid Is ignored if no MAP channels exist for the given virtual channel
	     */
	    etl::expected<void, ServiceChannelNotification> applySDLSSecurityTxTC(uint8_t vid, uint8_t mapid);

	    //     - Virtual Channel Generation
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


	    //         -- FOP-1 User services and debugging methods

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

	    //     - All frames generation
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

	private:
	    /**
         * @brief The Master Channel essentially stores the configuration of your channel. It partitions the physical
         * channel into virtual channels, each of which has different parameters in order to easily manage incoming traffic.
	     */
	    MasterChannelGroundSegment masterChannel;

#ifdef ENABLE_BUFFER_ACCESS
	public:
	    MasterChannelGroundSegment& getMasterChannel() {
		    return masterChannel;
	    }
#endif // ENABLE_BUFFER_ACCESS
	};
#endif // GROUND_SEGMENT
} // namespace CCSDSDataLinkLayer