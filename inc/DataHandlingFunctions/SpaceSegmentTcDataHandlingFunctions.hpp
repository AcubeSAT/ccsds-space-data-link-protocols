/**
 * @file SpaceSegmentTcDataHandlingFunctions.hpp
 * @brief Functions for receiving and processing TC Transfer Frames
 */

#pragma once
#include "etl/expected.h"
#include "Alert.hpp"
#include "CcsdsDefinitions.hpp"
#include "StructureGeneration.hpp"

#ifdef INCLUDE_SPACE_SEGMENT_CODE
namespace CCSDSDataLinkLayer {
    class SpaceSegmentTcServices;

    class SpaceSegmentTcDataHandling {
        friend class SpaceSegmentTcServices;

        /**
         * The  All  Frames  Generation  Function  shall  be  used  to  perform  error  control
         * encoding defined by this Recommendation, along with other standard checks. Serves as an entry point
         * for frames.
         *
         * @see p. 4.4.9 from TC SPACE DATA LINK PROTOCOL CCSDS
         *
         * @param frameSource Frame to be inserted. Note that the length of the frame is determined by its
         *        frame length field, not the size of the span. This means that the user can input
         *        constant length spans, and the function will figure out the length by itself.
         *
         */
        static etl::expected<void, ServiceChannelNotification> allFramesReception(
            const PhysicalChannel& phyChan,
            etl::span<uint8_t> frameSource);

        /**
         * The Virtual Channel Reception Function shall perform the Frame Acceptance and
         * Reporting Mechanism (FARM), which is a sub-procedure of the Communications Operation
         * Procedure (COP).
         *
         * @see  p. 4.4.5 from TC Space Data Link Protocol
         *
         * @returns A service channel notification. If cop is in effect for the specific virtual channel,
         *          it also returns the detected event in the farm state machine. If cop is not in effect or
         *          a farm error occurred, this value is 0.
         */
        static etl::pair<ServiceChannelNotification, uint8_t> virtualChannelReception(
            VirtualChannelSsTc& vcChan);

        /**
         * @brief Processes TC frames that belong in a security association and discards them if they do not pass checks.
         */
        static etl::expected<void, ServiceChannelNotification> processSdlsSecurity(
            PhysicalChannel& phyChan,
            VirtualChannelSsTc& vcChan,
            Defs::ServiceType serviceType);

        /**
         * The VC Packet Extraction Function shall be used to extract variable-length
         * Packets from Frame Data Units on a Virtual Channel
         * @see 4.4.1 from TC Data Link Protocol
         */
        static etl::expected<void, ServiceChannelNotification>
        packetExtraction(
            PhysicalChannel& phyChan,
            etl::variant<etl::reference_wrapper<MAPChannelSs>, etl::reference_wrapper<VirtualChannelSsTc>> channel,
            Defs::ServiceType serviceType,
            uint8_t* packetDestination);

        // Helper functions for packet extraction
        /**
         * @brief Erase frame pointer and master copy
         */
        static void eraseFrame(
            MasterChannelSsTc* mcChan,
            Queue<TransferFrameTC*>* framesAfterSdlsProcessing,
            etl::optional<Defs::SegmentedPacketConstructorTc*> segmentedPacketConstructorTc);

        /**
         * @brief Validate a packet's pvn and returns its length
         */
        static etl::expected<uint16_t, ServiceChannelNotification> validateNextPacket(
            TransferFrameTC *frameTcPtr,
            uint16_t preDataFieldLength);

        /**
         * @brief Evaluates if new and previous sequence flags combination is valid
         */
        static bool validateSequenceFlag(
            Defs::SegmentedPacketConstructorTc *segmentedPacketConstructor,
            TransferFrameTC *frameTcPtr);

        enum class PacketExtractionType : uint8_t {
            INVALID_SCENARIO,
            REJECTION_MODE,
            SEGMENTED_PACKET,
            SINGLE_OR_BLOCKED_PACKETS,
        };

        /**
         * @brief Studies the given fields to determine how packet extraction should be performed
         */
        static PacketExtractionType detectExtractionScenario(
            bool segmentationHeaderPresent,
            Defs::SegmentedPacketConstructorTc* segmentedPacketConstructor,
            TransferFrameTC *frameTcPtr,
            bool blockingAllowed,
            bool segmentationAllowed);
    };
} // CCSDSDataLinkLayer
#endif // INCLUDE_SPACE_SEGMENT_CODE
