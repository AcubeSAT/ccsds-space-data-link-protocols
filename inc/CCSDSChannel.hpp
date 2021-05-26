#ifndef CCSDS_CHANNEL_HPP
#define CCSDS_CHANNEL_HPP

#pragma once

#include <cstdint>
#include <etl/map.h>
#include <etl/flat_map.h>
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
    etl::list<Packet *, max_received_tc_in_map_channel> unprocessedPacketList;
};

struct VirtualChannel {
    friend class ServiceChannel;

    friend class MasterChannel;

    friend class FrameOperationProcedure;

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
        return unprocessedPacketList.available();
    }

    /**
    * @brief MAP channels of the virtual channel
    */
    etl::flat_map<uint8_t, MAPChannel, max_map_channels> mapChannels;

    VirtualChannel(MasterChannel &master_channel, const uint8_t vcid, const bool segment_header_present,
                   const uint16_t max_frame_length,
                   const uint8_t clcw_rate, const bool blocking, const uint8_t repetition_type_a_frame,
                   const uint8_t repetition_cop_ctrl,
                   etl::flat_map<uint8_t, MAPChannel, max_map_channels> map_chan
    ) :
            masterChannel(master_channel), VCID(vcid & 0x3FU), GVCID((mcid << 0x06U) + VCID),
            segmentHeaderPresent(segment_header_present),
            maxFrameLength(max_frame_length), clcwRate(clcw_rate), blocking(blocking),
            repetitionTypeAFrame(repetition_type_a_frame), repetitionCOPCtrl(repetition_cop_ctrl),
            waitQueue(), sentQueue(), fop(FrameOperationProcedure(this, &waitQueue, &sentQueue, repetition_cop_ctrl)) {
        mapChannels = map_chan;
    }


    VirtualChannel(const VirtualChannel &v) :
            VCID(v.VCID), GVCID(v.GVCID), segmentHeaderPresent(v.segmentHeaderPresent),
            maxFrameLength(v.maxFrameLength), clcwRate(v.clcwRate), repetitionTypeAFrame(v.repetitionTypeAFrame),
            repetitionCOPCtrl(v.repetitionCOPCtrl), waitQueue(v.waitQueue), sentQueue(v.sentQueue),
            unprocessedPacketList(v.unprocessedPacketList), fop(v.fop), masterChannel(v.masterChannel),
            blocking(v.blocking), mapChannels(v.mapChannels) {
        fop.vchan = this;
        fop.sentQueue = &sentQueue;
        fop.waitQueue = &waitQueue;
    }

    VirtualChannelAlert store(Packet *packet);

    /**
     * @bried Add MAP channel to virtual channel
     */
    VirtualChannelAlert add_map(const uint8_t mapid, const DataFieldContent data_field_content);

    MasterChannel &master_channel() {
        return masterChannel;
    }

private:
    /**
     * @brief Buffer to store_out incoming packets before being processed by COP
     */
    etl::list<Packet *, max_received_tc_in_wait_queue> waitQueue;

    /**
     * @brief Buffer to store_out outcoming packets after being processed by COP
     */
    etl::list<Packet *, max_received_tc_in_sent_queue> sentQueue;

    /**
     * @brief Buffer to store_out unprocessed packets that are directly processed in the virtual instead of MAP channel
     */
    etl::list<Packet *, max_received_unprocessed_tc_in_virt_buffer> unprocessedPacketList;

    /**
     * @brief Holds the FOP state of the virtual channel
     */
    FrameOperationProcedure fop;

    /**
     * @brief The Master Channel the Virtual Channel belongs in
     */
    MasterChannel &masterChannel;
};


struct MasterChannel {
    friend class ServiceChannel;

    /**
     * @brief Virtual channels of the master channel
     */
    // TODO: Type aliases because this is getting out of hand
    etl::flat_map<uint8_t, VirtualChannel, max_virtual_channels> virtChannels;
    bool errorCtrlField;

    MasterChannel(
            bool errorCtrlField)
            :
            virtChannels(), txOutFramesList(), errorCtrlField(errorCtrlField) {
    }

    MasterChannelAlert store_out(Packet *packet);

    MasterChannelAlert store_transmitted_out(Packet *packet);

    /**
     * @brief Add virtual channel to master channel
     */
    MasterChannelAlert add_vc(const uint8_t vcid, const bool segment_header_present, const uint16_t max_frame_length,
                              const uint8_t clcw_rate, const bool blocking, const uint8_t repetition_type_a_frame,
                              const uint8_t repetition_cop_ctrl,
                              etl::flat_map<uint8_t, MAPChannel, max_map_channels> map_chan);

private:
    // Packets stored in frames list, before being processed by the all frames generation service
    etl::list<Packet *, max_received_tc_in_master_buffer> txOutFramesList;
    // Packets ready to be transmitted having passed through the all frames generation service
    etl::list<Packet *, max_received_tc_out_in_master_buffer> toBeTransmittedFramesList;

    /**
     * @brief Buffer holding the master copy of the packets that are currently being processed
     */
    etl::list<Packet, max_received_tc_in_master_buffer> masterCopy;
};


#endif //CCSDS_CHANNEL_HPP
