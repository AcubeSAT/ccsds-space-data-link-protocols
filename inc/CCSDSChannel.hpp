/**
 * @file CCSDSChannel.hpp
 * @brief The data channels of the data link.
 * @details The channels is a concept that defines the flow of frames through the data link. In this implementation:
 *          Physical channel: Holds basic management parameters for the data link such as bitrate and maximum frame size.
 *                            Only one instance should exist.
 *          Master channel: Central channel. Contains some management parameters, and the frame master copies.
 *                          Only one instance should exist.
 *          Virtual channel: Multiple instances of virtual channels can exist under a master channel, allowing the user
 *                           to categorize TM and TC frames based on the type of information they carry.
 *          Map channel: Multiple instances of map channels can exist under a virtual channel. Their existence is optional.
 *                       They offer even more granularity for TC frames and the ability to segment telecommands to multiple
 *                       frames.
 */

#pragma once

#include <cstdint>
#include <iostream>
#include "etl/map.h"
#include "etl/flat_map.h"
#include "etl/list.h"
#include "etl/queue.h"
#include "etl/deque.h"
#include "etl/circular_buffer.h"
#include "CCSDS_Definitions.hpp"
#include "FrameOperationProcedure.hpp"
#include "FrameAcceptanceReporting.hpp"
#include "TransferFrameTC.hpp"
#include "TransferFrameTM.hpp"
#include "MemoryPool.hpp"
#include "CLCW.hpp"
#include "Alert.hpp"
#include "CCSDSLoggerImpl.h"

namespace CCSDSDataLinkLayer {

/**
 * @see Table 5-1 from TC SPACE DATA LINK PROTOCOL
 */
    struct PhysicalChannel {
    public:
        PhysicalChannel(const uint16_t maxFrameLength, const uint16_t maxFramesPdu, const uint16_t maxPduLength,
                        const uint32_t bitrate, const uint16_t repetitions)
                : maxFrameLength(maxFrameLength), maxFramePdu(maxFramesPdu), maxPDULength(maxPduLength),
                  bitrate(bitrate),
                  repetitions(repetitions) {}

        /**
         * @brief Empty default constructor.
         */
        PhysicalChannel() : maxFrameLength(0), maxFramePdu(0), maxPDULength(0), bitrate(0), repetitions(0) {}

        /**
         * @brief Get the maximum allowed frame length in this physical channel.
         */
        [[nodiscard]] uint16_t getMaxFrameLength() const {
            return maxFrameLength;
        }

        /**
         * @brief Sets the maximum number of transfer frames that can be transferred in a single data unit.
         */
        [[nodiscard]] uint16_t getMaxFramePdu() const {
            return maxFramePdu;
        };

        /**
         * @brief Maximum length of a data unit.
         */
        [[nodiscard]] uint16_t getMaxPDULength() const {
            return maxPDULength;
        };

        /**
         * @brief Maximum bit rate (bits per second).
         */
        [[nodiscard]] uint32_t getBitrate() const {
            return bitrate;
        };

        /**
         * @brief Maximum number of retransmissions for a data unit.
         */
        [[nodiscard]] uint16_t getRepetitions() const {
            return repetitions;
        }

    private:
        /**
         * @brief Maximum length of a single transfer frame.
         */
        const uint16_t maxFrameLength;

        /**
         * @brief Sets the maximum number of transfer frames that can be transferred in a single data unit.
         */
        const uint16_t maxFramePdu;

        /**
         * @brief Maximum length of a data unit.
         */
        const uint16_t maxPDULength;

        /**
         * @brief Maximum bit rate (bits per second).
         */
        const uint32_t bitrate;

        /**
         * @brief Maximum number of retransmissions for a data unit.
         */
        const uint16_t repetitions;
    };

    /**
     * Base map channel class containing parameters common among space and ground segment code
     */
    class BaseMAPChannel {
	    friend class BaseServiceChannel;
	    friend class ServiceChannelSpaceSegment;
	    friend class ServiceChannelGroundSegment;

    public:
	    BaseMAPChannel(const uint8_t mapid, bool blockingTC, bool segmentationTC)
                : mapId(mapid & 0x3F), blockingTC(blockingTC), segmentationTC(segmentationTC) {
        };

	    [[nodiscard]] uint8_t getMAPID() const {
		    return mapId;
	    }

    protected:
        /**
         * @brief MAP Channel Identifier.
         */
        const uint8_t mapId; // 6 bits

        /**
         * @brief Determines whether smaller data units can be combined into a single TC transfer frame
         * (applies for Type AD, BD frames). Supersedes blockingTC flag of virtual channel.
         */
        const bool blockingTC;

        /**
         * @brief Determines whether large packets can be segmented to multiple TC transfer frames
         * (applies for Type AD, BD).
         */
        const bool segmentationTC;
    };

	/**
	 * Base virtual channel class containing parameters common among space and ground segment code
	 */
    class BaseVirtualChannel {
	    friend class BaseServiceChannel;
	    friend class ServiceChannelSpaceSegment;
	    friend class ServiceChannelGroundSegment;

