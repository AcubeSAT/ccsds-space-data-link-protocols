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
#include "etl/list.h"
#include "etl/queue.h"
#include "etl/deque.h"
#include "etl/span.h"
#include "CCSDSDefinitionsAndUtilities.hpp"
#include "TransferFrameTC.hpp"
#include "TransferFrameTM.hpp"
#include "MemoryPool.hpp"

namespace CCSDSDataLinkLayer {
    class ChannelsInterface;
    /**
     * @see Table 5-1 from TC SPACE DATA LINK PROTOCOL
     */
    class PhysicalChannel {
        friend class ChannelsInterface;

    public:
        PhysicalChannel(const DefsAndUtils::TransferFrameVersionNumber tfvn, const etl::span<uint16_t> &validScids,
                        const uint16_t maxTcFrameLength, const uint16_t tmFrameLength,
                        const uint16_t maxFramesPdu, const uint16_t maxPduLength,
                        const uint32_t bitrate, const uint16_t repetitions, const bool frameErrorControlFieldPresent)
            : tfvn(tfvn), maxTcFrameLength(maxTcFrameLength), tmFrameLength(tmFrameLength),
              maxFramePdu(maxFramesPdu), maxPDULength(maxPduLength), bitrate(bitrate),
              repetitions(repetitions), frameErrorControlFieldPresent(frameErrorControlFieldPresent) {
            for (uint8_t i = 0; i < std::min(validScids.size(), DefsAndUtils::MaxSCIDInPhysicalChannel); i++) {
                this->validScids[i] = validScids[i];
            }
        }

        /**
         * @brief Defines the type of protocol used
         * @see SANA transfer frame version numbers registry
         */
        [[nodiscard]] DefsAndUtils::TransferFrameVersionNumber getTFVN() const {
            return tfvn;
        }

        /**
         * @brief Returns whether given scid belongs in the physical channel's scid list.
         */
        [[nodiscard]] bool isValidScid(const uint16_t scid) const {
            for (uint8_t i = 0; i < validScids.size(); i++) {
                if (validScids[i] == scid) {
                    return true;
                }
            }
            return false;
        }

        /**
         * @brief Get the maximum allowed TC frame length in this physical channel.
         */
        [[nodiscard]] uint16_t getMaxTcFrameLength() const {
            return maxTcFrameLength;
        }

        /**
         * @brief Get length of TM frames in this virtual channel
         */
        [[nodiscard]] uint16_t getTMFrameLength() const {
            return tmFrameLength;
        }

        /**
         * @brief Sets the maximum number of transfer frames that can be transferred in a single data unit.
         */
        [[nodiscard]] uint16_t getMaxFramePdu() const {
            return maxFramePdu;
        }

        /**
         * @brief Maximum length of a data unit.
         */
        [[nodiscard]] uint16_t getMaxPDULength() const {
            return maxPDULength;
        }

        /**
         * @brief Maximum bit rate (bits per second).
         */
        [[nodiscard]] uint32_t getBitrate() const {
            return bitrate;
        }

        /**
         * @brief Maximum number of retransmissions for a data unit.
         */
        [[nodiscard]] uint16_t getRepetitions() const {
            return repetitions;
        }

        /**
         * @brief Defines whether the ECF service is present in transfer frames.
         */
        [[nodiscard]] bool getFrameErrorControlFieldPresent() const {
            return frameErrorControlFieldPresent;
        }

    private:
        const DefsAndUtils::TransferFrameVersionNumber tfvn;
        etl::array<uint16_t, DefsAndUtils::MaxSCIDInPhysicalChannel> validScids;
        const uint16_t maxTcFrameLength;
        const uint16_t tmFrameLength;
        const uint16_t maxFramePdu;
        const uint16_t maxPDULength;
        const uint32_t bitrate;
        const uint16_t repetitions;
        const bool frameErrorControlFieldPresent;
    };

    /**
     * Base map channel class containing parameters common among space and ground segment code
     */
    class BaseMAPChannel {
        friend class ChannelsInterface;

    public:
        BaseMAPChannel(const uint32_t gmapid, const bool blockingTC, const bool segmentationTC,
                       const etl::optional<uint16_t> &associatedSdlsSPI = etl::nullopt)
            : gmapid(gmapid & 0x00FFFFFFU), blockingTC(blockingTC), segmentationTC(segmentationTC),
              associatedSdlsSPI(associatedSdlsSPI) {
        }

