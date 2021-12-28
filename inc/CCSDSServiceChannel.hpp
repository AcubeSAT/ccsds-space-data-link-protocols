#ifndef CCSDS_SERVICECHANNEL_HPP
#define CCSDS_SERVICECHANNEL_HPP

#include <CCSDSChannel.hpp>
#include <Alert.hpp>

#include <PacketTC.hpp>
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

    // @todo Think about whether we need to keep the size of the packets in a queue. Technically, we can determine this
    // from the header of the frame. Not sure what makes more sense

public:
    // Public methods that are called by the scheduler

    /**
     * @brief Stores an incoming  TC packet in the ring buffer
     *
     * @param packet Data of the packet
     * @param packet_length PacketTC length
     * @param gvcid Global Virtual Channel ID
     * @param mapid MAP ID
     * @param sduid SDU ID
     * @param service_type Service Type - Type-A or Type-B
     */
	ServiceChannelNotification storeTC(uint8_t *packet, uint16_t packet_length, uint8_t gvcid, uint8_t mapid, uint16_t sduid,
                              ServiceType service_type);

    /**
     * @brief Stores an incoming  TM packet in the ring buffer
     *
     * @param packet Data of the packet
     * @param packet_length PacketTC length
     * @param gvcid Global Virtual Channel ID
     * @param scid Spacecraft ID
     */

	ServiceChannelNotification storeTM(uint8_t *packet, uint16_t packet_length, uint8_t gvcid, uint16_t scid);


    /**
     * @brief This service is used for storing incoming packets in the master channel
     * @param packet Raw packet data
     * @param packet_length The length of the packet
     */
	ServiceChannelNotification store(uint8_t *packet, uint16_t packet_length);

    /**
     * @brief Requests to process the last packet stored in the buffer of the specific MAPP channel
     * (possible more if blocking is enabled). The packets are segmented or blocked together
     * and then transferred to the buffer of the virtual channel
     */
	ServiceChannelNotification mappRequest(uint8_t vid, uint8_t mapid);

#if max_received_unprocessed_tc_in_virt_buffer > 0

    /**
     * @brief  Requests to process the last packet stored in the buffer of the specific virtual channel
     * (possible more if blocking is enabled). The packets are segmented or blocked together
     * and then stored in the buffer of the virtual channel
     */
    ServiceChannelNotif vcpp_request(uint8_t vid);