    public:
	    BaseVirtualChannel(const uint8_t vcid, const uint8_t vcRepetitions,
	                   bool frameErrorControlFieldPresent, const uint16_t maxFrameLengthTC,
	                   const bool segmentHeaderTCPresent, const bool blockingTC, const bool blockingTM,
	                   const bool segmentationTM, const bool operationalControlFieldTMPresent,
	                   const bool secondaryHeaderTMPresent, const uint8_t secondaryHeaderTMLength,
	                   const SynchronizationFlag synchronization)
	            : vcid(vcid & 0x3FU), vcRepetitions(vcRepetitions), frameErrorControlFieldPresent(frameErrorControlFieldPresent),
	              maxFrameLengthTC(maxFrameLengthTC), segmentHeaderTCPresent(segmentHeaderTCPresent),
	              blockingTC(blockingTC), blockingTM(blockingTM), segmentationTM(segmentationTM),
	              operationalControlFieldTMPresent(operationalControlFieldTMPresent),
	              secondaryHeaderTMLength(secondaryHeaderTMLength), secondaryHeaderTMPresent(secondaryHeaderTMPresent),
	              synchronizationTM(synchronization) {};

	    BaseVirtualChannel(const BaseVirtualChannel&v)
                : vcid(v.vcid), vcRepetitions(v.vcRepetitions),
	              frameErrorControlFieldPresent(v.frameErrorControlFieldPresent),
	              maxFrameLengthTC(v.maxFrameLengthTC), segmentHeaderTCPresent(v.segmentHeaderTCPresent),
	              blockingTC(v.blockingTC), blockingTM(v.blockingTM), segmentationTM(v.segmentationTM),
	              operationalControlFieldTMPresent(v.operationalControlFieldTMPresent),
	              secondaryHeaderTMPresent(v.secondaryHeaderTMPresent),
	              secondaryHeaderTMLength(v.secondaryHeaderTMLength), synchronizationTM(v.synchronizationTM) {};

	    [[nodiscard]] uint8_t getVCID() const {
		    return vcid;
	    }

	protected:
        /**
         * @brief Virtual Channel Identifier.
         */
        const uint8_t vcid; // 6 bits

        /**
         * @brief Determines the number of times a frame will be repeated in transmission in the Physical Layer.
         * @TODO ??
         */
        const uint8_t vcRepetitions;

        /**
         * @brief Defines whether the ECF service is present in transfer frames.
         */
        const bool frameErrorControlFieldPresent;

        /**
         * @brief Maximum length of a single TC transfer frame
         */
        const uint16_t maxFrameLengthTC;

        /**
         * @brief Determines whether the Segment Header field is present if TC transfer frames
         * (enables MAP services for Type-AD/BD packets).
         */
        const bool segmentHeaderTCPresent;

        /**
         * @brief Determines whether smaller data units can be combined into a single TC transfer frame.
         * (applies for Type-AD/BD frames in case MAP services are disabled).
         */
        const bool blockingTC;

        /**
         * @brief Determines whether smaller data units can be combined into a single TM transfer frame.
         */
        const bool blockingTM;

        /**
        * @brief Determines whether large packets can be segmented to multiple TM transfer frames.
        */
        const bool segmentationTM;

        /**
         * @brief Defines whether the OCF field is present in TM transfer frames.
         */
        const bool operationalControlFieldTMPresent;

        /**
         * @brief Indicates whether secondary header field is present in TM transfer frames.
         */
        const bool secondaryHeaderTMPresent;

        /**
         * @brief Indicates the length of the secondary header for this VC. If the secondary header is disabled for this VC,
         * it is ignored.
         */
        const uint8_t secondaryHeaderTMLength;

        /**
         * @brief Defines whether octet and forward-ordered synchronization is used for TM transfer frames.
         */
        const SynchronizationFlag synchronizationTM;
    };

    /**
	 * Base virtual channel class containing parameters common among space and ground segment code
     */
	class BaseMasterChannel {
	    friend class BaseServiceChannel;
	    friend class ServiceChannelSpaceSegment;
	    friend class ServiceChannelGroundSegment;

	public:
	    explicit BaseMasterChannel(uint16_t scid) : scid(scid & 0x03FF) {}

	    [[nodiscard]] uint16_t getSCID() const {
		    return scid;
	    }

	protected:
	    const uint16_t scid; // 10 bits
	};

#ifdef SPACE_SEGMENT
    class MAPChannelSpaceSegment : public BaseMAPChannel {
	    friend class BaseServiceChannel;
	    friend class ServiceChannelSpaceSegment;
	    friend class ServiceChannelGroundSegment;
	public:
	    MAPChannelSpaceSegment(const uint8_t mapid, bool blockingTC, bool segmentationTC) :
	        BaseMAPChannel(mapid, blockingTC, segmentationTC), nextPacketPositionTypeAD(0), nextPacketPositionTypeBD(0),
	        previousFrameSequenceFlagTypeAD(SequenceFlags::NoSegmentation),
	        previousFrameSequenceFlagTypeBD(SequenceFlags::NoSegmentation) {}

	    MAPChannelSpaceSegment(const MAPChannelSpaceSegment& m):
	          BaseMAPChannel(m), framesAfterSDLSProcessingTypeADRxTC(m.framesAfterSDLSProcessingTypeADRxTC),
	          framesAfterSDLSProcessingTypeBDRxTC(m.framesAfterSDLSProcessingTypeBDRxTC),
	          frameWithMultiplePacketsTypeADRxTC(m.frameWithMultiplePacketsTypeADRxTC),
	          frameWithMultiplePacketsTypeBDRxTC(m.frameWithMultiplePacketsTypeBDRxTC),
	          nextPacketPositionTypeAD(m.nextPacketPositionTypeAD), nextPacketPositionTypeBD(m.nextPacketPositionTypeBD),
	          framesWithSegmentedPacketsTypeADRxTC(m.framesWithSegmentedPacketsTypeADRxTC),
	          framesWithSegmentedPacketsTypeBDRxTC(m.framesWithSegmentedPacketsTypeBDRxTC),
	          previousFrameSequenceFlagTypeAD(m.previousFrameSequenceFlagTypeAD),
	          previousFrameSequenceFlagTypeBD(m.previousFrameSequenceFlagTypeBD) {}