        BaseMAPChannel(const BaseMAPChannel &m) = default;

        [[nodiscard]] uint32_t getGmapid() const {
            return gmapid;
        }

        [[nodiscard]] bool getBlockingTC() const {
            return blockingTC;
        }

        [[nodiscard]] bool getSegmentationTC() const {
            return segmentationTC;
        }

        [[nodiscard]] etl::optional<uint16_t> getAssociatedSdlsSPI() const {
            return associatedSdlsSPI;
        }

    protected:
        /**
         * @brief Global MAP channel identifier
         * @details gmapid = | TFVN (2 bits) | SCID (10 bits) | VCID (6 bits) | MAP ID (6 bits) |
         * @see p. 2.1.3 of CCSDS TC SPACE DATA LINK PROTOCOL
         */
        const uint32_t gmapid;

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

        /**
         * @brief The presence of an SDLS SPI (Security Parameter Index) shall indicate that this map channel
         *        instance is associated with a specific SecurityAssociation, therefore the according authentication
         *        and encryption services will be applied to its frames.
         */
        const etl::optional<uint16_t> associatedSdlsSPI;
    };

    /**
     * Base virtual channel class containing parameters common among space and ground segment code
     */
    class BaseVirtualChannel {
        friend class ChannelsInterface;

    public:
        BaseVirtualChannel(const uint32_t gvcid, const uint8_t vcRepetitions, const uint16_t maxFrameLengthTC,
                           const bool segmentHeaderTCPresent, const bool blockingTC,
                           const bool operationalControlFieldTMPresent,
                           const bool secondaryHeaderTMPresent, const uint8_t secondaryHeaderTMLength,
                           const DefsAndUtils::SynchronizationFlag synchronization,
                           const etl::optional<uint16_t> &associatedSdlsSPI = etl::nullopt)
            : gvcid(gvcid & 0x0003FFFFU), vcRepetitions(vcRepetitions),
              maxFrameLengthTC(maxFrameLengthTC), segmentHeaderTCPresent(segmentHeaderTCPresent),
              blockingTC(blockingTC),
              operationalControlFieldTMPresent(operationalControlFieldTMPresent),
              secondaryHeaderTMPresent(secondaryHeaderTMPresent), secondaryHeaderTMLength(secondaryHeaderTMLength),
              synchronizationTM(synchronization), associatedSdlsSPI(associatedSdlsSPI) {
        }

        BaseVirtualChannel(const BaseVirtualChannel &v) = default;

        [[nodiscard]] uint32_t getGvcid() const {
            return gvcid;
        }

        [[nodiscard]] uint8_t getVcRepetitions() const {
            return vcRepetitions;
        }

        [[nodiscard]] uint16_t getMaxFrameLengthTC() const {
            return maxFrameLengthTC;
        }

        [[nodiscard]] bool getSegmentHeaderTCPresent() const {
            return segmentHeaderTCPresent;
        }

        [[nodiscard]] bool getBlockingTC() const {
            return blockingTC;
        }

        [[nodiscard]] bool getOperationalControlFieldTMPresent() const {
            return operationalControlFieldTMPresent;
        }

        [[nodiscard]] bool getSecondaryHeaderTMPresent() const {
            return secondaryHeaderTMPresent;
        }

        [[nodiscard]] uint8_t getSecondaryHeaderTMLength() const {
            return secondaryHeaderTMLength;
        }

        [[nodiscard]] DefsAndUtils::SynchronizationFlag getSynchronization() const {
            return synchronizationTM;
        }

        [[nodiscard]] etl::optional<uint16_t> getAssociatedSdlsSPI() const {
            return associatedSdlsSPI;
        }

    protected:
        /**
         * @brief Global Virtual Channel Identifier.
         * @details gvcid = | TFVN (2 bits) | SCID (10 bits) | VCID (6 bits) |
         * @see p. 2.1.3 of CCSDS TC SPACE DATA LINK PROTOCOL
         */
        const uint32_t gvcid; // 6 bits

        /**
         * @brief Determines the number of times a frame will be repeated in transmission in the Physical Layer.
         * TODO ??
         */
        const uint8_t vcRepetitions;

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
        const DefsAndUtils::SynchronizationFlag synchronizationTM;

