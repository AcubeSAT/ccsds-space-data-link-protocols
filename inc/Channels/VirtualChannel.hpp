/**
 * @file  VirtualChannel.hpp
 */

#pragma once
#include <cstdint>
#include "ExternalContainers.hpp"
#include "TransferFrameTC.hpp"
#include "TransferFrameTM.hpp"
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
        explicit VirtualChannelBase(const Defs::Vcid vcid, const Defs::Scid parentScid,
                                    const Defs::Spi associatedSdlsSPI, const uint16_t frameCapacity,
                                    const uint16_t maxExpectedPacketSize,
                                    const uint8_t priorityWeight)
            : channelMutex(Mutex()), vcid(vcid & 0x3FU), parentScid(parentScid & 0x03FFU),
              associatedSdlsSPI(associatedSdlsSPI), frameCapacity(frameCapacity),
              maxExpectedPacketSize(maxExpectedPacketSize), priorityWeight(priorityWeight) {
            if (associatedSdlsSPI != 0) {
                this->associatedSdlsSPI = etl::optional(associatedSdlsSPI);
            }
        }

        /**
         * @brief Protects against concurrent access to resources
         */
        Mutex channelMutex;

        [[nodiscard]] Defs::Vcid getVcid() const {
            return vcid;
        }

        [[nodiscard]] Defs::Scid getParentScid() const {
            return parentScid;
        }

        [[nodiscard]] etl::optional<Defs::Spi> getAssociatedSdlsSPI() const {
            return associatedSdlsSPI;
        }

        [[nodiscard]] uint16_t getFrameCapacity() const {
            return frameCapacity;
        }

        void incrementFrameCapacity(const uint16_t amount) {
            this->frameCapacity += amount;
        }

        [[nodiscard]] uint16_t getMaxExpectedPacketSize() const {
            return maxExpectedPacketSize;
        }

        [[nodiscard]] uint8_t getPriorityWeight() const {
            return priorityWeight;
        }

    protected:
        /**
         * @brief Global Virtual Channel Identifier.
         * @details 6 bit identifier for this virtual channel
         * @see p. 2.1.3 of CCSDS TC SPACE DATA LINK PROTOCOL
         */
        const Defs::Vcid vcid;

        /**
         * @brief scid of parent master channel
         */
        const Defs::Scid parentScid;

        /**
         * @brief The presence of an SDLS SPI (Security Parameter Index) shall indicate that this virtual channel
         *        instance is associated with a specific SecurityAssociation, therefore the according authentication
         *        and encryption services will be applied to its frames.
         */
        etl::optional<Defs::Spi> associatedSdlsSPI;

        /**
         * @brief States how many frames this channel should support (used during the memory pool allocation process).
         *        In the derivative VirtualChannelSsTc, VirtualChannelGsTc, it should have a value 0, if map channels
         *        exist (this is because the actual amount of frames a virtual channel can support will be determined
         *        by the sum of frame capacities of the map channels)
         */
        uint16_t frameCapacity;

        /**
         * @brief The maximum packet size this channel can accept. Overriden in the scenario MAP channels exist
         *        under this virtual channel
         */
        uint16_t maxExpectedPacketSize;

        /**
         * @brief In order to decide which virtual channel frames should be transferred to the master channel first,
         *        a weighted priority scheduling policy is used. The higher the weight, the higher the priority of the
         *        respective channel.
         */
        const uint8_t priorityWeight;
    };