	private:
	    /**
         * @brief Buffer to hold pointers to Type-AD frames after they are processed by SDLS.
	     */
	    etl::list<TransferFrameTC *, MaxReceivedUnprocessedTxTcInVirtBuffer> framesAfterSDLSProcessingTypeADRxTC;

	    /**
         * @brief Buffer to hold pointers to Type-BD frames after they are processed by SDLS.
	     */
	    etl::list<TransferFrameTC *, MaxReceivedUnprocessedTxTcInVirtBuffer> framesAfterSDLSProcessingTypeBDRxTC;

	    /**
         * @brief Used by the packet extraction function to temporarily hold a pointer to a Type-AD frame with multiple packets
         * (the packet extraction function should return only one packet per call).
	     */
	    etl::optional<TransferFrameTC *> frameWithMultiplePacketsTypeADRxTC;

	    /**
         * @brief Used by the packet extraction function to temporarily hold a pointer to a Type-BD frame with multiple packets
         * (the packet extraction function shall return only one packet per call).
	     */
	    etl::optional<TransferFrameTC *> frameWithMultiplePacketsTypeBDRxTC;

	    /**
         * @brief Indicates the position of the next packet to deliver to the user, inside
         * frameWithMultiplePacketsTypeADRxTC. Position 0 is the start of the frame.
	     */
	    uint8_t nextPacketPositionTypeAD;

	    /**
         * @brief Indicates the position of the next packet to deliver to the user, inside
         * frameWithMultiplePacketsTypeBDRxTC. Position 0 is the start of the frame.
	     */
	    uint8_t nextPacketPositionTypeBD;

	    /**
         * @brief Queue used by the packet extraction function to temporarily hold pointers to Type-AD frames, that
         * contain partial (segmented) packets (the packet extraction function shall return complete packets).
	     */
	    etl::queue<TransferFrameTC *, MaxFramesWithSegmentedPackets> framesWithSegmentedPacketsTypeADRxTC;

	    /**
         * @brief Queue used by the packet extraction function to temporarily hold pointers to Type-AD frames, that
         * contain partial (segmented) packets (the packet extraction function shall return complete packets).
	     */
	    etl::queue<TransferFrameTC *, MaxFramesWithSegmentedPackets> framesWithSegmentedPacketsTypeBDRxTC;

	    /**
         * @brief Stores the sequence flag value of the last frame placed inside framesWithSegmentedPacketsTypeADRxTC.
	     */
	    SequenceFlags previousFrameSequenceFlagTypeAD;

	    /**
         * @brief Stores the sequence flag value of the last frame placed inside framesWithSegmentedPacketsTypeBDRxTC.
	     */
	    SequenceFlags previousFrameSequenceFlagTypeBD;

// Allow access to internal buffers when unit testing
#ifdef ENABLE_BUFFER_ACCESS
	public:
	    etl::list<TransferFrameTC *, MaxReceivedUnprocessedTxTcInVirtBuffer>& getFramesAfterSDLSProcessingTypeADRxTC() {
		    return framesAfterSDLSProcessingTypeADRxTC;
	    }

	    etl::list<TransferFrameTC *, MaxReceivedUnprocessedTxTcInVirtBuffer>& getFramesAfterSDLSProcessingTypeBDRxTC() {
		    return framesAfterSDLSProcessingTypeBDRxTC;
	    }

	    etl::queue<TransferFrameTC *, MaxFramesWithSegmentedPackets>& getFramesWithSegmentedPacketsTypeADRxTC() {
		    return framesWithSegmentedPacketsTypeADRxTC;
	    }

	    etl::queue<TransferFrameTC *, MaxFramesWithSegmentedPackets>& getFramesWithSegmentedPacketsTypeBDRxTC() {
		    return framesWithSegmentedPacketsTypeBDRxTC;
	    }
#endif // ENABLE_BUFFER_ACCESS
    };

class VirtualChannelSpaceSegment : public BaseVirtualChannel {
	friend class BaseServiceChannel;
	friend class ServiceChannelSpaceSegment;
	friend class ServiceChannelGroundSegment;
	public:
	    VirtualChannelSpaceSegment(const uint8_t vcid, const uint8_t vcRepetitions,
	                   bool frameErrorControlFieldPresent, const uint16_t maxFrameLengthTC,
	                   const bool segmentHeaderTCPresent, const bool blockingTC, const bool blockingTM,
	                   const bool segmentationTM, const bool operationalControlFieldTMPresent,
	                   const bool secondaryHeaderTMPresent, const uint8_t secondaryHeaderTMLength,
	                   const SynchronizationFlag synchronization,
	                   etl::flat_map<uint8_t, etl::queue<CLCW, 1>, MaxVirtualChannels>& virtualChannelClcwQueues,
	                   etl::list<TransferFrameTC, MaxRxInMasterChannel>& masterCopyRxTC,
	                   MemoryPool& memoryPoolRxTC,
	                   const uint8_t farmSlidingWinWidth,
	                   const uint8_t farmPositiveWinWidth,
	                   const uint8_t farmNegativeWinWidth)
	        : BaseVirtualChannel(vcid, vcRepetitions, frameErrorControlFieldPresent,
	          				 maxFrameLengthTC, segmentHeaderTCPresent, blockingTC, blockingTM, segmentationTM,
	          				 operationalControlFieldTMPresent, secondaryHeaderTMLength, secondaryHeaderTMPresent,
	                         synchronization),
	          virtualChannelClcwQueues(virtualChannelClcwQueues),frameCountTM(0), nextPacketPositionTypeAD(0),
	          nextPacketPositionTypeBD(0),
	          farm(FrameAcceptanceReporting(vcid, frameErrorControlFieldPresent, inFramesBeforeVcReceptionRxTC,
	                                        inFramesAfterVCReceptionTypeBDRxTC,
	                                        inFramesAfterVCReceptionTypeADRxTC, masterCopyRxTC, memoryPoolRxTC,
	                                        farmSlidingWinWidth,
	                                        farmPositiveWinWidth,
	                                        farmNegativeWinWidth)) {
		    virtualChannelClcwQueues.emplace(vcid, etl::queue<CLCW, 1>());
		    farm.registerClcwOutputBuffer(&virtualChannelClcwQueues.at(vcid));
	    }

