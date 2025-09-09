/**
 * @file  GroundSegmentTcServices.hpp
 * @brief User interface for the Ground Segment Tc Data Link
 *
 * @see TC SPACE DATA LINK CCSDS for definitions on all the services. This implementation supports the following
 *      services:
 *
 *      - MAP Packet Service (MAPP)
 *      - MAP Access Service (MAPA)
 *      - Virtual Channel Packet Service (VCP)
 *      - Virtual Channel Access Service (VCA)
 *      - COP Management Service: Directive Request
 *      - COP Management Service: Directive Notify
 *      - COP Management Service: Async Notify
 *      - COP Management Service: Push CLCW (custom service, used to send CLCWs to FOP)
 */

#pragma once
#include "ChannelObjects.hpp"
#include "CcsdsDefinitions.hpp"

namespace CCSDSDataLinkLayer {
#ifdef INCLUDE_GROUND_SEGMENT_CODE
    class GroundSegmentTcServices {
    public:
        /**
         * @details The MAP Channel Packet (MAPP) Service transfers a sequence of variable-length,
         *          delimited, octet-aligned service data units known as Packets across a space link. The Packets
         *          transferred by this service must have a Packet Version Number (PVN) authorized by CCSDS.
         *
         * @see p. 3.3 of TC SPACE DATA LINK CCSDS
         * @see SANA Packet Version Number registry for a list of supported packets types.
         *
         * @note For a MAP channel to support the VCP service, its data field content flag must be defined
         *        as: "DataFieldContent::PACKET" (CCSDSDataLink.def). Multiplexing
         *        of different packet types is allowed
         *
         *
         * @param mapChanName MAP Channel Name, as defined in CCSDSDataLink.def
         * @param packet The raw packet bytes. Note that the length of the packet is determined by the appropriate
         *                space packet length field, not the size of the span itself.
         * @param serviceType Sent either a Type-AD packet (is subject to COP-1) or a Type-BD (expedited)
         *
         * @returns TODO
         */
        static etl::expected<void, ServiceChannelNotification> mapChannelPacketServiceRequest(
            Objects::MapChannelName mapChanName,
            etl::span<uint8_t> packet,
            Defs::ServiceType serviceType);

        /**
         * @details The MAP Channel Access (MAPA) Service provides transfer of a sequence of privately
         *          formatted service data units of fixed length, along with status fields, across a space link.
         *          The length of the provided vca sdu needs to be equal to the  transfer frame data field length.
         *
         * @see p. 3.5 of TC SPACE DATA LINK CCSDS
         *
         * @note For a MAP channel to support the VCP service, its data field content flag must be defined
         *        as: "DataFieldContent::VCA_SDU" (CCSDSDataLink.def).
         *
         * @param mapChanName MAP Channel Name, as defined in CCSDSDataLink.def
         * @param vcaSdu The raw sdu bytes. Note that the length of the sdu is determined by the size of the span.
         *               The function returns an error if this size is larger than that of maximum transfer frame data
         *               field length
         * @param serviceType Sent either a Type-AD vca sdu (is subject to COP-1) or a Type-BD (expedited)
         *
         * @returns TODO
         */
        static etl::expected<void, ServiceChannelNotification> mapChannelAccessServiceRequest(
            Objects::MapChannelName mapChanName,
            etl::span<uint8_t> vcaSdu,
            Defs::ServiceType serviceType);

        /**
         * @details The Virtual Channel Packet (VCP) Service transfers a sequence of variable-length,
         *          delimited, octet-aligned service data units known as Packets across a space link. The Packets
         *          transferred by this service must have a Packet Version Number (PVN) authorized by CCSDS.
         *
         * @see p. 3.4 of TC SPACE DATA LINK CCSDS
         * @see SANA Packet Version Number registry for a list of supported packets types.
         *
         * @note For a virtual channel to support the VCP service, its data field content flag must be defined
         *        as: "DataFieldContent::PACKET" (CCSDSDataLink.def). Multiplexing
         *        of different packet types is allowed
         *
         *
         * @param vcChanName Virtual Channel Name, as defined in CCSDSDataLink.def
         * @param packet The raw packet bytes. Note that the length of the packet is determined by the appropriate
         *                space packet length field, not the size of the span itself.
         * @param serviceType Sent either a Type-AD packet (is subject to COP-1) or a Type-BD (expedited)
         *
         * @returns TODO
         */
        static etl::expected<void, ServiceChannelNotification> virtualChannelPacketServiceRequest(
            Objects::VirtualChannelTmName vcChanName,
            etl::span<uint8_t> packet,
            Defs::ServiceType serviceType);

