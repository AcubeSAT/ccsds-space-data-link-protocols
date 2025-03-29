/**
 * @file CCSDSChannelConfiguration.hpp
 *
 * @brief User defined channel configuration
 */

#pragma once
#include "etl/variant.h"
#include "etl/flat_map.h"
#include "CCSDSChannel.hpp"
#include "CLCW.hpp"

namespace CCSDSDataLinkLayer {
#ifdef SPACE_SEGMENT
    using MasterChannelSpaceSegmentVariant = etl::variant<
        MasterChannelSpaceSegment<10>&,
        MasterChannelSpaceSegment<20>&
    >;

    using VirtualChannelSpaceSegmentVariant = etl::variant<
        VirtualChannelSpaceSegment<10>&,
        VirtualChannelSpaceSegment<20>&
    >;

    using MAPChannelSpaceSegmentVariant = etl::variant<
        MAPChannelSpaceSegment<10>&,
        MAPChannelSpaceSegment<20>&
    >;

    using VirtualChannelSearchFunctionType = etl::optional<VirtualChannelSpaceSegmentVariant> (*)(uint8_t);
    using MapChannelSearchFunctionType = etl::optional<MAPChannelSpaceSegmentVariant> (*)(uint8_t, uint8_t);
#endif // SPACE_SEGMENT

#ifdef GROUND_SEGMENT
    using MasterChannelGroundSegmentVariant = etl::variant<
        MasterChannelGroundSegment<10>&,
        MasterChannelGroundSegment<20>&
    >;

    using VirtualChannelGroundSegmentVariant = etl::variant<
        VirtualChannelGroundSegment<10>&,
        VirtualChannelGroundSegment<20>&
    >;
    using MAPChannelGroundSegmentVariant = etl::variant<
        MAPChannelGroundSegment<10>&,
        MAPChannelGroundSegment<20>&
    >;
#endif // GROUND_SEGMENT
    /**
     * Example configuration of channels for communication in a single RF band
     * (only a single physical channel instance is possible, but multiple master/virtual/MAP channel instances
     *  may exist).
     */
    class ChannelConfig {
        auto tfvn = DefsAndUtils::TransferFrameVersionNumber::TM_TC_SYNCHRONOUS_TRANSFER_FRAME_V1;
        uint16_t scid = 0x00FF;
#ifdef SPACE_SEGMENT
        /** =========================================
         *   MasterChannelSpaceSegment configuration
         *  =========================================
         */


        /** ==========================================
         *   VirtualChannelSpaceSegment configuration
         *  ==========================================
         */

        VirtualChannelSpaceSegment<10> vcChan1 = VirtualChannelSpaceSegment<10>(
            0, 1, true, 100,
            true, true, true, true,
            true, false,0,
            DefsAndUtils::SynchronizationFlag::OCTET_SYNCHRONIZED_FORWARD_ORDERED);


        /**
         *  @brief Returns the virtual channel that corresponds to the given vid, if it exists
         */
        etl::optional<VirtualChannelSpaceSegmentVariant> virtualChannelSearchFunctionImpl(const uint8_t vcid) {
            if (vcid == DefsAndUtils::extractVcidFromGvcid(vcChan1.getGvcid())) {
                return VirtualChannelSpaceSegmentVariant(vcChan1);
            }
            return etl::nullopt;
        }

        /** ======================================
         *   MAPChannelSpaceSegment configuration
         *  ======================================
         */

        MAPChannelSpaceSegment<10> mapChan1 = MAPChannelSpaceSegment<10>(
            DefsAndUtils::buildGmapid(tfvn, scid, 0, 0),
            true,
            true);
        MAPChannelSpaceSegment<20> mapChan2 = MAPChannelSpaceSegment<20>(
            DefsAndUtils::buildGmapid(tfvn, scid, 0, 1),
            true,
            true);


        etl::optional<MAPChannelSpaceSegmentVariant> mapChannelSearchFunctionImpl(const uint8_t vcid, const uint8_t mapid) {
            uint32_t gmapid = mapChan1.getGmapid();
            if (DefsAndUtils::extractVcidFromGmapid(gmapid) == vcid &&
                DefsAndUtils::extractMapidFromGmapid(gmapid) == mapid) {
                return MAPChannelSpaceSegmentVariant(mapChan1);
            }

            gmapid = mapChan2.getGmapid();
            if (DefsAndUtils::extractVcidFromGmapid(gmapid) == vcid &&
                DefsAndUtils::extractMapidFromGmapid(gmapid) == mapid) {
                return MAPChannelSpaceSegmentVariant(mapChan2);
            }
            return etl::nullopt;
        }

        /** ==========================================================================
         *   CLCW containers
         *   @brief A map of etl::optional objects that hold generated CLCWs from every
         *          virtual channel's FARM. The TM Link (TxTM chain) pops and places
         *          them inside the OCF field of TM frames.
         *  ===========================================================================
         */
        etl::flat_map<uint32_t, etl::optional<CLCW>, 2> virtualChannelClcwQueues = {
            {DefsAndUtils::buildGvcid(tfvn, scid, 0), etl::nullopt},
            {DefsAndUtils::buildGvcid(tfvn, scid, 1), etl::nullopt}
        };

        /** =================================
         *   FARM-1 configuration (optional)
         *  =================================
         */

        /** ===============================================
         *   Security Association configuration (optional)
         *  ===============================================
         */

#endif	// SPACE_SEGMENT

#ifdef GROUND_SEGMENT
        /** =========================================
         *  MasterChannelGroundSegment configuration
         *  =========================================
         */


        /** ==========================================
         *  VirtualChannelGroundSegment configuration
         *  ==========================================
         */


        /** ======================================
         *  MAPChannelGroundSegment configuration
         *  ======================================
         */

        /** ================================
         *   FOP-1 configuration (optional)
         *  ================================
         */

        /** ===============================================
         *   Security Association configuration (optional)
         *  ===============================================
         */

#endif // GROUND_SEGMENT
    };
} // namespace CCSDSDataLinkLayer
