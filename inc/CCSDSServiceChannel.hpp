#ifndef CCSDS_SERVICECHANNEL_HPP
#define  CCSDS_SERVICECHANNEL_HPP

#include <MAPP.hpp>
#include <CCSDSChannel.hpp>
#include <memory>
#include <etl/circular_buffer.h>

/**
 * @brief This provides a way to interconnect all different CCSDS Space Data Protocol Services and provides a
 * bidirectional interface between the receiving and transmitting parties
 */

// Currently there is just a very vague outline of the interface of the class
class ServiceChannel{
private:
    /**
     * @brief The Master Channel essentially stores the configuration of your project. It segments the physical channel
     * into virtual channels, each of which has different parameters in order to easily serve a different purpose.
     */
    std::unique_ptr<MasterChannel> masterChannel;

    /**
     * @brief The buffer used to store incoming packets
     */
     etl::circular_buffer<uint8_t, RECEIVED_TC_BUFFER_MAX_SIZE * sizeof(uint8_t)>;

public:
    // Public methods that are called by the scheduler
    
    /**
     * @brief Stores an incoming packet in the ring buffer
     */
    void store();

    /**
     * @brief Processes all packets that are currently stored in memory
     */
    void process_all();

    /**
     * @brief Processes the packet at the head of the buffer
     */
    void process();

    /**
     * @brief Processes the first n packets at the head of the buffer
     */
    void process(uint8_t n);
};
#endif //CCSDS_CCSDSSERVICECHANNEL_HPP
