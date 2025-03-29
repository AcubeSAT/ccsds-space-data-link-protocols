/**
 * @file CCSDSChannelsInterface.hpp
 * @brief Contains the ChannelsInterface class, which is used for accessing channel internals.
 */

#pragma once
#include <cstdint>
#include "etl/expected.h"
#include "CCSDSChannelConfiguration.hpp"
#include "CLCW.hpp"

namespace CCSDSDataLinkLayer {
    class FrameAcceptanceReporting;
    class FrameOperationProcedure;

    /**
     * @brief A class offering a channel size agnostic interface for internal container and parameter accessing.
     *
     * @details Due to MAP/Virtual/Master channels being templated, generic accessing methods for access
     *          had to be implemented. Specifically, all methods accept one/two channel variants and take
     *          advantage of etl::visit(), ETL's alternative to std::visit(). Users of this interface are:
     *          @ref ServiceChannelSpaceSegment
     *          @ref ServiceChannelGroundSegment
     *          @ref FarmAcceptanceReporting
     *          @ref FrameOperationProcedure
     */
    class ChannelsInterface {
        friend class ServiceChannelSpaceSegment;
        friend class ServiceChannelGroundSegment;
        friend class FrameAcceptanceReporting;
        friend class FrameOperationProcedure;
#ifdef ENABLE_CHANNEL_ACCESS

    public: // give the user access when debugging/testing
#else
    private:
#endif // ENABLE_CHANNEL_ACCESS
#ifdef SPACE_SEGMENT
        /** =======================================
         *  @name MapChannelSpaceSegment operations
         *  =======================================
         *  @{
         */

        /**
         * @brief Takes a MapChannelSpaceSegment variant and returns a pointer of it's base class type.
         *        Useful for accessing common parameters.
         */
        static BaseMAPChannel* upcastToBase(MAPChannelSpaceSegmentVariant &mapChannelVariant);

        static uint16_t frameListAvailableMapChannelSpaceSegment(MAPChannelSpaceSegmentVariant &mapChannelVariant);

        /**
         * @brief Push frame pointer to the back of the frame processing list.
         */
        static etl::expected<void, MapChannelAlert>
        pushFrameMapChannelSpaceSegment(MAPChannelSpaceSegmentVariant &mapChannelVariant,
                                        TransferFrameTC *frameTC);


        /**
         * @brief Erase frame pointer from the frame processing list. Search starts from the front.
         */
        static etl::expected<void, MapChannelAlert>
        popFrameMapChannelSpaceSegment(MAPChannelSpaceSegmentVariant &mapChannelVariant,
                                       TransferFrameTC *frameTC);

        static etl::expected<TransferFrameTC *, MapChannelAlert>
        getFrameMapChannelSpaceSegment(MAPChannelSpaceSegmentVariant &mapChannelVariant,
                                       DefsAndUtils::ServiceType serviceType,
                                       DefsAndUtils::TcFrameProcessingStage processingStage);
        /**
         * @}
         */

        /** ===========================================
         *  @name VirtualChannelSpaceSegment operations
         *  ===========================================
         *  @{
         */

        /**
         * @brief Takes a VirtualChannelSpaceSegment variant and returns a pointer of it's base class type.
         *        Useful for accessing common parameters.
         */
        static BaseVirtualChannel* upcastToBase(VirtualChannelSpaceSegmentVariant &virtualChannelVariant);

        enum class VchanBuffType : uint8_t {
            UNDER_PROCESSING_TM,
            AFTER_ALL_FRAMES_RECEPTION_TC,
            AFTER_VC_GENERATION_TC_TYPE_AD,
            AFTER_VC_GENERATION_TC_TYPE_BD
        };

        static uint16_t frameListAvailableVirtualChannelSpaceSegment(
            VirtualChannelSpaceSegmentVariant &virtualChannelVariant,
            VchanBuffType vchanBuffType
        );

        /**
         * @brief Push frame pointer to the back of the specified frame processing list.
         *
         * @param vchanBuffType @ref VirtualChannelSpaceSegment contains multiple frame processing containers. This
         *                      enum member specifies the container.
         *
         * @note The user should ensure that the given virtual channel belongs to the given master channel.
         * @note The 'AFTER_VC_GENERATION_TC_TYPE_BD' container is treated as a circular buffer. In case this buffer is full,
         *       the oldest frame pointer and the master copy are deleted.
         */
        static etl::expected<void, VirtualChannelAlert>
        pushFrameVirtualChannelSpaceSegment(MasterChannelSpaceSegmentVariant &masterChannelVariant,
                                            VirtualChannelSpaceSegmentVariant &virtualChannelVariant,
                                            etl::variant<TransferFrameTM *, TransferFrameTC *> frame,
                                            VchanBuffType vchanBuffType
        );