        /**
         * @details The Virtual Channel Access (VCA) Service provides transfer of a sequence of privately
         *          formatted service data units of fixed length, along with status fields, across a space link.
         *          The length of the provided vca sdu needs to be equal to the  transfer frame data field length.
         *
         * @see p. 3.6 of TC SPACE DATA LINK CCSDS
         *
         * @note For a virtual channel to support the VCP service, its data field content flag must be defined
         *        as: "DataFieldContent::VCA_SDU" (CCSDSDataLink.def).
         *
         * @param vcChanName Virtual Channel Name, as defined in CCSDSDataLink.def
         * @param vcaSdu The raw sdu bytes. Note that the length of the sdu is determined by the size of the span.
         *               The function returns an error if this size is larger than that of maximum transfer frame data
         *               field length
         * @param serviceType Sent either a Type-AD vca sdu (is subject to COP-1) or a Type-BD (expedited)
         *
         * @returns TODO
         */
        static etl::expected<void, ServiceChannelNotification> virtualChannelAccessServiceRequest(
            Objects::VirtualChannelTmName vcChanName,
            etl::span<uint8_t> vcaSdu,
            Defs::ServiceType serviceType);

        /**
         * @brief A pipeline of data handling functions that insert packets to frames and process them. Notifications
         *        are returned if serious problem is encountered, for error handling or logging purposes.
         *
         * @details Implementation in a process/thread: This function should be called inside a dedicated process/thread,
         *          in a while loop. A blocking delay can be used between consecutive calls to control the processing
         *          frequency
         *
         * @returns A ServiceChannelNotification, which indicates possible problems with the link. The meaning of each
         *         notification is described below:
         *  TODO
         *
         */
        static etl::expected<void, ServiceChannelNotification> groundSegmentTcProcessing(
            Objects::PhysicalChannelName physicalChannelName);

        /**
         * @details Get a frame that is ready for delivery to the Channel Coding and Synchronization Sublayer.
         *
         * @param physicalChannelName Physical Channel Name, as defined in CCSDSDataLink.def
         * @param packetDestination A user provided buffer to copy the frame. Ensure its length is at least
         *                          equal to that of the maximum transfer frame length
         *
         * @return TODO
         */
        static etl::expected<void, ServiceChannelNotification> getReadyFrameForTransmission(
            Objects::PhysicalChannelName physicalChannelName,
            uint8_t* packetDestination);

        /**
         * @brief Pass a request to FOP of a specific virtual channel
         *
         * @param vcChanName Virtual Channel Name, as defined in CCSDSDataLink.def
         * @param directive Structure with a directive id, the directive and possibly a qualifier. The directive id
         *                  is used to later identify the respective directive notification. For a list of directives
         *                  and directive qualifiers, @see table 4-1 of COP-1 CCSDS
         *
         */
        static etl::expected<void, ServiceChannelNotification> copManagementServiceDirectiveRequest(
            Objects::VirtualChannelTcName vcChanName,
            DirectiveRequestSignal directive
            );

        /**
         * @brief Returns directive notifications from FOP of a specific virtual channel.
         *
         * @param vcChanName Virtual Channel Name, as defined in CCSDSDataLink.def
         *
         * @returns A structure with the request identifier and the directive notification. For a list of directive
         *         notifications, see p. 4.2 of COP-1 CCSDS
         */
        static etl::expected<DirectiveNotificationSignalUser, ServiceChannelNotification> copManagementServiceDirectiveNotify(
            Objects::VirtualChannelTcName vcChanName
            );

        /**
         * @brief Returns asynchronous notifications from FOP of a specific virtual channel.
         *
         * @returns A structure with the notification type and possibly an alert event. For a list of asynchronous
         *          notifications, @see p. 4.3 of COP-1 CCSDS
         */
        static etl::expected<AsynchronousNotificationSignal, ServiceChannelNotification> copManagementServiceAsyncNotification(
            Objects::VirtualChannelTcName vcChanName
            );

        /**
         * @brief Send a clcws to all FOPs of a specific physical channel
         *
         * @param physicalChannelName Physical Channel Name, as defined in CCSDSDataLink.def
         */
        static etl::expected<void, ServiceChannelNotification> copManagementServicePushCLCW(
            Objects::PhysicalChannelName physicalChannelName,
            uint32_t clcw
            );


        /**
         * {Key to identify FOP, State machine notification, State after state machine execution, Occurred event}
         */
        using FopOutputData = std::tuple<Defs::VcidScidKey, FOPNotification, Defs::FOPState, uint8_t>;

        /**
         * @brief Calls the state machines of all FOPs of a specific physical channel
         *
         * @details Implementation in a process/thread: This function should be called inside a dedicated process/thread,
         *          in a while loop. A blocking delay can be used between consecutive calls to control the processing
         *          frequency
         *
         * @param physicalChannelName Physical Channel Name, as defined in CCSDSDataLink.def
         * @param fopDataVector If supplied, then the function will return information about the fop state machines.
         *                      Ensure size of span is large enough to accommodate all FOPs.
         *
         */
        static etl::expected<void, ServiceChannelNotification> executeFopStateMachines(
            Objects::PhysicalChannelName physicalChannelName,
            etl::optional<etl::span<FopOutputData>> fopDataVector);

        /**
         * Reset all channels under a specific physical channel to their initial state
         */
        static etl::expected<void, ServiceChannelNotification> resetChain(Objects::PhysicalChannelName physicalChannelName);
    };
#endif // INCLUDE_GROUND_SEGMENT_CODE
} // CCSDSDataLinkLayer

