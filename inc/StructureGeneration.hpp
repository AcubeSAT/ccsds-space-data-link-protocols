/**
 * @file CCSDSStructureGeneration.hpp
 *
 * @brief Generate CCSDS objects at compile time using the configuration in CCSDSDataLink.def
 */

#pragma once
#include "etl/flat_map.h"
#include "etl/tuple.h"
#include "DefinitionsAndUtilities.hpp"
#include "COP1/FrameAcceptanceReporting.hpp"
#include "COP1/FrameOperationProcedure.hpp"
#include "SecurityAssociation.hpp"
#include "CCSDSPhysicalChannel.hpp"
#include "CCSDSMasterChannel.hpp"
#include "CCSDSVirtualChannel.hpp"
#include "CCSDSMAPChannel.hpp"
#include "TransferFrameTC.hpp"
#include "TransferFrameTM.hpp"
#include "MemoryPool.hpp"

namespace CCSDSDataLinkLayer {
// Initially define all macros as empty. Then selectively define some them to create different objects statically
#define PHYSICAL_CHANNEL(physicalChannelName, pcid, tfvn, maxTcLength, tmLength, maxFramesPdu, maxPduLength, bitRate, fecPresent)
#define MASTER_CHANNEL_TM(masterChannelName, scid, parentPcid)
#define MASTER_CHANNEL_TC(masterChannelName, scid, parentPcid)
#define VIRTUAL_CHANNEL_TM(virtualChannelName, vcid, parentScid, associatedSdlsSPI, secondaryHeaderPresent, secondaryHeaderLength, ocfFieldPresent, synchronization, repetitions, frameCapacity, packetCapacity)
#define VIRTUAL_CHANNEL_TC(virtualChannelName, vcid, parentScid, segHeaderPresent, blocking, copInEffect, associatedSdlsSPI, repetitions, frameCapacity, typeAdPacketCapacity, typeBdPacketCapacity))
#define MAP_CHANNEL(mapChannelName, mapid, parentVcid, parentScid, blocking, segmentation, associatedSdlsSPI, frameCapacity, typeAdPacketCapacity, typeBdPacketCapacity)
#define COP1(vcid, tiInitial, transmissionLimit, fopSlidingWindowWidth, timeoutType, farmSlidingWindowWidth, farmPositiveWindowWidth, farmNegativeWindowWidth, clcwReportInterval)
#define SECURITY_ASSOCIATION(spi, authenticationAlgorithm, enryptionAlgorithm, authKey)

/** Calculate channel counts
 *  Note: Each "channel" is constituted of a space and ground segment, with the exception of the physical channel.
 *        Therefore, 3 TC virtual channels actually result in the construction of 6 virtual channel objects (assuming
 *        both space and ground segment code are compiled)
 */

    inline constexpr uint8_t PhysicalChannelCount = (0
#define PHYSICAL_CHANNEL(physicalChannelName, pcid, tfvn, maxTcLength, tmLength, maxFramesPdu, maxPduLength, bitRate, fecPresent) +1
#include "CCSDSDataLink.def"
    );
#define PHYSICAL_CHANNEL(physicalChannelName, pcid, tfvn, maxTcLength, tmLength, maxFramesPdu, maxPduLength, bitRate, fecPresent)

    inline constexpr uint8_t MasterChannelTmCount = (0
#define MASTER_CHANNEL_TM(masterChannelName, scid, parentPcid) +1
#include "CCSDSDataLink.def"
    );
#define MASTER_CHANNEL_TM(masterChannelName, scid, parentPcid)

    inline constexpr uint8_t MasterChannelTcCount = (0
#define MASTER_CHANNEL_TC(masterChannelName, scid, parentPcid) +1
#include "CCSDSDataLink.def"
    );
#define MASTER_CHANNEL_TC(masterChannelName, scid, parentPcid)