        /**
         * @brief Pop frame pointer from the specified frame processing list.
         *
         * @param vchanBuffType @ref VirtualChannelSpaceSegment contains multiple frame processing containers. This
         *                      enum member specifies the container.
         */
        static etl::expected<void, VirtualChannelAlert>
        popFrameVirtualChannelSpaceSegment(VirtualChannelSpaceSegmentVariant &virtualChannelVariant,
                                           etl::variant<TransferFrameTM *, TransferFrameTC *> frame,
                                           VchanBuffType vchanBuffType);

        /**
         * @brief Get frame from specified frame processing list and processing stage.
         */
        static etl::expected<etl::variant<TransferFrameTM *, TransferFrameTC *>, VirtualChannelAlert>
        getFrameVirtualChannelSpaceSegment(VirtualChannelSpaceSegmentVariant &virtualChannelVariant,
                                           VchanBuffType vchanBuffType,
                                           etl::variant<DefsAndUtils::TcFrameProcessingStage,
                                               DefsAndUtils::TmFrameProcessingStage> processingStage);

        /**
         * @brief Read and increase by one the virtual channel frame counter for TM frames.
         */
        static uint8_t readAndUpdateTmFrameCountVirtualChannelSpaceSegment(
            VirtualChannelSpaceSegmentVariant &virtualChannelVariant);

        /**
         *  @brief Push a packet that will later be inserted in a TM frame.
         *  @param pushToFront: The data structures used for storing the packets are dequeues. If this parameter
         *                     is set to true, the packet data and lengths will be pushed to the front of the queues instead.
         */
        static etl::expected<void, VirtualChannelAlert>
        pushTmPacketVirtualChannelSpaceSegment(VirtualChannelSpaceSegmentVariant &virtualChannelVariant,
                                               const uint8_t *packetSource,
                                               uint16_t packetLength,
                                               bool pushToFront = false);


        /**
         * @brief Pop the length of the next packet waiting in the queue.
         *
         *  @param popFromBack: The data structures used for storing the packet are dequeues. If this parameter
         *                     is set to true, the packet length will be popped from the back of the queue instead.
         */
        static etl::expected<uint16_t, VirtualChannelAlert> popTmPacketLengthVirtualChannelSpaceSegment(
            VirtualChannelSpaceSegmentVariant &virtualChannelVariant,
            bool popFromBack = false);

        /**
         * @brief Pop a partial packet (destined for TM frames) from the virtual channel
         *        queue.
        *  @param popFromBack: The data structures used for storing the packets are dequeues. If this parameter
         *                     is set to true, the packet data and lengths will be popped from the back of the queue instead.
         * @param numOctets: The amount of octets to pop
         * @return The packet's length
         */
        static etl::expected<void, VirtualChannelAlert>
        popTmPacketSegmentVirtualChannelSpaceSegment(VirtualChannelSpaceSegmentVariant &virtualChannelVariant,
                                              uint8_t *packetDestination,
                                              uint16_t numOctets,
                                              bool popFromBack = false);
        /**
         * @}
         */

        /** ==========================================
         *  @name MasterChannelSpaceSegment operations
         *  ==========================================
         *  @{
         */

        /**
         * @brief Takes a MasterChannelSpaceSegment variant and returns a pointer of it's base class type.
         *        Useful for accessing common parameters.
         */
        static BaseMasterChannel* upcastToBase(MasterChannelSpaceSegmentVariant &masterChannelVariant);

        static uint16_t frameListAvailableMasterChannelSpaceSegment(MasterChannelSpaceSegmentVariant &masterChannelVariant);

        /**
         * @brief Push TM frame pointer to the back of the processing list.
         */
        static etl::expected<void, MasterChannelAlert> pushTmFrameMasterChannelSpaceSegment(
            MasterChannelSpaceSegmentVariant &masterChannelVariant,
            TransferFrameTM *frameTM);

