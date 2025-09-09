/**
 * @file ChannelObjects.hpp
 * @brief This header cleanly exposes all channel "name to id" enumerations and the "id to channel instance" maps,
 *        from ChannelGeneration.hpp under a new namespace, as well as a function to initialize the channel containers.
 *
 * @note The only purpose of this header is to offer a cleaner interface. It is recommended to access enums and maps
 *       through the "Objects" namespace, instead of directly including from ChannelGeneration.hpp
 */

#pragma once
#include "ChannelGeneration.hpp"

namespace CCSDSDataLinkLayer::Objects {
    /**
     * Use those enumerations to access any channel by their name
     */
    using GeneratedChannels::PhysicalChannelName;
    using GeneratedChannels::MasterChannelTmName;
    using GeneratedChannels::MasterChannelTcName;
    using GeneratedChannels::VirtualChannelTmName;
    using GeneratedChannels::VirtualChannelTcName;
    using GeneratedChannels::MapChannelName;

    /**
     * By casting the value of the above enums to:
     * - Pcid (for physical channels)
     * - VcidScidKey (for virtual channels, farm, fop and security associations)
     * - MapidVcidScidKey (for map channels)
     * and using it as a key for the maps below, every channel, farm, fop and security association
     * instance can be accessed.
     */
    inline auto& physicalChannelMap = GeneratedChannels::physicalChannelMap;

    #ifdef INCLUDE_SPACE_SEGMENT_CODE
    inline auto& masterChannelSsTmMap = GeneratedChannels::masterChannelSsTmMap;
    inline auto& masterChannelSsTcMap = GeneratedChannels::masterChannelSsTcMap;
    inline auto& virtualChannelSsTmMap = GeneratedChannels::virtualChannelSsTmMap;
    inline auto& virtualChannelSsTcMap = GeneratedChannels::virtualChannelSsTcMap;
    inline auto& mapChannelSsMap = GeneratedChannels::mapChannelSsMap;
    inline auto& farmMap = GeneratedChannels::farmMap;
    inline auto& saSpaceSegmentMap = GeneratedChannels::saSpaceSegmentMap;
    #endif

    #ifdef INCLUDE_GROUND_SEGMENT_CODE
    inline auto& masterChannelGsTcMap = GeneratedChannels::masterChannelGsTcMap;
    inline auto& virtualChannelGsTcMap = GeneratedChannels::virtualChannelGsTcMap;
    inline auto& mapChannelGsMap = GeneratedChannels::mapChannelGsMap;
    inline auto& fopMap = GeneratedChannels::fopMap;
    inline auto& saGroundSegmentMap = GeneratedChannels::saGroundSegmentMap;
    #endif

    /**
     * Octets of TM and TC are stored here
     */
    inline auto& frameOctetPool = GeneratedChannels::frameOctetPool;

    /**
     * @brief Allocate memory blocks for the channel containers
     * @note The user must call this before attempting to use any data link service/data handling function
     *
     * @return Whether the operation was successful or not
     */
    bool initializeChannelContainers();
} // CCSDSDataLinkLayer::Objects