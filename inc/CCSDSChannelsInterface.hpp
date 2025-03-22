#pragma once
#include <cstdint>
#include "etl/expected.h"
#include "CCSDSChannelConfiguration.hpp"

/**
 * A class offering a channel size agnostic interface for internal container and parameter accessing.
 * The users of this interface are the service channel, FARM and FOP.
 */
namespace CCSDSDataLinkLayer {
    class FrameAcceptanceReporting;
    class FrameOperationProcedure;

    class ChannelsInterface {
        friend class ServiceChannelSpaceSegment;
        friend class ServiceChannelGroundSegment;
        friend class FrameAcceptanceReporting;
        friend class FrameOperationProcedure;
#ifdef ENABLE_CHANNEL_ACCESS
    public:  // give the user access when debugging/testing
#else
    private:
#endif // ENABLE_CHANNEL_ACCESS
#ifdef SPACE_SEGMENT
        /** ===================================
         *   MapChannelSpaceSegment operations
         *  ===================================
         */

        /**
         * @brief Takes a MapChannelGroundSegment variant and returns a copy of it's base class. Useful for accessing
         *        common parameters.
         */
        static BaseMAPChannel upcastToBase(ChannelConfig::MAPChannelSpaceSegmentVariant &mapChannelVariant);

        static etl::expected<void, MapChannelAlert>
        pushFrameMapChannelSpaceSegment(ChannelConfig::MAPChannelSpaceSegmentVariant &mapChannelVariant,
                                        TransferFrameTC *frameTC);


        static etl::expected<void, MapChannelAlert>
        popFrameMapChannelSpaceSegment(ChannelConfig::MAPChannelSpaceSegmentVariant &mapChannelVariant,
                                       TransferFrameTC *frameTC);

        static etl::expected<TransferFrameTC *, MapChannelAlert>
        getFrameMapChannelSpaceSegment(ChannelConfig::MAPChannelSpaceSegmentVariant &mapChannelVariant,
                                       DefsAndUtils::ServiceType serviceType,
                                       DefsAndUtils::TcFrameProcessingStage processingStage);

        /** =======================================
         *   VirtualChannelSpaceSegment operations
         *  =======================================
         */

        /**
         * @brief Takes a VirtualChannelSpaceSegment variant and returns a copy of it's base class. Useful for accessing
         *        common parameters.
         */
        static BaseVirtualChannel upcastToBase(ChannelConfig::VirtualChannelSpaceSegmentVariant &virtualChannelVariant);

        static FARMNotification applyFarmStateTable(
            ChannelConfig::MasterChannelSpaceSegmentVariant &masterChannelVariant,
            ChannelConfig::VirtualChannelSpaceSegmentVariant &virtualChannelVariant);


        enum class VchanBuffType :  uint8_t {
            UNDER_PROCESSING_TM,
            AFTER_ALL_FRAMES_RECEPTION_TC,
            AFTER_VC_GENERATION_TC_TYPE_AD,
            AFTER_VC_GENERATION_TC_TYPE_BD
        };

        // TODO In case a push occurs in the circular buff, do not forget to delete the master copy of the
        //      the frame that will be pushed out
        static etl::expected<void, VirtualChannelAlert>
        pushFrameVirtualChannelSpaceSegment(ChannelConfig::MasterChannelSpaceSegmentVariant &masterChannelVariant,
                                            ChannelConfig::VirtualChannelSpaceSegmentVariant &virtualChannelVariant,
                                            etl::variant<TransferFrameTM *, TransferFrameTC *> frame,
                                            VchanBuffType vchanBuffType
        );

        static etl::expected<void, VirtualChannelAlert>
        popFrameVirtualChannelSpaceSegment(ChannelConfig::VirtualChannelSpaceSegmentVariant &virtualChannelVariant,
                                           etl::variant<TransferFrameTM *, TransferFrameTC *> frame,
                                           VchanBuffType vchanBuffType);

        static etl::expected<etl::variant<TransferFrameTM *, TransferFrameTC *>, VirtualChannelAlert>
        getFrameVirtualChannelSpaceSegment(ChannelConfig::VirtualChannelSpaceSegmentVariant &virtualChannelVariant,
                                           VchanBuffType vchanBuffType,
                                           etl::variant<DefsAndUtils::TcFrameProcessingStage,
                                               DefsAndUtils::TmFrameProcessingStage> proccessingStage);

        /**
         * @brief Read and increase by one the virtual channel frame counter for TM frames.
         */
        static uint8_t readAndUpdateTmFrameCountVirtualChannelSpaceSegment(
            ChannelConfig::MasterChannelSpaceSegmentVariant &masterChannelVariant);

        static etl::expected<void, VirtualChannelAlert>
        pushTmPacketVirtualChannelSpaceSegment(ChannelConfig::VirtualChannelSpaceSegmentVariant &virtualChannelVariant,
                                          uint8_t *packetSource,
                                          uint16_t packetLength);

        static etl::expected<uint16_t, VirtualChannelAlert>
        popTmPacketVirtualChannelSpaceSegment(ChannelConfig::VirtualChannelSpaceSegmentVariant &virtualChannelVariant,
                                         uint8_t *packetDestination);


        /** ======================================
         *   MasterChannelSpaceSegment operations
         *  ======================================
         */

        /**
         * @brief Takes a MasterChannelSpaceSegment variant and returns a copy of it's base class. Useful for accessing
         *        common parameters.
         */
        static BaseMasterChannel upcastToBase(ChannelConfig::MasterChannelSpaceSegmentVariant &masterChannelVariant);