        /**
         * @brief Erase TC frame pointer from the processing list. Search starts from the front.
         */
        static etl::expected<void, MasterChannelAlert> popTmFrameMasterChannelSpaceSegment(
            MasterChannelSpaceSegmentVariant &masterChannelVariant,
            TransferFrameTM *frameTM);

        static etl::expected<TransferFrameTM *, MasterChannelAlert>
        getTmFrameMasterChannelSpaceSegment(MasterChannelSpaceSegmentVariant &masterChannelVariant,
                                            DefsAndUtils::TmFrameProcessingStage processingStage);

        /**
         * @brief Indicates if there is enough capacity for storing new frames.
         *
         * @param numberOfFrames Used to determine if there is enough space in the master copy buffer.
         * @param numberOfOctets The total amount of bytes/octets that need to be allocated in the memory pool.
         *
         */
        static bool hasCapacityForFrameDataMasterChannelSpaceSegment(
            MasterChannelSpaceSegmentVariant &masterChannelVariant,
            DefsAndUtils::FrameType frameType,
            uint16_t numberOfFrames,
            uint16_t numberOfOctets);

        /**
         * @brief Allocate frame data to the memory pool.
         *
         * @return A pointer to the start of the allocated data.
         */
        static etl::expected<uint8_t *, MasterChannelAlert>
        addFrameOctetsToMemPoolMasterChannelSpaceSegment(
            MasterChannelSpaceSegmentVariant &masterChannelVariant,
            DefsAndUtils::FrameType frameType,
            uint8_t *octetsSource, uint16_t frameLength);

        /**
         * @brief Push frame object to the back of the master copy buffer.
         *
         * @returns A pointer to the frame object.
         */
        static etl::expected<etl::variant<TransferFrameTM*, TransferFrameTC*>, MasterChannelAlert>
        addFrameObjectToMasterCopyBufferMasterChannelSpaceSegment(
            MasterChannelSpaceSegmentVariant &masterChannelVariant,
            DefsAndUtils::FrameType frameType,
            etl::variant<TransferFrameTM &, TransferFrameTC &> frame);

        /**
         * @brief Remove specified frame object from the master copy buffer, as well as it's octets
         *        from the memory pool.
         */
        static etl::expected<void, MasterChannelAlert>
        removeFrameDataMasterChannelSpaceSegment(MasterChannelSpaceSegmentVariant &masterChannelVariant,
                                                 etl::variant<TransferFrameTM *, TransferFrameTC *> frame);

        /**
         *  @brief Return the current value of the master channel TM frame counter and increase it by one.
         */
        static uint8_t readAndUpdateTmFrameCountMasterChannelSpaceSegment(
            MasterChannelSpaceSegmentVariant &masterChannelVariant);

        /**
         * @}
         */

#endif // SPACE_SEGMENT

#ifdef GROUND_SEGMENT
        /** ========================================
         *  @name MapChannelGroundSegment operations
         *  ========================================
         *  @{
         */

        /**
         * @brief Takes a MapChannelGroundSegment variant and returns a pointer of it's base class type.
         *        Useful for accessing common parameters.
         */
        static BaseMAPChannel* upcastToBase(MAPChannelGroundSegmentVariant &mapChannelVariant);

        /**
         *  @brief Push a packet that will later be inserted in a type-AD or type-BD frame.
         */
        static etl::expected<void, MapChannelAlert>
        pushPacketMAPChannelGroundSegment(MAPChannelGroundSegmentVariant &mapChannelVariant,
                                          DefsAndUtils::ServiceType serviceType,
                                          uint8_t *packetSource,
                                          uint16_t packetLength);

        static etl::expected<uint16_t, MapChannelAlert>
        popPacketLengthMAPChannelGroundSegment(MAPChannelGroundSegmentVariant &mapChannelVariant,
                                               DefsAndUtils::ServiceType serviceType);
        /**
         * @brief Pop a packet segment (destined for type-AD or type-BD frames) from the MAP channel
         *        queues.
         * @param numOctets: The length of the packet segment
         */
        static etl::expected<void, MapChannelAlert>
        popPacketSegmentMAPChannelGroundSegment(MAPChannelGroundSegmentVariant &mapChannelVariant,
                                         DefsAndUtils::ServiceType serviceType,
                                         uint8_t *packetDestination,
                                         uint16_t numOctets);

