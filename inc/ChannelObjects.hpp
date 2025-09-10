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
#include "SecurityAssociationKeys.hpp"

namespace CCSDSDataLinkLayer::Objects {
    /**
     * Use those enumerations to access any channel by their name
     */
    using Generated::PhysicalChannelName;
    using Generated::MasterChannelTmName;
    using Generated::MasterChannelTcName;
    using Generated::VirtualChannelTmName;
    using Generated::VirtualChannelTcName;
    using Generated::MapChannelName;

    /**
     * By casting the value of the above enums to:
     * - Pcid (for physical channels)
     * - VcidScidKey (for virtual channels, farm, fop and security associations)
     * - MapidVcidScidKey (for map channels)
     * and using it as a key for the maps below, every channel, farm, fop and security association
     * instance can be accessed.
     */
    inline auto& physicalChannelMap = Generated::physicalChannelMap;

    #ifdef INCLUDE_SPACE_SEGMENT_CODE
    inline auto& masterChannelSsTmMap = Generated::masterChannelSsTmMap;
    inline auto& masterChannelSsTcMap = Generated::masterChannelSsTcMap;
    inline auto& virtualChannelSsTmMap = Generated::virtualChannelSsTmMap;
    inline auto& virtualChannelSsTcMap = Generated::virtualChannelSsTcMap;
    inline auto& mapChannelSsMap = Generated::mapChannelSsMap;
    inline auto& farmMap = Generated::farmMap;
    inline auto& saSpaceSegmentMap = Generated::saSpaceSegmentMap;
    #endif

    #ifdef INCLUDE_GROUND_SEGMENT_CODE
    inline auto& masterChannelGsTcMap = Generated::masterChannelGsTcMap;
    inline auto& virtualChannelGsTcMap = Generated::virtualChannelGsTcMap;
    inline auto& mapChannelGsMap = Generated::mapChannelGsMap;
    inline auto& fopMap = Generated::fopMap;
    inline auto& saGroundSegmentMap = Generated::saGroundSegmentMap;
    #endif

    /**
     * Octets of TM and TC are stored here
     */
    inline auto& frameOctetPool = Generated::frameOctetPool;

    /**
     * Authentication keys for security associations
     */
    inline auto& authenticationKeyMap = Generated::authenticationKeyMap;

    /**
     * @brief Allocate memory blocks for the channel containers
     * @note The user must call this before attempting to use any data link service/data handling function
     *
     * @return Whether the operation was successful or not
     */
    bool initializeChannelContainers();
} // CCSDSDataLinkLayer::Objects