        static etl::expected<void, MasterChannelAlert> pushTmFrameMasterChannelSpaceSegment(
            ChannelConfig::MasterChannelSpaceSegmentVariant &masterChannelVariant,
            TransferFrameTM *frameTM);

        static etl::expected<void, MasterChannelAlert> popTmFrameMasterChannelSpaceSegment(
            ChannelConfig::MasterChannelSpaceSegmentVariant &masterChannelVariant,
            TransferFrameTM *frameTM);

        static etl::expected<TransferFrameTM *, MasterChannelAlert>
        getTmFrameMasterChannelSpaceSegment(ChannelConfig::MasterChannelSpaceSegmentVariant &masterChannelVariant,
                                            DefsAndUtils::TmFrameProcessingStage processingStage);

        static bool hasCapacityForFrameDataMasterChannelSpaceSegment(ChannelConfig::MasterChannelSpaceSegmentVariant &masterChannelVariant,
            DefsAndUtils::FrameType frameType,
                                      uint16_t numberOfFrames,
                                      uint16_t numberOfOctets);

        static etl::expected<uint8_t*, MasterChannelAlert>
        addFrameOctetsToMemPoolMasterChannelSpaceSegment(ChannelConfig::MasterChannelSpaceSegmentVariant &masterChannelVariant,
                           uint8_t* octetsSource, uint16_t frameLength);

        static etl::expected<void, MasterChannelAlert>
        addFrameObjectToMasterCopyBufferMasterChannelSpaceSegment(ChannelConfig::MasterChannelSpaceSegmentVariant &masterChannelVariant,
            etl::variant<TransferFrameTM&, TransferFrameTC&> frame);

        static etl::expected<void, MasterChannelAlert>
        removeFrameDataMasterChannelSpaceSegment(ChannelConfig::MasterChannelSpaceSegmentVariant &masterChannelVariant,
                              etl::variant<TransferFrameTM *, TransferFrameTC *> frame);

        /**
         *  @brief Read and increase by one the master channel frame counter for TM frames.
         */
        static uint8_t readAndUpdateTmFrameCountMasterChannelSpaceSegment(
            ChannelConfig::MasterChannelSpaceSegmentVariant &masterChannelVariant);

#endif // SPACE_SEGMENT

#ifdef GROUND_SEGMENT
        /** ====================================
         *   MapChannelGroundSegment operations
         *  ====================================
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
         * @brief Pop a complete packet (destined for type-AD or type-BD frames) from the MAP channel
         *        queues
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

        /** ========================================
         *   VirtualChannelGroundSegment operations
         *  ========================================
         */

        /**
         * @brief Takes a VirtualChannelGroundSegment variant and returns a copy of it's base class. Useful for accessing
         *        common parameters.
         */
        static BaseVirtualChannel upcastToBase(ChannelConfig::VirtualChannelGroundSegmentVariant &virtualChannelVariant);

        static std::pair<FOPNotification, uint8_t> applyFopStateTable(
            ChannelConfig::MasterChannelGroundSegmentVariant &masterChannelVariant,
            ChannelConfig::VirtualChannelGroundSegmentVariant &virtualChannelVariant);

        static etl::expected<void, FOPNotification>
        pushSignalToFop(ChannelConfig::VirtualChannelGroundSegmentVariant &virtualChannelVariant,
                        etl::variant<DefsAndUtils::DirectiveRequestSignal,
                            DefsAndUtils::FduTransferSignal,
                            DefsAndUtils::LowerLayerResponseSignal,
                            CLCW>);

        enum class FopOutputQueueType : uint8_t {
            DIRECTIVE_NOTIFICATION_QUEUE,
            TRANSFER_NOTIFICATION_QUEUE,
            ASYNCHRONOUS_NOTIFICATION_QUEUE,
            FOP_TO_LOWER_LAYER_REQUEST_QUEUE
        };

        static etl::expected<etl::variant<DefsAndUtils::DirectiveNotificationSignal,
            DefsAndUtils::TransferNotificationSignal,
            DefsAndUtils::AsynchronousNotificationSignal,
            DefsAndUtils::FopToLowerLayerRequestSignal>, FOPNotification>
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

        /** =======================================
         *   MasterChannelGroundSegment operations
         *  =======================================
         */

        /**
         * @brief Takes a MasterChannelGroundSegment variant and returns a copy of it's base class. Useful for accessing
         *        common parameters.
         */
        static BaseMasterChannel upcastToBase(ChannelConfig::MasterChannelGroundSegmentVariant &masterChannelVariant);

        static bool hasCapacityForFrameDataMasterChannelGroundSegment(ChannelConfig::MasterChannelGroundSegmentVariant &masterChannelVariant,
                              DefsAndUtils::FrameType frameType,
                              uint16_t numberOfFrames,
                              uint16_t numberOfOctets);

        static etl::expected<uint8_t*, MasterChannelAlert>
        addFrameOctetsToMemPoolMasterChannelGroundSegment(ChannelConfig::MasterChannelGroundSegmentVariant &masterChannelVariant,
                           uint8_t* octetsSource, uint16_t frameLength);

        static etl::expected<void, MasterChannelAlert>
        addFrameObjectToMasterCopyBufferMasterChannelGroundSegment(ChannelConfig::MasterChannelGroundSegmentVariant &masterChannelVariant,
            etl::variant<TransferFrameTM&, TransferFrameTC&> frame);

        static etl::expected<void, MasterChannelAlert>
        removeFrameDataMasterChannelGroundSegment(ChannelConfig::MasterChannelGroundSegmentVariant &masterChannelVariant,
                              etl::variant<TransferFrameTM *, TransferFrameTC *> frame);


#endif //GROUND_SEGMENT
    };
} // namespace CCSDSDataLinkLayer
