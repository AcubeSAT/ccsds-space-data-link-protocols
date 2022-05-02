/*
 * All necessary definitions used throughout the program
 */

#include <cstdint>

#ifndef CCSDS_CCSDS_DEFINITIONS_HPP
#define CCSDS_CCSDS_DEFINITIONS_HPP

#define tm_error_control_field_exists 1

// TODO? See if some of those constants don't need to be global and can be defined in the class instead
const uint16_t SpacecraftIdentifier = 567; // A 10-bit unique identifier, assigned by the SANA

/* TM Packet Definitions */
const uint16_t TmTransferFrameSize = 128;
const uint8_t TmPrimaryHeaderSize = 6;
const uint8_t TmSecondaryHeaderSize = 0; // Size set to zero if the secondary header is not used

/* TC Packet Definitions */
const uint8_t TcPrimaryHeaderSize = 5;
const bool TcErrorControlFieldExists = false;

const uint16_t MCID = SpacecraftIdentifier;

// @todo Make this specific to each MAP/virtual channel. Probably requires some clever memory management
const uint8_t MaxReceivedTcInMapChannel = 5;
const uint8_t MaxReceivedTmInMapChannel = 5;

// Maximum received TX TCs in wait queue (before COP checks).
const uint8_t MaxReceivedTxTcInWaitQueue = 10;
// Maximum received RX TCs in wait queue (before COP checks).
const uint8_t MaxReceivedRxTcInWaitQueue = 10;

// Maximum received TX TCs in sent queue (following COP checks).
const uint8_t MaxReceivedTxTcInSentQueue = 10;
// Maximum received RX TCs in sent queue (following COP checks).
const uint8_t MaxReceivedRxTcInSentQueue = 10;

// Maximum received TC in the master buffer, before being passed to the all frames generation service
const uint8_t MaxReceivedTxTcInMasterBuffer = 100;
// Maximum received TC in the master buffer, ready to be transmitted to the lower procedures (doesn't include
// repetitions)
const uint8_t MaxReceivedTxTcOutInMasterBuffer = 100;
// Maximum received TM in the master buffer, before being passed to the all frames generation service
const uint8_t MaxReceivedTxTmInMasterBuffer = 100;

const uint8_t MaxReceivedTxTmOutInVCBuffer = 100;
// Maximum received TM in the master buffer, before being passed to the all frames generation service
const uint8_t MaxReceivedTxTmInVCBuffer = 100;

// Maximum received TM in the master buffer, ready to be transmitted to the lower procedures (doesn't include
// repetitions)

const uint8_t MaxReceivedTxTmOutInMasterBuffer = 100;

// Maximum received TC in the master buffer, before being passed to the all frames reception service
const uint8_t MaxReceivedRxTcInMasterBuffer = 100;
// Maximum received TC in the master buffer, ready to be transmitted to the higher procedures
const uint8_t MaxReceivedRxTcOutInMasterBuffer = 100;
// Maximum received TM in the master buffer, before being passed to the all frames reception service
const uint8_t MaxReceivedRxTmInMasterBuffer = 100;
// Maximum received TM in the master buffer, ready to be transmitted to the higher procedures
const uint8_t MaxReceivedRxTmOutInMasterBuffer = 100;

const uint8_t MaxVirtualChannels = 10;
const uint8_t MaxMapChannels = 3;

// Number of master copies of TX transfer frames that are stored in the master channel. This holds all the transfer
// frames that are stored in all services
const uint8_t MaxTxInMasterChannel = 200;
// Number of master copies of RX transfer frames that are stored in the master channel. This holds all the transfer
// frames that are stored in all services
const uint8_t MaxRxInMasterChannel = 200;

// Raw TX packets stored directly in the virtual channel buffer. Set to 0 if VC processing service isn't used
[[maybe_unused]] const uint8_t MaxReceivedUnprocessedTxTcInVirtBuffer = 6;
[[maybe_unused]] const uint8_t MaxReceivedUnprocessedTxTmInVirtBuffer = 6;

const uint8_t FopSlidingWindowInitial = 255;
const uint8_t FopTimerInitial = 60; // sec

enum FlagState { NOT_READY = 0, READY = 1 };
#define LOGGER_MAX_MESSAGE_SIZE 512
#define LOG_VERBOSE 0
#endif // CCSDS_TCCSDS_DEFINITIONS_HPP
