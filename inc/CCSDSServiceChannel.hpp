#ifndef CCSDS_SERVICECHANNEL_HPP
#define CCSDS_SERVICECHANNEL_HPP

#include <CCSDSChannel.hpp>
#include <Alert.hpp>
#include <optional>
#include <TransferFrameTC.hpp>
#include <utility>


/**
 * @brief This provides a way to interconnect all different CCSDS Space Data Protocol Services and provides a
 * bidirectional interface between the receiving and transmitting parties
 */

class ServiceChannel {
private:
	/**
	 * @brief The Master Channel essentially stores the configuration of your channel. It partitions the physical
	 * channel into virtual channels, each of which has different parameters in order to easily manage incoming traffic
	 */
	MasterChannel masterChannel;
	/**
	 * @brief PhysicalChannel is used to simply represent parameters of the physical channel like the maximum frame
	 * length
	 * @TODO: Replace defines for maxFrameLength
	 */
	PhysicalChannel physicalChannel;

	// @todo Think about whether we need to keep the size of the packets in a queue. Technically, we can determine this
	// from the header of the frame. Not sure what makes more sense

	uint8_t packetCountTM;

public:
	// Public methods that are called by the scheduler

    /**
     * @brief Fetch packet in the top of the MC buffer
     */
    const TransferFrameTM*packetMasterChannel() const{
        return masterChannel.txProcessedPacketListBufferTM.front();
    }

    uint16_t availableMcTM() const{
		return masterChannel.txProcessedPacketListBufferTM.available();
	}

	/**
	 * @brief Stores an incoming  TC packet in the ring buffer
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
	 * @brief This service is used for storing incoming TM packets in the master channel. It also includes the
	 * Virtual Channel Generation Service
	 *
	 * @param packet Data of the packet
	 * @param packetLength TransferFrameTC length
	 * @param gvcid Global Virtual Channel ID
	 * @param scid Spacecraft ID
	 */

	ServiceChannelNotification storeTM(uint8_t* packet, uint16_t packetLength, uint8_t gvcid);

	/**
	 * @brief This service is used for storing incoming TM packets in the master channel
	 * @param packet Raw packet data
	 * @param packetLength The length of the packet
	 */
	 
	ServiceChannelNotification storeTM(uint8_t* packet, uint16_t packetLength);

	/**
	 * @brief This service is used for storing incoming TC packets in the master channel
	 * @param packet Raw packet data
	 * @param packetLength The length of the packet
	 */
	ServiceChannelNotification storeTC(uint8_t* packet, uint16_t packetLength);

	/**
	 * @brief Requests to process the last packet stored in the buffer of the specific MAPP channel
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
	 * @brief  Requests to process the last packet stored in the buffer of the specific virtual channel
	 * (possible more if blocking is enabled). The packets are segmented or blocked together
	 * and then stored in the buffer of the virtual channel
	 */
	ServiceChannelNotif vcpp_request(uint8_t vid);

#endif

	/**
	 * @brief The Master Channel Reception Function shall be used to extract service data units
	 * contained in the Transfer Frame Secondary Header and Operational Control Field from
	 * Transfer Frames of a Master Channel.
	 @see p. 4.3.5 from TM Space Data Link Protocol (CCSDS 132.0-B-3)
	 */
	ServiceChannelNotification mcReceptionTMRequest();

	/**
	 * @brief The Master Channel Generation Service shall be used to insert Transfer Frame
	 * Secondary Header and/or Operational Control Field service data units into Transfer Frames
	 * of a Master Channel.
	 @see p. 4.2.5 from TM Space Data Link Protocol (CCSDS 132.0-B-3)
	 */
	ServiceChannelNotification mcGenerationTMRequest();
	/**
	 * @brief The  Virtual  Channel  Generation  Function  shall  perform  the  following  two
	 * procedures in the following order:
	 * 		a) the  Frame  Operation  Procedure  (FOP),  which  is  a  sub-procedure  of  the
	 * 		Communications Operation Procedure (COP); and
	 * 		b) the Frame Generation Procedure in this order.
	 * @see p. 4.3.5 from TC Space Data Link Protocol
	 */
	ServiceChannelNotification vcGenerationRequestTC(uint8_t vid);

	/**
	 * @brief The  All  Frames  Generation  Function  shall  be  used  to  perform  error  control
	 * encoding defined by this Recommendation and to deliver Transfer Frames at an appropriate
	 * rate to the Channel Coding Sublayer.
	 * @see p. 4.3.8 from TC Space Data Link Protocol
	 */
	ServiceChannelNotification allFramesGenerationTCRequest();