	    VirtualChannelSpaceSegment(const VirtualChannelSpaceSegment& v)
	    : BaseVirtualChannel(v), virtualChannelClcwQueues(v.virtualChannelClcwQueues), frameCountTM(v.frameCountTM),
	      farm(v.farm), inFramesBeforeVcReceptionRxTC(v.inFramesBeforeVcReceptionRxTC),
	      inFramesAfterVCReceptionTypeADRxTC(v.inFramesAfterVCReceptionTypeADRxTC),
	      inFramesAfterVCReceptionTypeBDRxTC(v.inFramesAfterVCReceptionTypeBDRxTC),
	      framesAfterSDLSProcessingTypeADRxTC(v.framesAfterSDLSProcessingTypeADRxTC),
	      framesAfterSDLSProcessingTypeBDRxTC(v.framesAfterSDLSProcessingTypeBDRxTC),
	      frameWithMultiplePacketsTypeADRxTC(v.frameWithMultiplePacketsTypeADRxTC),
	      frameWithMultiplePacketsTypeBDRxTC(v.frameWithMultiplePacketsTypeBDRxTC),
	      nextPacketPositionTypeAD(v.nextPacketPositionTypeAD), nextPacketPositionTypeBD(v.nextPacketPositionTypeBD),
	      packetLengthBufferTxTM(v.packetLengthBufferTxTM), packetBufferTxTM(v.packetBufferTxTM) {}

	    VirtualChannelAlert addMAPChannel(const MAPChannelSpaceSegment& m) {
		    if (mapChannels.full()) {
			    ccsdsLogNotice(TxRx::Tx, NotificationType::TypeVirtualChannelAlert,
			                   VirtualChannelAlert::MAX_AMOUNT_OF_MAP_CHANNELS);
			    return VirtualChannelAlert::MAX_AMOUNT_OF_MAP_CHANNELS;
		    }

		    mapChannels.emplace(m.getMAPID(), m);
		    return VirtualChannelAlert::NO_VC_ALERT;
	    }

	private:
	    etl::flat_map<uint8_t, MAPChannelSpaceSegment, MaxMapChannels> mapChannels;

	    /**
		 * @brief Reference to the master channel's map structure that stores FARM-1 generated CLCWs.
	     */
	    etl::flat_map<uint8_t, etl::queue<CLCW, 1>, MaxVirtualChannels>& virtualChannelClcwQueues;

	    /**
         * @brief Counter for the amount of TM transfer frames transmitted.
	     */
	    uint8_t frameCountTM;

	    /**
         * @brief Object that stores and manages the FARM state of the virtual channel.
	     */
	    FrameAcceptanceReporting farm;

	    /**
         * @brief Buffer that stores pointers to transfer frames after allFramesReception and before vcReception (FARM
         * processing).
	     */
	    etl::list<TransferFrameTC *, MaxReceivedRxTcInWaitQueue> inFramesBeforeVcReceptionRxTC;

	    /**
         * @brief Buffer that stores pointers to Type-AD transfer frames after vcReception (FARM processing) and before
         * SDLS processing.
	     */
	    etl::list<TransferFrameTC *, MaxReceivedRxTcInVirtualChannelBuffer> inFramesAfterVCReceptionTypeADRxTC;

	    /**
         * @brief Buffer that stores pointers to Type-BD transfer frames after vcReception (FARM processing) and before
         * SDLS processing.
	     */
	    etl::circular_buffer<TransferFrameTC *, MaxReceivedRxTcInVirtualChannelBuffer> inFramesAfterVCReceptionTypeBDRxTC;

	    /**
         * @brief Buffer that stores pointers to Type-AD transfer after SDLS processing and before packet extraction.
	     */
	    etl::list<TransferFrameTC *, MaxReceivedUnprocessedTxTcInVirtBuffer> framesAfterSDLSProcessingTypeADRxTC;

	    /**
         * @brief Buffer that stores pointers to Type-BD transfer after SDLS processing and before packet extraction.
	     */
	    etl::list<TransferFrameTC *, MaxReceivedUnprocessedTxTcInVirtBuffer> framesAfterSDLSProcessingTypeBDRxTC;

	    /**
         * @brief This variable is used by the packet extraction function and temporarily stores a pointer to a Type-AD
         * transfer frame that stores more than one packet.
	     */
	    etl::optional<TransferFrameTC *> frameWithMultiplePacketsTypeADRxTC;

