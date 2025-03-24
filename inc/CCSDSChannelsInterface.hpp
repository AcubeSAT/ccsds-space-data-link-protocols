/**
 * @file CCSDSChannelsInterface.hpp
 * @brief Contains the ChannelsInterface class, which is used for accessing channel internals.
 */

#pragma once
#include <cstdint>
#include "etl/expected.h"
#include "CCSDSChannelConfiguration.hpp"

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
         * @brief Takes a MapChannelGroundSegment variant and returns a copy of it's base class. Useful for accessing
         *        common parameters.
         */
        static BaseMAPChannel upcastToBase(ChannelConfig::MAPChannelSpaceSegmentVariant &mapChannelVariant);

        /**
         * @brief Push frame pointer to the back of the frame processing list.
         */
        static etl::expected<void, MapChannelAlert>
        pushFrameMapChannelSpaceSegment(ChannelConfig::MAPChannelSpaceSegmentVariant &mapChannelVariant,
                                        TransferFrameTC *frameTC);


        /**
         * @brief Erase frame pointer from the frame processing list. Search starts from the front.
         */
        static etl::expected<void, MapChannelAlert>
        popFrameMapChannelSpaceSegment(ChannelConfig::MAPChannelSpaceSegmentVariant &mapChannelVariant,
                                       TransferFrameTC *frameTC);

        static etl::expected<TransferFrameTC *, MapChannelAlert>
        getFrameMapChannelSpaceSegment(ChannelConfig::MAPChannelSpaceSegmentVariant &mapChannelVariant,
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
         * @brief Takes a VirtualChannelSpaceSegment variant and returns a copy of it's base class. Useful for accessing
         *        common parameters.
         */
        static BaseVirtualChannel upcastToBase(ChannelConfig::VirtualChannelSpaceSegmentVariant &virtualChannelVariant);

        /**
         * @brief Execute FARM-1 state machine.
         *
         * @param masterChannelVariant Used by FARM for to delete type-BC frames.
         * @param virtualChannelVariant The virtual channel FARM belongs to. Used for obtaining access
         *                              to frame processing lists.
         *
         * @return A FARM notification and the state machine event code.
         * @note The user should ensure that the given virtual channel belongs to the given master channel.
         */
        static etl::pair<FARMNotification, uint8_t> applyFarmStateTable(
            ChannelConfig::MasterChannelSpaceSegmentVariant &masterChannelVariant,
            ChannelConfig::VirtualChannelSpaceSegmentVariant &virtualChannelVariant);


        enum class VchanBuffType : uint8_t {
            UNDER_PROCESSING_TM,
            AFTER_ALL_FRAMES_RECEPTION_TC,
            AFTER_VC_GENERATION_TC_TYPE_AD,
            AFTER_VC_GENERATION_TC_TYPE_BD
        };

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
        pushFrameVirtualChannelSpaceSegment(ChannelConfig::MasterChannelSpaceSegmentVariant &masterChannelVariant,
                                            ChannelConfig::VirtualChannelSpaceSegmentVariant &virtualChannelVariant,
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
        popFrameVirtualChannelSpaceSegment(ChannelConfig::VirtualChannelSpaceSegmentVariant &virtualChannelVariant,
                                           etl::variant<TransferFrameTM *, TransferFrameTC *> frame,
                                           VchanBuffType vchanBuffType);

        /**
         * @brief Get frame from specified frame processing list and processing stage.
         */
        static etl::expected<etl::variant<TransferFrameTM *, TransferFrameTC *>, VirtualChannelAlert>
        getFrameVirtualChannelSpaceSegment(ChannelConfig::VirtualChannelSpaceSegmentVariant &virtualChannelVariant,
                                           VchanBuffType vchanBuffType,
                                           etl::variant<DefsAndUtils::TcFrameProcessingStage,
                                               DefsAndUtils::TmFrameProcessingStage> processingStage);

        /**
         * @brief Read and increase by one the virtual channel frame counter for TM frames.
         */
        static uint8_t readAndUpdateTmFrameCountVirtualChannelSpaceSegment(
            ChannelConfig::VirtualChannelSpaceSegmentVariant &virtualChannelVariant);

        /**
         *  @brief Push a packet that will later be inserted in a TM frame.
         *  @param pushToFront: The data structures used for storing the packets are dequeues. If this parameter
         *                     is set to true, the packet data and lengths will be pushed to the front of the queues instead.
         */
        static etl::expected<void, VirtualChannelAlert>
        pushTmPacketVirtualChannelSpaceSegment(ChannelConfig::VirtualChannelSpaceSegmentVariant &virtualChannelVariant,
                                               const uint8_t *packetSource,
                                               uint16_t packetLength,
                                               bool pushToFront = false);

        /**
         * @brief Pop a packet (destined for TM frames) from the virtual channel
         *        queue.
        *  @param popFromBack: The data structures used for storing the packets are dequeues. If this parameter
         *                     is set to true, the packet data and lengths will be popped from the back of the queue instead.
         * @return The packet's length
         */
        static etl::expected<uint16_t, VirtualChannelAlert>
        popTmPacketVirtualChannelSpaceSegment(ChannelConfig::VirtualChannelSpaceSegmentVariant &virtualChannelVariant,
                                              uint8_t *packetDestination,
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
         * @brief Takes a MasterChannelSpaceSegment variant and returns a copy of it's base class. Useful for accessing
         *        common parameters.
         */
        static BaseMasterChannel upcastToBase(ChannelConfig::MasterChannelSpaceSegmentVariant &masterChannelVariant);

        /**
         * @brief Push TM frame pointer to the back of the processing list.
         */
        static etl::expected<void, MasterChannelAlert> pushTmFrameMasterChannelSpaceSegment(
            ChannelConfig::MasterChannelSpaceSegmentVariant &masterChannelVariant,
            TransferFrameTM *frameTM);

        /**
         * @brief Erase TC frame pointer from the processing list. Search starts from the front.
         */
        static etl::expected<void, MasterChannelAlert> popTmFrameMasterChannelSpaceSegment(
            ChannelConfig::MasterChannelSpaceSegmentVariant &masterChannelVariant,
            TransferFrameTM *frameTM);

        static etl::expected<TransferFrameTM *, MasterChannelAlert>
        getTmFrameMasterChannelSpaceSegment(ChannelConfig::MasterChannelSpaceSegmentVariant &masterChannelVariant,
                                            DefsAndUtils::TmFrameProcessingStage processingStage);

        /**
         * @brief Indicates if there is enough capacity for storing new frames.
         *
         * @param numberOfFrames Used to determine if there is enough space in the master copy buffer.
         * @param numberOfOctets The total amount of bytes/octets that need to be allocated in the memory pool.
         *
         */
        static bool hasCapacityForFrameDataMasterChannelSpaceSegment(
            ChannelConfig::MasterChannelSpaceSegmentVariant &masterChannelVariant,
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
            ChannelConfig::MasterChannelSpaceSegmentVariant &masterChannelVariant,
            DefsAndUtils::FrameType frameType,
            uint8_t *octetsSource, uint16_t frameLength);

        /**
         * @brief Push frame object to the back of the master copy buffer.
         */
        static etl::expected<void, MasterChannelAlert>
        addFrameObjectToMasterCopyBufferMasterChannelSpaceSegment(
            ChannelConfig::MasterChannelSpaceSegmentVariant &masterChannelVariant,
            DefsAndUtils::FrameType frameType,
            etl::variant<TransferFrameTM &, TransferFrameTC &> frame);

        /**
         * @brief Remove specified frame object from the master copy buffer, as well as it's octets
         *        from the memory pool.
         */
        static etl::expected<void, MasterChannelAlert>
        removeFrameDataMasterChannelSpaceSegment(ChannelConfig::MasterChannelSpaceSegmentVariant &masterChannelVariant,
                                                 etl::variant<TransferFrameTM *, TransferFrameTC *> frame);

        /**
         *  @brief Return the current value of the master channel TM frame counter and increase it by one.
         */
        static uint8_t readAndUpdateTmFrameCountMasterChannelSpaceSegment(
            ChannelConfig::MasterChannelSpaceSegmentVariant &masterChannelVariant);

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
         * @brief Takes a MapChannelGroundSegment variant and returns a copy of it's base class. Useful for accessing
         *        common parameters.
         */
        static BaseMAPChannel upcastToBase(ChannelConfig::MAPChannelGroundSegmentVariant &mapChannelVariant);

        /**
         *  @brief Push a packet that will later be inserted in a type-AD or type-BD frame.
         */
        static etl::expected<void, MapChannelAlert>
        pushPacketMAPChannelGroundSegment(ChannelConfig::MAPChannelGroundSegmentVariant &mapChannelVariant,
                                          DefsAndUtils::ServiceType serviceType,
                                          uint8_t *packetSource,
                                          uint16_t packetLength);

        /**
         * @brief Pop a packet (destined for type-AD or type-BD frames) from the MAP channel
         *        queues.
         * @return The packet's length
         */
        static etl::expected<uint16_t, MapChannelAlert>
        popPacketMAPChannelGroundSegment(ChannelConfig::MAPChannelGroundSegmentVariant &mapChannelVariant,
                                         DefsAndUtils::ServiceType serviceType,
                                         uint8_t *packetDestination);

        static etl::expected<void, MapChannelAlert>
        pushFrameMapChannelGroundSegment(ChannelConfig::MAPChannelGroundSegmentVariant &mapChannelVariant,
                                         TransferFrameTC *frameTC);


        static etl::expected<void, MapChannelAlert>
        popFrameMapChannelGroundSegment(ChannelConfig::MAPChannelGroundSegmentVariant &mapChannelVariant,
                                        TransferFrameTC *frameTC);

        static etl::expected<TransferFrameTC *, MapChannelAlert>
        getFrameMapChannelGroundSegment(ChannelConfig::MAPChannelGroundSegmentVariant &,
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
         * @brief Takes a VirtualChannelGroundSegment variant and returns a copy of it's base class. Useful for accessing
         *        common parameters.
         */
        static BaseVirtualChannel
        upcastToBase(ChannelConfig::VirtualChannelGroundSegmentVariant &virtualChannelVariant);

        /**
         * @brief Execute FOP-1 state machine.
         *
         * @param masterChannelVariant Used by FOP for to create type-BC frames.
         * @param virtualChannelVariant The virtual channel FOP belongs to.
         *
         * @return A FOP notification and the state machine event code.
         * @note The user should ensure that the given virtual channel belongs to the given master channel.
         */
        static std::pair<FOPNotification, uint8_t> applyFopStateTable(
            ChannelConfig::MasterChannelGroundSegmentVariant &masterChannelVariant,
            ChannelConfig::VirtualChannelGroundSegmentVariant &virtualChannelVariant);

        /**
         * @brief Push a signal to a virtual channel's FOP state machine.
         * @param signal There are 4 types of signals FOP can accept:\n
         *      DirectiveRequestSignal: Order FOP to change a parameter or synchronize with FARM\n
         *      FduTransferSignal: Send a new frame for FOP to process.\n
         *      LowerLayerResponseSignal: Respond to FOP's request of moving a frame to the lower layer\n
         *      CLCW: A report of FARM's status.
         *
         * @note Directive requests and CLCWs are provided by the data link user, so
         *       wrapper functions for this purpose are provided in the service channel.
         */
        static etl::expected<void, VirtualChannelAlert>
        pushSignalToFop(ChannelConfig::VirtualChannelGroundSegmentVariant &virtualChannelVariant,
                        etl::variant<DefsAndUtils::DirectiveRequestSignal,
                            DefsAndUtils::FduTransferSignal,
                            DefsAndUtils::LowerLayerResponseSignal,
                            CLCW> signal);

        enum class FopOutputQueueType : uint8_t {
            DIRECTIVE_NOTIFICATION_QUEUE,
            TRANSFER_NOTIFICATION_QUEUE,
            ASYNCHRONOUS_NOTIFICATION_QUEUE,
            FOP_TO_LOWER_LAYER_REQUEST_QUEUE
        };

        /**
         * @brief Get a signal from a virtual channel's FOP. There are 3 types of signals:\name
         *        DirectiveNotificationSignal: Informs if the directive is accepted and successfully executed by FOP.\n
         *        TransferNotificationSignal: Informs if the transfer frame is accepted and sent by FOP.\n
         *        AsynchronousNotificationSignal: Informs of an unrecoverable problem or if FOP is suspended.\n
         * @param queueType Which queue to pop a signal from.
         * @return The requested signal type.
         */
        static etl::expected<etl::variant<DefsAndUtils::DirectiveNotificationSignal,
            DefsAndUtils::TransferNotificationSignal,
            DefsAndUtils::AsynchronousNotificationSignal,
            DefsAndUtils::FopToLowerLayerRequestSignal>, VirtualChannelAlert>
        popSignalFromFop(ChannelConfig::VirtualChannelGroundSegmentVariant &virtualChannelVariant,
                         FopOutputQueueType queueType);

        static etl::expected<void, VirtualChannelAlert>
        pushFrameVirtualChannelGroundSegment(ChannelConfig::VirtualChannelGroundSegmentVariant &virtualChannelVariant,
                                             TransferFrameTC *frameTC);

        static etl::expected<void, VirtualChannelAlert>
        popFrameVirtualChannelGroundSegment(ChannelConfig::VirtualChannelGroundSegmentVariant &virtualChannelVariant,
                                            TransferFrameTC *frameTC);

        static etl::expected<TransferFrameTC *, VirtualChannelAlert>
        getFrameVirtualChannelGroundSegment(ChannelConfig::VirtualChannelGroundSegmentVariant &virtualChannelVariant,
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
         * @brief Takes a MasterChannelGroundSegment variant and returns a copy of it's base class. Useful for accessing
         *        common parameters.
         */
        static BaseMasterChannel upcastToBase(ChannelConfig::MasterChannelGroundSegmentVariant &masterChannelVariant);

        /**
         * @brief Indicates if there is enough capacity for storing new frames.
         *
         * @param numberOfFrames Used to determine if there is enough space in the master copy buffer.
         * @param numberOfOctets The total amount of bytes/octets that need to be allocated in the memory pool.
         *
         */
        static bool hasCapacityForFrameDataMasterChannelGroundSegment(
            ChannelConfig::MasterChannelGroundSegmentVariant &masterChannelVariant,
            uint16_t numberOfFrames,
            uint16_t numberOfOctets);

        /**
         * @brief Allocate frame data to the memory pool.
         *
         * @return A pointer to the start of the allocated data.
         */
        static etl::expected<uint8_t *, MasterChannelAlert>
        addFrameOctetsToMemPoolMasterChannelGroundSegment(
            ChannelConfig::MasterChannelGroundSegmentVariant &masterChannelVariant,
            uint8_t *octetsSource, uint16_t frameLength);

        /**
         * @brief Remove specified frame object from the master copy buffer, as well as it's octets
         *        from the memory pool.
         */
        static etl::expected<void, MasterChannelAlert>
        removeFrameDataMasterChannelGroundSegment(
            ChannelConfig::MasterChannelGroundSegmentVariant &masterChannelVariant,
            TransferFrameTC& frame);

        /**
         * @}
         */
#endif //GROUND_SEGMENT
    };
} // namespace CCSDSDataLinkLayer