    inline constexpr uint8_t VirtualChannelTmCount = (0
#define VIRTUAL_CHANNEL_TM(virtualChannelName, vcid, parentScid, associatedSdlsSPI, secondaryHeaderPresent, secondaryHeaderLength, ocfFieldPresent, synchronization, repetitions, frameCapacity, packetCapacity) +1
#include "CCSDSDataLink.def"
    );
#define VIRTUAL_CHANNEL_TM(virtualChannelName, vcid, parentScid, associatedSdlsSPI, secondaryHeaderPresent, secondaryHeaderLength, ocfFieldPresent, synchronization, repetitions, frameCapacity, packetCapacity)

    inline constexpr uint8_t VirtualChannelTcCount = (0
#define VIRTUAL_CHANNEL_TC(virtualChannelName, vcid, parentScid, segHeaderPresent, blocking, copInEffect, associatedSdlsSPI, repetitions, frameCapacity, typeAdPacketCapacity, typeBdPacketCapacity)) + 1
#include "CCSDSDataLink.def"
    );
#define VIRTUAL_CHANNEL_TC(virtualChannelName, vcid, parentScid, segHeaderPresent, blocking, copInEffect, associatedSdlsSPI, repetitions, frameCapacity, typeAdPacketCapacity, typeBdPacketCapacity))

    inline constexpr uint8_t MapChannelCount = (0
#define MAP_CHANNEL(mapChannelName, mapid, parentVcid, parentScid, blocking, segmentation, associatedSdlsSPI, frameCapacity, typeAdPacketCapacity, typeBdPacketCapacity) +1
#include "CCSDSDataLink.def"
    );
#define MAP_CHANNEL(mapChannelName, mapid, parentVcid, parentScid, blocking, segmentation, associatedSdlsSPI, frameCapacity, typeAdPacketCapacity, typeBdPacketCapacity)

    inline constexpr uint8_t cop1Count = (0
#define COP1(vcid, tiInitial, transmissionLimit, fopSlidingWindowWidth, timeoutType, farmSlidingWindowWidth, farmPositiveWindowWidth, farmNegativeWindowWidth, clcwReportInterval) +1
#include "CCSDSDataLink.def"
    );
#define COP1(vcid, tiInitial, transmissionLimit, fopSlidingWindowWidth, timeoutType, farmSlidingWindowWidth, farmPositiveWindowWidth, farmNegativeWindowWidth, clcwReportInterval)

    inline constexpr uint8_t saCount = (0
#define SECURITY_ASSOCIATION(spi, authenticationAlgorithm, encryptionAlgorithm, authKey) +1
#include "CCSDSDataLink.def"
    );
#define SECURITY_ASSOCIATION(spi, authenticationAlgorithm, encryptionAlgorithm, authKey)

/** Construct Physical channel objects **/
    enum class PhysicalChannelNameToKey : uint8_t {
#define PHYSICAL_CHANNEL(physicalChannelName, pcid, tfvn, maxTcLength, tmLength, maxFramesPdu, maxPduLength, bitRate, fecPresent) physicalChannelName = pcid,
#include "CCSDSDataLink.def"
        SentinelValue
    };

    inline etl::flat_map<uint8_t, PhysicalChannel, PhysicalChannelCount> physicalChannelMap = {
#define PHYSICAL_CHANNEL(physicalChannelName, pcid, tfvn, maxTcLength, tmLength, maxFramesPdu, maxPduLength, bitRate, fecPresent) \
    { \
    static_cast<uint8_t>(pcid), \
    PhysicalChannel(            \
    pcid,                       \
    tfvn,                       \
    maxTcLength,                \
    tmLength,                   \
    maxPduLength,               \
    bitRate,                    \
    fecPresent                  \
    )                           \
    },
#include "CCSDSDataLink.def"
    };
#define PHYSICAL_CHANNEL(physicalChannelName, pcid, tfvn, maxTcLength, tmLength, maxFramesPdu, maxPduLength, bitRate, fecPresent)

/** Construct Master channel TM objects **/
    enum class MasterChannelTmNameToKey : uint8_t {
#define MASTER_CHANNEL_TM(masterChannelName, scid, parentPcid) masterChannelName = scid,
#include "CCSDSDataLink.def"
        SentinelValue
    };

#ifdef INCLUDE_SPACE_SEGMENT_CODE
    inline etl::flat_map<uint16_t, MasterChannelSsTm, MasterChannelTmCount> masterChannelSsTmMap = {
#define MASTER_CHANNEL_TM(masterChannelName, scid, parentPcid) \
    { \
    static_cast<uint16_t>(scid), \
    MasterChannelSsTm(           \
    scid,                        \
    parentPcid                   \
    )                            \
    },
#include "CCSDSDataLink.def"
    };
#endif // INCLUDE_SPACE_SEGMENT_CODE
#define MASTER_CHANNEL_TM(masterChannelName, scid, parentPcid)

