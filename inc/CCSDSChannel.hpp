#ifndef CCSDS_CHANNEL_HPP
#define CCSDS_CHANNEL_HPP

#pragma once

#include <cstdint>
#include <etl/map.h>
#include <etl/list.h>

#include <CCSDS_Definitions.hpp>
#include <FrameOperationProcedure.h>
#include <Packet.hpp>

class MasterChannel;

enum DataFieldContent {
    PACKET = 0,
    VCA_SDU = 1 // Not currently supported
};

struct PhysicalChannel {
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
                    const uint16_t max_pdu_length, const uint32_t bitrate, const uint16_t repetitions) :
            maxFrameLength(max_frame_length), errorControlFieldPresent(error_control_present),
            maxFramePdu(max_frames_pdu),
            maxPDULength(max_pdu_length), bitrate(bitrate), repetitions(repetitions) {}
};

struct MAPChannel {
    friend class ServiceChannel;

    /**
     * @brief MAP Channel Identifier
     */
    const uint8_t MAPID; // 6 bits

    /**
     * @brief Determines whether the incoming data content type (Packet/MAP SDU)
     */
    const DataFieldContent dataFieldContent;

    void store(Packet packet);

    /**
     * @brief Returns available space in the buffer
     */
    const uint16_t available() const {
        return unprocessedPacketList.available();
    }

    MAPChannel(const uint8_t mapid, const DataFieldContent data_field_content) :
            MAPID(mapid), dataFieldContent(data_field_content) {
        uint8_t d = unprocessedPacketList.size();
        unprocessedPacketList.full();
    };

protected:
    /**
     * Store unprocessed received TCs
     */
    etl::list<Packet*, MAX_RECEIVED_TC_IN_MAP_BUFFER> unprocessedPacketList;
};

struct VirtualChannel {
    friend class ServiceChannel;
    friend class MasterChannel;

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
     * @brief Returns available space in the buffer
     */
    const uint16_t available() const {
        return waitQueue.available();
    }

    /**
    * @brief MAP channels of the virtual channel
    */
    etl::map<uint8_t, MAPChannel, MAX_MAP_CHANNELS> mapChannels;

    VirtualChannel(const uint8_t vcid, const bool segment_header_present, const uint16_t max_frame_length,
                   const uint8_t clcw_rate, const bool blocking, const uint8_t repetition_type_a_frame,
                   const uint8_t repetition_cop_ctrl,
                   etl::map<uint8_t, MAPChannel, MAX_MAP_CHANNELS> map_chan
    ) :
            VCID(vcid & 0x3FU), GVCID((MCID << 0x06U) + VCID), segmentHeaderPresent(segment_header_present),
            maxFrameLength(max_frame_length), clcwRate(clcw_rate), blocking(blocking),
            repetitionTypeAFrame(repetition_type_a_frame), repetitionCOPCtrl(repetition_cop_ctrl),
            waitQueue(), sentQueue(), fop(FrameOperationProcedure(this, &waitQueue, &sentQueue, repetition_cop_ctrl)){
                mapChannels = map_chan;
    }

    VirtualChannelAlert store(Packet* packet);

    VirtualChannelAlert store_unprocessed(Packet packet);

    MasterChannel* master_channel(){
        return masterChannel;
    }

private:
    /**
     * @brief Buffer to store_out incoming packets before being processed by COP
     */
    etl::list<Packet*, MAX_RECEIVED_TC_IN_WAIT_QUEUE> waitQueue;

    /**
     * @brief Buffer to store_out outcoming packets after being processed by COP
     */
    etl::list<Packet*, MAX_RECEIVED_TC_IN_SENT_QUEUE> sentQueue;

    /**
     * @brief Buffer to store_out unprocessed packets that are directly processed in the virtual instead of MAP channel
     */
    etl::list<Packet, MAX_RECEIVED_UNPROCESSED_TC_IN_VIRT_BUFFER> unprocessedPacketList;

    /**
     * @brief Holds the FOP state of the virtual channel
     */
    FrameOperationProcedure fop;

    /**
     * @brief Pointer to the Master Channel the Virtual Channel belongs in
     */
    MasterChannel* masterChannel;

    void set_master_channel(MasterChannel* master_channel){
        masterChannel = master_channel;
    }
};


struct MasterChannel {
    friend class ServiceChannel;

    /**
     * @brief Virtual channels of the master channel
     */
    // TODO: Type aliases because this is getting out of hand
    etl::map<uint8_t, VirtualChannel, MAX_VIRTUAL_CHANNELS> virtChannels;
    bool errorCtrlField;

    MasterChannel(
            etl::map<uint8_t, VirtualChannel, MAX_VIRTUAL_CHANNELS> virt_chan,
            bool errorCtrlField)
            :
            outFramesList(), errorCtrlField(errorCtrlField) {

        virtChannels = virt_chan;
        for (auto virt_channel : virtChannels){
            virt_channel.second.set_master_channel(this);
        }
    }
    MasterChannelAlert store_out(Packet* packet);
    MasterChannelAlert store_transmitted_out(Packet* packet);

private:
    // Packets stored in frames list, before being processed by the all frames generation service
    etl::list<Packet*, MAX_RECEIVED_TC_IN_MASTER_BUFFER> outFramesList;

    // Packets ready to be transmitted having passed thorugh the all frames generation service
    etl::list<Packet*, MAX_RECEIVED_TC_OUT_IN_MASTER_BUFFER> toBeTransmittedFramesList;

    /**
     * @brief Buffer holding the master copy of the packets that are currently being processed
     */
    etl::list<Packet, MAX_RECEIVED_TC_IN_MASTER_BUFFER> masterCopy;
};


#endif //CCSDS_CHANNEL_HPP
