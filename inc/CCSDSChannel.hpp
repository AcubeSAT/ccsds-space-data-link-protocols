#pragma once

#include <cstdint>
#include <etl/map.h>
#include <etl/flat_map.h>
#include <etl/list.h>
#include <etl/queue.h>
#include <etl/deque.h>
#include <etl/circular_buffer.h>

#include <CCSDS_Definitions.hpp>
#include <FrameOperationProcedure.hpp>
#include <FrameAcceptanceReporting.hpp>
#include <TransferFrameTC.hpp>
#include <TransferFrameTM.hpp>
#include <iostream>
#include "MemoryPool.hpp"
#include "CLCW.hpp"

namespace CCSDSDataLinkLayer {
    class MasterChannel;

/**
 * @see Table 5-1 from TC SPACE DATA LINK PROTOCOL
 */
    struct PhysicalChannel {
    private:
        /**
         * Maximum length of a single transfer frame
         */
        const uint16_t maxFrameLength;

        /**
         * Sets the maximum number of transfer frames that can be transferred in a single data unit
         */
        const uint16_t maxFramePdu;

        /**
         * Maximum length of a data unit
         */
        const uint16_t maxPDULength;

        /**
         * Maximum bit rate (bits per second)
         */
        const uint32_t bitrate;

        /**
         * Maximum number of retransmissions for a data unit
         */
        const uint16_t repetitions;

    public:
        PhysicalChannel(const uint16_t maxFrameLength, const uint16_t maxFramesPdu, const uint16_t maxPduLength,
                        const uint32_t bitrate, const uint16_t repetitions)
                : maxFrameLength(maxFrameLength), maxFramePdu(maxFramesPdu), maxPDULength(maxPduLength),
                  bitrate(bitrate),
                  repetitions(repetitions) {}

        uint16_t getMaxFrameLength() const {
            return maxFrameLength;
        }

        /**
         * Empty default constructor
         */
        PhysicalChannel() : maxFrameLength(0), maxFramePdu(0), maxPDULength(0), bitrate(0), repetitions(0) {}

        /**
         * Sets the maximum number of transfer frames that can be transferred in a single data unit
         */
        uint16_t getMaxFramePdu() const {
            return maxFramePdu;
        };

        /**
         * Maximum length of a data unit
         */
        uint16_t getMaxPDULength() const {
            return maxPDULength;
        };

        /**
         * Maximum bit rate (bits per second)
         */
        uint32_t getBitrate() const {
            return bitrate;
        };

        /**
         * Maximum number of retransmissions for a data unit
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

    public:
        MAPChannel(const uint8_t mapid, bool blockingTC, bool segmentationTC)
                : MAPID(mapid), blockingTC(blockingTC), segmentationTC(segmentationTC) {
        };

    private:
        /**
         * MAP Channel Identifier
         */
        const uint8_t MAPID; // 6 bits

        /**
         * Determines whether smaller data units can be combined into a single TC transfer frame
         * (applies for Type AD, BD frames). Supersedes blockingTC flag of virtual channel.
         */
        const bool blockingTC;

        /**
         * Determines whether large packets can be segmented to multiple TC transfer frames
         * (applies for Type AD, BD)
         */
        const bool segmentationTC;

        /**
         * @brief Queue that stores the pointers of the packets that will eventually be concatenated to transfer frame data.
         * Applicable to Type-AD Frames
         */
        etl::queue<uint16_t, PacketBufferTcSize> packetLengthBufferTxTcTypeAD;

        /**
         * @brief Queue that stores the packets that will eventually be concatenated to transfer frame data.
         * Applicable to Type-AD Frames
         */
        etl::queue<uint8_t, PacketBufferTcSize> packetBufferTxTcTypeAD;
        /**
         * @brief Queue that stores the pointers of the packets that will eventually be concatenated to transfer frame data.
         * Applicable to Type-BD Frames
         */
        etl::queue<uint16_t, PacketBufferTcSize> packetLengthBufferTxTcTypeBD;