	    /**
         * @brief This variable is used by the packet extraction function and temporarily stores a pointer to a Type-BD
         * transfer frame that stores more than one packet.
	     */
	    etl::optional<TransferFrameTC *> frameWithMultiplePacketsTypeBDRxTC;

	    /**
         * @brief This variable indicates the position to the next packet, in frameWithMultiplePacketsTypeADRxTC.
         * @details Position 0 is defined as the first octet of the frame.
	     */
	    uint8_t nextPacketPositionTypeAD;

	    /**
         * @brief This variable indicates the position to the next packet, in frameWithMultiplePacketsTypeBDRxTC.
         * @details Position 0 is defined as the first octet of the frame.
	     */
	    uint8_t nextPacketPositionTypeBD;

	    /**
         *  @brief Queue that stores the pointers of the packets that will eventually be concatenated to TM transfer frame data.
	     */
	    etl::deque<uint16_t, PacketBufferTmSize> packetLengthBufferTxTM;

	    /**
         *  @brief Queue that stores the packet data that will eventually be concatenated to TM transfer frame data
	     */
	    etl::deque<uint8_t, PacketBufferTmSize> packetBufferTxTM;

// Allow access to internal buffers when unit testing
#ifdef ENABLE_BUFFER_ACCESS
	public:
	etl::list<TransferFrameTC *, MaxReceivedRxTcInWaitQueue>& getInFramesBeforeVcReceptionRxTC() {
		return inFramesBeforeVcReceptionRxTC;
	}

	etl::list<TransferFrameTC *, MaxReceivedRxTcInVirtualChannelBuffer>& getInFramesAfterVCReceptionTypeADRxTC() {
		return inFramesAfterVCReceptionTypeADRxTC;
	}

	etl::circular_buffer<TransferFrameTC *, MaxReceivedRxTcInVirtualChannelBuffer>& getInFramesAfterVCReceptionTypeBDRxTC() {
		return inFramesAfterVCReceptionTypeBDRxTC;
	}

	etl::list<TransferFrameTC *, MaxReceivedUnprocessedTxTcInVirtBuffer>& getFramesAfterSDLSProcessingTypeADRxTC() {
		return framesAfterSDLSProcessingTypeADRxTC;
	}

	etl::list<TransferFrameTC *, MaxReceivedUnprocessedTxTcInVirtBuffer>& getFramesAfterSDLSProcessingTypeBDRxTC() {
		return framesAfterSDLSProcessingTypeBDRxTC;
	}

	etl::optional<TransferFrameTC *>& getFrameWithMultiplePacketsTypeADRxTC() {
		return frameWithMultiplePacketsTypeADRxTC;
	}

	etl::optional<TransferFrameTC *>& getFrameWithMultiplePacketsTypeBDRxTC() {
		return frameWithMultiplePacketsTypeBDRxTC;
	}

	uint8_t getNextPacketPositionTypeAD() {
		return nextPacketPositionTypeAD;
	}

	uint8_t getNextPacketPositionTypeBD() {
		return nextPacketPositionTypeBD;
	}

	etl::deque<uint16_t, PacketBufferTmSize>& getPacketLengthBufferTxTM() {
		return packetLengthBufferTxTM;
	}

	etl::deque<uint8_t, PacketBufferTmSize>& getPacketBufferTxTM() {
		return packetBufferTxTM;
	}
#endif // ENABLE_BUFFER_ACCESS
    };

    class MasterChannelSpaceSegment : public BaseMasterChannel {
	    friend class BaseServiceChannel;
	    friend class ServiceChannelSpaceSegment;
	    friend class ServiceChannelGroundSegment;
	public:
	    explicit MasterChannelSpaceSegment(uint16_t scid) : BaseMasterChannel(scid), masterChannelFrameCountTM(0),
	          masterChannelPoolRxTC(MemoryPool()), masterChannelPoolTxTM(MemoryPool()) {}

	    MasterChannelSpaceSegment(const MasterChannelSpaceSegment& m)
	        : BaseMasterChannel(m), virtualChannels(m.virtualChannels), virtualChannelClcwQueues(m.virtualChannelClcwQueues),
	          masterCopyRxTC(m.masterCopyRxTC), masterChannelPoolRxTC(m.masterChannelPoolRxTC),
	          masterChannelFrameCountTM(m.masterChannelFrameCountTM),
	          framesAfterVcGenerationServiceTxTM(m.framesAfterVcGenerationServiceTxTM),
	          toBeTransmittedFramesAfterMCGenerationListTxTM(m.toBeTransmittedFramesAfterMCGenerationListTxTM),
	          masterCopyTxTM(m.masterCopyTxTM), masterChannelPoolTxTM(m.masterChannelPoolTxTM) {}

		MasterChannelAlert addVirtualChannel(const VirtualChannelSpaceSegment& v) {
			if (virtualChannels.full()) {
			    ccsdsLogNotice(TxRx::Tx, NotificationType::TypeMasterChannelAlert, MasterChannelAlert::MAX_AMOUNT_OF_VIRT_CHANNELS);
			    return MasterChannelAlert::MAX_AMOUNT_OF_VIRT_CHANNELS;
		    }

		    virtualChannels.emplace(v.getVCID(), v);
		    return MasterChannelAlert::NO_MC_ALERT;
		}

	private:
	    etl::flat_map<uint8_t, VirtualChannelSpaceSegment, MaxVirtualChannels> virtualChannels;

	    /**
		 * @brief A map of queues that hold generated CLCWs from every virtual channel's FARM. The TM Link
		 * (TxTM chain) pops and places them inside the OCF field of TM frames.
	     */
	    etl::flat_map<uint8_t, etl::queue<CLCW, 1>, MaxVirtualChannels> virtualChannelClcwQueues;

