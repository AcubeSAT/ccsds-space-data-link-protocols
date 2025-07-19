/**
 * @file  VirtualChannel.hpp
 */

#pragma once
#include <cstdint>
#include "ExternalContainers.hpp"
#include "TransferFrameTC.hpp"
#include "Mutex.hpp"

namespace CCSDSDataLinkLayer {
    class FrameAcceptanceReporting;
    class FrameOperationProcedure;
    class SpaceSegmentTmDataHandling;
    class SpaceSegmentTcDataHandling;
    class GroundSegmentTmDataHandling;
    class GroundSegmentTcDataHandling;

    /**
     * Base virtual channel class containing parameters common among space and ground segment code
     */
    class VirtualChannelBase {
    public:
        explicit VirtualChannelBase(const uint8_t vcid, const uint16_t parentScid,
                           const uint16_t associatedSdlsSPI, const uint16_t frameCapacity)
            : vcid(vcid & 0x3FU), parentScid(parentScid & 0x03FFU), associatedSdlsSPI(associatedSdlsSPI),
              frameCapacity(frameCapacity), channelMutex(Mutex()) {
            if (associatedSdlsSPI != 0) {
                this->associatedSdlsSPI = etl::optional(associatedSdlsSPI);
            }
        }

        /**
         * @brief Protects against concurrent access to resources
         */
        Mutex channelMutex;

        [[nodiscard]] uint32_t getVcid() const {
            return vcid;
        }

        [[nodiscard]] uint16_t getParentScid() const {
            return parentScid;
        }

        [[nodiscard]] etl::optional<uint16_t> getAssociatedSdlsSPI() const {
            return associatedSdlsSPI;
        }

        [[nodiscard]] uint16_t getFrameCapacity() const {
            return frameCapacity;
        }

        void incrementFrameCapacity(const uint16_t amount) {
            this->frameCapacity += amount;
        }

    protected:
        /**
         * @brief Global Virtual Channel Identifier.
         * @details 6 bit identifier for this virtual channel
         * @see p. 2.1.3 of CCSDS TC SPACE DATA LINK PROTOCOL
         */
        const uint8_t vcid;

        /**
         * @brief scid of parent master channel
         */
        const uint16_t parentScid;

        /**
         * @brief The presence of an SDLS SPI (Security Parameter Index) shall indicate that this virtual channel
         *        instance is associated with a specific SecurityAssociation, therefore the according authentication
         *        and encryption services will be applied to its frames.
         */
        etl::optional<uint16_t> associatedSdlsSPI;

        /**
         * @brief States how many frames this channel should support (used during the memory pool allocation process).
         *        In the derivative VirtualChannelSsTc, VirtualChannelGsTc, it should have a value 0, if map channels
         *        exist (this is because the actual amount of frames a virtual channel can support will be determined
         *        by the sum of frame capacities of the map channels)
         */
        uint16_t frameCapacity;
    };

#ifdef INCLUDE_SPACE_SEGMENT_CODE
    class VirtualChannelSsTm : public VirtualChannelBase {
        friend class SpaceSegmentTmDataHandling;
    public:
        explicit VirtualChannelSsTm(const uint8_t vcid, const uint16_t parentScid, const uint8_t vcRepetitions,
                                     const bool secondaryHeaderPresent, const uint8_t secondaryHeaderLength,
                                     const bool operationalControlFieldPresent,
                                     const Defs::SynchronizationFlag synchronization,
                                     const uint16_t associatedSdlsSPI, const uint16_t frameCapacity,
                                     const uint16_t packetCapacity)
            : VirtualChannelBase(vcid, parentScid, associatedSdlsSPI, frameCapacity), vcRepetitions(vcRepetitions),
              operationalControlFieldPresent(operationalControlFieldPresent),
              secondaryHeaderPresent(secondaryHeaderPresent),
              secondaryHeaderLength(secondaryHeaderLength),
              synchronization(synchronization), virtualChannelFrameCount(0), packetCapacity(packetCapacity) {}

        void initializeContainers(const etl::span<uint16_t> &packetLengthsBuff,
                                  const etl::span<uint8_t> &packetOctetsBuff) {
            packetLengths = Dequeue(packetLengthsBuff);
            packetOctets = Dequeue(packetOctetsBuff);
        }

        [[nodiscard]] uint16_t getPacketCapacity() const {
            return packetCapacity;
        }

