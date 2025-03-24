/**
 * @file CCSDSChannelConfiguration.hpp
 *
 * @brief User defined channel configuration
 */

#pragma once
#include "etl/variant.h"
#include "etl/flat_map.h"
#include "CCSDSChannel.hpp"

/**
 * Define your channel configuration here
 */
namespace CCSDSDataLinkLayer::ChannelConfig {
    inline auto tfvn = DefsAndUtils::TransferFrameVersionNumber::TM_TC_SYNCHRONOUS_TRANSFER_FRAME_V1;
    inline uint16_t scid = 0x00FF;
#ifdef SPACE_SEGMENT
    /** =========================================
     *   MasterChannelSpaceSegment configuration
     *  =========================================
     */
    using MasterChannelSpaceSegmentVariant = etl::variant<
        MasterChannelSpaceSegment<10>,
        MasterChannelSpaceSegment<20>
    >;


    /** ==========================================
     *   VirtualChannelSpaceSegment configuration
     *  ==========================================
     */
    using VirtualChannelSpaceSegmentVariant = etl::variant<
        VirtualChannelSpaceSegment<10>,
        VirtualChannelSpaceSegment<20>
    >;

    /** ======================================
     *   MAPChannelSpaceSegment configuration
     *  ======================================
     */
    using MAPChannelSpaceSegmentVariant = etl::variant<
        MAPChannelSpaceSegment<10>,
        MAPChannelSpaceSegment<20>
    >;

    inline auto chan1 = MAPChannelSpaceSegment<10>(
        DefsAndUtils::buildGmapid(tfvn, scid, 0, 0),
        true,
        true);
    inline auto chan2 = MAPChannelSpaceSegment<20>(
        DefsAndUtils::buildGmapid(tfvn, scid, 0, 1),
        true,
        true);


    /**
     * @brief A map of etl::optional objects that hold generated CLCWs from every virtual channel's FARM. The TM Link
     * (TxTM chain) pops and places them inside the OCF field of TM frames.
     */
    inline etl::flat_map<uint32_t, etl::optional<CLCW>, 2> virtualChannelClcwQueues = {
        {DefsAndUtils::buildGvcid(tfvn, scid, 0), etl::nullopt},
        {DefsAndUtils::buildGvcid(tfvn, scid, 0), etl::nullopt}
    };

#endif	// SPACE_SEGMENT

#ifdef GROUND_SEGMENT
    /** =========================================
     *  MasterChannelGroundSegment configuration
     *  =========================================
     */
    using MasterChannelGroundSegmentVariant = etl::variant<
        MasterChannelGroundSegment<10>,
        MasterChannelGroundSegment<20>
    >;


    /** ==========================================
     *  VirtualChannelGroundSegment configuration
     *  ==========================================
     */
    using VirtualChannelGroundSegmentVariant = etl::variant<
        VirtualChannelGroundSegment<10>,
        VirtualChannelGroundSegment<20>
    >;

    /** ======================================
     *  MAPChannelGroundSegment configuration
     *  ======================================
     */
    using MAPChannelGroundSegmentVariant = etl::variant<
        MAPChannelGroundSegment<10>,
        MAPChannelGroundSegment<20>
    >;
#endif // GROUND_SEGMENT
} // namespace ChannelConfig