 /** Construct Master channel TC objects **/
    enum class MasterChannelTcNameToKey : uint8_t {
#define MASTER_CHANNEL_TC(masterChannelName, scid, parentPcid) masterChannelName = scid,
#include "CCSDSDataLink.def"
        SentinelValue
    };

#ifdef INCLUDE_SPACE_SEGMENT_CODE
    inline etl::flat_map<uint16_t, MasterChannelSsTc, MasterChannelTcCount> masterChannelSsTcMap = {
#define MASTER_CHANNEL_TC(masterChannelName, scid, parentPcid) \
    { \
    static_cast<uint16_t>(scid), \
    MasterChannelSSTc(           \
    scid,                        \
    parentPcid                   \
    )                            \
    },
#include "CCSDSDataLink.def"
    };
#endif // INCLUDE_SPACE_SEGMENT_CODE

#ifdef INCLUDE_GROUND_SEGMENT_CODE
    inline etl::flat_map<uint16_t, MasterChannelGsTc, MasterChannelTcCount> masterChannelGsTcMap = {
#define MASTER_CHANNEL_TC(masterChannelName, scid, parentPcid) \
{ \
static_cast<uint16_t>(scid), \
MasterChannelGSTc(           \
scid,                        \
parentPcid                   \
)                            \
},
#include "CCSDSDataLink.def"
    };
#endif // INCLUDE_GROUND_SEGMENT_CODE
#define MASTER_CHANNEL_TC(masterChannelName, scid, parentPcid)

/** Construct Virtual channel TM objects **/
    enum class VirtualChannelTmNameToKey : DefsAndUtils::VcidScidKey {
#define VIRTUAL_CHANNEL_TM(virtualChannelName, vcid, parentScid, associatedSdlsSPI, secondaryHeaderPresent, secondaryHeaderLength, ocfFieldPresent, synchronization, repetitions, frameCapacity, packetCapacity) \
    virtualChannelName = DefsAndUtils::constructVcidScidKey(vcid, parentScid),
#include "CCSDSDataLink.def"
        SentinelValue
    };

#ifdef INCLUDE_SPACE_SEGMENT_CODE
    inline etl::flat_map<DefsAndUtils::VcidScidKey, VirtualChannelSsTm, VirtualChannelTmCount> virtualChannelSsTmMap = {
#define VIRTUAL_CHANNEL_TM(virtualChannelName, vcid, parentScid, associatedSdlsSPI, secondaryHeaderPresent, secondaryHeaderLength, ocfFieldPresent, synchronization, repetitions, frameCapacity, packetCapacity) \
    { \
    DefsAndUtils::constructVcidScidKey(vcid, parentScid), \
    VirtualChannelSsTm(         \
    vcid,                       \
    parentScid,                 \
    repetitions,                \
    secondaryHeaderPresent,     \
    secondaryHeaderLength,      \
    ocfFieldPresent,            \
    synchronization,            \
    associatedSdlsSPI,          \
    frameCapacity,              \
    packetCapacity              \
    )                           \
    },
#include "CCSDSDataLink.def"
    };
#endif // INCLUDE_SPACE_SEGMENT_CODE
#define VIRTUAL_CHANNEL_TM(virtualChannelName, vcid, parentScid, associatedSdlsSPI, secondaryHeaderPresent, secondaryHeaderLength, ocfFieldPresent, synchronization, repetitions, frameCapacity, packetCapacity)

