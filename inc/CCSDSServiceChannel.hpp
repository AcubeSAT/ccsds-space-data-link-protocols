#ifndef CCSDS_SERVICECHANNEL_HPP
#define CCSDS_SERVICECHANNEL_HPP

#include <CCSDSChannel.hpp>
#include <Alert.hpp>

#include <Packet.hpp>
#include <utility>

/**
 * @brief This provides a way to interconnect all different CCSDS Space Data Protocol Services and provides a
 * bidirectional interface between the receiving and transmitting parties
 */


class ServiceChannel {

private:
    /**
     * @brief The Master Channel essentially stores the configuration of your channel. It partitions the physical channel
     * into virtual channels, each of which has different parameters in order to easily manage incoming traffic
     */
    MasterChannel masterChannel;

    // @todo Think about whether we need to keep the size of the packets in a queue. Technically, we can determine this
    // from the header of the frame. Not sure what makes more sense

public:
    // Public methods that are called by the scheduler

    /**
     * @brief Stores an incoming packet in the ring buffer
     *
     * @param packet Data of the packet
     * @param packet_length Packet length
     * @param gvcid Global Virtual Channel ID
     * @param mapid MAP ID
     * @param sduid SDU ID
     * @param service_type Service Type - Type-A or Type-B
     */
    ServiceChannelNotif store(uint8_t *packet, uint16_t packet_length, uint8_t gvcid, uint8_t mapid, uint16_t sduid,
                              ServiceType service_type);

    /**
     * @brief Requests to process the last packet stored in the buffer of the specific MAPP channel
     * (possible more if blocking is enabled). The packets are segmented or blocked together
     * and then transferred to the buffer of the virtual channel
     */
    ServiceChannelNotif mapp_request(uint8_t vid, uint8_t mapid);

#if max_received_unprocessed_tc_in_virt_buffer > 0

    /**
     * @brief  Requests to process the last packet stored in the buffer of the specific virtual channel
     * (possible more if blocking is enabled). The packets are segmented or blocked together
     * and then stored in the buffer of the virtual channel
     */
    ServiceChannelNotif vcpp_request(uint8_t vid);

#endif

    ServiceChannelNotif vc_generation_request(uint8_t vid);

    ServiceChannelNotif all_frames_generation_request();

    ServiceChannelNotif all_frames_reception();

    ServiceChannelNotif transmit_frame(uint8_t *pack);

    // COP Directives
    // TODO: Properly handle Notifications

    void initiate_ad_no_clcw(uint8_t vid);

    void initiate_ad_clcw(uint8_t vid);

    void initiate_ad_unlock(uint8_t vid);

    void initiate_ad_vr(uint8_t vid, uint8_t vr);

    void terminate_ad_service(uint8_t vid);

    void resume_ad_service(uint8_t vid);

    void set_vs(uint8_t vid, uint8_t vs);

    void set_fop_width(uint8_t vid, uint8_t width);

    void set_t1_initial(uint8_t vid, uint16_t t1_init);

    void set_transmission_limit(uint8_t vid, uint8_t vr);

    void set_timeout_type(uint8_t vid, bool vr);

    void invalid_directive(uint8_t vid);

    /**
     * @brief Get FOP State of the virtual channel
     */
    const FOPState fop_state(uint8_t vid) const;

    /**
     * @brief Returns the value of the timer that is used to determine the time frame for acknowledging transferred
     * frames
     */
    const uint16_t t1_timer(uint8_t vid) const;

    /**
     * @brief Indicates the width of the sliding window which is used to proceed to the lockout state in case the
     * transfer frame number of the received packet deviates too much from the expected one.
     */
    const uint8_t fop_sliding_window_width(uint8_t vid) const;

    /**
     * @brief Returns the timeout action which is to be performed once the maximum transmission limit is reached and
     * the timer has expired.
     */
    const bool timeout_type(uint8_t vid) const;


    /**
     * @brief Returns the last frame sequence number, V(S), that will be placed in the header of the next transferred packet
     * @param vid Virtual Channel ID
     */
    const uint8_t transmitter_frame_seq_number(uint8_t vid) const;

    /**
    * @brief Returns the expected acknowledgement frame sequence number, NN(R). This is essentially the frame sequence
    * number of the oldest unacknowledged frame
    * @param vid Virtual Channel ID
   */
    const uint8_t expected_frame_seq_number(uint8_t vid) const;

    /**
     * @brief Processes the packet at the head of the buffer
     */
    void process();

    /**
     * @brief Available number of incoming frames in master channel buffer
     */
    const uint16_t in_available() const {
        return masterChannel.txOutFramesList.available();
    }

    /**
     * @brief Available number of outcoming frames in master channel buffer
     */
    const uint16_t out_available() const {
        return masterChannel.toBeTransmittedFramesList.available();
    }

    /**
    * @brief Available space in virtual channel buffer
    */
    const uint16_t tx_available(const uint8_t vid) const {
        masterChannel.virtChannels.at(vid).available();
    }

    /**
    * @brief Available space in MAP channel buffer
    */
    const uint16_t tx_available(const uint8_t vid, const uint8_t mapid) const {
        return masterChannel.virtChannels.at(vid).mapChannels.at(mapid).available();
    }

    /**
     * @brief Read first packet of the MAP channel buffer
     */
    std::pair<ServiceChannelNotif, const Packet *>

    out_packet(const uint8_t vid, const uint8_t mapid) const;

    /**
    * @brief Read first packet of the virtual channel buffer
    */
    std::pair<ServiceChannelNotif, const Packet *> tx_out_packet(const uint8_t vid) const;

    /**
    * @brief Return the last stored packet
    */
    std::pair<ServiceChannelNotif, const Packet *> tx_out_packet() const;

    // This is honestly a bit confusing
    ServiceChannel(MasterChannel master_channel) :
            masterChannel(master_channel) {
    }
};

#endif //CCSDS_CCSDSSERVICECHANNEL_HPP