        /**
         * @brief The presence of an SDLS SPI (Security Parameter Index) shall indicate that this virtual channel
         *        instance is associated with a specific SecurityAssociation, therefore the according authentication
         *        and encryption services will be applied to its frames.
         */
        const etl::optional<uint16_t> associatedSdlsSPI;
    };

    /**
	 * Base virtual channel class containing parameters common among space and ground segment code
     */
    class BaseMasterChannel {
        friend class ChannelsInterface;

    public:
        explicit BaseMasterChannel(const uint16_t mscid) : mscid(mscid) {
        }

        BaseMasterChannel(const BaseMasterChannel &) = default;

        [[nodiscard]] uint16_t getMscid() const {
            return mscid;
        }

    protected:
        /** @brief global master channel identifier
         *  @details mscid = | TFVN (2 bits) | SCID (10 bits) |
         *  @see p. 2.1.3 from CCSDS TC SPACE DATA LINK PROTOCOL
         */
        const uint16_t mscid;
    };

#ifdef SPACE_SEGMENT
    template<std::size_t T>
    class MAPChannelSpaceSegment : public BaseMAPChannel {
        friend class ChannelsInterface;

    public:
        MAPChannelSpaceSegment(const uint32_t gmapid, const bool blockingTC, const bool segmentationTC)
            : BaseMAPChannel(gmapid, blockingTC, segmentationTC) {
        }

        MAPChannelSpaceSegment(const MAPChannelSpaceSegment &m)
            : BaseMAPChannel(m), framesUnderProcessing(m.framesUnderProcessing) {
        }

    private:
        /**
         * Stores pointers to TC frames after security processing.
         */
        etl::list<TransferFrameTC *, T> framesUnderProcessing;

        // give the user access while debugging/testing
#ifdef ENABLE_CHANNEL_ACCESS
    public:
        etl::list<TransferFrameTC *, T>& getFramesUnderProcessing()  {
            return framesUnderProcessing;
        }
#endif // ENABLE_CHANNEL_ACCESS
    };

    template<std::size_t T>
    class VirtualChannelSpaceSegment : public BaseVirtualChannel {
        friend class ChannelsInterface;
        friend class FrameAcceptanceReporting;
        friend class ServiceChannelSpaceSegment;

    public:
        VirtualChannelSpaceSegment(const uint32_t gvcid, const uint8_t vcRepetitions, const uint16_t maxFrameLengthTC,
                                   const bool segmentHeaderTCPresent, const bool blockingTC,
                                   const bool operationalControlFieldTMPresent,
                                   const bool secondaryHeaderTMPresent, const uint8_t secondaryHeaderTMLength,
                                   const DefsAndUtils::SynchronizationFlag synchronization)
            : BaseVirtualChannel(gvcid, vcRepetitions,  maxFrameLengthTC, segmentHeaderTCPresent, blockingTC,
                                 operationalControlFieldTMPresent, secondaryHeaderTMLength,
                                 secondaryHeaderTMPresent,
                                 synchronization), frameCountTM(0) {
        }

        VirtualChannelSpaceSegment(const VirtualChannelSpaceSegment &v)
            : BaseVirtualChannel(v), framesAfterAllFramesReceptionTC(v.framesAfterAllFramesReceptionTC),
              framesAfterVCReceptionTCTypeAD(v.framesAfterVCReceptionTCTypeAD),
              framesAfterVCReceptionTCTypeBD(v.framesAfterVCReceptionTCTypeBD),
              frameCountTM(v.frameCountTM), framesUnderProcessingTM(v.framesUnderProcessingTM),
              packetLengthBufferTM(v.packetLengthBufferTM),
              packetBufferTM(v.packetBufferTM) {
        }

    private:
        /**
         * @brief Stores pointers to TC frames (of all types) after being created by all frames reception.
         */
        etl::list<TransferFrameTC *, T> framesAfterAllFramesReceptionTC;

        /**
         * @brief Stores pointers to type-AD TC frames after being processed by vc generation. If no MAP channels
         *        exist for this virtual channel, the pointers remain in this buffer after security processing.
         */
        etl::list<TransferFrameTC *, T> framesAfterVCReceptionTCTypeAD;