    /** Construct Virtual channel TC Space Segment objects **/
    enum class VirtualChannelTcNameToKey : DefsAndUtils::VcidScidKey {
#define VIRTUAL_CHANNEL_TC(virtualChannelName, vcid, parentScid, segHeaderPresent, blocking, copInEffect, associatedSdlsSPI, repetitions, frameCapacity, typeAdPacketCapacity, typeBdPacketCapacity)) \
    virtualChannelName = DefsAndUtils::constructVcidScidKey(vcid, parentScid),
#include "CCSDSDataLink.def"
        SentinelValue
    };

#ifdef INCLUDE_SPACE_SEGMENT_CODE
    inline etl::flat_map<DefsAndUtils::VcidScidKey, VirtualChannelSsTc, VirtualChannelTcCount> virtualChannelSsTcMap = {
#define VIRTUAL_CHANNEL_TC(virtualChannelName, vcid, parentScid, segHeaderPresent, blocking, copInEffect, associatedSdlsSPI, repetitions, frameCapacity, typeAdPacketCapacity, typeBdPacketCapacity)) \
    { \
    DefsAndUtils::constructVcidScidKey(vcid, parentScid), \
    VirtualChannelSsTc(         \
    vcid,                       \
    parentScid,                 \
    segHeaderPresent,           \
    blocking,                   \
    copInEffect,                \
    associatedSdlsSPI,          \
    frameCapacity,              \
    typeAdPacketCapacity,       \
    typeBdPacketCapacity        \
    )                           \
    },
#include "CCSDSDataLink.def"
    };
#endif // INCLUDE_SPACE_SEGMENT_CODE

#ifdef INCLUDE_GROUND_SEGMENT_CODE
    inline etl::flat_map<DefsAndUtils::VcidScidKey, VirtualChannelGsTc, VirtualChannelTcCount> virtualChannelGsTcMap = {
#define VIRTUAL_CHANNEL_TC(virtualChannelName, vcid, parentScid, segHeaderPresent, blocking, copInEffect, associatedSdlsSPI, repetitions, frameCapacity, typeAdPacketCapacity, typeBdPacketCapacity)) \
    { \
    DefsAndUtils::constructVcidScidKey(vcid, parentScid), \
    VirtualChannelGsTc(         \
    vcid,                       \
    parentScid,                 \
    repetitions,                \
    segHeaderPresent,           \
    blocking,                   \
    copInEffect,                \
    associatedSdlsSPI,          \
    frameCapacity,              \
    typeAdPacketCapacity,       \
    typeBdPacketCapacity        \
    )                           \
    },
#include "CCSDSDataLink.def"
    };
#endif // INCLUDE_GROUND_SEGMENT_CODE
#define VIRTUAL_CHANNEL_TC(virtualChannelName, vcid, parentScid, segHeaderPresent, blocking, copInEffect, associatedSdlsSPI, repetitions, frameCapacity, typeAdPacketCapacity, typeBdPacketCapacity))