	    /**
		 * @brief Buffer that stores the actual TC transfer frame objects for the RxTC chain.
	     */
	    etl::list<TransferFrameTC, MaxRxInMasterChannel> masterCopyRxTC;

	    /**
		 * @brief Remove TC a transfer frame object in the RxTC chain.
	     */
	    void removeMasterRxTC(TransferFrameTC *frame_ptr);

	    /**
		 * @brief An object that manages a statically allocated block of memory, storing the octets for TC frames in the
		 * RxTC chain.
	     */
	    MemoryPool masterChannelPoolRxTC;

	    /**
		 * @brief A counter that keeps track the number of TM transfer frames transmitted from this master channel. The
		 * master channel frame count is carried by TM transfer frames, hence the receiving side can deduce if frames
		 * were lost.
		 *
		 * @details The initial value of this counter should be zero.
	     */
	    uint8_t masterChannelFrameCountTM;

	    /**
		 * @brief Buffer that stores pointers to TM frames after vcGeneration and before mcGeneration.
	     */
	    etl::list<TransferFrameTM *, MaxReceivedUnprocessedTxTmInVirtBuffer> framesAfterVcGenerationServiceTxTM;

	    /**
		 * @brief Buffer that stores pointers to TM frames after mcGeneration and before allFramesGeneration.
	     */
	    etl::list<TransferFrameTM *, MaxReceivedTxTmOutInVCBuffer> toBeTransmittedFramesAfterMCGenerationListTxTM;

	    /**
		 * @brief Buffer that stores the actual TM transfer frame objects for the TxTM chain.
	     */
	    etl::list<TransferFrameTM, MaxTxInMasterChannel> masterCopyTxTM;

	    /**
		 * @brief Remove TM a transfer frame object in the TxTM chain.
	     */
	    void removeMasterTxTM(TransferFrameTM *frame_ptr);

	    /**
		 * @brief An object that manages a statically allocated block of memory, storing the octets for TM frames in the
		 * TxTM chain.
	     */
	    MemoryPool masterChannelPoolTxTM;

// Allow access to internal buffers when unit testing
#ifdef ENABLE_BUFFER_ACCESS
	public:
	    etl::flat_map<uint8_t, VirtualChannelSpaceSegment, MaxVirtualChannels>& getVirtualChannels() {
	        return virtualChannels;
	    };
	    etl::flat_map<uint8_t, etl::queue<CLCW, 1>, MaxVirtualChannels>& getVirtualChannelClcwQueues() {
		    return virtualChannelClcwQueues;
	    };
	    etl::list<TransferFrameTC, MaxRxInMasterChannel>& getMasterCopyRxTC() {
		    return masterCopyRxTC;
	    }
	    MemoryPool& getMasterChannelPoolRxTC() {
		    return masterChannelPoolRxTC;
	    } 
	    etl::list<TransferFrameTM *, MaxReceivedUnprocessedTxTmInVirtBuffer>& getFramesAfterVcGenerationServiceTxTM() {
		    return framesAfterVcGenerationServiceTxTM;
	    }
	    etl::list<TransferFrameTM *, MaxReceivedTxTmOutInVCBuffer>& getToBeTransmittedFramesAfterMCGenerationListTxTM() {
		    return toBeTransmittedFramesAfterMCGenerationListTxTM;
	    }
	    etl::list<TransferFrameTM, MaxTxInMasterChannel>& getMasterCopyTxTM() {
		    return masterCopyTxTM;
	    }
	    MemoryPool& getMasterChannelPoolTxTM() {
		    return masterChannelPoolTxTM;
	    } 
#endif // ENABLE_BUFFER_ACCESS
    };
#endif // SPACE_SEGMENT

#ifdef GROUND_SEGMENT
    class MAPChannelGroundSegment : public BaseMAPChannel {
	    friend class BaseServiceChannel;
	    friend class ServiceChannelSpaceSegment;
	    friend class ServiceChannelGroundSegment;
	public:
	    MAPChannelGroundSegment(const uint8_t mapid, bool blockingTC, bool segmentationTC) :
	          BaseMAPChannel(mapid, blockingTC, segmentationTC) {}

	    MAPChannelGroundSegment(const MAPChannelGroundSegment& m)
	        : BaseMAPChannel(m), packetLengthBufferTxTcTypeAD(m.packetLengthBufferTxTcTypeAD),
	          packetBufferTxTcTypeAD(m.packetBufferTxTcTypeAD),
	          packetLengthBufferTxTcTypeBD(m.packetLengthBufferTxTcTypeBD),
	          packetBufferTxTcTypeBD(m.packetBufferTxTcTypeBD) {}

	private:
	    /**
         * @brief Queue that stores lengths of packets that will eventually be concatenated to Type-AD transfer frame data.
	     */
	    etl::queue<uint16_t, PacketBufferTcSize> packetLengthBufferTxTcTypeAD;

	    /**
         * @brief Queue that stores the bytes of packets that will eventually be concatenated to Type-AD transfer frame data.
	     */
	    etl::queue<uint8_t, PacketBufferTcSize> packetBufferTxTcTypeAD;

	    /**
         * @brief Queue that stores lengths of packets that will eventually be concatenated to Type-BD transfer frame data.
	     */
	    etl::queue<uint16_t, PacketBufferTcSize> packetLengthBufferTxTcTypeBD;