#ifdef INCLUDE_SPACE_SEGMENT_CODE
    class VirtualChannelSsTm : public VirtualChannelBase {
    public:
        explicit VirtualChannelSsTm(const Defs::Vcid vcid, const Defs::Scid parentScid, const uint8_t vcRepetitions,
                                     const bool secondaryHeaderPresent, const uint8_t secondaryHeaderLength,
                                     const bool operationalControlFieldPresent,
                                     const Defs::SynchronizationFlag synchronization,
                                     const Defs::Spi associatedSdlsSPI, const uint16_t frameCapacity,
                                     const uint16_t packetCapacity, const uint16_t maxExpectedPacketSize, const uint8_t priorityWeight)
            : VirtualChannelBase(vcid, parentScid, associatedSdlsSPI, frameCapacity, maxExpectedPacketSize, priorityWeight),
              vcRepetitions(vcRepetitions),
              operationalControlFieldPresent(operationalControlFieldPresent),
              secondaryHeaderPresent(secondaryHeaderPresent),
              secondaryHeaderLength(secondaryHeaderLength),
              synchronization(synchronization), virtualChannelFrameCount(0), packetCapacity(packetCapacity) {}

        void initializeContainers(const etl::span<uint16_t> &packetLengthsBuff,
                                  const etl::span<uint8_t> &packetOctetsBuff,
                                  const etl::span<uint8_t> &secondaryHeaderDataFieldOctetsBuff,
                                  const etl::span<TransferFrameTM*> framesAfterVcGenerationBuff,
                                  const etl::span<TransferFrameTM*> framesAfterSecondaryHeaderPlacementBuff) {
            packetLengths = Dequeue(packetLengthsBuff);
            packetOctets = Dequeue(packetOctetsBuff);
            framesAfterVcGeneration = Queue(framesAfterVcGenerationBuff);
            framesAfterSecondaryHeaderPlacement = Queue(framesAfterSecondaryHeaderPlacementBuff);
            secondaryHeaderDataFieldOctets = Queue(secondaryHeaderDataFieldOctetsBuff);
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

        void resetVirtualChannelFrameCount() {
            virtualChannelFrameCount = 0;
        }

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

        /**
         * @brief Queue that stores octets will eventually be appended to TM secondary header
         */
        Queue<uint8_t> secondaryHeaderDataFieldOctets;

        /**
         * @brief Buffer that holds pointers to TM frames already processed by the vc generation data handling function
         *        and before secondary header is applied
         */
        Queue<TransferFrameTM*> framesAfterVcGeneration;

        /**
         * @brief Buffer that holds pointers to TM frames that had their secondary header placed and before SDLS processing
         */
        Queue<TransferFrameTM*> framesAfterSecondaryHeaderPlacement;

    private:
        /**
         * @brief Determines the number of times a frame will be repeated in transmission to the Channel Coding Layer.
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
    };
#endif // INCLUDE_SPACE_SEGMENT_CODE

    // unimplemented
    // class VirtualChannelGS_TM : public  VirtualChannelBase {
    //
    // }

#ifdef INCLUDE_SPACE_SEGMENT_CODE
    class VirtualChannelSsTc : public VirtualChannelBase {
    public:
        explicit VirtualChannelSsTc(const Defs::Vcid vcid, const Defs::Scid parentScid,
                                     const bool segmentHeaderPresent, const bool blocking,
                                     const bool copInEffect,
                                     const Defs::Spi associatedSdlsSPI,
                                     const Defs::DataFieldContent dataFieldContent,
                                     const uint16_t frameCapacity,
                                     const uint16_t typeAdPacketCapacity,
                                     const uint16_t typeBdPacketCapacity,
                                     const uint16_t maxExpectedPacketSize,
                                     const uint8_t priorityWeight)
            : VirtualChannelBase(vcid, parentScid, associatedSdlsSPI, frameCapacity, maxExpectedPacketSize, priorityWeight),
              segmentHeaderPresent(segmentHeaderPresent), blocking(blocking), copInEffect(copInEffect),
              dataFieldContent(dataFieldContent), typeAdPacketCapacity(typeAdPacketCapacity),
              typeBdPacketCapacity(typeBdPacketCapacity), clcwStatusField(0) {}

        void initializeContainers(const etl::span<TransferFrameTC *> &framesAfterAllFramesReceptionBuff,
                                  const etl::span<TransferFrameTC *> &framesAfterVcReceptionTypeADBuff,
                                  const etl::span<TransferFrameTC *> &framesAfterVcReceptionTypeBDBuff,
                                  const etl::span<TransferFrameTC *> &framesAfterProcessSDLSSecurityTypeADBuff,
                                  const etl::span<TransferFrameTC *> &framesAfterProcessSDLSSecurityTypeBDBuff) {
            framesAfterAllFramesReception = Queue(framesAfterAllFramesReceptionBuff);
            framesAfterVcReceptionTypeAD = Queue(framesAfterVcReceptionTypeADBuff);
            framesAfterVcReceptionTypeBD = CircularBuffer(framesAfterVcReceptionTypeBDBuff);
            framesAfterProcessSDLSSecurityTypeAD = Queue(framesAfterProcessSDLSSecurityTypeADBuff);
            framesAfterProcessSDLSSecurityTypeBD = Queue(framesAfterProcessSDLSSecurityTypeBDBuff);
        }

        [[nodiscard]] etl::optional<uint16_t> getTypeAdPacketCapacity() const {
            return typeAdPacketCapacity;
        }

        [[nodiscard]] etl::optional<uint16_t> getTypeBdPacketCapacity() const {
            return typeBdPacketCapacity;
        }

        [[nodiscard]] bool getSegmentHeaderPresent() const {
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

        [[nodiscard]] uint8_t getClcwStatusField() const {
            return clcwStatusField;
        }

        void setClcwStatusField(const uint8_t clcwStatusField) {
            this->clcwStatusField = clcwStatusField;
        }

        /**
         * @brief Stores pointers to TC frame pointers after all frames reception and before vcReception
         */
        Queue<TransferFrameTC*> framesAfterAllFramesReception;

        /**
         * @brief Stores pointers to Type-AD TC frame pointers after vc reception and before security processing
         */
        Queue<TransferFrameTC*> framesAfterVcReceptionTypeAD;

        /**
         * @brief Stores pointers to Type-BD TC frame pointers after vc reception and before security processing.
         *        Due to type BD frames being expedited, this buffer is circular, meaning oldest frames are deleted
         *        in case of congestion.
         *
         */
        CircularBuffer<TransferFrameTC*> framesAfterVcReceptionTypeBD;

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

        /**
         * @brief Ued by FARM to fill the status field when generating CLCWs. Updated by the user, using
         *        the respective service.
         */
        uint8_t clcwStatusField;

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
    };
#endif // INCLUDE_SPACE_SEGMENT_CODE

#ifdef INCLUDE_GROUND_SEGMENT_CODE
    class VirtualChannelGsTc : public VirtualChannelBase {
    public:
        explicit VirtualChannelGsTc(const Defs::Vcid vcid, const Defs::Scid parentScid,
                                    const uint8_t vcRepetitionsTypeAD, const uint8_t vcRepetitionsTypeBC,
                                    const bool segmentHeaderPresent, const bool blocking,
                                    const bool copInEffect,
                                    const Defs::Spi associatedSdlsSPI,
                                    const Defs::DataFieldContent dataFieldContent,
                                    const uint16_t frameCapacity,
                                    const uint16_t typeAdPacketCapacity,
                                    const uint16_t typeBdPacketCapacity,
                                    const uint16_t maxExpectedPacketSize,
                                    const uint16_t priorityWeight)
            : VirtualChannelBase(vcid, parentScid, associatedSdlsSPI, frameCapacity, maxExpectedPacketSize, priorityWeight),
              vcRepetitionsTypeAD(vcRepetitionsTypeAD),
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

        [[nodiscard]] uint8_t getVcRepetitionsTypeAD() const {
            return vcRepetitionsTypeAD;
        }

        [[nodiscard]] uint8_t getVcRepetitionsTypeBC() const {
            return vcRepetitionsTypeBC;
        }

        [[nodiscard]] bool getSegmentHeaderPresent() const {
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


    };
#endif // INCLUDE_GROUND_SEGMENT_CODE
} // namespace CCSDSDataLinkLayer