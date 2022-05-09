#ifndef CCSDS_CHANNEL_HPP
#define CCSDS_CHANNEL_HPP

#pragma once

#include <cstdint>
#include <etl/map.h>
#include <etl/flat_map.h>
#include <etl/list.h>
#include <etl/queue.h>

#include <CCSDS_Definitions.hpp>
#include <FrameOperationProcedure.hpp>
#include <PacketTC.hpp>
#include <PacketTM.hpp>
#include <MemoryPool.hpp>
#include <iostream>

class MasterChannel;

enum DataFieldContent {
    PACKET = 0,
    VCA_SDU = 1 // Not currently supported
};

/**
 * @see Table 5-1 from TC SPACE DATA LINK PROTOCOL
 */
struct PhysicalChannel {
private:
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
public:
    PhysicalChannel(const uint16_t maxFrameLength, const bool errorControlPresent, const uint16_t maxFramesPdu,
                    const uint16_t maxPduLength, const uint32_t bitrate, const uint16_t repetitions)
            : maxFrameLength(maxFrameLength), errorControlFieldPresent(errorControlPresent),
              maxFramePdu(maxFramesPdu), maxPDULength(maxPduLength), bitrate(bitrate), repetitions(repetitions) {}

    uint16_t getMaxFrameLength() const {
        return maxFrameLength;
    }

    /**
     * @brief Determines whether Error Control field is present
     */
    bool getErrorControlFieldPresent() const {
        return errorControlFieldPresent;
    }

    /**
     * @brief Sets the maximum number of transfer frames that can be transferred in a single data unit
     */
    uint16_t getMaxFramePdu() const {
        return maxFramePdu;
    };

    /**
     * @brief Maximum length of a data unit
     */
    uint16_t getMaxPDULength() const {
        return maxPDULength;
    };

    /**
     * @brief Maximum bit rate (bits per second)
     */
    uint32_t getBitrate() const {
        return bitrate;
    };

    /**
     * @brief Maximum number of retransmissions for a data unit
     */
    uint16_t getRepetitions() const {
        return repetitions;
    }

};

/**
 * @see Table 5-4 from TC SPACE DATA LINK PROTOCOL
 */
class MAPChannel {
    friend class ServiceChannel;

private:
    /**
     * @brief MAP Channel Identifier
     */
    const uint8_t MAPID; // 6 bits

    /**
     * @brief Determines whether the incoming data content type (PacketTC/MAP SDU)
     */
    const DataFieldContent dataFieldContent;

public:
    void storeMAPChannel(Packet packet);

    /**
     * @brief Returns availableBufferTC space in the MAP Channel buffer
     */
    uint16_t availableBufferTC() const {
        return unprocessedPacketListBufferTC.available();
    }

    MAPChannel(const uint8_t mapid, const DataFieldContent dataFieldContent)
            : MAPID(mapid), dataFieldContent(dataFieldContent) {
        uint8_t d = unprocessedPacketListBufferTC.size();
        unprocessedPacketListBufferTC.full();
    };

protected:
    /**
     * Store unprocessed received TCs
     */
    etl::list<PacketTC *, MaxReceivedTcInMapChannel> unprocessedPacketListBufferTC;
};

/**
 * @see Table 5-3 from TC SPACE DATA LINK PROTOCOL
 */
class VirtualChannel {
    friend class ServiceChannel;

    friend class MasterChannel;

    friend class FrameOperationProcedure;

//@TODO: Those variables shouldn't be public
public:
    /**
     * @brief Virtual Channel Identifier
     */
    const uint8_t VCID; // 6 bits

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
     * Determines the number of TM Transfer Frames transmitted
     */
    // I am not sure if this should be here or part of the PacketTM struct
    const uint8_t frameCount;

    /**
     * @brief Returns availableVCBufferTC space in the VC TC buffer
     */
    uint16_t availableBufferTC() const {
        return txUnprocessedPacketListBufferTC.available();
    }

    /**
     *
     * @brief MAP channels of the virtual channel
     */
    etl::flat_map<uint8_t, MAPChannel, MaxMapChannels> mapChannels;