        static uint16_t frameListAvailableMapChannelGroundSegment(MAPChannelGroundSegmentVariant &mapChannelVariant);

        static etl::expected<void, MapChannelAlert>
        pushFrameMapChannelGroundSegment(MAPChannelGroundSegmentVariant &mapChannelVariant,
                                         TransferFrameTC *frameTC);


        static etl::expected<void, MapChannelAlert>
        popFrameMapChannelGroundSegment(MAPChannelGroundSegmentVariant &mapChannelVariant,
                                        TransferFrameTC *frameTC);

        static etl::expected<TransferFrameTC *, MapChannelAlert>
        getFrameMapChannelGroundSegment(MAPChannelGroundSegmentVariant &,
                                        DefsAndUtils::ServiceType serviceType,
                                        DefsAndUtils::TcFrameProcessingStage processingStage);

        /**
         * @}
         */

        /** ============================================
         *  @name VirtualChannelGroundSegment operations
         *  ============================================
         *  @{
         */

        /**
         * @brief Takes a VirtualChannelGroundSegment variant and returns a pointer of it's base class type.
         *        Useful for accessing common parameters.
         */
        static BaseVirtualChannel*
        upcastToBase(VirtualChannelGroundSegmentVariant &virtualChannelVariant);

        static uint16_t frameListAvailableVirtualChannelGroundSegment(VirtualChannelGroundSegmentVariant &virtualChannelVariant);

        static etl::expected<void, VirtualChannelAlert>
        pushFrameVirtualChannelGroundSegment(VirtualChannelGroundSegmentVariant &virtualChannelVariant,
                                             TransferFrameTC *frameTC);

        static etl::expected<void, VirtualChannelAlert>
        popFrameVirtualChannelGroundSegment(VirtualChannelGroundSegmentVariant &virtualChannelVariant,
                                            TransferFrameTC *frameTC);

        static etl::expected<TransferFrameTC *, VirtualChannelAlert>
        getFrameVirtualChannelGroundSegment(VirtualChannelGroundSegmentVariant &virtualChannelVariant,
                                            DefsAndUtils::ServiceType serviceType,
                                            DefsAndUtils::TcFrameProcessingStage processingStage);

        /**
         * @}
         */

        /** ===========================================
         *  @name MasterChannelGroundSegment operations
         *  ===========================================
         *  @{
         */

        /**
         * @brief Takes a MasterChannelGroundSegment variant and returns a pointer of it's base class type.
         *        Useful for accessing common parameters.
         */
        static BaseMasterChannel* upcastToBase(MasterChannelGroundSegmentVariant &masterChannelVariant);

        /**
         * @brief Indicates if there is enough capacity for storing new frames.
         *
         * @param numberOfFrames Used to determine if there is enough space in the master copy buffer.
         * @param numberOfOctets The total amount of bytes/octets that need to be allocated in the memory pool.
         *
         */
        static bool hasCapacityForFrameDataMasterChannelGroundSegment(
            MasterChannelGroundSegmentVariant &masterChannelVariant,
            uint16_t numberOfFrames,
            uint16_t numberOfOctets);

        /**
         * @brief Allocate frame data to the memory pool.
         *
         * @return A pointer to the start of the allocated data.
         */
        static etl::expected<uint8_t *, MasterChannelAlert>
        addFrameOctetsToMemPoolMasterChannelGroundSegment(
            MasterChannelGroundSegmentVariant &masterChannelVariant,
            uint8_t *octetsSource, uint16_t frameLength);

        /**
         * @brief Push frame object to the back of the master copy buffer.
         *
         * @returns A pointer to the frame object
         */
        static etl::expected<etl::variant<TransferFrameTM*, TransferFrameTC*>, MasterChannelAlert>
        addFrameObjectToMasterCopyBufferMasterChannelGroundSegment(
            MasterChannelGroundSegmentVariant &masterChannelVariant,
            const TransferFrameTC& frame);

        /**
         * @brief Remove specified frame object from the master copy buffer, as well as it's octets
         *        from the memory pool.
         */
        static etl::expected<void, MasterChannelAlert>
        removeFrameDataMasterChannelGroundSegment(
            MasterChannelGroundSegmentVariant &masterChannelVariant,
            TransferFrameTC& frame);

        /**
         * @}
         */
#endif //GROUND_SEGMENT
    };
} // namespace CCSDSDataLinkLayer
