/**
 * @file MapChannel.hpp
 */

#pragma once
#include <cstdint>
#include "etl/span.h"
#include "ExternalContainers.hpp"

namespace CCSDSDataLinkLayer {
    class SpaceSegmentTcDataHandling;
    class GroundSegmentTcDataHandling;

    /**
     * Base map channel class containing parameters common among space and ground segment code
     */
    class MAPChannelBase {
    public:
        MAPChannelBase(const Defs::Mapid mapid, const Defs::Vcid parentVcid, const Defs::Scid parentScid,
                       const bool blocking, const bool segmentation, const Defs::Spi associatedSdlsSPI,
                       const Defs::DataFieldContent dataFieldContent, const uint16_t frameCapacity,
                       const uint16_t typeAdPacketCapacity, const uint16_t typeBdPacketCapacity,
                       const uint16_t maxExpectedPacketSize, const uint8_t priorityWeight)
            : channelMutex(Mutex()), mapid(mapid & 0x3FU), parentVcid(parentVcid & 0x3FU), parentScid(parentScid), blocking(blocking),
              segmentation(segmentation),  dataFieldContent(dataFieldContent), frameCapacity(frameCapacity), typeAdPacketCapacity(typeAdPacketCapacity),
              typeBdPacketCapacity(typeBdPacketCapacity), maxExpectedPacketSize(maxExpectedPacketSize), priorityWeight(priorityWeight){
            if (associatedSdlsSPI != 0) {
                this->associatedSdlsSPI = etl::optional(associatedSdlsSPI);
            }
        }

        /**
         * @brief Protects against concurrent access to resources
         */
        Mutex channelMutex;

        [[nodiscard]] Defs::Mapid getMapid() const {
            return mapid;
        }

        [[nodiscard]] Defs::Vcid getParentVcid() const {
            return parentVcid;
        }

        [[nodiscard]] Defs::Scid getParentScid() const {
            return parentScid;
        }

        [[nodiscard]] bool getBlocking() const {
            return blocking;
        }

        [[nodiscard]] bool getSegmentation() const {
            return segmentation;
        }

        [[nodiscard]] etl::optional<Defs::Spi> getAssociatedSdlsSPI() const {
            return associatedSdlsSPI;
        }

        [[nodiscard]] uint16_t getFrameCapacity() const {
            return frameCapacity;
        }

        [[nodiscard]] uint16_t getTypeAdPacketCapacity() const {
            return typeAdPacketCapacity;
        }

        [[nodiscard]] uint16_t getTypeBdPacketCapacity() const {
            return typeBdPacketCapacity;
        }

        [[nodiscard]] uint16_t getMaxExpectedPacketSize() const {
            return maxExpectedPacketSize;
        }

        [[nodiscard]] Defs::DataFieldContent getDataFieldContent() const {
            return dataFieldContent;
        }

        [[nodiscard]] uint8_t getPriorityWeight() const {
            return priorityWeight;
        }

    protected:
        /**
         * @brief MAP Channel Id
         * @details 6 bit identifier for this MAP channel
         * @see p. 2.1.3 of CCSDS TC SPACE DATA LINK PROTOCOL
         */
        const Defs::Mapid mapid;

        /**
         * @brief vcid of parent virtual channel
         */
        const Defs::Vcid parentVcid;

        /**
         * @brief Scid of master channel. This needs to be stored here, since there might
         *        be 2 map channels with that belong to virtual channels with the same vcid,
         *        where each virtual channel belongs to a different master channel
         */
        const Defs::Scid parentScid;

        /**
         * @brief Determines whether smaller data units can be combined into a single TC transfer frame
         * (applies for Type AD, BD frames). Supersedes blockingTC flag of virtual channel.
         */
        const bool blocking;

        /**
         * @brief Determines whether large packets can be segmented to multiple TC transfer frames
         * (applies for Type AD, BD).
         */
        const bool segmentation;

        /**
         * @brief The presence of an SDLS SPI (Security Parameter Index) shall indicate that this map channel
         *        instance is associated with a specific SecurityAssociation, therefore the according authentication
         *        and encryption services will be applied to its frames.
         */
        etl::optional<Defs::Spi> associatedSdlsSPI;

        /**
         * @brief Whether Packets or VCA_SDUs are used
         */
        const Defs::DataFieldContent dataFieldContent;

        /**
         * @brief States how many frames this channel should support (used during the memory pool allocation process).
         */
        const uint16_t frameCapacity;

        /**
         * @brief States how many Type-AD packets this channel should support (used during the memory pool allocation process).
         */
        const uint16_t typeAdPacketCapacity;

        /**
         *  @brief States how many Type-AD packets this channel should support (used during the memory pool allocation process).
         */
        const uint16_t typeBdPacketCapacity;

        /**
         * @brief The maximum packet size this channel can accept. Overriden in the scenario MAP channels exist
         *        under this virtual channel
         */
        const uint16_t maxExpectedPacketSize;

