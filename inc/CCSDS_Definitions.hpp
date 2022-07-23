/*
 * All necessary definitions used throughout the program
 */

#pragma once

#include <cstdint>

inline uint8_t const tm_error_control_field_exists = 1;
const uint16_t SpacecraftIdentifier = 567; // A 10-bit unique identifier, assigned by the SANA

// TODO? See if some of those constants don't need to be global and can be defined in the class instead

const uint16_t TmTransferFrameSize = 128;
const uint8_t TmPrimaryHeaderSize = 6;
const uint8_t TmTrailerSize = 6;

const uint8_t TcPrimaryHeaderSize = 5;
const bool TcErrorControlFieldExists = false;

const uint16_t MCID = SpacecraftIdentifier;

// @todo Make this specific to each MAP/virtual channel. Probably requires some clever memory management
const uint8_t MaxReceivedTcInMapChannel = 5;
const uint8_t MaxReceivedTmInMapChannel = 5;

const uint8_t MaxReceivedTxTcInWaitQueue = 10; ///> Maximum received TX TCs in wait queue (before COP checks).
const uint8_t MaxReceivedRxTcInWaitQueue = 10; ///> Maximum received RX TCs in wait queue (before COP checks).

const uint8_t MaxReceivedTxTcInFOPSentQueue = 10; ///> Maximum received TX TCs in sent queue (following COP checks).
const uint8_t MaxReceivedRxTcInFOPSentQueue = 10; ///> Maximum received RX TCs in sent queue (following COP checks).
const uint8_t MaxReceivedRxTmInVirtBuffer = 10; ///> Maxiumm received RX TMs in Virtual channel buffer

const uint8_t MaxReceivedTxTcInFARMSentQueue = 10; ///> Maximum received TX TMs in sent queue (following COP checks).
const uint8_t MaxReceivedRxTcInFARMSentQueue = 10; ///> Maximum received RX TCs in sent queue (following COP checks).

const uint8_t MaxReceivedTxTcInMasterBuffer = 100; ///> Maximum received TX TC in the master buffer

/**
 * Maximum received TX TCs in the master buffer, ready to be transmitted to the lower procedures (doesn't include
 * repetitions)
 */
const uint8_t MaxReceivedTxTcOutInMasterBuffer = 100;

/**
 * Maximum received TX TMs in the master buffer, ready to be transmitted to the lower procedures (doesn't include
 * repetitions)
 */
const uint8_t MaxReceivedTxTmInMasterBuffer = 100;

const uint8_t MaxReceivedTxTmOutInVCBuffer =
    100; ///> Maximum received TM in the master buffer, after passing through the all frames generation service
const uint8_t MaxReceivedTxTmInVCBuffer =
    100; ///> Maximum received TM in the master buffer, before being passed to the all frames generation service

const uint8_t MaxReceivedTxTmOutInMasterBuffer = 100; ///> Maximum received fully-processed TX TMs in the master buffer
const uint8_t MaxReceivedRxTcInMasterBuffer = 100; ///> Maximum received unprocessed RX TCs in the master buffer
const uint8_t MaxReceivedRxTcInVirtualChannelBuffer = 100; ///> Maximum received RX TCs in virtual channel buffer
const uint8_t MaxReceivedRxTcInMAPBuffer = 100; ///> Maximum received RX TCs in MAP buffer
const uint8_t MaxReceivedRxTcOutInMasterBuffer = 100; ///> Maximum received fully processed RX TCs in MAP buffer

const uint8_t MaxVirtualChannels = 10;
const uint8_t MaxMapChannels = 3;

// Number of master copies of TX transfer frames that are stored in the master channel. This holds all the transfer
// frames that are stored in all services
const uint8_t MaxTxInMasterChannel = 200;
/**
 * Number of master copies of RX transfer frames that are stored in the master channel.
 * This holds all the transfer frames that are stored in all services
 */

const uint8_t MaxRxInMasterChannel = 200;

const uint8_t MaxReceivedUnprocessedTxTcInVirtBuffer =
    6; ///> Raw TX TC packets stored directly in the virtual channel buffer.
/// Set to 0 if VC processing service isn't used
const uint8_t MaxReceivedUnprocessedTxTmInVirtBuffer =
    6; ///> Raw TX TM packets stored directly in the virtual channel buffer.
/// Set to 0 if VC processing service isn't used

const uint8_t FopSlidingWindowInitial = 255;
const uint8_t FopTimerInitial = 60; // sec