        [[nodiscard]] uint8_t getVcRepetitions() const {
            return vcRepetitions;
        }

        [[nodiscard]] bool getOperationalControlFieldPresent() const {
            return operationalControlFieldPresent;
        }

        [[nodiscard]] bool getSecondaryHeaderPresent() const {
            return secondaryHeaderPresent;
        }

        [[nodiscard]] uint8_t getSecondaryHeaderLength() const {
            return secondaryHeaderLength;
        }

        [[nodiscard]] Defs::SynchronizationFlag getSynchronization() const {
            return synchronization;
        }

        [[nodiscard]] uint16_t getVirtualChannelFrameCount() const {
            return virtualChannelFrameCount;
        }

        void incrementVirtualChannelFrameCount() {
            virtualChannelFrameCount++;
        }

    private:
        /**
         * @brief Determines the number of times a frame will be repeated in transmission to the Channel Coding Layer.
         * TODO ??
         */
        const uint8_t vcRepetitions;

        /**
         * @brief Defines whether the OCF field is present in TM transfer frames.
         */
        const bool operationalControlFieldPresent;

        /**
         * @brief Indicates whether secondary header field is present in TM transfer frames.
         */
        const bool secondaryHeaderPresent;

        /**
         * @brief Indicates the length of the secondary header for this VC. If the secondary header is disabled for this VC,
         * it is ignored.
         */
        const uint8_t secondaryHeaderLength;

        /**
         * @brief Defines whether octet and forward-ordered synchronization is used for TM transfer frames.
         */
        const Defs::SynchronizationFlag synchronization;

        /**
         * @brief A counter that keeps track the number of TM transfer frames transmitted from this virtual channel. The
         * virtual channel frame count is carried by TM transfer frames, hence the receiving side can deduce if frames
         * were lost.
         *
         * @details The initial value of this counter should be zero
         */
        uint8_t virtualChannelFrameCount;

        /**
         * @brief States how many packets this channel should support (used during the memory pool allocation process).
         */
        const uint16_t packetCapacity;

        /**
         * @brief If the channel supports Packets, this is a queue that stores lengths of packets that will eventually
         *        be concatenated to TM transfer frames by the vc generation data handling function. If the virtual
         *        channel supports VCA SDU, then it stores the user defined fields 'packet order flag' and
         *        'segment length identifier'. The format is the following:
         *
         *        | (13 bits) empty | (1 bit) packet order flag | (2 bits) segment length identifier |
         */
        Dequeue<uint16_t> packetLengths;

        /**
         * @brief Queue that stores octets of packets that will eventually be concatenated to TM transfer frames
         *        by the vc generation data handling function
         */
        Dequeue<uint8_t> packetOctets;

#ifdef ENABLE_CHANNEL_QUEUE_ACCESS
    public:
        Dequeue<uint16_t>& getPacketLengths() const {
            return packetLengths;
        }

        Dequeue<uint8_t>& getPacketOctets() const {
            return packetOctets;
        }
#endif // ENABLE_CHANNEL_QUEUE_ACCESS
    };
#endif // INCLUDE_SPACE_SEGMENT_CODE

    // unimplemented
    // class VirtualChannelGS_TM : public  VirtualChannelBase {
    //
    // }

#ifdef INCLUDE_SPACE_SEGMENT_CODE
    class VirtualChannelSsTc : public VirtualChannelBase {
       friend class FrameAcceptanceReporting;
       friend class SpaceSegmentTcDataHandling;
    public:
        explicit VirtualChannelSsTc(const uint8_t vcid, const uint16_t parentScid,
                                     const bool segmentHeaderPresent, const bool blocking,
                                     const bool copInEffect,
                                     const uint16_t associatedSdlsSPI,
                                     const Defs::DataFieldContent dataFieldContent,
                                     const uint16_t frameCapacity,
                                     const uint16_t typeAdPacketCapacity,
                                     const uint16_t typeBdPacketCapacity)
            : VirtualChannelBase(vcid, parentScid, associatedSdlsSPI, frameCapacity),
              segmentHeaderPresent(segmentHeaderPresent), blocking(blocking), copInEffect(copInEffect),
              dataFieldContent(dataFieldContent), typeAdPacketCapacity(typeAdPacketCapacity),
              typeBdPacketCapacity(typeBdPacketCapacity) {}