        /**
         * @brief Stores pointers to type-BD TC frames after being processed by vc generation. The separate buffer
         *        exists for the purpose of overwriting old type-BD frames with new ones (expedited service),
         *        without interrupting the type-AD service. If no MAP channels exist for this virtual channel,
         *        the pointers remain in this buffer after security processing.
         */
        etl::list<TransferFrameTC *, T> framesAfterVCReceptionTCTypeBD;

        /**
         * @brief Counter for the amount of TM transfer frames transmitted through this virtual channel.
         */
        uint8_t frameCountTM;

        /**
         * @brief Stores pointers to created TM frames after vc generation.
         */
        etl::list<TransferFrameTM *, T> framesUnderProcessingTM;

        /**
         *  @brief Queue that stores the lengths of packets that will eventually be concatenated to TM transfer frame data.
         */
        etl::deque<uint16_t, 10 * T> packetLengthBufferTM;

        /**
         *  @brief Queue that stores the packet data that will eventually be concatenated to TM transfer frame data.
         */
        etl::queue<uint8_t, 100 * T> packetBufferTM;

        // Give the user access while debugging, testing
#ifdef ENABLE_CHANNEL_ACCESS
    public:
        etl::list<TransferFrameTC*, T>& getFramesAfterAllFramesReceptionTC() {
            return framesAfterAllFramesReceptionTC;
        }

        etl::list<TransferFrameTC*, T>& getFramesAfterVCReceptionTCTypeAD() {
            return framesAfterVCReceptionTCTypeAD;
        }

        etl::list<TransferFrameTC*, T>& getFramesAfterVCReceptionTCTypeBD() {
            return framesAfterVCReceptionTCTypeBD;
        }

        [[nodiscard]] uint8_t getFrameCountTM() const {
            return frameCountTM;
        }

        etl::list<TransferFrameTM*, T>& getFramesUnderProcessingTM() {
            return framesUnderProcessingTM;
        }

        etl::deque<uint16_t, 100 * T>& getPacketLengthBufferTM() {
            return packetLengthBufferTM;
        }

        etl::deque<uint8_t, 100 * T>& getPacketBufferTM() {
            return packetBufferTM;
        }
#endif // ENABLE_CHANNEL_ACCESS
    };

    template<std::size_t T>
    class MasterChannelSpaceSegment : public BaseMasterChannel {
        friend class ChannelsInterface;

    public:
        explicit MasterChannelSpaceSegment(const uint16_t mscid) : BaseMasterChannel(mscid),
                                                                   masterChannelPoolTC(MemoryPool<1000 * T>()),
                                                                   masterChannelFrameCountTM(0),
                                                                   masterChannelPoolTM(MemoryPool<1000 * T>()) {
        }

        MasterChannelSpaceSegment(const MasterChannelSpaceSegment &m)
            : BaseMasterChannel(m), framesUnderProcessingTM(m.framesUnderProcessingTM),
              masterCopyTC(m.masterCopyTC), masterChannelPoolTC(m.masterChannelPoolTC),
              masterChannelFrameCountTM(m.masterChannelFrameCountTM),
              masterCopyTM(m.masterCopyTM), masterChannelPoolTM(m.masterChannelPoolTM) {
        }

    private:
        /**
         * @brief Buffer that holds pointers to TM frames after mc generation and before all frames generation.
         */
        etl::list<TransferFrameTM *, T> framesUnderProcessingTM;

        /**
         * @brief Buffer that stores the actual TC transfer frame objects for the RxTC chain.
         */
        etl::list<TransferFrameTC, T> masterCopyTC;

        /**
         * @brief An object that manages a statically allocated block of memory, storing the octets for TC frames in the
         * RxTC chain.
         */
        MemoryPool<1000 * T> masterChannelPoolTC;

        /**
         * @brief A counter that keeps track the number of TM transfer frames transmitted from this master channel. The
         * master channel frame count is carried by TM transfer frames, hence the receiving side can deduce if frames
         * were lost.
         *
         * @details The initial value of this counter should be zero.
         */
        uint8_t masterChannelFrameCountTM;

        /**
         * @brief Buffer that stores the actual TM transfer frame objects for the TxTM chain.
         */
        etl::list<TransferFrameTM, T> masterCopyTM;

        /**
         * @brief An object that manages a statically allocated block of memory, storing the octets for TM frames in the
         * TxTM chain.
         */
        MemoryPool<1000 * T> masterChannelPoolTM;