    /** Construct MAP channel objects **/
   enum class MapChannelNameToKey : DefsAndUtils::MapidVcidScidKey {
#define MAP_CHANNEL(mapChannelName, mapid, parentVcid, parentScid, blocking, segmentation, associatedSdlsSPI, frameCapacity, typeAdPacketCapacity, typeBdPacketCapacity) \
    mapChannelName = DefsAndUtils::constructMscidVcidScidKey(vcid, parentVcid, parentScid),
#include "CCSDSDataLink.def"
        SentinelValue
    };

#ifdef INCLUDE_SPACE_SEGMENT_CODE
    inline etl::flat_map<DefsAndUtils::MapidVcidScidKey, MAPChannelSs, MapChannelCount> mapChannelSsMap = {
#define MAP_CHANNEL(mapChannelName, mapid, parentVcid, parentScid, blocking, segmentation, associatedSdlsSPI, frameCapacity, typeAdPacketCapacity, typeBdPacketCapacity) \
    { \
    DefsAndUtils::constructMscidVcidScidKey(vcid, parentVcid, parentScid), \
    MapChannelSs(         \
    mapid,                \
    parentVcid,           \
    blocking,             \
    segmentation,         \
    associatedSdlsSPI,    \
    frameCapacity,        \
    typeAdPacketCapacity, \
    typeBdPacketCapacity  \
    )                     \
    },
#include "CCSDSDataLink.def"
    };
#endif // INCLUDE_SPACE_SEGMENT_CODE

#ifdef INCLUDE_GROUND_SEGMENT_CODE
    inline etl::flat_map<DefsAndUtils::MapidVcidScidKey, MAPChannelGs, MapChannelCount> mapChannelGsMap = {
#define MAP_CHANNEL(mapChannelName, mapid, parentVcid, parentScid, blocking, segmentation, associatedSdlsSPI, frameCapacity, typeAdPacketCapacity, typeBdPacketCapacity) \
    { \
    DefsAndUtils::constructMscidVcidScidKey(vcid, parentVcid, parentScid), \
    MapChannelGs(                \
    mapid,                       \
    parentVcid,                  \
    blocking,                    \
    segmentation,                \
    associatedSdlsSPI,           \
    frameCapacity,               \
    )                            \
    },
#include "CCSDSDataLink.def"
    };
#endif // INCLUDE_GROUND_SEGMENT_CODE
#define MAP_CHANNEL(mapChannelName, mapid, parentVcid, parentScid, blocking, segmentation, associatedSdlsSPI, frameCapacity, typeAdPacketCapacity, typeBdPacketCapacity)

/** Construct FARM and FOP Objects (COP-1) **/
#ifdef INCLUDE_GROUND_SEGMENT_CODE
    inline etl::flat_map<DefsAndUtils::VcidScidKey, FrameOperationProcedure, cop1Count> fopMap = {
#define COP1(vcid, tiInitial, transmissionLimit, fopSlidingWindowWidth, timeoutType, farmSlidingWindowWidth, farmPositiveWindowWidth, farmNegativeWindowWidth, clcwReportInterval) \
    {                           \
    DefsAndUtils::constructVcidScidKey(vcid, parentScid), \
    FrameOperationProcedure(    \
    vcid,                       \
    tiInitial,                  \
    transmissionLimit,          \
    fopSlidingWindowWidth,      \
    )                           \
    },
#include "CCSDSDataLink.def"
    };
#endif // INCLUDE_GROUND_SEGMENT_CODE

#ifdef INCLUDE_SPACE_SEGMENT_CODE
    inline etl::flat_map<DefsAndUtils::VcidScidKey, FrameAcceptanceReporting, cop1Count> farmMap = {
#define COP1(vcid, tiInitial, transmissionLimit, fopSlidingWindowWidth, timeoutType, farmSlidingWindowWidth, farmPositiveWindowWidth, farmNegativeWindowWidth, clcwReportInterval) \
    {                           \
    DefsAndUtils::constructVcidScidKey(vcid, parentScid), \
    FrameAcceptanceReporting(   \
    vcid,                       \
    farmSlidingWindowWidth,     \
    farmPositiveWindowWidth,    \
    farmNegativeWindowWidth,    \
    clcwReportInterval,         \
    )                           \
    },
#include "CCSDSDataLink.def"
    };
#endif // INCLUDE_SPACE_SEGMENT_CODE
#define COP1(vcid, tiInitial, transmissionLimit, fopSlidingWindowWidth, timeoutType, farmSlidingWindowWidth, farmPositiveWindowWidth, farmNegativeWindowWidth, clcwReportInterval)

/** Construct CLCW queues
 *  @details FARM generated CLCWs are placed in these buffers, which are then placed inside the ocf
 *  fields of TM frames.
 */
#ifdef INCLUDE_SPACE_SEGMENT_CODE
    //  count how many Tm virtual channels have an ocf field
    inline constexpr uint8_t WithOcfFieldCount = (0
#define VIRTUAL_CHANNEL_TM(virtualChannelName, vcid, parentScid, associatedSdlsSPI, secondaryHeaderPresent, secondaryHeaderLength, ocfFieldPresent, synchronization, repetitions, frameCapacity, packetCapacity) \
    + (ocfFieldPresent ? +1 : 0)
#include "CCSDSDataLink.def"
    );

