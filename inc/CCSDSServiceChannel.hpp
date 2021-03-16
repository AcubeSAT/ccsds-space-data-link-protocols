#ifndef CCSDS_SERVICECHANNEL_HPP
#define CCSDS_SERVICECHANNEL_HPP

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
     * @brief
     */

public:

};
#endif //CCSDS_CCSDSSERVICECHANNEL_HPP