const uint16_t MemoryPoolMemorySize = 5 * 128; // Size of memory pool

enum FlagState { NOT_READY = 0, READY = 1 };

const uint16_t PacketBufferTmSize = 512;

const uint16_t ClcwBufferSize = 100;

const uint8_t idle_data[] = {
    0x53, 0x45, 0x24, 0x3,  0xce, 0xf0, 0xd2, 0x75, 0x50, 0xb9, 0x57, 0x24, 0x70, 0x83, 0xa8, 0x4e, 0x44, 0xd4, 0xa6,
    0x90, 0xc2, 0x9d, 0x1a, 0xb,  0x42, 0xe9, 0x42, 0x66, 0x28, 0x80, 0x5e, 0x30, 0x1f, 0x75, 0x3b, 0x5,  0xcb, 0x93,
    0xe3, 0x2d, 0xfc, 0xc4, 0x3c, 0x91, 0xd0, 0x17, 0xde, 0xf3, 0x9d, 0xb6, 0x6a, 0xcf, 0x3,  0x3a, 0x5b, 0x67, 0x78,
    0x3d, 0xce, 0x2,  0x4,  0x3,  0xa8, 0xd,  0x52, 0xd4, 0x6f, 0x2,  0xab, 0xd3, 0x1a, 0x1c, 0x4e, 0xde, 0x87, 0x28,
    0xda, 0xfe, 0x7c, 0xfe, 0xb2, 0x63, 0x9b, 0x24, 0x10, 0x4,  0x7,  0x1a, 0xaa, 0xa0, 0x2f, 0x40, 0x1e, 0x6f, 0x2e,
    0xb9, 0x2b, 0x69, 0x7,  0xf0, 0x62, 0x42, 0xfd, 0x75, 0xb5, 0xe8, 0xd2, 0x18, 0xe,  0x19, 0xeb, 0x2a, 0x86, 0xda,
    0x55, 0x8c, 0x48, 0xaf, 0xd6, 0x1d, 0x17, 0x71, 0x2b, 0x0,  0x58, 0x90, 0xd2, 0x81, 0x92, 0x38, 0x5a, 0xf9, 0x72,
    0x58, 0xbb, 0xea, 0xa6, 0x17, 0x3f, 0x14, 0x8b, 0x78, 0xe3, 0xec, 0x2c, 0x8e, 0x94, 0x3b, 0x3d, 0x8,  0x20, 0x51,
    0x5c, 0x60, 0xe5, 0x80, 0x5d, 0xdb, 0x7c, 0x8e, 0xbf, 0xe0, 0xfd, 0x73, 0xef, 0xf0, 0xd8, 0x50, 0x1c, 0xa6, 0xc6,
    0x7e, 0x2,  0x11, 0x44, 0xe8, 0x31, 0x54, 0x83, 0x2f, 0x18, 0xf5, 0x44, 0x36, 0x36, 0xe5, 0xf6, 0x2f, 0x4c, 0x10,
    0xf8, 0xc4, 0x4,  0x41, 0x1f, 0xa5, 0x99, 0x88, 0xac, 0x54, 0x74, 0x13, 0xba, 0x3,  0x21, 0xde, 0xdb, 0x56, 0x5e,
    0x14, 0x67, 0xe2, 0xae, 0xcb, 0xb6, 0x93, 0x99, 0x95, 0xdf, 0xbc, 0x51, 0xd2, 0x7b, 0x74, 0xa,  0xbb, 0x87, 0x8,
    0x4f, 0x82, 0x1b, 0x24, 0xfd, 0xd1, 0xf9, 0xc1, 0x21, 0x8e, 0x23, 0xce, 0xa0, 0xcc, 0xa,  0x50, 0xa6, 0x29, 0x60,
    0x53, 0x32, 0x1f, 0xe3, 0x5e, 0x9c, 0x6b, 0x5f, 0x2e, 0xbf, 0x3d, 0xd6, 0xf2, 0x9f, 0xd7, 0xf6, 0x5,  0xcc, 0xc8,
    0x80, 0xdb, 0x7b, 0x61, 0x1c, 0x17, 0xbb, 0xf3, 0xcd, 0x13, 0x16, 0x31, 0x33, 0xcb, 0xa7, 0xbb, 0x3a, 0x5a, 0x52,
    0x58, 0x3e, 0xa7, 0x27, 0x81, 0xe7, 0xec, 0x19, 0x91, 0xa4, 0x61, 0xa2, 0x66, 0x7e, 0x2c, 0xde, 0xc4, 0x9c, 0x3d,
    0xc9, 0xe9, 0xb4, 0xb,  0x1d, 0x8a, 0xa,  0x89, 0x43, 0x80, 0x1d, 0xfd, 0xab, 0xbd, 0xd2, 0x58, 0xf6, 0x25, 0x26,
    0xcc, 0xe7, 0x88, 0xf,  0xd,  0x2,  0x16, 0x74, 0x3,  0xc7, 0x64, 0xed, 0x91, 0xa5, 0xd,  0x2d, 0x9a, 0xc,  0x74,
    0x30, 0xb4, 0x5c, 0xbd, 0x19, 0x2,  0xd3, 0x6c, 0xd,  0x62, 0x4d, 0xa0, 0xa9, 0xa5, 0x4,  0x2,  0xba, 0x16, 0xae,
    0x2d, 0x19, 0xe2, 0xa1, 0xad, 0x3e, 0x83, 0x3c, 0x3d, 0x8d, 0x72, 0x34, 0xca, 0x9a, 0x19, 0x5d, 0x2b, 0x77, 0x1c,
    0x40, 0x42, 0x64, 0xc5, 0xac, 0x59, 0x72, 0x8f, 0xac, 0x6b, 0xc9, 0x88, 0x1,  0x63, 0xb5, 0xfa, 0x4,  0x70, 0x3a,
    0x2d, 0xcf, 0x93, 0x50, 0xc1, 0x32, 0xf3, 0xf,  0xf8, 0x44, 0x9c, 0x84, 0x87, 0xe8, 0x62, 0xc7, 0x28, 0x2b, 0xdd,
    0x7d, 0xfd, 0xa2, 0xb4, 0xad, 0xbd, 0xbf, 0xfe, 0x5e, 0x98, 0x31, 0xfc, 0xdd, 0x6e, 0xfe, 0x6,  0x82, 0xc2, 0xe5,
    0x50, 0xac, 0x3d, 0xe4, 0x4c, 0xf4, 0x7a, 0xf1, 0x3a, 0x6e, 0xdd, 0x5f, 0x99, 0x5e, 0xd4, 0x12, 0xe7, 0xfb, 0x5d,
    0xfa, 0x61, 0xfa, 0xda, 0x89, 0x34, 0xa6, 0x22, 0x92, 0x1c, 0xd0, 0x2d, 0x19, 0x6f, 0xd6, 0x53, 0x4e, 0x71, 0xf4,
    0x2d, 0x35, 0xe6, 0x99, 0xde, 0x1d, 0xff, 0x7f, 0x4a, 0x2a, 0xab, 0xe,  0xe6, 0x2,  0xc0, 0x68, 0x49, 0x54, 0x2a,
    0x50, 0xc6, 0xef, 0x1f, 0x7,  0x12, 0xc1, 0x54, 0x36, 0x68, 0xa6, 0xc9, 0x3c, 0x8d, 0xfe, 0x74, 0x7,  0x16, 0x7b,
    0x1f, 0xc4, 0x8b, 0x30, 0x5,  0xd3, 0x7,  0x28, 0xf9, 0x4,  0x7c, 0x9c, 0x86, 0x24, 0xc8, 0x65, 0x5a, 0x65, 0xa3,
    0xfa, 0xfa, 0xe1, 0xac, 0xc0, 0xef, 0xc1, 0xe2, 0x44, 0x4a, 0x9f, 0xbe, 0x84, 0xda, 0x83, 0x85, 0xd1, 0xc3, 0x5a,
    0xfe, 0x3a, 0xd3, 0x4d, 0x5a, 0x42, 0x28, 0x1a, 0x94, 0x81, 0xab, 0x6,  0xd,  0x49, 0x27, 0x27, 0x2c, 0x5a, 0x12,
    0xe,  0x56, 0x6b, 0x2d, 0xa4, 0x47, 0xa3, 0x89, 0xed, 0x6,  0xcf, 0xef, 0x71, 0x8a, 0x41, 0x86, 0x93, 0x50, 0x38,
    0x9e, 0xf2, 0x14, 0xfe, 0x97, 0x80, 0x84, 0xb3, 0x5a, 0xd,  0x60, 0x92, 0x48, 0xae, 0xb5, 0xea, 0xeb, 0x13, 0xf7,
    0xce, 0x1a, 0xfa, 0xfb, 0x83, 0xbf, 0x3c, 0x8f, 0x11, 0x63, 0xff, 0xad, 0x3b, 0x5c, 0x34, 0x33, 0x7,  0x50, 0x46,
    0x7b, 0x2,  0x9f, 0x4b, 0x15, 0xbb, 0x8d, 0x90, 0x8b, 0x9c, 0x51, 0x22, 0xed, 0xa,  0xf4, 0xa4, 0x9b, 0x6c, 0xe4,
    0x9f, 0x2c, 0x88, 0x6c, 0xd0, 0xc9, 0x47, 0x2e, 0xe9, 0xf,  0x76, 0x7,  0xa5, 0xe,  0x86, 0xd,  0xf7, 0x3d, 0x26,
    0xf3, 0xfe, 0xd7, 0xb5, 0x9f, 0x80, 0x0,  0x9e, 0xc4, 0x67, 0xde, 0xc8, 0xf4, 0x9,  0xdc, 0xa,  0xf2, 0x46, 0xe8,
    0x98, 0x86, 0xe5, 0x34, 0xab, 0xf2, 0xbd, 0xf4, 0x2a, 0x65, 0xb6, 0x90, 0xba, 0xdd, 0xda, 0x8b, 0xaf, 0x42, 0x25,
    0x42, 0xd7, 0x68, 0xd1, 0x91, 0x70, 0x1d, 0x3c, 0xf3, 0xfc, 0x84, 0x8d, 0x99, 0x1d, 0x27, 0xb5, 0xd0, 0xab, 0xbf,
    0xbd, 0xbb, 0xf,  0x6,  0x95, 0xfd, 0x96, 0x95, 0xb2, 0x1b, 0xaa, 0x90, 0x90, 0x73, 0xb5, 0xc,  0x3b, 0x4e, 0xcf,
    0x7f, 0x33, 0x14, 0x14, 0x72, 0x4d, 0xa9, 0x9,  0x65, 0x1f, 0x50, 0x6f, 0x39, 0xa2, 0xe3, 0x53, 0x8d, 0x48, 0xb6,
    0x82, 0xd1, 0x3e, 0x10, 0x9b, 0x2c, 0x23, 0x10, 0x1e, 0xd5, 0x7e, 0x87, 0x7,  0x2e, 0x81, 0xf4, 0x76, 0xb5, 0xa6,
    0x7b, 0x4c, 0xd0, 0x34, 0x35, 0x50, 0x0,  0x4c, 0x25, 0xa6, 0xbf, 0xa1, 0xee, 0x93, 0x20, 0xb,  0x81, 0x94, 0x4,
    0xc9, 0xc,  0x64, 0x19, 0xdf, 0x90, 0x1,  0x7d, 0xf7, 0x5a, 0x24, 0xb3, 0x5d, 0xca, 0xe0, 0x9a, 0x8e, 0x53, 0x52,
    0xd4, 0x54, 0xa,  0xab, 0x9a, 0x88, 0x7c, 0x99, 0x21, 0x74, 0xc6, 0x6f, 0xbe, 0x49, 0x75, 0x3d, 0x38, 0xa3, 0x99,
    0x8,  0xbd, 0x2,  0x12, 0x23, 0x43, 0xcb, 0x5d, 0x85, 0x7b, 0x45, 0xac, 0xe0, 0x2f, 0x71, 0x85, 0x6f, 0x79, 0x7c,
    0xdf, 0x9e, 0x7,  0xfe, 0x1b, 0x99, 0x11, 0x3f, 0xdd, 0xc5, 0xd1, 0xac, 0x9e, 0x1,  0x7e, 0xf0, 0x6d, 0xde, 0x11,
    0x64, 0xf,  0x5e, 0xcc, 0x39, 0x70, 0x2e, 0x79, 0x13, 0xac, 0x52, 0xbd, 0xe7, 0xb1, 0xe5, 0x81, 0x4c, 0xd7, 0x7f,
    0x8d, 0x59, 0x4a, 0x52, 0xe0, 0x26, 0x66, 0xf2, 0x8a, 0x10, 0xb6, 0x38, 0x53, 0x60, 0x3d, 0xbc, 0x58, 0xa4, 0x9c,
    0x91, 0xcb, 0x68, 0x5e, 0xdc, 0xe1, 0x94, 0x1c, 0x38, 0x73, 0xb0, 0x45, 0xdb, 0x41, 0xe4, 0x1f, 0xa3, 0x6a, 0xb,
    0x58, 0x7,  0x2,  0x4b, 0xcf, 0xfb, 0x2f, 0x16, 0x7e, 0x9b, 0xdb, 0x7a, 0xa6, 0xe8, 0x15, 0xd5, 0x9b, 0x44, 0x32,
    0x13, 0x52, 0x5,  0x77, 0xb,  0xce, 0xf5, 0xa0, 0x15, 0x5f, 0xda, 0x25, 0xb4, 0xdb, 0xc9, 0xdb, 0x6d, 0xdc, 0xb0,
    0x2f, 0xf7, 0x3a, 0xae, 0xeb, 0xf6, 0xcc, 0x0,  0xd9, 0xce, 0x37, 0x1b, 0xb3, 0xf2, 0x93, 0xa5, 0x75, 0xef, 0xfd,
    0x72, 0xf8, 0xb,  0x24, 0xa7, 0xa5, 0x39, 0x21, 0x8d, 0xaa, 0x11, 0x5b, 0xe5, 0xd1, 0x77, 0xaf, 0x80, 0x4,  0x53,
    0x84, 0x6a, 0x52, 0x98, 0x1c, 0x19, 0x8d, 0x1f, 0xde, 0x7a, 0x27, 0xd7, 0xe8, 0x4f, 0x13, 0x17, 0xa0, 0x53, 0x45,
    0x24, 0x3,  0xce, 0xf0, 0xd2, 0x75, 0x50, 0xb9, 0x57, 0x24, 0x70, 0x83, 0xa8, 0x4e, 0x44, 0xd4, 0xa6, 0x90, 0xc2,
    0x9d, 0x1a, 0xb,  0x42, 0xe9, 0x42, 0x66, 0x28, 0x80, 0x5e, 0x30, 0x1f, 0x75, 0x3b, 0x5,  0xcb, 0x93, 0xe3, 0x2d,
    0xfc, 0xc4, 0x3c, 0x91, 0xd0, 0x17, 0xde, 0xf3, 0x9d, 0xb6, 0x6a, 0xcf, 0x3,  0x3a, 0x5b, 0x67, 0x78, 0x3d, 0xce,
    0x2,  0x4,  0x3,  0xa8, 0xd,  0x52, 0xd4, 0x6f, 0x2,  0xab, 0xd3, 0x1a, 0x1c, 0x4e, 0xde, 0x87, 0x28, 0xda, 0xfe,
    0x7c, 0xfe, 0xb2, 0x63, 0x9b, 0x24, 0x10, 0x4,  0x7,  0x1a, 0xaa, 0xa0, 0x2f, 0x40, 0x1e, 0x6f, 0x2e, 0xb9, 0x2b,
    0x69, 0x7,  0xf0, 0x62, 0x42, 0xfd, 0x75, 0xb5, 0xe8, 0xd2, 0x18, 0xe,  0x19, 0xeb, 0x2a, 0x86, 0xda, 0x55, 0x8c,
    0x48, 0xaf, 0xd6, 0x1d, 0x17, 0x71, 0x2b, 0x0,  0x58, 0x90, 0xd2, 0x81, 0x92, 0x38, 0x5a, 0xf9, 0x72, 0x58, 0xbb,
    0xea, 0xa6, 0x17, 0x3f, 0x14, 0x8b, 0x78, 0xe3, 0xec, 0x2c, 0x8e, 0x94, 0x3b, 0x3d, 0x8,  0x20, 0x51, 0x5c, 0x60,
    0xe5, 0x80, 0x5d, 0xdb, 0x7c, 0x8e, 0xbf, 0xe0, 0xfd, 0x73, 0xef, 0xf0, 0xd8, 0x50, 0x1c, 0xa6, 0xc6, 0x7e, 0x2,
    0x11, 0x44, 0xe8, 0x31, 0x54, 0x83, 0x2f, 0x18, 0xf5, 0x44, 0x36, 0x36, 0xe5, 0xf6, 0x2f, 0x4c, 0x10, 0xf8, 0xc4,
    0x4,  0x41, 0x1f, 0xa5, 0x99, 0x88, 0xac, 0x54, 0x74, 0x13, 0xba, 0x3,  0x21, 0xde, 0xdb, 0x56, 0x5e, 0x14, 0x67,
    0xe2, 0xae, 0xcb, 0xb6, 0x93, 0x99, 0x95, 0xdf, 0xbc, 0x51, 0xd2, 0x7b, 0x74, 0xa,  0xbb, 0x87, 0x8,  0x4f, 0x82,
    0x1b, 0x24, 0xfd, 0xd1, 0xf9, 0xc1, 0x21, 0x8e, 0x23, 0xce, 0xa0, 0xcc, 0xa,  0x50, 0xa6, 0x29, 0x60, 0x53, 0x32,
    0x1f, 0xe3, 0x5e, 0x9c, 0x6b, 0x5f, 0x2e, 0xbf, 0x3d, 0xd6, 0xf2, 0x9f, 0xd7, 0xf6, 0x5,  0xcc, 0xc8, 0x80, 0xdb,
    0x7b, 0x61, 0x1c, 0x17, 0xbb, 0xf3, 0xcd, 0x13, 0x16, 0x31, 0x33, 0xcb, 0xa7, 0xbb, 0x3a, 0x5a, 0x52, 0x58, 0x3e,
    0xa7, 0x27, 0x81, 0xe7, 0xec, 0x19, 0x91, 0xa4, 0x61, 0xa2, 0x66, 0x7e, 0x2c, 0xde, 0xc4, 0x9c, 0x3d, 0xc9, 0xe9,
    0xb4, 0xb,  0x1d, 0x8a, 0xa,  0x89, 0x43, 0x80, 0x1d, 0xfd, 0xab, 0xbd, 0xd2, 0x58, 0xf6, 0x25, 0x26, 0xcc, 0xe7,
    0x88, 0xf,  0xd,  0x2,  0x16, 0x74, 0x3,  0xc7, 0x64, 0xed, 0x91, 0xa5, 0xd,  0x2d, 0x9a, 0xc,  0x74, 0x30, 0xb4,
    0x5c, 0xbd, 0x19, 0x2,  0xd3, 0x6c, 0xd,  0x62, 0x4d, 0xa0, 0xa9, 0xa5, 0x4,  0x2,  0xba, 0x16, 0xae, 0x2d, 0x19,
    0xe2, 0xa1, 0xad, 0x3e, 0x83, 0x3c, 0x3d, 0x8d, 0x72, 0x34, 0xca, 0x9a, 0x19, 0x5d, 0x2b, 0x77, 0x1c, 0x40, 0x42,
    0x64, 0xc5, 0xac, 0x59, 0x72, 0x8f, 0xac, 0x6b, 0xc9, 0x88, 0x1,  0x63, 0xb5, 0xfa, 0x4,  0x70, 0x3a, 0x2d, 0xcf,
    0x93, 0x50, 0xc1, 0x32, 0xf3, 0xf,  0xf8, 0x44, 0x9c, 0x84, 0x87, 0xe8, 0x62, 0xc7, 0x28, 0x2b, 0xdd, 0x7d, 0xfd,
    0xa2, 0xb4, 0xad, 0xbd, 0xbf, 0xfe, 0x5e, 0x98, 0x31, 0xfc, 0xdd, 0x6e, 0xfe, 0x6,  0x82, 0xc2, 0xe5, 0x50, 0xac,
    0x3d, 0xe4, 0x4c, 0xf4, 0x7a, 0xf1, 0x3a, 0x6e, 0xdd, 0x5f, 0x99, 0x5e, 0xd4, 0x12, 0xe7, 0xfb, 0x5d, 0xfa, 0x61,
    0xfa, 0xda, 0x89, 0x34, 0xa6, 0x22, 0x92, 0x1c, 0xd0, 0x2d, 0x19, 0x6f, 0xd6, 0x53, 0x4e, 0x71, 0xf4, 0x2d, 0x35,
    0xe6, 0x99, 0xde, 0x1d, 0xff, 0x7f, 0x4a, 0x2a, 0xab, 0xe,  0xe6, 0x2,  0xc0, 0x68, 0x49, 0x54, 0x2a, 0x50, 0xc6,
    0xef, 0x1f, 0x7,  0x12, 0xc1, 0x54, 0x36, 0x68, 0xa6, 0xc9, 0x3c, 0x8d, 0xfe, 0x74, 0x7,  0x16, 0x7b, 0x1f, 0xc4,
    0x8b, 0x30, 0x5,  0xd3, 0x7,  0x28, 0xf9, 0x4,  0x7c, 0x9c, 0x86, 0x24, 0xc8, 0x65, 0x5a, 0x65, 0xa3, 0xfa, 0xfa,
    0xe1, 0xac};

inline uint16_t const logger_max_message_size = 512;

inline uint8_t const log_verbose = 0;