    inline etl::flat_map<DefsAndUtils::VcidScidKey, etl::queue<CLCW, DefsAndUtils::ClcwQueueSize>, WithOcfFieldCount> clcwQueueMap = {
#define VIRTUAL_CHANNEL_TM(virtualChannelName, vcid, parentScid, associatedSdlsSPI, secondaryHeaderPresent, secondaryHeaderLength, ocfFieldPresent, synchronization, repetitions, frameCapacity, packetCapacity) \
    { \
    DefsAndUtils::constructVcidScidKey(vcid, parentScid), \
    etl::queue<CLCW, DefsAndUtils::ClcwQueueSize>{}       \
    },
#include "CCSDSDataLink.def"
    };
#endif// INCLUDE_SPACE_SEGMENT_CODE
#define VIRTUAL_CHANNEL_TM(virtualChannelName, vcid, parentScid, associatedSdlsSPI, secondaryHeaderPresent, secondaryHeaderLength, ocfFieldPresent, synchronization, repetitions, frameCapacity, packetCapacity)


/** Construct Security Association Objects **/
#define SECURITY_ASSOCIATION(spi, authenticationAlgorithm, encryptionAlgorithm, authKey) \
    {                           \
    DefsAndUtils::constructVcidScidKey(vcid, parentScid), \
    SecurityAssociation(        \
    spi,                        \
    authenticationAlgorithm,    \
    encryptionAlgorithm         \
    )                           \
    },

// The sender and receiver use the same object, but due to some shared variables, separate
// instances are required
#ifdef INCLUDE_SPACE_SEGMENT_CODE
    inline etl::flat_map<DefsAndUtils::VcidScidKey, SecurityAssociation, saCount> saSpaceSegmentMap = {
#include "CCSDSDataLink.def"
    };
#endif// INCLUDE_SPACE_SEGMENT_CODE

#ifdef INCLUDE_SPACE_SEGMENT_CODE
    inline etl::flat_map<DefsAndUtils::VcidScidKey, SecurityAssociation, saCount> saGroundSegmentMap = {
#include "CCSDSDataLink.def"
    };
#endif // INCLUDE_GROUND_SEGMENT_CODE

/** Memory allocation for containers and Memory pool creation for frame octets  **/
#ifdef INCLUDE_GROUND_SEGMENT_CODE
    inline constexpr bool IncludeSpaceCode = true;
#else
    inline constexpr bool IncludeSpaceCode = false;
#endif

#ifdef INCLUDE_GROUND_SEGMENT_CODE
    inline constexpr bool IncludeGroundCode = true;
#else
    inline constexpr bool IncludeGroundCode = false;
#endif

// Calculate total frame capacity for virtual and map channels
    inline constexpr uint16_t TotalVirtualChannelTcFrameCapacity = (0
#define VIRTUAL_CHANNEL_TC(virtualChannelName, vcid, parentScid, segHeaderPresent, blocking, copInEffect, associatedSdlsSPI, repetitions, frameCapacity, typeAdPacketCapacity, typeBdPacketCapacity)) +frameCapacity
#include "CCSDSDataLink.def"
);
#define VIRTUAL_CHANNEL_TC(virtualChannelName, vcid, parentScid, segHeaderPresent, blocking, copInEffect, associatedSdlsSPI, repetitions, frameCapacity, typeAdPacketCapacity, typeBdPacketCapacity))

    inline constexpr uint16_t TotalVirtualChannelTmFrameCapacity = (0
#define VIRTUAL_CHANNEL_TM(virtualChannelName, vcid, parentScid, associatedSdlsSPI, secondaryHeaderPresent, secondaryHeaderLength, ocfFieldPresent, synchronization, repetitions, frameCapacity, packetCapacity) +frameCapacity
#include "CCSDSDataLink.def"
);
#define VIRTUAL_CHANNEL_TM(virtualChannelName, vcid, parentScid, associatedSdlsSPI, secondaryHeaderPresent, secondaryHeaderLength, ocfFieldPresent, synchronization, repetitions, frameCapacity, packetCapacity)