        /**
         * @brief In order to decide which virtual channel frames should be transferred to the master channel first,
         *        a weighted priority scheduling policy is used. The higher the weight, the higher the priority of the
         *        respective channel.
         */
        const uint8_t priorityWeight;
    };

    /**
     * Space segment variant of a map channel
     */
#ifdef INCLUDE_SPACE_SEGMENT_CODE
    class MAPChannelSs : public MAPChannelBase {
    public:
        MAPChannelSs(const Defs::Mapid mapid, const Defs::Vcid parentVcid, const Defs::Scid parentScid,
                     const bool blocking, const bool segmentation, const Defs::Spi associatedSdlsSPI,
                     const Defs::DataFieldContent dataFieldContent, const uint16_t frameCapacity, const uint16_t typeAdPacketCapacity,
                     const uint16_t typeBdPacketCapacity, const uint16_t maxExpectedPacketSize, const uint8_t priorityWeight)
            : MAPChannelBase(mapid, parentVcid, parentScid, blocking, segmentation, associatedSdlsSPI, dataFieldContent, frameCapacity,
                typeAdPacketCapacity, typeBdPacketCapacity, maxExpectedPacketSize, priorityWeight),
        segmentedPacketConstructor(Defs::SegmentedPacketConstructorTc(maxExpectedPacketSize)) {}

        void initializeContainers(
            const etl::span<TransferFrameTC*>& framesAfterProcessSdlsSecurityTypeADBuff,
            const etl::span<TransferFrameTC*>& framesAfterProcessSdlsSecurityTypeBDBuff,
            const etl::span<uint8_t> &segmentedPacketBuff) {
            framesAfterProcessSDLSSecurityTypeAD = Queue(framesAfterProcessSdlsSecurityTypeADBuff);
            framesAfterProcessSDLSSecurityTypeBD = Queue(framesAfterProcessSdlsSecurityTypeBDBuff);
            segmentedPacketConstructor.initializeQueue(segmentedPacketBuff);
        }

        /**
         * @brief Stores pointers to Type-AD TC frame pointers after security processing and before packet extraction
         */
        Queue<TransferFrameTC*> framesAfterProcessSDLSSecurityTypeAD;

        /**
         * @brief Stores pointers to Type-BD TC frame pointers after security processing and before packet extraction
         */
        Queue<TransferFrameTC*> framesAfterProcessSDLSSecurityTypeBD;

        /**
         * @brief Used to build segmented packets and contain information about the previous extracted packet/packet piece
         */
        Defs::SegmentedPacketConstructorTc segmentedPacketConstructor;
    };
#endif // INCLUDE_SPACE_SEGMENT_CODE

    /**
     * Ground segment variant of a map channel
     */
#ifdef INCLUDE_GROUND_SEGMENT_CODE
    class MAPChannelGs : public MAPChannelBase{
    public:
        MAPChannelGs(const Defs::Mapid mapid, const Defs::Vcid parentVcid, const Defs::Scid parentScid, const bool blocking, const bool segmentation,
            const Defs::Spi associatedSdlsSPI, const Defs::DataFieldContent dataFieldContent, uint16_t frameCapacity, const uint16_t typeAdPacketCapacity,
            const uint16_t typeBdPacketCapacity, const uint16_t maxExpectedPacketSize, const uint8_t priorityWeight)
            : MAPChannelBase(mapid, parentVcid, parentScid, blocking, segmentation, associatedSdlsSPI, dataFieldContent,
                frameCapacity, typeAdPacketCapacity, typeBdPacketCapacity, maxExpectedPacketSize, priorityWeight) {}

        void initializeContainers(const etl::span<uint16_t>& packetLengthsTypeADBuff,
            const etl::span<uint8_t>& packetOctetsTypeADBuff,
            const etl::span<uint16_t>& packetLengthsTypeBDBuff,
            const etl::span<uint8_t>& packetOctetsTypeBDBuff) {
            packetLengthsTypeAD = Queue(packetLengthsTypeADBuff);
            packetOctetsTypeAD = Queue(packetOctetsTypeADBuff);
            packetLengthsTypeBD = Queue(packetLengthsTypeBDBuff);
            packetOctetsTypeBD = Queue(packetOctetsTypeBDBuff);
        }

        /**
         * @brief Queue that stores lengths of packets that will eventually be concatenated to Type-AD
         * transfer frame data by packetProcessing.
         */
        Queue<uint16_t> packetLengthsTypeAD;

        /**
         * @brief Queue that stores the bytes of packets that will eventually be concatenated to Type-AD
         * transfer frame data by packetProcessing.
         */
        Queue<uint8_t> packetOctetsTypeAD;

        /**
         * @brief Queue that stores lengths of packets that will eventually be concatenated to Type-BD
         * transfer frame data by packetProcessing.
         */
        Queue<uint16_t> packetLengthsTypeBD;

        /**
         * @brief Queue that stores the bytes of packets that will eventually be concatenated to Type-BD
         * transfer frame data by packetProcessing.
         */
        Queue<uint8_t> packetOctetsTypeBD;
    };
#endif // INCLUDE_GROUND_SEGMENT_CODE
} // namespace CCSDSDataLinkLayer
