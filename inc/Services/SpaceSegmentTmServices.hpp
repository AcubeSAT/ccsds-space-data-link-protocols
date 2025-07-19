/**
 * @file  SpaceSegmentTmServices.hpp
 * @brief User interface for the Space Segment Tm Data Link
 *
 * @see TM SPACE DATA LINK CCSDS for definitions on all the services. This implementation supports the following
 *      services:
 *      - Virtual Channel Packet Service (VCP)
 *      - Virtual Channel Access Service (VCA)
 *      - Operational Control Field Service (Hybrid of MC_OCF, VC_OCF for more flexibility, see below)
 */

#pragma once
#include <cstdint>
#include "etl/span.h"
#include "StructureGeneration.hpp"

namespace CCSDSDataLinkLayer {
#ifdef INCLUDE_SPACE_SEGMENT_CODE
    class SpaceSegmentTmServices {
    public:
        /**
         * @details The Virtual Channel Packet (VCP) Service transfers a sequence of variable-length,
         *          delimited, octet-aligned service data units known as Packets across a space link. The Packets
         *          transferred by this service must have a Packet Version Number (PVN) authorized by CCSDS.
         *
         * @see p. 3.3 of TM SPACE DATA LINK CCSDS
         * @see SANA Packet Version Number registry for a list of supported packets types.
         *
         * @note For a virtual channel to support the VCP service, its synchronization flag must be defined
         *        as: "SynchronizationFlag::OCTET_SYNCHRONIZED_FORWARD_ORDERED" (CCSDSDataLink.def). Multiplexing
         *        of different packet types is allowed
         *
         *
         * @param vcChanName Virtual Channel Name, as defined in CCSDSDataLink.def
         * @param packet The raw packet bytes. Note that the length of the packet is determined by the appropriate
         *                space packet length field, not the size of the span itself.
         *
         * @returns TODO
         */
        static etl::expected<void, ServiceChannelNotification> virtualChannelPacketServiceRequest(Objects::VirtualChannelTmName vcChanName,
            etl::span<uint8_t> packet);

        /**
         * @details The Virtual Channel Access (VCA) Service provides transfer of a sequence of privately
         *          formatted service data units of fixed length, along with status fields, across a space link.
         *          The length of the provided vca sdu needs to be equal to the  transfer frame data field length.
         *
         * @see p. 3.4 of TM SPACE DATA LINK CCSDS
         *
         * @note For a virtual channel to support the VCP service, its synchronization flag must be defined
         *        as: "SynchronizationFlag::VCA_SDU" (CCSDSDataLink.def).
         *
         * @param vcChanName Virtual Channel Name, as defined in CCSDSDataLink.def
         * @param vcaSdu The raw sdu bytes. Note that the length of the sdu is determined by the size of the span.
         *               The function returns an error if this size is different that the transfer frame data field length
         * @param packetOrderFlag User defined flag (optional)
         * @param segmentLengthIdentifier 2 bit user defined flags (optional). The 2 least significant bits of the uint8_t
         *                                are used
         * @returns TODO
         */
        static etl::expected<void, ServiceChannelNotification> virtualChannelAccessServiceRequest(
            Objects::VirtualChannelTmName vcChanName,
            etl::span<uint8_t> vcaSdu,
            bool packetOrderFlag = Defs::PacketOrderFlag,
            uint8_t segmentLengthIdentifier = Defs::SegmentLengthIdentifierLegacy);

        /**
         * @details The Operational Control Field (MC_OCF) Service provides synchronous
         *          transfer of fixed-length data units, each consisting of four octets, in the Operational Control
         *          Field (OCF) of Transfer Frames of a Master Channel. This service is a hybrid of the CCSDS defined
         *          services (VC_OCF, MC_OCF), in the sense that a provided ocf sdu can be carried by any frame in a
         *          particular master channel, like MC_OCF, but with the ability to select which virtual channels have
         *          an operational control field, like VC_OCF (configured in CCSDSDataLink.def). This approach is more
         *          flexible and valid for CLCWs, due to the fact that they carry a virtual channel id themselves, so
         *          they do not have to be transported using frames from the same virtual channel.
         *
         * @note It is evident that at least one virtual channel needs to have an operational control field for the
         *       service to function.
         * @note This service is already utilized by FARM to send generated CLCWs. To multiplex user defined OCF_SDUs
         *        set their "Control Word Type" bit as 1 (@see p. 4.2.1.2 of TC SPACE DATA LINK CCSDS).
         *
         * @param mcChanName Master Channel Name, as defined in CCSDSDataLink.def
         * @param ocfSdu 4 byte service data unit (user defined, needs to have "Control Word Type" == 1)
         *
         *  @returns TODO
         */
        static etl::expected<void, ServiceChannelNotification> virtualChannelOperationalControlFieldServiceRequest(
            Objects::MasterChannelTmName mcChanName,
            uint32_t ocfSdu);

        /**
         * @brief A pipeline of data handling functions that insert packets to frames and process them. Notifications
         *        are returned if serious problem is encountered, for error handling or logging purposes.
         *
         * @details Implementation in a process/thread: This function should be called inside a dedicated process/thread,
         *          inside a while loop. A delay between function calls can be used to control the frame generation
         *          rate (in the absence of sufficient Packets or VCA_SDUs, the pipeline still generates 'Only Idle
         *          Data Frames' in an attempt to hold a roughly constant frame generation rate and keep the OCF service
         *          functioning).
         *
         * @returns A ServiceChannelNotification, which indicates possible problems with the link. The meaning of each
         *         notification is described below:
         *  TODO
         *
         */
        static etl::expected<void, ServiceChannelNotification> spaceSegmentTmProcessing(
            Objects::MasterChannelTmName mcChanName);

        /**
         * @details Get a frame that is ready for delivery to the Channel Coding and Synchronization Sublayer.
         *
         * @param mcChanName Master Channel Name, as defined in CCSDSDataLink.def
         * @param packetDestination A user provided buffer to copy the frame. Ensure its length is at least
         *                          equal to that of a transfer frame.
         *
         * @return TODO
         */
        static etl::expected<void, ServiceChannelNotification> getReadyFrameForTransmission(
            Objects::MasterChannelTmName mcChanName,
            uint8_t* packetDestination);
    };
#endif // INCLUDE_SPACE_SEGMENT_CODE
} // CCSDSDataLinkLayer