    inline constexpr uint16_t TotalMapChannelFrameCapacity = (0
#define MAP_CHANNEL(mapChannelName, mapid, parentVcid, parentScid, blocking, segmentation, associatedSdlsSPI, frameCapacity, typeAdPacketCapacity, typeBdPacketCapacity) +frameCapacity
#include "CCSDSDataLink.def"
);
#define MAP_CHANNEL(mapChannelName, mapid, parentVcid, parentScid, blocking, segmentation, associatedSdlsSPI, frameCapacity, typeAdPacketCapacity, typeBdPacketCapacity)

// Calculate total packet capacity for TM channels:
    inline constexpr uint16_t TotalTmPacketCapacity = (0
#define VIRTUAL_CHANNEL_TM(virtualChannelName, vcid, parentScid, associatedSdlsSPI, secondaryHeaderPresent, secondaryHeaderLength, ocfFieldPresent, synchronization, repetitions, frameCapacity, packetCapacity) +packetCapacity
#include "CCSDSDataLink.def"
    );
#define VIRTUAL_CHANNEL_TM(virtualChannelName, vcid, parentScid, associatedSdlsSPI, secondaryHeaderPresent, secondaryHeaderLength, ocfFieldPresent, synchronization, repetitions, frameCapacity, packetCapacity)

// Calculate total packet capacity for TC channels:
    inline constexpr uint16_t TotalMapChannelTypeAdPacketCapacity = (0
#define MAP_CHANNEL(mapChannelName, mapid, parentVcid, parentScid, blocking, segmentation, associatedSdlsSPI, frameCapacity, typeAdPacketCapacity, typeBdPacketCapacity) +typeAdPacketCapacity
#include "CCSDSDataLink.def"
);
#define MAP_CHANNEL(mapChannelName, mapid, parentVcid, parentScid, blocking, segmentation, associatedSdlsSPI, frameCapacity, typeAdPacketCapacity, typeBdPacketCapacity)

    inline constexpr uint16_t TotalVirtualChannelTypeAdCapacity = (0
#define VIRTUAL_CHANNEL_TC(virtualChannelName, vcid, parentScid, segHeaderPresent, blocking, copInEffect, associatedSdlsSPI, repetitions, frameCapacity, typeAdPacketCapacity, typeBdPacketCapacity)) +typeAdPacketCapacity
#include "CCSDSDataLink.def"
    );
#define VIRTUAL_CHANNEL_TC(virtualChannelName, vcid, parentScid, segHeaderPresent, blocking, copInEffect, associatedSdlsSPI, repetitions, frameCapacity, typeAdPacketCapacity, typeBdPacketCapacity))

    inline constexpr uint16_t TotalMapChannelTypeBdPacketCapacity = (0
#define MAP_CHANNEL(mapChannelName, mapid, parentVcid, parentScid, blocking, segmentation, associatedSdlsSPI, frameCapacity, typeAdPacketCapacity, typeBdPacketCapacity) +typeBdPacketCapacity
#include "CCSDSDataLink.def"
);
#define MAP_CHANNEL(mapChannelName, mapid, parentVcid, parentScid, blocking, segmentation, associatedSdlsSPI, frameCapacity, typeAdPacketCapacity, typeBdPacketCapacity)

    inline constexpr uint16_t TotalVirtualChannelTypeBdCapacity = (0
#define VIRTUAL_CHANNEL_TC(virtualChannelName, vcid, parentScid, segHeaderPresent, blocking, copInEffect, associatedSdlsSPI, repetitions, frameCapacity, typeAdPacketCapacity, typeBdPacketCapacity)) +typeBdPacketCapacity
#include "CCSDSDataLink.def"
    );
#define VIRTUAL_CHANNEL_TC(virtualChannelName, vcid, parentScid, segHeaderPresent, blocking, copInEffect, associatedSdlsSPI, repetitions, frameCapacity, typeAdPacketCapacity, typeBdPacketCapacity))

#undef PHYSICAL_CHANNEL
#undef MASTER_CHANNEL_TM
#undef MASTER_CHANNEL_TC
#undef VIRTUAL_CHANNEL_TM
#undef VIRTUAL_CHANNEL_TC
#undef MAP_CHANNEL
#undef COP1
#undef SECURITY_ASSOCIATION

// Calculate amount of slots each type of data structure needed
// Note1: The required capacity for a master channel is the sum of the capacities of its virtual channels
// Note2: For a TC virtual channel that has map channels, it's frame capacity was defined as 0 in the .def file,
//       since the actual capacity is the sum of said map channels. Therefore, the TotalVirtualChannelFrameCapacity
//       variable actually contains capacities of virtual channels that DO NOT contain map channels. The total capacity
//       required for virtual channel WITH map channels is the TotalMapChannelFrameCapacity

