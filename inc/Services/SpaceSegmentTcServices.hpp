/**
 * @file  SpaceSegmentTcServices.hpp
 * @brief User interface for the Space Segment Tc Data Link
 *
 * @see TC SPACE DATA LINK CCSDS for definitions on all the services. This implementation supports the following
 *      services:
 *
 *      - MAP Packet Service (MAPP)
 *      - MAP Access Service (MAPA)
 *      - Virtual Channel Packet Service (VCP)
 *      - Virtual Channel Access Service (VCA)
 *      - COP Management Service: Get CLCW (custom service, used to get generated CLCWs from FARM)
 */

#pragma once
#include "StructureGeneration.hpp"
#include "CcsdsDefinitions.hpp"

namespace CCSDSDataLinkLayer {
#ifdef INCLUDE_SPACE_SEGMENT_CODE
    class SpaceSegmentTcServices {
    public:
        /**
         * @details Insert a received frame from the channel codeing and synchronization sublayer
         *
         * @param physicalChannelName  Master Channel Name, as defined in CCSDSDataLink.def
         * @param frameSource Frame to be inserted. Note that the length of the frame is determined by its
         *                    frame length field, not the size of the span.
         *
         * @return TODO
         */
        static etl::expected<void, ServiceChannelNotification> insertReceivedFrame(
            Objects::PhysicalChannelName physicalChannelName,
            etl::span<uint8_t> frameSource);

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
         * @param packetDestination A user supplied buffer to place the packet. Note that the length of the buffer
         *                   needs to be at least the maximum expected packet size
         *
         * @returns TODO
         */
        static etl::expected<void, ServiceChannelNotification> mapChannelPacketServiceIndication(
            Objects::MapChannelName mapChanName,
            etl::span<uint8_t> packetDestination,
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
         * @param vcaSduDestination  A user supplied buffer to place the vca sdu. Note that the length of the buffer
         *                    needs to be at least the maximum transfer frame data field size
         * @returns TODO
         */
        static etl::expected<void, ServiceChannelNotification> mapChannelAccessServiceIndication(
            Objects::MapChannelName mapChanName,
            etl::span<uint8_t> vcaSduDestination,
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
         * @param packetDestination A user supplied buffer to place the packet. Note that the length of the buffer
         *                   needs to be at least the maximum expected packet size
         *
         * @returns TODO
         */
        static etl::expected<void, ServiceChannelNotification> virtualChannelPacketServiceIndication(
            Objects::VirtualChannelTcName vcChanName,
            etl::span<uint8_t> packetDestination,
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
         * @param vcaSduDestination  A user supplied buffer to place the vca sdu. Note that the length of the buffer
         *                    needs to be at least the maximum transfer frame data field size
         * @returns TODO
         */
        static etl::expected<void, ServiceChannelNotification> virtualChannelAccessServiceIndication(
            Objects::VirtualChannelTcName vcChanName,
            etl::span<uint8_t> vcaSduDestination,
            Defs::ServiceType serviceType);

        /**
         * @brief A pipeline of data handling functions that processes frames and extracts packets. Notifications
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
        static etl::expected<void, ServiceChannelNotification> spaceSegmentTcProcessing(
            Objects::PhysicalChannelName physicalChannelName);

        /**
         * @brief Get a clcw generated from FARMs of the specifiec physical channel
         *
         * @param physicalChannelName Physical Channel Name, as defined in CCSDSDataLink.def
         *
         * @returns Raw clcw bytes
         */
        static etl::expected<uint32_t, ServiceChannelNotification> copManagementServiceGetCLCW(
            Objects::PhysicalChannelName physicalChannelName);


        using StatusFieldVector = etl::span<etl::pair<Objects::VirtualChannelTcName, uint8_t>>;
        /**
         * @brief CLCWs contain some optional, mission specified fields, unused by COP-1, but potentially useful
         *        for operation enhancement. This function can be used to update those fields in the next incoming clcw.
         *        If fields are not updated until the next incoming clcw, the older stored value will be used.
         *
         * @see For more details p. 4.2 of TC SPACE DATA LINK CCSDS
         *
         * @param statusFieldVector  A vector 2 bit flags for each FARM, their meaning is entirely mission specified.
         * @param noRfAvailable A 1 bit flag. A value of 1 indicates that the physical layer is not available for
         *                      transmission, and manual intervention is required. The definition of "availability" is
         *                      mission specified. If provided, it will be applied to all virtual channels that belong to
         *                      the corresponding master channel.
         * @param noBitLock A 1 bit flag. It shall be used as a quality indicator of the physical layer (mission specified).
         *                  If provided, it will be applied in all  to all virtual channels that belong to
         *                  the corresponding master channel.
         */
        static etl::expected<void, ServiceChannelNotification> modifyOptionalClcwFields(
            Objects::PhysicalChannelName physicalChannelName,
            etl::optional<StatusFieldVector> statusFieldVector,
            etl::optional<bool> noRfAvailable,
            etl::optional<bool> noBitLock);

    private:
        static etl::expected<void, ServiceChannelNotification> mapChannelHelperFunc(
            Objects::MapChannelName mapChanName,
            etl::span<uint8_t> packetDestination,
            Defs::ServiceType serviceType,
            Defs::DataFieldContent dataFieldContent);

        static etl::expected<void, ServiceChannelNotification> virtualChannelHelperFunc(
            Objects::VirtualChannelTcName vcChanName,
            etl::span<uint8_t> packetDestination,
            Defs::ServiceType serviceType,
            Defs::DataFieldContent dataFieldContent);
    };
#endif // INCLUDE_SPACE_SEGMENT_CODE
} // CCSDSDataLinkLayer
