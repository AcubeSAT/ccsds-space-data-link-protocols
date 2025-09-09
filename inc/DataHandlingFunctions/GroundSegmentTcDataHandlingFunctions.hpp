/**
 * @file GroundSegmentTcDataHandlingFunctions.hpp
 * @brief Functions for creating and processing TC Transfer Frames
 */

#pragma once
#include "etl/expected.h"
#include "etl/variant.h"
#include "DataLinkNotifications.hpp"
#include "CcsdsDefinitions.hpp"
#include "ChannelObjects.hpp"
#include "AddressingAndParsingUtilities.hpp"

namespace CCSDSDataLinkLayer {
#ifdef INCLUDE_GROUND_SEGMENT_CODE
    class GroundSegmentTcDataHandling {
    public:
        /**
         * Reset the state of a MAP Channel (also deletes the frame master copies)
         */
        static void resetMapChannel(MAPChannelGs& mapChan);

        /**
         * Reset the state of a Virtual Channel as well as the corresponding FOP,
         * if it exists for that Virtual Channel (also deletes the frame master copies)
         */
        static void resetVirtualChannel(VirtualChannelGsTc& vcChan);

        /**
         * Reset the state of a Master Channel (also deletes the frame master copies)
         */
        static void resetMasterChannel(MasterChannelGsTc& mcChan);

        /**
         * Serves as the main entry point from the upper layers, by storing
         * octet synchronized and forward ordered packets along with their length so they can be later inserted to
         * transfer frames, and transmitted.
         *
         * @see p. 3.2.2 of CCSDS TC SPACE DATA LINK PROTOCOL for a definition of the 'packet' data structure
         * @see SANA Packet Version Number registry for a list of supported packets types.
         *
         * @param chanVariant Choose in which channel to insert a packet. If a virtual channel is chosen, the function
         *                     assumes that it has no map channels. It is the responsibility of the user to choose the
         *                     channels correctly.
         * @param packetSource Packet to be inserted. Note that the length of the packet is decided by the
         *         length data field, not the size of the span. This means that the user can input
         *         constant length spans, and the function will figure out the length by itself.
         * @param serviceType Store the packet either as Type-AD or Type-BD
         */
        static etl::expected<void, ServiceChannelNotification> storePacket(
            const PhysicalChannel& phyChan,
            etl::variant<etl::reference_wrapper<VirtualChannelGsTc>, etl::reference_wrapper<MAPChannelGs>> chanVariant,
            etl::span<uint8_t> packetSource,
            Defs::ServiceType serviceType);

        /**
         * @brief Insert a service data unit. Unlike 'Packets' its structure is not known to the Data Link (not
         *        'Space Packets' or 'Encapsulation Packets').
         *
         * @param chanVariant Choose in which channel to insert a packet. If a virtual channel is chosen, the function
         *                     assumes that it has no map channels. It is the responsibility of the user to choose the
         *                     channels correctly.
         * @param vcaSduSource Service data unit to be inserted. Note that since the sdu is of unknown structure, its
         *                     length is assumed to be the size of the span and can be non fixed. However, it has to
         *                     be contained within the maximum transfer frame data field length and blocking/segmentation
         *                     is not supported
         * @param serviceType Store the packet either as Type-AD or Type-BD
         *
         */
        static etl::expected<void, ServiceChannelNotification> storeVcaSdu(
            const PhysicalChannel& phyChan,
            etl::variant<etl::reference_wrapper<VirtualChannelGsTc>, etl::reference_wrapper<MAPChannelGs>> chanVariant,
            etl::span<uint8_t> vcaSduSource,
            Defs::ServiceType serviceType);

        /**
         * Requests to process the last packet/VCA_SDU stored in the buffer of the specific MAP/VC channel
         * (possible more for packets, if blocking is enabled). Packets are segmented or blocked together
         * from data of the memory pool, a transfer frame is created with some primary header fields initialized
         * (as well as the segment header if required) and then transferred to the buffer of the virtual channel.
         * VCA_SDUs always fit in a single transfer frame. This method implements the following data handling functions.
         * (@see TC Space Data Link Protocol):
         * a) MAP Packet Processing (@see p. 4.3.1) and VC Packet Processing (@see p. 4.3.4)
         * b) The Frame Initialization Procedure (@see p. 4.3.5.2) of the Virtual Channel Generation function (@see p. 4.3.5)
         *
         * @param chanVariant Choose in which channel to insert a packet. If a virtual channel is chosen, the function
         *                     assumes that it has no map channels. It is the responsibility of the user to choose the
         *                     channels correctly.
         * @param serviceType Service type of resulting frame (Type-AD or Type-BD). Only packets from the respective
         *                    service will be grouped together, should blocking be enabled.
         */
        static etl::expected<void, ServiceChannelNotification> packetProcessing(
            const PhysicalChannel& phyChan,
            MasterChannelGsTc& mcChan,
            etl::variant<etl::reference_wrapper<VirtualChannelGsTc>, etl::reference_wrapper<MAPChannelGs>> chanVariant,
            Defs::ServiceType serviceType);

        /**
         * @brief Apply security services for TC frames (if the frame is not associated with any security association,
         * then it gets pushed to the next stage)
         */
        static etl::expected<void, ServiceChannelNotification> applySDLSSecurity(
            const PhysicalChannel& phyChan,
            VirtualChannelGsTc& vcChan);

        /**
         * @brief Handles message exchange of a virtual channel with the respective FOP-1 module. If cop-1
         *        is inactive in this virtual channel, then the frame just gets passed down to the next queue.
         */
        static etl::expected<void, ServiceChannelNotification> virtualChannelGeneration(
            MasterChannelGsTc& mcChan,
            VirtualChannelGsTc& vcChan);

        /**
         * @brief The All Frames generation function is responsible for error control encoding
         *
         * @details Implementation in a process/thread: This function should be called inside a dedicated process/thread,
         *          inside a while loop. A blocking delay can be used between consecutive calls to control the processing
         *          frequency.
         */
        static etl::expected<void, ServiceChannelNotification> allFramesGeneration(
            const PhysicalChannel& phyChan,
            MasterChannelGsTc& mcChan,
            uint8_t *frameDestination);
    };
#endif // INCLUDE_GROUND_SEGMENT_CODE
} // CCSDSDataLinkLayer