    inline constexpr uint16_t TotalTransferFrameTcSlots = (IncludeGroundCode + IncludeSpaceCode) *
        (TotalMapChannelFrameCapacity + TotalVirtualChannelTcFrameCapacity);

    // MapChannelSs: 1 queue -> TotalMapChannelFrameCapacity
    // VirtualChannelSsTc: 5 queues ->  5 * (TotalMapChannelFrameCapacity + TotalVirtualChannelTcFrameCapacity)
    // MasterChannelSsTc: No queues
    // MapChannelGsTc: No queues
    // VirtualChannelGsTc: 2 queues -> 2 * (TotalMapChannelFrameCapacity + TotalVirtualChannelTcFrameCapacity)
    // MasterChannelGsTc: 1 queue -> TotalMapChannelFrameCapacity + TotalVirtualChannelTcFrameCapacity
    inline constexpr uint16_t TotalTransferFrameTcPtrSlots =
        IncludeSpaceCode * (
            5 * (TotalMapChannelFrameCapacity + TotalVirtualChannelTcFrameCapacity) +
            TotalMapChannelFrameCapacity
        ) +
        IncludeGroundCode * (
            3 * (TotalMapChannelFrameCapacity + TotalVirtualChannelTcFrameCapacity)
        );

    inline constexpr uint16_t TotalTransferFrameTmSlots = IncludeSpaceCode *
        TotalVirtualChannelTmFrameCapacity;

    // VirtualChannelSsTm: No queues
    // MasterChannelSsTm: 2 queues
    // VirtualChannelGsTm: Unimplemented
    // MasterChannelGsTm: Unimplemented
    inline constexpr uint16_t TotalTransferFrameTmPtrSlots = IncludeSpaceCode *
        2* TotalVirtualChannelTmFrameCapacity;

    inline constexpr uint16_t TotalTypeAdPacketSlots = IncludeGroundCode * (TotalVirtualChannelTypeAdCapacity + TotalMapChannelTypeAdPacketCapacity);
    inline constexpr uint16_t TotalTypeBdPacketSlots = IncludeGroundCode * (TotalVirtualChannelTypeBdCapacity + TotalMapChannelTypeBdPacketCapacity);
    inline constexpr uint16_t TotalTmPacketSlots = TotalTmPacketCapacity;

    inline TransferFrameTC transferFrameTcArray[TotalTransferFrameTcSlots];
    inline TransferFrameTC* transferFrameTcPtrArray[TotalTransferFrameTcPtrSlots];
    inline TransferFrameTM transferFrameTmArray[TotalTransferFrameTmSlots];
    inline TransferFrameTM* transferFrameTmPtrArray[TotalTransferFrameTmPtrSlots];
    inline uint16_t packetLengthsArray[TotalTypeAdPacketSlots + TotalTypeBdPacketSlots + TotalTmPacketSlots];
    inline uint8_t packetOctetsArray[(TotalTypeAdPacketSlots + TotalTypeBdPacketSlots + TotalTmPacketSlots) * DefsAndUtils::MaxExpectedSpacePacketSize];

    inline MemoryPool<TotalTransferFrameTcSlots * DefsAndUtils::MaxTcTransferFrameLength + TotalTransferFrameTmSlots * DefsAndUtils::MaxTmTransferFrameLength,
    TotalTransferFrameTcSlots + TotalTransferFrameTmSlots> frameOctetPool;

    /**
     * @brief Allocate memory blocks to the channel containers, from the arrays defined above
     * @return Whether the operation was successful or not
     */
    bool initializeChannelContainers();
} // namespace CCSDSDataLinkLayer