        void initializeContainers(const etl::span<TransferFrameTC *> &framesAfterAllFramesReceptionBuff,
                                  const etl::span<TransferFrameTC *> &framesAfterVcReceptionTypeADBuff,
                                  const etl::span<TransferFrameTC *> &framesAfterVcReceptionTypeBDBuff,
                                  const etl::span<TransferFrameTC *> &framesAfterProcessSDLSSecurityTypeADBuff,
                                  const etl::span<TransferFrameTC *> &framesAfterProcessSDLSSecurityTypeBDBuff) {
            framesAfterAllFramesReception = Queue(framesAfterAllFramesReceptionBuff);
            framesAfterVcReceptionTypeAD = Queue(framesAfterVcReceptionTypeADBuff);
            framesAfterVcReceptionTypeBD = Queue(framesAfterVcReceptionTypeBDBuff);
            framesAfterProcessSDLSSecurityTypeAD = Queue(framesAfterProcessSDLSSecurityTypeADBuff);
            framesAfterProcessSDLSSecurityTypeBD = Queue(framesAfterProcessSDLSSecurityTypeBDBuff);
        }

        [[nodiscard]] etl::optional<uint16_t> getTypeAdPacketCapacity() const {
            return typeAdPacketCapacity;
        }

        [[nodiscard]] etl::optional<uint16_t> getTypeBdPacketCapacity() const {
            return typeBdPacketCapacity;
        }

        void incrementTypeAdPacketCapacity(const uint16_t amount) {
            this->typeAdPacketCapacity += amount;
        }

        void incrementTypeBdPacketCapacity(const uint16_t amount) {
            this->typeBdPacketCapacity += amount;
        }

        [[nodiscard]] bool getsegmentHeaderPresent() const {
            return segmentHeaderPresent;
        }

        [[nodiscard]] bool getBlocking() const {
            return blocking;
        }

        [[nodiscard]] bool getCopInEffect() const {
            return copInEffect;
        }

        [[nodiscard]] Defs::DataFieldContent getDataFieldContent() const {
            return dataFieldContent;
        }

    private:
        /**
         * @brief Determines whether the Segment Header field is present if TC transfer frames
         * (enables MAP services for Type-AD/BD packets).
         */
        const bool segmentHeaderPresent;

        /**
         * @brief Determines whether smaller data units can be combined into a single TC transfer frame.
         * @note Applies for Type-AD/BD frames in case MAP services are disabled (segmentHeaderPresent == false)
         */
        const bool blocking;

        /**
         * @brief Determines whether FARM procedures will apply for frames in this virtual channel (should be set to
         *        true if a FARM instance is created)
         */
        const bool copInEffect;

        /**
         * @brief Whether Packets or VCA_SDUs are used
         */
        const Defs::DataFieldContent dataFieldContent;

        /**
         * @brief States how many Type-AD packets this channel should support (used during the memory pool allocation process).
         */
        uint16_t typeAdPacketCapacity;

        /**
         *  @brief States how many Type-AD packets this channel should support (used during the memory pool allocation process).
         */
        uint16_t typeBdPacketCapacity;

        /**
         * @brief Stores pointers to TC frame pointers after all frames reception and before vcReception
         */
        Queue<TransferFrameTC*> framesAfterAllFramesReception;

        /**
         * @brief Stores pointers to Type-AD TC frame pointers after vc reception and before security processing
         */
        Queue<TransferFrameTC*> framesAfterVcReceptionTypeAD;

        /**
         * @brief Stores pointers to Type-AD TC frame pointers after vc reception and before security processing
         */
        Queue<TransferFrameTC*> framesAfterVcReceptionTypeBD;

        /**
         * @brief Stores pointers to Type-AD TC frame pointers after security processing and before packet extraction
         * @note  This queue is used only in the scenario where MAP channels do not exist under this virtual
         *        channel (segmentHeaderPresent == false)
         */
        Queue<TransferFrameTC*> framesAfterProcessSDLSSecurityTypeAD;

        /**
         * @brief Stores pointers to Type-BD TC frame pointers after security processing and before packet extraction
         * @note  This queue is used only in the scenario where MAP channels do not exist under this virtual
         *        channel (segmentHeaderPresent == false)
         */
        Queue<TransferFrameTC*> framesAfterProcessSDLSSecurityTypeBD;
#ifdef ENABLE_CHANNEL_QUEUE_ACCESS
    public:
        Queue<TransferFrameTC*>& getFramesAfterAllFramesReception() const {
            return framesAfterAllFramesReception;
        }