        /**
         * @brief Queue that stores the packets that will eventually be concatenated to transfer frame data.
         * Applicable to Type-BD Frames
         */
        etl::queue<uint8_t, PacketBufferTcSize> packetBufferTxTcTypeBD;

        /**
         * Buffers to hold created frame after SDLS processing
         */
        etl::list<TransferFrameTC *, MaxReceivedUnprocessedTxTcInVirtBuffer> framesAfterSDLSProcessingTypeADRxTC;
        etl::list<TransferFrameTC *, MaxReceivedUnprocessedTxTcInVirtBuffer> framesAfterSDLSProcessingTypeBDRxTC;

        /**
         * Used by the packet extraction function to temporarily hold TC frame with multiple packets (only one packet may
         * be returned at a time by the packet extraction function).
         */
        etl::optional<TransferFrameTC *> frameWithMultiplePacketsTypeADRxTC;
        etl::optional<TransferFrameTC *> frameWithMultiplePacketsTypeBDRxTC;

        /**
         * Indicate the position of the next packet to deliver to the user, inside frameWithMultiplePacketsType*DRxTC
         * Position 0 is the start of the frame.
         */
        uint8_t nextPacketPositionTypeAD;
        uint8_t nextPacketPositionTypeBD;

        /**
         * Used by the packet extraction function to temporarily hold frames that contain partial (segmented) packets.
         */
        etl::queue<TransferFrameTC *, MaxFramesWithSegmentedPackets> framesWithSegmentedPacketsTypeADRxTC;
        etl::queue<TransferFrameTC *, MaxFramesWithSegmentedPackets> framesWithSegmentedPacketsTypeBDRxTC;

        /**
         * Holds the sequence flag of the last frame placed inside framesWithSegmentedPacketsType*DRxTC.
         */
        SequenceFlags previousFrameSequenceFlagTypeAD = SequenceFlags::NoSegmentation;
        SequenceFlags previousFrameSequenceFlagTypeBD = SequenceFlags::NoSegmentation;
    };

/**
 * @see Table 5-3 from TC SPACE DATA LINK PROTOCOL
 */
    class VirtualChannel {
        friend class ServiceChannel;

    public:
        VirtualChannel(const uint8_t vcid, const bool segmentHeaderTCPresent, const uint16_t maxFrameLengthTC,
                       const bool blockingTM, const bool segmentationTM, const bool blockingTC,
                       const bool secondaryHeaderTMPresent, const uint8_t secondaryHeaderTMLength,
                       const bool operationalControlFieldTMPresent, bool frameErrorControlFieldPresent,
                       const SynchronizationFlag synchronization, const uint8_t farmSlidingWinWidth,
                       const uint8_t farmPositiveWinWidth, const uint8_t farmNegativeWinWidth,
                       const uint8_t vcRepetitions,
                       const etl::flat_map<uint8_t, MAPChannel, MaxMapChannels> mapChan,
                       etl::flat_map<uint8_t, etl::queue<CLCW, 1>, MaxVirtualChannels> &virtualChannelClcwQueues,
                       etl::list<TransferFrameTC, MaxRxInMasterChannel> &masterCopyRxTC,
                       etl::list<TransferFrameTC, MaxTxInMasterChannel> &masterCopyTxTC,
                       MemoryPool &memoryPoolTxTC,
                       MemoryPool &memoryPoolRxTC)
                : VCID(vcid & 0x3FU), GVCID((MCID << 0x06U) + VCID),
                  secondaryHeaderTMPresent(secondaryHeaderTMPresent), secondaryHeaderTMLength(secondaryHeaderTMLength),
                  segmentHeaderTCPresent(segmentHeaderTCPresent), maxFrameLengthTC(maxFrameLengthTC),
                  blockingTM(blockingTM),
                  segmentationTM(segmentationTM), blockingTC(blockingTC), vcRepetitions(vcRepetitions),
                  frameErrorControlFieldPresent(frameErrorControlFieldPresent),
                  operationalControlFieldTMPresent(operationalControlFieldTMPresent), synchronization(synchronization),
                  frameCountTM(0), mapChannels(mapChan), virtualChannelClcwQueues(virtualChannelClcwQueues),
                  fop(FrameOperationProcedure(VCID, frameErrorControlFieldPresent, masterCopyTxTC, memoryPoolTxTC)),
                  farm(FrameAcceptanceReporting(VCID, frameErrorControlFieldPresent, inFramesBeforeVcReceptionRxTC,
                                                inFramesAfterVCReceptionTypeBDRxTC,
                                                inFramesAfterVCReceptionTypeADRxTC, masterCopyRxTC, memoryPoolRxTC,
                                                virtualChannelClcwQueues.at(vcid), farmSlidingWinWidth,
                                                farmPositiveWinWidth,
                                                farmNegativeWinWidth)) {
        }

