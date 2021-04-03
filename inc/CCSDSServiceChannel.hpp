#ifndef CCSDS_SERVICECHANNEL_HPP
#define  CCSDS_SERVICECHANNEL_HPP

#include <CCSDSChannel.hpp>
#include <Alert.hpp>
#include <etl/circular_buffer.h>

#include <CCSDSTransferFrameTC.hpp>
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

#if MAX_RECEIVED_UNPROCESSED_TC_IN_VIRT_BUFFER > 0

    /**
     * @brief  Requests to process the last packet stored in the buffer of the specific virtual channel
     * (possible more if blocking is enabled). The packets are segmented or blocked together
     * and then stored in the buffer of the virtual channel
     */
    ServiceChannelNotif vcpp_request(uint8_t vid);

#endif

    ServiceChannelNotif vc_generation_request(uint8_t vid);

    /**
     * @brief Processes the packet at the head of the buffer
     */
    void process();

    /**
     * @brief Available space in master channel buffer
     */
    const uint16_t available() const {
        return masterChannel.framesList.available();
    }

    /**
    * @brief Available space in virtual channel buffer
    */
    const uint16_t available(const uint8_t vid) const {
        masterChannel.virtChannels.at(vid).available();
    }

    /**
    * @brief Available space in MAP channel buffer
    */
    const uint16_t available(const uint8_t vid, const uint8_t mapid) const {
        return masterChannel.virtChannels.at(vid).mapChannels.at(mapid).available();
    }

    ServiceChannel(MasterChannel master_channel) :
            masterChannel(master_channel) {
    }
};

#endif //CCSDS_CCSDSSERVICECHANNEL_HPP