        Queue<TransferFrameTC*>& getFramesAfterVcReceptionTypeAD() const {
            return framesAfterVcReceptionTypeAD;
        }

        Queue<TransferFrameTC*>& getFramesAfterVcReceptionTypeBD() const {
            return framesAfterVcReceptionTypeBD;
        }

        Queue<TransferFrameTC*>& getFramesAfterProcessSDLSSecurityTypeAD() const {
            return framesAfterProcessSDLSSecurityTypeAD;
        }

        Queue<TransferFrameTC*>& getFramesAfterProcessSDLSSecurityTypeBD() const {
            return framesAfterProcessSDLSSecurityTypeBD;
        }
#endif
    };
#endif // INCLUDE_SPACE_SEGMENT_CODE

#ifdef INCLUDE_GROUND_SEGMENT_CODE
    class VirtualChannelGsTc : public VirtualChannelBase {
        friend class FrameOperationProcedure;
        friend class GroundSegmentTcDataHandling;
    public:
        explicit VirtualChannelGsTc(const uint8_t vcid, const uint16_t parentScid,
                                    const uint8_t vcRepetitionsTypeAD, const uint8_t vcRepetitionsTypeBD,
                                    const bool segmentHeaderPresent, const bool blocking,
                                    const bool copInEffect,
                                    const uint16_t associatedSdlsSPI,
                                    const Defs::DataFieldContent dataFieldContent,
                                    const uint16_t frameCapacity,
                                    const uint16_t typeAdPacketCapacity,
                                    const uint16_t typeBdPacketCapacity)
            : VirtualChannelBase(vcid, parentScid, associatedSdlsSPI, frameCapacity), vcRepetitionsTypeAD(vcRepetitionsTypeAD),
              vcRepetitionsTypeBC(vcRepetitionsTypeBC),
              segmentHeaderPresent(segmentHeaderPresent),
              blocking(blocking), copInEffect(copInEffect), dataFieldContent(dataFieldContent),
              typeAdPacketCapacity(typeAdPacketCapacity), typeBdPacketCapacity(typeBdPacketCapacity) {}

        void initializeContainers(const etl::span<uint16_t> &packetLengthsTypeADBuff,
                                  const etl::span<uint8_t> &packetOctetsTypeADBuff,
                                  const etl::span<uint16_t> &packetLengthsTypeBDBuff,
                                  const etl::span<uint8_t> &packetOctetsTypeBDBuff,
                                  const etl::span<TransferFrameTC *> &framesAfterPacketProcessingBuff,
                                  const etl::span<TransferFrameTC *> &framesAfterApplySDLSSecurityBuff) {
            packetLengthsTypeAD = Queue(packetLengthsTypeADBuff);
            packetOctetsTypeAD = Queue(packetOctetsTypeADBuff);
            packetLengthsTypeBD = Queue(packetLengthsTypeBDBuff);
            packetOctetsTypeBD = Queue(packetOctetsTypeBDBuff);
            framesAfterPacketProcessing = Queue(framesAfterPacketProcessingBuff);
            framesAfterApplySDLSSecurity = Dequeue(framesAfterApplySDLSSecurityBuff);
        }

        [[nodiscard]] uint16_t getTypeAdPacketCapacity() const {
            return typeAdPacketCapacity;
        }

        [[nodiscard]] uint16_t getTypeBdPacketCapacity() const {
            return typeBdPacketCapacity;
        }

        void incrementTypeAdPacketCapacity(const uint16_t amount) {
            this->typeAdPacketCapacity += amount;
        }

        void incrementTypeBdPacketCapacity(const uint16_t amount) {
            this->typeBdPacketCapacity += amount;
        }

        [[nodiscard]] uint8_t getVcRepetitionsTypeAD() const {
            return vcRepetitionsTypeAD;
        }

        [[nodiscard]] uint8_t getVcRepetitionsTypeBC() const {
            return vcRepetitionsTypeBC;
        }

        [[nodiscard]] bool getsegmentHeaderPresent() const {
            return segmentHeaderPresent;
        }

        [[nodiscard]] bool getBlocking() const {
            return blocking;
        }

        [[nodiscard]] bool getCopInEffect() const {
            return copInEffect;
        }