        VirtualChannel(const VirtualChannel &v)
                : VCID(v.VCID), GVCID(v.GVCID), segmentHeaderTCPresent(v.segmentHeaderTCPresent),
                  maxFrameLengthTC(v.maxFrameLengthTC), vcRepetitions(v.vcRepetitions), frameCountTM(v.frameCountTM),
                  unprocessedFrameListBufferTxTC(v.unprocessedFrameListBufferTxTC),
                  fop(v.fop), farm(v.farm), blockingTM(v.blockingTM), segmentationTM(v.segmentationTM),
                  blockingTC(v.blockingTC),
                  synchronization(v.synchronization), secondaryHeaderTMPresent(v.secondaryHeaderTMPresent),
                  secondaryHeaderTMLength(v.secondaryHeaderTMLength),
                  frameErrorControlFieldPresent(v.frameErrorControlFieldPresent),
                  operationalControlFieldTMPresent(v.operationalControlFieldTMPresent), mapChannels(v.mapChannels),
                  virtualChannelClcwQueues(v.virtualChannelClcwQueues) {
        }

    private:
        /**
         * @bried Add MAP channel to virtual channel
         */
        VirtualChannelAlert add_map(const uint8_t mapid);

        /**
         * Virtual Channel Identifier
         */
        const uint8_t VCID; // 6 bits

        /**
         * Global Virtual Channel Identifier
         */
        const uint16_t GVCID; // 16 bits (assumes TFVN is set to 0)

        /**
         * Determines whether the Segment Header is present (enables MAP services for type AD, BD packets)
         */
        const bool segmentHeaderTCPresent;

        /**
         * Maximum length of a single transfer frame
         */
        const uint16_t maxFrameLengthTC;

        /**
         * Determines whether smaller data units can be combined into a single TM transfer frame.
         */
        const bool blockingTM;

        /**
        * Determines whether large packets can be segmented to multiple TM transfer frames.
        */
        const bool segmentationTM;

        /**
         * Determines whether smaller data units can be combined into a single TC transfer frame.
         * (applies for Type AD, BD frames if a segmentHeader is not present)
         */
        const bool blockingTC;

        /**
         * Determines the number of times a frame will be repeated in transmission in the Physical Layer
         */
        const uint8_t vcRepetitions;

        /**
         * Determines the number of TM Transfer Frames transmitted
         */
        uint8_t frameCountTM;

        /**
         * Defines whether the OCF service is present
         */
        const bool operationalControlFieldTMPresent;

        /**
         * Defines whether the ECF service is present
         */
        const bool frameErrorControlFieldPresent;

        /**
         * Defines whether octet or forward-ordered synchronization is used
         */
        const SynchronizationFlag synchronization;

        /**
         * Indicates whether secondary header is present in this VC
         */
        const bool secondaryHeaderTMPresent;

