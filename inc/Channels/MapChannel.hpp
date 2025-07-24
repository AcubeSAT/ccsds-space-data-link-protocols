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
        MAPChannelBase(const uint8_t mapid, const uint8_t parentVcid, const uint16_t parentScid, const bool blocking, const bool segmentation,
                       const uint16_t associatedSdlsSPI, const Defs::DataFieldContent dataFieldContent, const uint16_t frameCapacity, const uint16_t typeAdPacketCapacity, const uint16_t typeBdPacketCapacity)
            : channelMutex(Mutex()), mapid(mapid & 0x3FU), parentVcid(parentVcid & 0x3FU), parentScid(parentScid), blocking(blocking),
              segmentation(segmentation),  dataFieldContent(dataFieldContent), frameCapacity(frameCapacity), typeAdPacketCapacity(typeAdPacketCapacity),
              typeBdPacketCapacity(typeBdPacketCapacity){
            if (associatedSdlsSPI != 0) {
                this->associatedSdlsSPI = etl::optional(associatedSdlsSPI);
            }
        }

        /**
         * @brief Protects against concurrent access to resources
         */
        Mutex channelMutex;

        [[nodiscard]] uint32_t getMapid() const {
            return mapid;
        }

        [[nodiscard]] uint8_t getParentVcid() const {
            return parentVcid;
        }

        [[nodiscard]] uint16_t getParentScid() const {
            return parentScid;
        }

        [[nodiscard]] bool getBlocking() const {
            return blocking;
        }

        [[nodiscard]] bool getSegmentation() const {
            return segmentation;
        }

        [[nodiscard]] etl::optional<uint16_t> getAssociatedSdlsSPI() const {
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

        [[nodiscard]] Defs::DataFieldContent getDataFieldContent() const {
            return dataFieldContent;
        }

    protected:
        /**
         * @brief MAP Channel Id
         * @details 6 bit identifier for this MAP channel
         * @see p. 2.1.3 of CCSDS TC SPACE DATA LINK PROTOCOL
         */
        const uint8_t mapid;

        /**
         * @brief vcid of parent virtual channel
         */
        const uint8_t parentVcid;

        /**
         * @brief Scid of master channel. This needs to be stored here, since there might
         *        be 2 map channels with that belong to virtual channels with the same vcid,
         *        where each virtual channel belongs to a different master channel
         */
        const uint16_t parentScid;

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
        etl::optional<uint16_t> associatedSdlsSPI;

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
    };

    /**
     * Space segment variant of a map channel
     */
#ifdef INCLUDE_SPACE_SEGMENT_CODE
    class MAPChannelSs : public MAPChannelBase {
        friend class SpaceSegmentTcDataHandling;
    public:
        MAPChannelSs(const uint8_t mapid, const uint8_t parentVcid, const uint16_t parentScid, const bool blocking, const bool segmentation,
            const uint16_t associatedSdlsSPI, const Defs::DataFieldContent dataFieldContent, const uint16_t frameCapacity, const uint16_t typeAdPacketCapacity,
            const uint16_t typeBdPacketCapacity)
            : MAPChannelBase(mapid, parentVcid, parentScid, blocking, segmentation, associatedSdlsSPI, dataFieldContent, frameCapacity,
                typeAdPacketCapacity, typeBdPacketCapacity), segmentedPacketConstructor(Defs::SegmentedPacketConstructorTc()) {}

        void initializeContainers(
            const etl::span<TransferFrameTC*>& framesAfterProcessSdlsSecurityTypeADBuff,
            const etl::span<TransferFrameTC*>& framesAfterProcessSdlsSecurityTypeBDBuff) {
            framesAfterProcessSDLSSecurityTypeAD = Queue(framesAfterProcessSdlsSecurityTypeADBuff);
            framesAfterProcessSDLSSecurityTypeBD = Queue(framesAfterProcessSdlsSecurityTypeBDBuff);
        }

    private:
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

        // give the user access while debugging/testing
#ifdef ENABLE_CHANNEL_QUEUE_ACCESS
    public:
        Queue<TransferFrameTC*>& getFramesAfterProcessSdlsSecurity()  {
            return framesAfterProcessSdlsSecurity;
        }

        Queue<TransferFrameTC*>& getFramesAfterProcessSDLSSecurityTypeAD() {
            return framesAfterProcessSDLSSecurityTypeAD;
        }

        Queue<TransferFrameTC*>& getFramesAfterProcessSDLSSecurityTypeBD() {
            return framesAfterProcessSDLSSecurityTypeBD;
        }

        Defs::SegmentedPacketConstructorTc& getSegmentedPacketConstructor() {
            return segmentedPacketConstructor;
        }
#endif // ENABLE_CHANNEL_QUEUE_ACCESS
    };
#endif // INCLUDE_SPACE_SEGMENT_CODE

    /**
     * Ground segment variant of a map channel
     */
#ifdef INCLUDE_GROUND_SEGMENT_CODE
    class MAPChannelGs : public MAPChannelBase{
        friend class GroundSegmentTcDataHandling;
    public:
        MAPChannelGs(const uint8_t mapid, const uint8_t parentVcid, const uint16_t parentScid, const bool blocking, const bool segmentation,
            const uint16_t associatedSdlsSPI, const Defs::DataFieldContent dataFieldContent, uint16_t frameCapacity, const uint16_t typeAdPacketCapacity,
            const uint16_t typeBdPacketCapacity)
            : MAPChannelBase(mapid, parentVcid, parentScid, blocking, segmentation, associatedSdlsSPI, dataFieldContent,
                frameCapacity, typeAdPacketCapacity, typeBdPacketCapacity) {}

        void initializeContainers(const etl::span<uint16_t>& packetLengthsTypeADBuff,
            const etl::span<uint8_t>& packetOctetsTypeADBuff,
            const etl::span<uint16_t>& packetLengthsTypeBDBuff,
            const etl::span<uint8_t>& packetOctetsTypeBDBuff) {
            packetLengthsTypeAD = Queue(packetLengthsTypeADBuff);
            packetOctetsTypeAD = Queue(packetOctetsTypeADBuff);
            packetLengthsTypeBD = Queue(packetLengthsTypeBDBuff);
            packetOctetsTypeBD = Queue(packetOctetsTypeBDBuff);
        }

    private:
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

        // give the user access while debugging/testing
#ifdef ENABLE_CHANNEL_QUEUE_ACCESS
    public:
        Queue<uint16_t>& getPacketLengthsTypeAD()  {
            return packetLengthsTypeAD;
        }

        Queue<uint8_t>& getPacketOctetsTypeAD()  {
            return packetOctetsTypeAD;
        }

        Queue<uint16_t>& getPacketLengthsTypeBD()  {
            return packetLengthsTypeBD;
        }

        Queue<uint8_t>& getPacketOctetsTypeBD()  {
            return packetOctetsTypeBD;
        }
#endif // ENABLE_CHANNEL_QUEUE_ACCESS
    };
#endif // INCLUDE_GROUND_SEGMENT_CODE
} // namespace CCSDSDataLinkLayer