	/**
	 * @brief The  All  Frames  Reception  Function  shall  be  used  to  perform  error  control
	 * encoding defined by this Recommendation and to deliver Transfer Frames at an appropriate
	 * rate to the Channel Coding Sublayer.
	 * @see p. 4.3.7 from TM Space Data Link Protocol (CCSDS 132.0-B-3)
	 */
	ServiceChannelNotification allFramesReceptionTMRequest();

	/**
	 * @brief The  All  Frames  Generation  Function  shall  be  used  to  perform  error  control
	 * encoding defined by this Recommendation and to deliver Transfer Frames at an appropriate
	 * rate to the Channel Coding Sublayer.
	 * @see p. 4.2.7 from TC Space Data Link Protocol
	 */
	ServiceChannelNotification allFramesGenerationTMRequest(uint8_t* packet_data, uint16_t packet_length=TmTransferFrameSize);

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
	 * @brief Get FOP State of the virtual channel
	 */
	FOPState fopState(uint8_t vid) const;

	/**
	 * @brief Returns the value of the timer that is used to determine the time frame for acknowledging transferred
	 * frames
	 */
	uint16_t t1Timer(uint8_t vid) const;

	/**
	 * @brief Indicates the width of the sliding window which is used to proceed to the lockout state in case the
	 * transfer frame number of the received packet deviates too much from the expected one.
	 */
	uint8_t fopSlidingWindowWidth(uint8_t vid) const;

	/**
	 * @brief Returns the timeout action which is to be performed once the maximum transmission limit is reached and
	 * the timer has expired.
	 */
	bool timeoutType(uint8_t vid) const;

	/**
	 * @brief Returns the last frame sequence number, V(S), that will be placed in the header of the next transferred
	 * packet
	 * @param vid Virtual Channel ID
	 */
	uint8_t transmitterFrameSeqNumber(uint8_t vid) const;

	/**
	 * @brief Returns the expected acknowledgement frame sequence number, NN(R). This is essentially the frame sequence
	 * number of the oldest unacknowledged frame
	 * @param vid Virtual Channel ID
	 */
	uint8_t expectedFrameSeqNumber(uint8_t vid) const;

	/**
	 * @brief Processes the packet at the head of the buffer
	 */
	void process();

	/**
	 * @brief Available number of incoming TM frames in master channel buffer
	 */

	uint16_t rxInAvailableTM() const {
		return masterChannel.rxInFramesBeforeAllFramesReceptionListTM.available();
	}
	/**
	 * @brief Available number of outcoming TX frames in master channel buffer
	 */
	uint16_t txOutAvailable() const {
		return masterChannel.txToBeTransmittedFramesAfterAllFramesGenerationListTC.available();
	}

	/**
	 * @brief Available number of outcoming TC RX frames in master channel buffer
	 */
	uint16_t rxOutAvailableTC() const {
		return masterChannel.rxToBeTransmittedFramesAfterAllFramesReceptionListTC.available();
	}

	/**
	 * @brief Available space in TC virtual channel buffer
	 */
	uint16_t txAvailableTC(const uint8_t vid) const {
		return masterChannel.virtChannels.at(vid).availableBufferTC();
	}

	/**
	 * @brief Available space in TC MAP channel buffer
	 */
	uint16_t txAvailableTC(const uint8_t vid, const uint8_t mapid) const {
		return masterChannel.virtChannels.at(vid).mapChannels.at(mapid).availableBufferTC();
	}

	/**
	 * @brief Read first packet of the TC MAP channel buffer (unprocessedPacketListBufferTC)
	 */
	std::pair<ServiceChannelNotification, const TransferFrameTC*> txOutPacketTC(uint8_t vid, uint8_t mapid) const;

	/**
	 * @brief Read first TC packet of the virtual channel buffer (txUnprocessedPacketListBufferTC)
	 */
	std::pair<ServiceChannelNotification, const TransferFrameTC*> txOutPacketTC(uint8_t vid) const;

	/**
	 * @brief Return the last stored packet from txMasterCopyTC
	 */
	std::pair<ServiceChannelNotification, const TransferFrameTC*> txOutPacketTC() const;

	/**
	 * @brief Return the last stored packet from txMasterCopyTM
	 */
	std::pair<ServiceChannelNotification, const TransferFrameTM*> txOutPacketTM() const;

	/**
	 * @brief Return the last processed packet
	 */
	std::pair<ServiceChannelNotification, const TransferFrameTM*> txOutProcessedPacketTM() const;
	std::pair<ServiceChannelNotification, const TransferFrameTC*> txOutProcessedPacketTC() const;

	// This is honestly a bit confusing
	ServiceChannel(MasterChannel masterChannel, PhysicalChannel physicalChannel)
	    : masterChannel(masterChannel), physicalChannel(physicalChannel) {}
};

#endif // CCSDS_CCSDSSERVICECHANNEL_HPP