        /**
         * Indicates the length of the secondary header for this VC. If secondary header is disabled for this VC,
         * it is ignored
         */
        const uint8_t secondaryHeaderTMLength;

        /**
         *
         *  MAP channels of the virtual channel
         */
        etl::flat_map<uint8_t, MAPChannel, MaxMapChannels> mapChannels;

        /**
         * References to certain master channel queues for pushing/popping clcws
         */
        etl::flat_map<uint8_t, etl::queue<CLCW, 1>, MaxVirtualChannels> &virtualChannelClcwQueues;

        /**
         * TM transfer frames after being processed by the MasterChannelReception Service
         */
        etl::list<TransferFrameTM *, MaxReceivedRxTmInVirtBuffer> framesAfterMcReceptionRxTM;

        /**
         * Buffer to store incoming transfer frames BEFORE being processed by COP
         */
        etl::list<TransferFrameTC *, MaxReceivedRxTcInWaitQueue> inFramesBeforeVcReceptionRxTC;

        /**
         * Buffer to store incoming TYPE-AD transfer frames AFTER being processed by FARM
         */
        etl::list<TransferFrameTC *, MaxReceivedRxTcInVirtualChannelBuffer> inFramesAfterVCReceptionTypeADRxTC;

        /**
         * Buffer to store incoming TYPE-BD transfer frames AFTER being processed by FARM
         */
        etl::circular_buffer<TransferFrameTC *, MaxReceivedRxTcInVirtualChannelBuffer> inFramesAfterVCReceptionTypeBDRxTC;

        /**
         * Buffer to store created frames during and after blocking and segmentation
         */
        etl::list<TransferFrameTC *, MaxReceivedUnprocessedTxTcInVirtBuffer> unprocessedFrameListBufferTxTC;

        /**
         * Buffer to hold created frame after packet processing and before SDLS processing
         */
        etl::list<TransferFrameTC *, MaxReceivedUnprocessedTxTcInVirtBuffer> framesBeforeSDLSProcessingTxTC;

        /**
         * Buffers to hold created frame after SDLS processing
         */
        etl::list<TransferFrameTC *, MaxReceivedUnprocessedTxTcInVirtBuffer> framesAfterSDLSProcessingTypeADRxTC;
        etl::list<TransferFrameTC *, MaxReceivedUnprocessedTxTcInVirtBuffer> framesAfterSDLSProcessingTypeBDRxTC;

        /**
         * Holds the FOP state of the virtual channel
         */
        FrameOperationProcedure fop;

        /**
         * Buffer holding the CLCW that is received
         */
        etl::list<CLCW, 1> receivedClcwBuffer;

        /**
         * Holds the FARM state of the virtual channel
         */
        FrameAcceptanceReporting farm;

        /**
         *  Queue that stores the pointers of the packets that will eventually be concatenated to TM transfer frame data.
         */
        etl::deque<uint16_t, PacketBufferTmSize> packetLengthBufferTxTM;

        /**
         *  Queue that stores the packet data that will eventually be concatenated to TM transfer frame data
         */
        etl::deque<uint8_t, PacketBufferTmSize> packetBufferTxTM;

        /**
         * @brief Queue that stores the pointers of the packets that will eventually be concatenated to TC transfer frame data.
         * Applicable to Type-AD Frames
         */
        etl::queue<uint16_t, PacketBufferTcSize> packetLengthBufferTxTcTypeAD;

        /**
         * @brief Queue that stores the packets that will eventually be concatenated to TC transfer frame data.
         * Applicable to Type-AD Frames
         */
        etl::queue<uint8_t, PacketBufferTcSize> packetBufferTxTcTypeAD;
        /**
         * @brief Queue that stores the pointers of the packets that will eventually be concatenated to TC transfer frame data.
         * Applicable to Type-BD Frames
         */
        etl::queue<uint16_t, PacketBufferTcSize> packetLengthBufferTxTcTypeBD;