#endif

	/**
	 * @brief The  Virtual  Channel  Generation  Function  shall  perform  the  following  two
	 * procedures in the following order:
	 * 		a) the  Frame  Operation  Procedure  (FOP),  which  is  a  sub-procedure  of  the
	 * 		Communications Operation Procedure (COP); and
	 * 		b) the Frame Generation Procedure in this order.
	 * @see p. 4.3.5 from TC SPACE DATA LINK PROTOCOL
	 */
	ServiceChannelNotification vcGenerationRequest(uint8_t vid);

	/**
	 * @brief The  All  Frames  Generation  Function  shall  be  used  to  perform  error  control
	 * encoding defined by this Recommendation and to deliver Transfer Frames at an appropriate
	 * rate to the Channel Coding Sublayer.
	 * @see p. 4.3.8 from TC SPACE DATA LINK PROTOCOL
	 */
	ServiceChannelNotification allFramesGenerationRequest();

	/**
	 * @return The front TC Packet from txOutFramesBeforeAllFramesGenerationList
	 */
    std::optional<PacketTC> getTxProcessedPacket();

	/**
	 * @brief The All Frames Reception Function shall be used to reconstitute Transfer Frames
	 * from the data stream provided by the Channel Coding Sublayer and to perform checks to
	 * determine whether the reconstituted Transfer Frames are valid or not.
	 * @see p.4.4.8 from TC SPACE DATA LINK PROTOCOL
	 */
	ServiceChannelNotification allFramesReceptionRequest();

	// COP Directives

	ServiceChannelNotification transmitFrame(uint8_t *pack);

	ServiceChannelNotification transmitAdFrame(uint8_t vid);

	ServiceChannelNotification pushSentQueue(uint8_t vid);

    // TODO: Properly handle Notifications
    void acknowledgeFrame(uint8_t vid, uint8_t frame_seq_number);

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
    const FOPState fopState(uint8_t vid) const;

    /**
     * @brief Returns the value of the timer that is used to determine the time frame for acknowledging transferred
     * frames
     */
    const uint16_t t1Timer(uint8_t vid) const;

    /**
     * @brief Indicates the width of the sliding window which is used to proceed to the lockout state in case the
     * transfer frame number of the received packet deviates too much from the expected one.
     */
    const uint8_t fopSlidingWindowWidth(uint8_t vid) const;

    /**
     * @brief Returns the timeout action which is to be performed once the maximum transmission limit is reached and
     * the timer has expired.
     */
    const bool timeoutType(uint8_t vid) const;

    /**
     * @brief Returns the last frame sequence number, V(S), that will be placed in the header of the next transferred
     * packet
     * @param vid Virtual Channel ID
     */
    const uint8_t transmitterFrameSeqNumber(uint8_t vid) const;

    /**
     * @brief Returns the expected acknowledgement frame sequence number, NN(R). This is essentially the frame sequence
     * number of the oldest unacknowledged frame
     * @param vid Virtual Channel ID
     */
    const uint8_t expectedFrameSeqNumber(uint8_t vid) const;

    /**
     * @brief Processes the packet at the head of the buffer
     */
    void process();

    /**
     * @brief Available number of incoming frames in master channel buffer
     */
    const uint16_t inAvailable() const {
        return masterChannel.txOutFramesBeforeAllFramesGenerationList.available();
    }

    /**
     * @brief Available number of outcoming TX frames in master channel buffer
     */
    const uint16_t txOutAvailable() const {
        return masterChannel.txToBeTransmittedFramesAfterAllFramesGenerationList.available();
    }

    /**
     * @brief Available number of outcoming RX frames in master channel buffer
     */
    const uint16_t rxOutAvailable() const {
        return masterChannel.rxToBeTransmittedFramesAfterAllFramesReceptionList.available();
    }

    /**
     * @brief Available space in virtual channel buffer
     */
    const uint16_t txAvailable(const uint8_t vid) const {
        return masterChannel.virtChannels.at(vid).availableBufferTC();
    }

    /**
     * @brief Available space in MAP channel buffer
     */
    const uint16_t txAvailable(const uint8_t vid, const uint8_t mapid) const {
        return masterChannel.virtChannels.at(vid).mapChannels.at(mapid).availableBufferTC();
    }

    /**
     * @brief Read first packet of the MAP channel buffer (unprocessedPacketListBufferTC)
     */
    std::pair<ServiceChannelNotification, const PacketTC*> txOutPacket(const uint8_t vid, const uint8_t mapid) const;

    /**
     * @brief Read first TC packet of the virtual channel buffer (txUnprocessedPacketListBufferTC)
     */
    std::pair<ServiceChannelNotification, const PacketTC*> txOutPacket(const uint8_t vid) const;

    /**
     * @brief Return the last stored packet from txMasterCopyTC
     */
    std::pair<ServiceChannelNotification, const PacketTC *> txOutPacketTC() const;

    /**
 * @brief Return the last stored packet from txMasterCopyTM
 */
    std::pair<ServiceChannelNotification, const PacketTM *> txOutPacketTM() const;

    /**
     * @brief Return the last processed packet from txToBeTransmittedFramesAfterAllFramesGenerationList
     */
    std::pair<ServiceChannelNotification, const PacketTC *> txOutProcessedPacket() const;

    // This is honestly a bit confusing
    ServiceChannel(MasterChannel master_channel) : masterChannel(master_channel) {}
};

#endif // CCSDS_CCSDSSERVICECHANNEL_HPP