	    /**
         * @brief Queue that stores the bytes of packets that will eventually be concatenated to Type-BD transfer frame data.
	     */
	    etl::queue<uint8_t, PacketBufferTcSize> packetBufferTxTcTypeBD;

// Allow access to internal buffers when unit testing
#ifdef ENABLE_BUFFER_ACCESS
	public:
	    etl::queue<uint16_t, PacketBufferTcSize>& getPacketLengthBufferTxTcTypeAD() {
		    return packetLengthBufferTxTcTypeAD;
	    }

	    etl::queue<uint8_t, PacketBufferTcSize>& getPacketBufferTxTcTypeAD() {
		    return packetBufferTxTcTypeAD;
	    }

	    etl::queue<uint16_t, PacketBufferTcSize>& getPacketLengthBufferTxTcTypeBD() {
		    return packetLengthBufferTxTcTypeBD;
	    }

	    etl::queue<uint8_t, PacketBufferTcSize>& getPacketBufferTxTcTypeBD() {
		    return packetBufferTxTcTypeBD;
	    }
#endif // ENABLE_BUFFER_ACCESS
    };

	class VirtualChannelGroundSegment : public BaseVirtualChannel {
	    friend class BaseServiceChannel;
	    friend class ServiceChannelSpaceSegment;
	    friend class ServiceChannelGroundSegment;
	public:
	    VirtualChannelGroundSegment(const uint8_t vcid,
	                               const etl::flat_map<uint8_t, BaseMAPChannel, MaxMapChannels>& mapChan, const uint8_t vcRepetitions,
	                               bool frameErrorControlFieldPresent, const uint16_t maxFrameLengthTC,
	                               const bool segmentHeaderTCPresent, const bool blockingTC, const bool blockingTM,
	                               const bool segmentationTM, const bool operationalControlFieldTMPresent,
	                               const bool secondaryHeaderTMPresent, const uint8_t secondaryHeaderTMLength,
	                               const SynchronizationFlag synchronization,
	                               etl::list<TransferFrameTC, MaxTxInMasterChannel>& masterCopyTxTC,
	                               MemoryPool& memoryPoolTxTC)
	        : BaseVirtualChannel(vcid, vcRepetitions, frameErrorControlFieldPresent,
	                         maxFrameLengthTC, segmentHeaderTCPresent, blockingTC, blockingTM, segmentationTM,
	                         operationalControlFieldTMPresent, secondaryHeaderTMLength, secondaryHeaderTMPresent,
	                         synchronization),
	          fop(FrameOperationProcedure(vcid, frameErrorControlFieldPresent, masterCopyTxTC, memoryPoolTxTC)) {}

	    VirtualChannelGroundSegment(const VirtualChannelGroundSegment& v)
	        : BaseVirtualChannel(v), fop(v.fop), packetLengthBufferTxTcTypeAD(v.packetLengthBufferTxTcTypeAD),
	          packetBufferTxTcTypeAD(v.packetBufferTxTcTypeAD),
	          packetLengthBufferTxTcTypeBD(v.packetLengthBufferTxTcTypeBD),
	          packetBufferTxTcTypeBD(v.packetBufferTxTcTypeBD),
	          framesBeforeSDLSProcessingTxTC(v.framesBeforeSDLSProcessingTxTC),
	          unprocessedFrameListBufferTxTC(v.unprocessedFrameListBufferTxTC) {}

	    VirtualChannelAlert addMAPChannel(const MAPChannelGroundSegment& m) {
		    if (mapChannels.full()) {
			    ccsdsLogNotice(TxRx::Tx, NotificationType::TypeVirtualChannelAlert,
			                   VirtualChannelAlert::MAX_AMOUNT_OF_MAP_CHANNELS);
			    return VirtualChannelAlert::MAX_AMOUNT_OF_MAP_CHANNELS;
		    }

		    mapChannels.emplace(m.getMAPID(), m);
		    return VirtualChannelAlert::NO_VC_ALERT;
	    }

	private:
	    etl::flat_map<uint8_t, MAPChannelGroundSegment, MaxMapChannels> mapChannels;

	    /**
	     * @brief Object that stores and manages the FOP state of the virtual channel.
	     */
	    FrameOperationProcedure fop;

	    /**
         * @brief Queue that stores the lengths of the packets that will eventually be concatenated
         * to Type-AD transfer frame data.
	     */
	    etl::queue<uint16_t, PacketBufferTcSize> packetLengthBufferTxTcTypeAD;

	    /**
         * @brief Queue that stores the bytes of the packets that will eventually be concatenated to
         * Type-AD transfer frame data.
	     */
	    etl::queue<uint8_t, PacketBufferTcSize> packetBufferTxTcTypeAD;

	    /**
         * @brief Queue that stores the lengths of the packets that will eventually be concatenated
         * to Type-BD transfer frame data.
	     */
	    etl::queue<uint16_t, PacketBufferTcSize> packetLengthBufferTxTcTypeBD;

	    /**
         * @brief Queue that stores the bytes of the packets that will eventually be concatenated to
         * Type-BD transfer frame data.
	     */
	    etl::queue<uint8_t, PacketBufferTcSize> packetBufferTxTcTypeBD;

	    /**
         * @brief Buffer that stores pointers to frames after packet processing and before SDLS processing.
	     */
	    etl::list<TransferFrameTC *, MaxReceivedUnprocessedTxTcInVirtBuffer> framesBeforeSDLSProcessingTxTC;