        /**
         * @brief Queue that stores the packets that will eventually be concatenated to TC transfer frame data.
         * Applicable to Type-BD Frames
         */
        etl::queue<uint8_t, PacketBufferTcSize> packetBufferTxTcTypeBD;

        /**
         * Used by the packet extraction function to temporarily hold TC frame with multiple packets (only one packet may
         * be returned at a time by the packet extraction function).
         */
        etl::optional<TransferFrameTC *> frameWithMultiplePacketsTypeADRxTC;
        etl::optional<TransferFrameTC *> frameWithMultiplePacketsTypeBDRxTC;

        /**
         * Indicate the position of the next packet to deliver to the user, inside frameWithMultiplePacketsType*DRxTC
         * Position 0 is the start of the frame.
         */
        uint8_t nextPacketPositionTypeAD;
        uint8_t nextPacketPositionTypeBD;
    };

    struct MasterChannel {
        friend class ServiceChannel;

    public:
        MasterChannel()
                : virtualChannels(), outFramesBeforeAllFramesGenerationListTxTC(), currFrameCountTM(0) {}

        MasterChannel(const MasterChannel &m)
                : virtualChannels(m.virtualChannels), frameCount(m.frameCount),
                  outFramesBeforeAllFramesGenerationListTxTC(m.outFramesBeforeAllFramesGenerationListTxTC),
                  masterCopyRxTC(m.masterCopyRxTC), masterCopyRxTM(m.masterCopyRxTM),
                  currFrameCountTM(m.currFrameCountTM) {
        }


    private:
        /**
         * Virtual channels of the master channel
         */
        // TODO: Type aliases because this is getting out of hand
        etl::flat_map<uint8_t, VirtualChannel, MaxVirtualChannels> virtualChannels;

        /**
         * Queues that hold generated clcws from every virtual channel's FARM. The TM Link (sending side) pops and
         * places them inside the OCF field of TM frames.
         */
        etl::flat_map<uint8_t, etl::queue<CLCW, 1>, MaxVirtualChannels> virtualChannelClcwQueues;
        uint8_t frameCount{};

        /**
         *
         * @param transferFrameTm TM
         *  stores TM transfer frames in order to be processed by the All Frames Generation Service
         */
        MasterChannelAlert storeOut(TransferFrameTM *transferFrameTm);

        /**
         *
         * @param transferFrameTm TM
         *  stores TM transfer frames after they have been processed by the All Frames Generation Service
         */
        MasterChannelAlert storeTransmittedOut(TransferFrameTM *transferFrameTm);

        /**
         * Keeps track of last master channel frame count. If lost frames in a master channel are detected, then a warning
         * is logged. However, this isn't considered a reason for raising an error as per CCSDS TM Data Link.
         * Upon initialization of the channel, a MC count of 0 is expected.
         */
        uint8_t currFrameCountTM;

        /**
         *
         * @param transferFrameTc TC
         *  stores TC transfer frames in order to be processed by the All Frames Generation Service
         */
        MasterChannelAlert storeOut(TransferFrameTC *transferFrameTc);

        /**
         *
         * @param transferFrameTc TC
         *  stores TC transfer frames after they have been processed by the All Frames Generation Service
         */
        MasterChannelAlert storeTransmittedOut(TransferFrameTC *transferFrameTc);

        /**
         * Returns the last stored Transfer Frame in txMasterCopyTc
         */
        TransferFrameTC getLastTxMasterCopyTcFrame();

        /**
         * Returns the first stored Transfer Frame in masterCopyTxTC
         */
        TransferFrameTC geFirstTxMasterCopyTcFrame();