    VirtualChannel(std::reference_wrapper<MasterChannel> masterChannel, const uint8_t vcid,
                   const bool segmentHeaderPresent, const uint16_t maxFrameLength, const uint8_t clcwRate,
                   const bool blocking, const uint8_t repetitionTypeAFrame, const uint8_t repetitionCopCtrl,
                   const uint8_t frameCount, etl::flat_map<uint8_t, MAPChannel, MaxMapChannels> mapChan)
            : masterChannel(masterChannel), VCID(vcid & 0x3FU), GVCID((MCID << 0x06U) + VCID),
              segmentHeaderPresent(segmentHeaderPresent), maxFrameLength(maxFrameLength), clcwRate(clcwRate),
              blocking(blocking), repetitionTypeAFrame(repetitionTypeAFrame), repetitionCOPCtrl(repetitionCopCtrl),
              frameCount(frameCount), txWaitQueue(), sentQueue(),
              fop(FrameOperationProcedure(this, &txWaitQueue, &sentQueue, repetitionCopCtrl)) {
        mapChannels = mapChan;
    }

    VirtualChannel(const VirtualChannel &v)
            : VCID(v.VCID), GVCID(v.GVCID), segmentHeaderPresent(v.segmentHeaderPresent),
              maxFrameLength(v.maxFrameLength),
              clcwRate(v.clcwRate), repetitionTypeAFrame(v.repetitionTypeAFrame),
              repetitionCOPCtrl(v.repetitionCOPCtrl), frameCount(v.frameCount),
              txWaitQueue(v.txWaitQueue), sentQueue(v.sentQueue),
              txUnprocessedPacketListBufferTC(v.txUnprocessedPacketListBufferTC),
              fop(v.fop),
              masterChannel(v.masterChannel), blocking(v.blocking), mapChannels(v.mapChannels) {
        fop.vchan = this;
        fop.sentQueue = &sentQueue;
        fop.waitQueue = &txWaitQueue;
    }

    VirtualChannelAlert storeVC(PacketTC *packet);

    /**
     * @bried Add MAP channel to virtual channel
     */
    VirtualChannelAlert add_map(const uint8_t mapid, const DataFieldContent dataFieldContent);

    MasterChannel &master_channel() {
        return masterChannel;
    }

    /**
     * Method that stores a TM packet pointer and the TM  packet data to the packetPtrBufferTm and packetBufferTm queues
     * @param packet pointer to the packet data
     * @param packetLength length of the packet
     */
    void storePacketInPacketBufferTm(uint8_t *packet, uint16_t packetLength);

    /**
     * @brief packetPtrBufferTm getter
     */
    etl::queue<std::pair<uint8_t *, uint16_t>, PacketBufferTmSize> getPacketPtrBufferTm();

    /**
     * @brief packetBufferTm getter
     */
    etl::queue<uint8_t, PacketBufferTmSize> getPacketBufferTm();

    /**
     * @brief packetPtrBufferTm setter
     */
     void setPacketPtrBufferTm(etl::queue<std::pair<uint8_t *, uint16_t>, PacketBufferTmSize> &packetPtrBuffer);

     /**
      * @brief packetBufferTm setter
      */
      void setPacketBufferTm(etl::queue<uint8_t, PacketBufferTmSize> &packetBufferTm);
private:
    /**
     * @brief Buffer to storeOut incoming packets BEFORE being processed by COP
     */
    etl::list<PacketTC *, MaxReceivedTxTcInWaitQueue> txWaitQueue;

    /**
     * @brief Buffer to storeOut incoming packets AFTER being processed by COP
     */
    etl::list<PacketTC *, MaxReceivedTxTcInWaitQueue> rxWaitQueue;

    /**
	 * @brief Buffer to storeOut outcoming packets AFTER being processed by COP
	 */
    etl::list<PacketTC *, MaxReceivedTxTcInSentQueue> sentQueue;

    /**
     * @brief Buffer to storeOut unprocessed packets that are directly processed in the virtual instead of MAP channel
     */
    etl::list<PacketTC *, MaxReceivedUnprocessedTxTcInVirtBuffer> txUnprocessedPacketListBufferTC;

    /**
     * @brief Holds the FOP state of the virtual channel
     */
    FrameOperationProcedure fop;

    /**
     * @brief The Master Channel the Virtual Channel belongs in
     */
    std::reference_wrapper<MasterChannel> masterChannel;

    /**
     * @brief Queue that stores the pointers of the packets that will eventually be concatenated to transfer frame data.
     */
    etl::queue<std::pair<uint8_t *, uint16_t>, PacketBufferTmSize> packetPtrBufferTm;

    /**
     * @brief Queue that stores the packet data that will eventually be concatenated to transfer frame data
     */
    etl::queue<uint8_t, PacketBufferTmSize> packetBufferTm;

    /**
     * A Memory Pool that stores the packet data contiguously and returns a pointer to the data
     */
    MemoryPool virtualChannelPool = MemoryPool();
};

struct MasterChannel {
    friend class ServiceChannel;

    friend class FrameOperationProcedure;

