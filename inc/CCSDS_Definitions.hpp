/*
 * All necessary definitions used throughout the program
 */

#include <cstdint>

#ifndef CCSDS_CCSDS_DEFINITIONS_HPP
#define CCSDS_CCSDS_DEFINITIONS_HPP

// TODO? See if some of those constants don't need to be global and can be defined in the class instead
const uint16_t spacecraft_identifier = 567; // A 10-bit unique identifier, assigned by the SANA

/* TM Packet Definitions */
const uint16_t tm_transfer_frame_size = 128;
const uint8_t tm_primary_header_size = 6;
const uint8_t tm_secondary_header_size = 0; // Size set to zero if the secondary header is not used

const uint8_t tc_synch_bits_size = 8;

const bool tm_operational_control_field_exists = false;
const bool tm_error_control_field_exists = false;

#if tm_error_control_field_exists && tm_operational_control_field_exists
const uint16_t tm_frame_data_field_size =
    (tm_transfer_frame_size - tm_primary_header_size - tm_secondary_header_size - 6);
#elif tm_operational_control_field_exists
const uint16_t tm_frame_data_field_size =
    (tm_transfer_frame_size - tm_primary_header_size - tm_secondary_header_size - 4);
#elif tm_error_control_field_exists
const uint16_t tm_frame_data_field_size =
    (tm_transfer_frame_size - tm_primary_header_size - tm_secondary_header_size - 2);
#else
const uint16_t tm_frame_data_field_size = (tm_transfer_frame_size - tm_primary_header_size - tm_secondary_header_size);
#endif

#define TM_MAX_PACKET_SIZE 32768U // TODO: Check whether this is defined in ECSS of OBC

/* TC Packet Definitions */
const uint8_t tc_primary_header_size = 5;
const bool tc_error_control_field_exists = false;

const bool tc_segment_header_exists = 0;

const uint16_t tc_max_header_size = 1024;

#if tc_error_control_field_exists
const uint16_t tc_max_data_field_size 1017;
#else
const uint16_t tc_max_data_field_size = 1019;
#endif

const uint16_t mcid = spacecraft_identifier;

// @todo Make this specific to each MAP/virtual channel. Probably requires some clever memory management
const uint8_t max_received_tc_in_map_channel = 5;

// Maximum received TX TCs in wait queue (before COP checks).
const uint8_t max_received_tx_tc_in_wait_queue = 10;
// Maximum received RX TCs in wait queue (before COP checks).
const uint8_t max_received_rx_tc_in_wait_queue = 10;

// Maximum received TX TCs in sent queue (following COP checks).
const uint8_t max_received_tx_tc_in_sent_queue = 10;
// Maximum received RX TCs in sent queue (following COP checks).
const uint8_t max_received_rx_tc_in_sent_queue = 10;

// Maximum received TC in the master buffer, before being passed to the all frames generation service
const uint8_t max_received_tx_tc_in_master_buffer = 100;
// Maximum received TC in the master buffer, ready to be transmitted to the lower procedures (doesn't include
// repetitions)
const uint8_t max_received_tx_tc_out_in_master_buffer = 100;

// Maximum received TC in the master buffer, before being passed to the all frames reception service
const uint8_t max_received_rx_tc_in_master_buffer = 100;
// Maximum received TC in the master buffer, ready to be transmitted to the higher procedures
const uint8_t max_received_rx_tc_out_in_master_buffer = 100;

const uint8_t max_virtual_channels = 10;
const uint8_t max_map_channels = 3;

// Number of master copies of TX transfer frames that are stored in the master channel. This holds all the transfer
// frames that are stored in all services
const uint8_t max_tx_in_master_channel = 200;
// Number of master copies of RX transfer frames that are stored in the master channel. This holds all the transfer
// frames that are stored in all services
const uint8_t max_rx_in_master_channel = 200;

// Raw TX packets stored directly in the virtual channel buffer. Set to 0 if VC processing service isn't used
const uint8_t max_received_unprocessed_tx_tc_in_virt_buffer = 6;

const uint8_t fop_sliding_window_initial = 255;
const uint8_t fop_timer_initial = 60; // sec

enum FlagState {
    NOT_READY = 0, READY = 1
};


#endif // CCSDS_TCCSDS_DEFINITIONS_HPP