        // Give the user access while debugging, testing
#ifdef ENABLE_CHANNEL_ACCESS
        public:
        etl::list<TransferFrameTM *, T>& getFramesUnderProcessingTM() {
            return framesUnderProcessingTM;
        }

        etl::list<TransferFrameTC, T>& getMasterCopyRxTC() {
            return masterCopyTC;
        }

        MemoryPool<1000 * T>& getMasterChannelPoolRxTC() {
            return masterChannelPoolTC;
        }

        uint8_t getMasterChannelFrameCountTM() {
            return masterChannelFrameCountTM;
        }

        etl::list<TransferFrameTM, T>& getMasterCopyTxTM() {
            return masterCopyTM;
        }

        MemoryPool<1000 * T>& getMasterChannelPoolTxTM() {
            return masterChannelPoolTM;
        }
#endif // ENABLE_CHANNEL_ACCESS
    };
#endif // SPACE_SEGMENT

#ifdef GROUND_SEGMENT
    template<std::size_t T>
    class MAPChannelGroundSegment : public BaseMAPChannel {
        friend class ChannelsInterface;

    public:
        MAPChannelGroundSegment(const uint32_t gmapid, const bool blockingTC,
                                const bool segmentationTC) : BaseMAPChannel(
            gmapid, blockingTC, segmentationTC) {
        }

        MAPChannelGroundSegment(const MAPChannelGroundSegment &m)
            : BaseMAPChannel(m), packetLengthBufferTypeAD(m.packetLengthBufferTypeAD),
              packetBufferTypeAD(m.packetBufferTypeAD),
              packetLengthBufferTypeBD(m.packetLengthBufferTypeBD),
              packetBufferTypeBD(m.packetBufferTypeBD), framesUnderProcessing(m.framesUnderProcessing) {
        }

    private:
        /**
         * @brief Queue that stores lengths of packets that will eventually be concatenated to Type-AD
         * transfer frame data by packetProcessing.
         */
        etl::queue<uint16_t, 10 * T> packetLengthBufferTypeAD;

        /**
         * @brief Queue that stores the bytes of packets that will eventually be concatenated to Type-AD
         * transfer frame data by packetProcessing.
         */
        etl::queue<uint8_t, 100 * T> packetBufferTypeAD;

        /**
         * @brief Queue that stores lengths of packets that will eventually be concatenated to Type-BD
         * transfer frame data by packetProcessing.
         */
        etl::queue<uint16_t, 10 * T> packetLengthBufferTypeBD;

        /**
         * @brief Queue that stores the bytes of packets that will eventually be concatenated to Type-BD
         * transfer frame data by packetProcessing.
         */
        etl::queue<uint8_t, 100 * T> packetBufferTypeBD;

        /**
         * Holds pointers to frames after packetProcessing.
         */
        etl::list<TransferFrameTC *, T> framesUnderProcessing;

        // Give the user access when debugging/testing
#ifdef ENABLE_CHANNEL_ACCESS
    public:
        etl::queue<uint16_t, 10 * T>& getPacketLengthBufferTypeAD() {
            return packetLengthBufferTypeAD;
        }

        etl::queue<uint8_t, 100 * T>& getPacketBufferTypeAD() {
            return packetBufferTypeAD;
        }

        etl::queue<uint16_t, 10 * T>& getPacketLengthBufferTypeBD() {
            return packetLengthBufferTypeBD;
        }

        etl::queue<uint8_t, 100 * T>& getPacketBufferTypeBD() {
            return packetBufferTypeBD;
        }

        etl::list<TransferFrameTC *, T>& getFramesUnderProcessing() {
            return framesUnderProcessing;
        }
#endif
    };

    template<std::size_t T>
    class VirtualChannelGroundSegment : public BaseVirtualChannel {
        friend class ChannelsInterface;

    public:
        VirtualChannelGroundSegment(const uint32_t gvcid, const uint8_t vcRepetitions, const uint16_t maxFrameLengthTC,
                                    const bool segmentHeaderTCPresent, const bool blockingTC,
                                    const bool operationalControlFieldTMPresent,
                                    const bool secondaryHeaderTMPresent, const uint8_t secondaryHeaderTMLength,
                                    const DefsAndUtils::SynchronizationFlag synchronization)
            : BaseVirtualChannel(gvcid, vcRepetitions, maxFrameLengthTC, segmentHeaderTCPresent, blockingTC,
                                 operationalControlFieldTMPresent, secondaryHeaderTMLength, secondaryHeaderTMPresent,
                                 synchronization) {
        }

