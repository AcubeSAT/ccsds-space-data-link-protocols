#ifndef CCSDS_CHANNEL_HPP
#define CCSDS_CHANNEL_HPP

#include <cstdint>
#include <etl/map.h>
#include <memory>

#include "CCSDS_Definitions.hpp"

enum DataFieldContent{
    PACKET = 0,
    VCA_SDU = 1
};

struct PhysicalChannel{
    /**
      * @brief Maximum length of a single transfer frame
      */
    const uint16_t maxFrameLength;

    /**
     * @brief Determines whether Error Control field is present
     */
     const bool errorControlFieldPresent;

     /**
      * @brief Sets the maximum number of transfer frames that can be transferred in a single data unit
      */
     const uint16_t maxFramePdu;

     /**
      * @brief Maximum length of a data unit
      */
     const uint16_t maxPDULength;

     /**
      * @brief Maximum bit rate (bits per second)
      */
     const uint32_t bitrate;

     /**
      * @brief Maximum number of retransmissions for a data unit
     */
     const uint16_t repetitions;

    PhysicalChannel(const uint16_t max_frame_length, const bool error_control_present, const uint16_t max_frames_pdu,
                    const uint16_t max_pdu_length, const uint32_t bitrate, const uint16_t repetitions):
    maxFrameLength(max_frame_length), errorControlFieldPresent(error_control_present), maxFramePdu(max_frames_pdu),
    maxPDULength(max_pdu_length), bitrate(bitrate), repetitions(repetitions)
    {}
};

struct MAPChannel{
    /**
     * @brief MAP Channel Identifier
     */
    const uint8_t MAPID; // 6 bits


    /**
     * @brief Determines whether the incoming data content type (Packet/MAP SDU)
     */
    const DataFieldContent dataFieldContent;

    /**
      * @brief Maximum length of a single transfer frame
      */
    const uint16_t transferFrameLength;

    /**
     * @brief Determines whether smaller data units can be combined into a single transfer frame
     */
    const bool blocking;

    /**
     * @brief Determines whether bigger data units can be broken down to smaller segments
     */
    const bool segmentation;

    /**
     * @brief Maximum length of a MAP SDU that is used to determine the segments a SDU is broken into (if segmentation
     * is enabled in the MAP Channel)
     */
    const uint16_t maxSDULength;

    MAPChannel(const uint8_t mapid, const DataFieldContent data_field_content,
            const uint16_t transfer_frame_length, const bool blocking, const bool segmentation,
            const uint16_t max_sdu_length):
            MAPID(mapid), dataFieldContent(data_field_content), transferFrameLength(transfer_frame_length),
            blocking(blocking), segmentation(segmentation), maxSDULength(max_sdu_length){
    };
};

struct VirtualChannel{
    /**
     * @brief Virtual Channel Identifier
     */
    const uint8_t VCID;  // 6 bits

    /**
     * @brief Global Virtual Channel Identifier
     */
    const uint16_t GVCID; // 16 bits (assumes TFVN is set to 0)

    /**
     * @brief Determines whether the Segment Header is present (enables MAP services)
     */
    const bool segmentHeaderPresent;

    /**
     * @brief Maximum length of a single transfer frame
     */
    const uint16_t maxFrameLength;

    /**
     * @brief CLCW report rate (number per second)
     */
    const uint8_t clcwRate;

    /**
    * @brief Determines whether smaller data units can be combined into a single transfer frame
    */
    const bool blocking;

    /**
     * Determines the number of times Type A frames will be re-transmitted
     */
    const uint8_t repetitionTypeAFrame;

    /**
     * Determines the number of times COP Control frames will be re-transmitted
     */
    const uint8_t repetitionCOPCtrl;

    /**
     * @brief MAP channels of the virtual channel
     */
    const etl::map<uint8_t, std::shared_ptr<MAPChannel>, sizeof(std::shared_ptr<MAPChannel>)> mapChannels;

    VirtualChannel(const uint8_t vcid, const bool segment_header_present, const uint16_t max_frame_length,
                   const uint8_t clcw_rate, const bool blocking, const uint8_t repetition_type_a_frame,
                   const uint8_t repetition_cop_ctrl,
                   const etl::map<uint8_t, std::shared_ptr<MAPChannel>, sizeof(std::shared_ptr<MAPChannel>)> map_chan
                   ):
            VCID(vcid & 0x3FU), GVCID((MCID << 0x06U) + VCID), segmentHeaderPresent(segment_header_present),
            maxFrameLength(max_frame_length), clcwRate(clcw_rate), blocking(blocking),
            repetitionTypeAFrame(repetition_type_a_frame), repetitionCOPCtrl(repetition_cop_ctrl), mapChannels(map_chan)
    {}
};


struct MasterChannel{
    /**
     * @brief Virtual channels of the master channel
     */
     // TODO: Type aliases because this is getting out of hand
     // TODO: Consider turning it into a unique_ptr
    const etl::map<uint8_t, std::shared_ptr<VirtualChannel>, sizeof(std::shared_ptr<VirtualChannel>)> virtChannels;

    MasterChannel(
            const etl::map<uint8_t, std::shared_ptr<VirtualChannel>, sizeof(std::shared_ptr<VirtualChannel>)> virt_chan)
            :virtChannels(virt_chan){};
};

#endif //CCSDS_CHANNEL_HPP
