/*
 * All necessary definitions used throughout the program
 */

#include <cstdint>

#ifndef CCSDS_CCSDS_DEFINITIONS_HPP
#define CCSDS_CCSDS_DEFINITIONS_HPP
// TODO? See if some of those constants don't need to be global and can be defined in the class instead
const uint16_t SPACECRAFT_IDENTIFIER = 567; // A 10-bit unique identifier, assigned by the SANA

/* TM Packet Definitions */
const uint16_t TM_TRANSFER_FRAME_SIZE = 128;
const uint8_t TM_PRIMARY_HEADER_SIZE = 6;
const uint8_t TM_SECONDARY_HEADER_SIZE = 0; // Size set to zero if the secondary header is not used

[[maybe_unused]]
const uint8_t TC_SYNCH_BITS_SIZE = 8;

const bool TM_OPERATION_CONTROL_FIELD_EXISTS = false;
const bool TM_ERROR_CONTROL_FIELD_EXISTS = false;

#if TM_ERROR_CONTROL_FIELD_EXISTS && TM_OPERATION_CONTROL_FIELD_EXISTS
const uint16_t tm_frame_data_field_size =
    (tm_transfer_frame_size - tm_primary_header_size - tm_secondary_header_size - 6);
#elif TM_OPERATION_CONTROL_FIELD_EXISTS
const uint16_t tm_frame_data_field_size =
    (tm_transfer_frame_size - tm_primary_header_size - tm_secondary_header_size - 4);
#elif TM_ERROR_CONTROL_FIELD_EXISTS
const uint16_t tm_frame_data_field_size =
    (tm_transfer_frame_size - tm_primary_header_size - tm_secondary_header_size - 2);
#else
[[maybe_unused]]
const uint16_t TM_FRAME_DATA_FIELD_SIZE = (TM_TRANSFER_FRAME_SIZE - TM_PRIMARY_HEADER_SIZE - TM_SECONDARY_HEADER_SIZE);
#endif

/* TC Packet Definitions */
const uint8_t TC_PRIMARY_HEADER_SIZE = 5;
const bool TC_ERROR_CONTROL_FIELD_EXISTS = false;

#if TC_ERROR_CONTROL_FIELD_EXISTS
[[maybe_unused]]
const uint16_t tc_max_data_field_size = 1017;
#else
[[maybe_unused]]
const uint16_t TC_MAX_DATA_FIELD_SIZE = 1019;
#endif

const uint16_t MCID = SPACECRAFT_IDENTIFIER;

// @todo Make this specific to each MAP/virtual channel. Probably requires some clever memory management
const uint8_t MAX_RECEIVED_TC_IN_MAP_CHANNEL = 5;

// Maximum received TX TCs in wait queue (before COP checks).
const uint8_t MAX_RECEIVED_TX_TC_IN_WAIT_QUEUE = 10;
// Maximum received RX TCs in wait queue (before COP checks).
const uint8_t MAX_RECEIVED_RX_TC_IN_WAIT_QUEUE = 10;

// Maximum received TX TCs in sent queue (following COP checks).
const uint8_t MAX_RECEIVED_TX_TC_IN_SENT_QUEUE = 10;
// Maximum received RX TCs in sent queue (following COP checks).
const uint8_t MAX_RECEIVED_RX_TC_IN_SENT_QUEUE = 10;

// Maximum received TC in the master buffer, before being passed to the all frames generation service
const uint8_t MAX_RECEIVED_TX_TC_IN_MASTER_BUFFER = 100;
// Maximum received TC in the master buffer, ready to be transmitted to the lower procedures (doesn't include
// repetitions)
const uint8_t MAX_RECEIVED_TX_TC_OUT_IN_MASTER_BUFFER = 100;

// Maximum received TC in the master buffer, before being passed to the all frames reception service
const uint8_t MAX_RECEIVED_RX_TC_IN_MASTER_BUFFER = 100;
// Maximum received TC in the master buffer, ready to be transmitted to the higher procedures
const uint8_t MAX_RECEIVED_RX_TC_OUT_IN_MASTER_BUFFER = 100;

const uint8_t MAX_VIRTUAL_CHANNELS = 10;
const uint8_t MAX_MAP_CHANNELS = 3;

// Number of master copies of TX transfer frames that are stored in the master channel. This holds all the transfer
// frames that are stored in all services
const uint8_t MAX_TX_IN_MASTER_CHANNEL = 200;
// Number of master copies of RX transfer frames that are stored in the master channel. This holds all the transfer
// frames that are stored in all services
const uint8_t MAX_RX_IN_MASTER_CHANNEL = 200;

// Raw TX packets stored directly in the virtual channel buffer. Set to 0 if VC processing service isn't used
[[maybe_unused]]
const uint8_t MAX_RECEIVED_UNPROCESSED_TX_TC_IN_VIRT_BUFFER = 6;
[[maybe_unused]]
const uint8_t MAX_RECEIVED_UNPROCESSED_TX_TM_IN_VIRT_BUFFER = 6;

const uint8_t FOP_SLIDING_WINDOW_INITIAL = 255;
const uint8_t FOP_TIMER_INITIAL = 60; // sec

enum FlagState {
    NOT_READY = 0, READY = 1
};
#define LOGGER_MAX_MESSAGE_SIZE 512
#define LOG_VERBOSE 0
#endif // CCSDS_TCCSDS_DEFINITIONS_HPP