        [[nodiscard]] Defs::DataFieldContent getDataFieldContent() const {
            return dataFieldContent;
        }
    private:
        /**
         * @brief Determines the number of times a Type-AD frame will be repeated in transmission to Channel Coding Layer.
         *        Note that TYPE-BD frames do not get retransmitted, since by definition they need to be expedited.
         */
        const uint8_t vcRepetitionsTypeAD;

        /**
         * @brief Determines the number of times a Type-BC frame will be repeated in transmission to Channel Coding Layer.
         */
        const uint8_t vcRepetitionsTypeBC;

        /**
         * @brief Determines whether the Segment Header field is present if TC transfer frames
         * (enables MAP services for Type-AD/BD packets).
         */
        const bool segmentHeaderPresent;

        /**
         * @brief Determines whether smaller data units can be combined into a single TC transfer frame.
         * @note Applies for Type-AD/BD frames in case MAP services are disabled (segmentHeaderPresent == false)
         */
        const bool blocking;

        /**
         * @brief Determines whether FOP procedures will apply for frames in this virtual channel (should be set to
         *        true if a FOP instance is created)
         */
        const bool copInEffect;

        /**
         * @brief Whether Packets or VCA_SDUs are used
         */
        const Defs::DataFieldContent dataFieldContent;

        /**
         * @brief States how many Type-AD packets this channel should support (used during the memory pool allocation process).
         */
        uint16_t typeAdPacketCapacity;

        /**
         *  @brief States how many Type-AD packets this channel should support (used during the memory pool allocation process).
         */
        uint16_t typeBdPacketCapacity;

        /**
         * @brief Queue that stores lengths of Type-AD packets that will eventually be concatenated to Type-AD TC
         *        transfer frames, by the packet processing data handling function
         * @note  This queue is used only in the scenario where MAP channels do not exist under this virtual
         *        channel (segmentHeaderPresent == false)
         */
        Queue<uint16_t> packetLengthsTypeAD;

        /**
         * @brief Queue that stores octets of Type-AD packets that will eventually be concatenated to Type-AD TC
         *        transfer frames, by the packet processing data handling function
         * @note  This queue is used only in the scenario where MAP channels do not exist under this virtual
         *        channel (segmentHeaderPresent == false)
         */
        Queue<uint8_t> packetOctetsTypeAD;

        /**
         * @brief Queue that stores lengths of Type-BD packets that will eventually be concatenated to Type-BD TC
         *        transfer frames, by the packet processing data handling function
         * @note  This queue is used only in the scenario where MAP channels do not exist under this virtual
         *        channel (segmentHeaderPresent == false)
         */
        Queue<uint16_t> packetLengthsTypeBD;

        /**
         * @brief Queue that stores octets of Type-BD packets that will eventually be concatenated to Type-BD TC
         *        transfer frames, by the packet processing data handling function
         * @note  This queue is used only in the scenario where MAP channels do not exist under this virtual
         *        channel (segmentHeaderPresent == false)
         */
        Queue<uint8_t> packetOctetsTypeBD;

        /**
         * @brief Stores pointers to TC frame pointers after packet processing and before security processing
         */
        Queue<TransferFrameTC*> framesAfterPacketProcessing;

        /**
         * @brief Stores pointers to TC frame pointers after security processing and before vc generation
         */
        Dequeue<TransferFrameTC*> framesAfterApplySDLSSecurity;

#ifdef ENABLE_CHANNEL_QUEUE_ACCESS
    public:
        Queue<uint16_t>& getPacketLengthsTypeAD() const {
            return packetLengthsTypeAD;
        }

        Queue<uint8_t>& getPacketOctetsTypeAD() const {
            return packetOctetsTypeAD;
        }

        Queue<uint16_t>& getPacketLengthsTypeBD() const {
            return packetLengthsTypeBD;
        }

        Queue<uint8_t>& getPacketOctetsTypeBD() const {
            return packetOctetsTypeBD;
        }

        Queue<TransferFrameTC*>& getFramesAfterPacketProcessing() const {
            return framesAfterPacketProcessing;
        }

        Dequeue<TransferFrameTC*>& getFramesAfterApplySDLSSecurity() const {
            return framesAfterApplySDLSSecurity;
        }
#endif // ENABLE_CHANNEL_QUEUE_ACCESS
    };
#endif // INCLUDE_GROUND_SEGMENT_CODE
} // namespace CCSDSDataLinkLayer