	    /**
         * @brief Buffer that stores pointers to frames after SDLS processing and before vcGeneration (FOP processing).
	     */
	    etl::list<TransferFrameTC *, MaxReceivedUnprocessedTxTcInVirtBuffer> unprocessedFrameListBufferTxTC;

	    // RxTM chain
	    //        /**
	    //         * @brief TM transfer frames after being processed by the MasterChannelReception Service.
	    //         */
	    //        etl::list<TransferFrameTM *, MaxReceivedRxTmInVirtBuffer> framesAfterMcReceptionRxTM;
	    //
	    //        /**
	    //         * @brief Buffer holding the CLCW that is received
	    //         */
	    //        etl::list<CLCW, 1> receivedClcwBuffer;

// Allow access to internal buffers when unit testing
#ifdef ENABLE_BUFFER_ACCESS
	public:
	    etl::queue<uint16_t, PacketBufferTcSize>& getPacketLengthBufferTxTcTypeAD() {
		    return packetLengthBufferTxTcTypeAD;
	    }

	    etl::queue<uint8_t, PacketBufferTcSize>& getPacketBufferTxTcTypeAD() {
		    return packetBufferTxTcTypeAD;
	    }

	    etl::queue<uint16_t, PacketBufferTcSize>& getPacketLengthBufferTxTcTypeBD() {
		    return packetLengthBufferTxTcTypeAD;
	    }

	    etl::queue<uint8_t, PacketBufferTcSize>& getPacketBufferTxTcTypeBD() {
		    return packetBufferTxTcTypeBD;
	    }

	    etl::list<TransferFrameTC *, MaxReceivedUnprocessedTxTcInVirtBuffer>& getUnprocessedFrameListBufferTxTC() {
		    return unprocessedFrameListBufferTxTC;
	    }

	    etl::list<TransferFrameTC *, MaxReceivedUnprocessedTxTcInVirtBuffer>& getFramesBeforeSDLSProcessingTxTC() {
		    return framesBeforeSDLSProcessingTxTC;
	    }
#endif // ENABLE_BUFFER_ACCESS
    };

    class MasterChannelGroundSegment : public BaseMasterChannel {
	    friend class BaseServiceChannel;
	    friend class ServiceChannelSpaceSegment;
	    friend class ServiceChannelGroundSegment;
	public:
	    explicit MasterChannelGroundSegment(uint16_t scid)
	        : BaseMasterChannel(scid), masterChannelPoolTxTC(MemoryPool()){}

	    MasterChannelGroundSegment(const MasterChannelGroundSegment& m)
	        : BaseMasterChannel(m),
	          virtualChannels(m.virtualChannels), outFramesBeforeAllFramesGenerationListTxTC(m.outFramesBeforeAllFramesGenerationListTxTC),
	          masterCopyTxTC(m.masterCopyTxTC), masterChannelPoolTxTC(m.masterChannelPoolTxTC) {}

	    MasterChannelAlert addVirtualChannel(const VirtualChannelGroundSegment& v) {
		    if (virtualChannels.full()) {
			    ccsdsLogNotice(TxRx::Tx, NotificationType::TypeMasterChannelAlert, MasterChannelAlert::MAX_AMOUNT_OF_VIRT_CHANNELS);
			    return MasterChannelAlert::MAX_AMOUNT_OF_VIRT_CHANNELS;
		    }
		    virtualChannels.emplace(v.getVCID(), v);

		    return MasterChannelAlert::NO_MC_ALERT;
	    }

	private:
	    etl::flat_map<uint8_t, VirtualChannelGroundSegment, MaxVirtualChannels> virtualChannels;

	    /**
		 * @brief Buffer that stores pointers to TC frames after vcGeneration (FOP processing) and before
		 * allFramesGeneration.
	     */
	    etl::list<TransferFrameTC *, MaxReceivedTxTcInMasterBuffer> outFramesBeforeAllFramesGenerationListTxTC;

	    /**
		 * @brief Buffer that stores the actual TC transfer frame objects for the TxTC chain.
	     */
	    etl::list<TransferFrameTC, MaxTxInMasterChannel> masterCopyTxTC;

	    /**
		 * @brief Remove TC a transfer frame object in the TxTC chain.
	     */
	    void removeMasterTxTC(TransferFrameTC *frame_ptr);

	    /**
		 * @brief An object that manages a statically allocated block of memory, storing the octets for TC frames in the
		 * TxTC chain.
	     */
	    MemoryPool masterChannelPoolTxTC;

	    // RxTM chain
	    // etl::list<TransferFrameTM, MaxRxInMasterChannel> masterCopyRxTM;
	    // void removeMasterRxTM(TransferFrameTM *frame_ptr);

// Allow access to internal buffers when unit testing
#ifdef ENABLE_BUFFER_ACCESS
	public:
	    etl::flat_map<uint8_t, VirtualChannelGroundSegment, MaxVirtualChannels>& getVirtualChannels() {
		    return virtualChannels;
	    };
	    etl::list<TransferFrameTC *, MaxReceivedTxTcInMasterBuffer>& getOutFramesBeforeAllFramesGenerationListTxTC() {
		    return outFramesBeforeAllFramesGenerationListTxTC;
	    }
	    etl::list<TransferFrameTC, MaxTxInMasterChannel>& getMasterCopyTxTC() {
		    return masterCopyTxTC;
	    }
	    MemoryPool& getMasterChannelPoolTxTC() {
		    return masterChannelPoolTxTC;
	    } 
#endif // ENABLE_BUFFER_ACCESS
    };
#endif // GROUND_SEGMENT

} // namespace CCSDSDataLinkLayer