    /**
     * @brief Virtual channels of the master channel
     */
    // TODO: Type aliases because this is getting out of hand
    etl::flat_map<uint8_t, VirtualChannel, MaxVirtualChannels> virtChannels;
    bool errorCtrlField;
    uint8_t frameCount{};

    MasterChannel(bool errorCtrlField, uint8_t frameCount)
            : virtChannels(), txOutFramesBeforeAllFramesGenerationList(),
              txToBeTransmittedFramesAfterAllFramesGenerationList(), errorCtrlField(errorCtrlField) {}

    MasterChannel(const MasterChannel &m)
            : virtChannels(m.virtChannels), errorCtrlField(m.errorCtrlField),
              frameCount(m.frameCount),
              txOutFramesBeforeAllFramesGenerationList(m.txOutFramesBeforeAllFramesGenerationList),
              txToBeTransmittedFramesAfterAllFramesGenerationList(
                      m.txToBeTransmittedFramesAfterAllFramesGenerationList) {
        for (auto &vc: virtChannels) {
            vc.second.masterChannel = *this;
        }
    }

    /**
     *
     * @param packet TC
     * @brief stores TC packet in txOutFramesBeforeAllFramesGenerationList in order to be processed by the All Frames Generation Service
     */
    MasterChannelAlert storeOut(PacketTC *packet);

    /**
     *
     * @param packet TC
     * @brief stores TC packet in txToBeTransmittedFramesAfterAllFramesGenerationList after it has been processed by the All Frames Generation Service
     */
    MasterChannelAlert storeTransmittedOut(PacketTC *packet);

    /**
     * @return
     */
    uint16_t availableSpaceBufferTM() const {
        return txMasterCopyTM.available();
    }

    /**
     * @brief Add virtual channel to master channel
     */
    MasterChannelAlert addVC(const uint8_t vcid, const bool segmentHeaderPresent, const uint16_t maxFrameLength,
                             const uint8_t clcwRate, const bool blocking, const uint8_t repetitionTypeAFrame,
                             const uint8_t repetitionCopCtrl, const uint8_t frameCount,
                             etl::flat_map<uint8_t, MAPChannel, MaxMapChannels> mapChan);

    /**
     * Method that merges the packets into TM transfer frame data
     * @param vc the virtual channel of the transfer frames
     * @param maxTransferFrameDataLength the maximum length of the TM transfer frame data. NOTE: this is supposed to be constant
     * but we added in case we want to modify it.
     */
    void mergePacketsToTransferFrame(VirtualChannel *vc, uint16_t maxTransferFrameDataLength);

private:
    // Packets stored in frames list, before being processed by the all frames generation service
    etl::list<PacketTC *, MaxReceivedTxTcInMasterBuffer> txOutFramesBeforeAllFramesGenerationList;
    // Packets ready to be transmitted having passed through the all frames generation service
    etl::list<PacketTC *, MaxReceivedTxTcOutInMasterBuffer> txToBeTransmittedFramesAfterAllFramesGenerationList;

    // Packets that are received, before being received by the all frames reception service
    etl::list<PacketTC *, MaxReceivedRxTcInMasterBuffer> rxInFramesBeforeAllFramesReceptionList;
    // Packets that are ready to be transmitted to higher procedures following all frames generation service
    etl::list<PacketTC *, MaxReceivedRxTcOutInMasterBuffer> rxToBeTransmittedFramesAfterAllFramesReceptionList;

    /**
     * @brief Buffer holding the master copy of TX packets that are currently being processed
     */
    etl::list<PacketTC, MaxTxInMasterChannel> txMasterCopyTC;

    /**
     * @brief Buffer holding the master copy of TX packets that are currently being processed
     */
    etl::list<PacketTM, MaxTxInMasterChannel> txMasterCopyTM;

    /**
     * @brief Buffer holding the master copy of RX packets that are currently being processed
     */
    etl::list<PacketTC, MaxRxInMasterChannel> rxMasterCopyTC;

    /**
     * @brief Queue that stores the pointers of the transfer frame data
     */
    etl::queue<uint8_t *, MasterTransferFrameBufferTmSize> masterTransferFramePtrBufferTm;

    /**
     * @brief Queue that stores the transfer frames
     */
    etl::queue<PacketTM, MasterTransferFrameBufferTmSize> masterTransferFrameBufferTm;

    /**
     * Queue that stores the transfer frame data
     */
    etl::queue<uint8_t, MasterTransferFrameBufferTmSize> masterTransferFrameDataBufferTm;

};

#endif // CCSDS_CHANNEL_HPP