        VirtualChannelGroundSegment(const VirtualChannelGroundSegment &v)
            : BaseVirtualChannel(v), packetLengthBufferTcTypeAD(v.packetLengthBufferTcTypeAD),
              packetBufferTcTypeAD(v.packetBufferTcTypeAD),
              packetLengthBufferTcTypeBD(v.packetLengthBufferTcTypeBD),
              packetBufferTcTypeBD(v.packetBufferTcTypeBD),
              framesUnderProcessingTC(v.framesUnderProcessingTC) {
        }

    private:
        /**
         * @brief Queue that stores the lengths of the packets that will eventually be concatenated
         * to Type-AD transfer frame data.
         */
        etl::queue<uint16_t, 10 * T> packetLengthBufferTcTypeAD;

        /**
         * @brief Queue that stores the bytes of the packets that will eventually be concatenated to
         * Type-AD transfer frame data.
         */
        etl::queue<uint8_t, DefsAndUtils::MaxSpacePacketSize * T> packetBufferTcTypeAD;

        /**
         * @brief Queue that stores the lengths of the packets that will eventually be concatenated
         * to Type-BD transfer frame data.
         */
        etl::queue<uint16_t, 10 * T> packetLengthBufferTcTypeBD;

        /**
         * @brief Queue that stores the bytes of the packets that will eventually be concatenated to
         * Type-BD transfer frame data.
         */
        etl::queue<uint8_t, DefsAndUtils::MaxSpacePacketSize * T> packetBufferTcTypeBD;

        /**
         * @brief Holds pointers to TC frames after being created by packetProcessing and during applySecurity,
         *        vcGeneration.
         */
        etl::list<TransferFrameTC *, T> framesUnderProcessingTC;

        // Give the user access when debugging/testing
#ifdef ENABLE_CHANNEL_ACCESS
    public:
        etl::queue<uint16_t, 10 * T>& getPacketLengthBufferTcTypeAD() {
            return packetLengthBufferTcTypeAD;
        }

        etl::queue<uint8_t, DefsAndUtils::MaxSpacePacketSize * T>& getPacketBufferTcTypeAD() {
            return packetBufferTcTypeAD;
        }

        etl::queue<uint16_t, 10 * T>& getPacketLengthBufferTcTypeBD() {
            return packetLengthBufferTcTypeBD;
        }

        etl::queue<uint8_t, DefsAndUtils::MaxSpacePacketSize * T>& getPacketBufferTcTypeBD() {
            return packetBufferTcTypeBD;
        }

        etl::list<TransferFrameTC *, T>& getFramesUnderProcessingTC() {
            return framesUnderProcessingTC;
        }
#endif
    };

    template<std::size_t T>
    class MasterChannelGroundSegment : public BaseMasterChannel {
        friend class ChannelsInterface;

    public:
        explicit MasterChannelGroundSegment(const uint16_t mscid)
            : BaseMasterChannel(mscid), masterChannelPoolTC(MemoryPool<1000 * T>()) {
        }

        MasterChannelGroundSegment(const MasterChannelGroundSegment &m)
            : BaseMasterChannel(m),
              masterCopyTC(m.masterCopyTC), masterChannelPoolTC(m.masterChannelPoolTC) {
        }

    private:
        /**
         * @brief Buffer that stores the actual TC transfer frame objects for the TxTC chain.
         */
        etl::list<TransferFrameTC, T> masterCopyTC;

        /**
         * @brief An object that manages a statically allocated block of memory, storing the octets for TC frames in the
         * TxTC chain.
         */
        MemoryPool<1000 * T> masterChannelPoolTC;

        // Give the user access when debugging/testing
#ifdef ENABLE_CHANNEL_ACCESS
    public:
        etl::list<TransferFrameTC, T>& getMasterCopyTxTC() {
            return masterCopyTC;
        }

        MemoryPool<1000*T>& getMasterChannelPoolTxTC() {
            return masterChannelPoolTC;
        }
#endif
    };
#endif // GROUND_SEGMENT
} // namespace CCSDSDataLinkLayer