        /**
         * Add virtual channel to master channel
         */
        MasterChannelAlert
        addVC(const uint8_t vcid, const bool segmentHeaderPresent, const uint16_t maxFrameLength, const bool blockingTM,
              const bool segmentationTM, const bool blockingTC,
              const bool frameErrorControlFieldPresent, const bool secondaryHeaderTMPresent,
              const uint8_t secondaryHeaderTMLength, const bool operationalControlFieldTMPresent,
              SynchronizationFlag synchronization, const uint8_t farmSlidingWinWidth,
              const uint8_t farmPositiveWinWidth, const uint8_t farmNegativeWinWidth,
              const uint8_t vcRepetitions, const etl::flat_map<uint8_t, MAPChannel, MaxMapChannels> mapChan);

        /**
         * Add virtual channel to master channel
         */
        MasterChannelAlert
        addVC(const uint8_t vcid, const bool segmentHeaderPresent, const uint16_t maxFrameLength, const bool blockingTM,
              const bool segmentationTM, const bool blockingTC,
              const bool frameErrorControlFieldPresent, const bool secondaryHeaderTMPresent,
              const uint8_t secondaryHeaderTMLength, const bool operationalControlFieldTMPresent,
              SynchronizationFlag synchronization, const uint8_t farmSlidingWinWidth,
              const uint8_t farmPositiveWinWidth, const uint8_t farmNegativeWinWidth,
              const uint8_t vcRepetitions);

        // TC transfer frames stored in frames list, before being processed by the all frames generation service
        etl::list<TransferFrameTC *, MaxReceivedTxTcInMasterBuffer> outFramesBeforeAllFramesGenerationListTxTC;

        // TM transfer frames ready to be transmitted having passed through the vc generation service
        etl::list<TransferFrameTM *, MaxReceivedTxTmOutInVCBuffer> toBeTransmittedFramesAfterMCGenerationListTxTM;

        // Buffer to store TM transfer frames that are processed by VC Generation services
        etl::list<TransferFrameTM *, MaxReceivedUnprocessedTxTmInVirtBuffer> framesAfterVcGenerationServiceTxTM;

        /**
         * Buffer holding the master copy of TC TX transfer frames that are currently being processed
         */
        etl::list<TransferFrameTC, MaxTxInMasterChannel> masterCopyTxTC;

        /**
         * Removes TC transfer frames from the Tx master buffer
         */
        void removeMasterTx(TransferFrameTC *frame_ptr);

        /**
         * Buffer holding the master copy of TM TX transfer frames that are currently being processed
         */
        etl::list<TransferFrameTM, MaxTxInMasterChannel> masterCopyTxTM;

        /**
         * Removes TM transfer frames from the Tx master buffer
         */
        void removeMasterTx(TransferFrameTM *frame_ptr);

        /**
         * Buffer holding the master copy of TC RX transfer frames that are currently being processed (held up until
         * packet extraction, or discarded upon all frames generation in case they are invalid)
         */
        etl::list<TransferFrameTC, MaxRxInMasterChannel> masterCopyRxTC;

        /**
         * Removes TC transfer frames from the Rx master buffer
         */
        void removeMasterRx(TransferFrameTC *frame_ptr);

        /**
         * Buffer holding the master copy of TM RX transfer frames that are currently being processed
         */
        etl::list<TransferFrameTM, MaxRxInMasterChannel> masterCopyRxTM;

        /**
         * Removes TM frames from the RX master buffer
         */
        void removeMasterRx(TransferFrameTM *frame_ptr);

        /**
         * Sets the acknowledgement flag of a transfer frame to true
         */
        void acknowledgeFrame(uint8_t frameSequenceNumber);

        /**
         * Sets the toBeRetransmitted flag of a transfer frame to true
         */
        void setRetransmitFrame(uint8_t frameSequenceNumber);

        MemoryPool masterChannelPoolTxTM = MemoryPool();
        MemoryPool masterChannelPoolTxTC = MemoryPool();
        MemoryPool masterChannelPoolRxTC = MemoryPool();
    };

} // namespace CCSDSDataLinkLayer