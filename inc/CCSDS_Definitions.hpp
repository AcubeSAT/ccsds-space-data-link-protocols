/*
 * All necessary definitions used throughout the program
 */

#ifndef CCSDS_CCSDS_DEFINITIONS_HPP
#define CCSDS_CCSDS_DEFINITIONS_HPP

#define SPACECRAFT_IDENTIFIER 567U // A 10-bit unique identifier, assigned by the SANA

/* TM Packet Definitions */
#define TM_TRANSFER_FRAME_SIZE 128U
#define TM_PRIMARY_HEADER_SIZE 6U
#define TM_SECONDARY_HEADER_SIZE 0U // Size set to zero if the secondary header is not used

#define TC_SYNCH_BITS_SIZE 8U

#define TM_OPERATIONAL_CONTROL_FIELD_EXISTS 0
#define TM_ERROR_CONTROL_FIELD_EXISTS 0

#if TM_ERROR_CONTROL_FIELD_EXISTS && TM_OPERATIONAL_CONTROL_FIELD_EXISTS
#define TM_FRAME_DATA_FIELD_SIZE (TM_TRANSFER_FRAME_SIZE - TM_PRIMARY_HEADER_SIZE - TM_SECONDARY_HEADER_SIZE - 6) // Ignore-MISRA
#elif TM_OPERATIONAL_CONTROL_FIELD_EXISTS
#define TM_FRAME_DATA_FIELD_SIZE (TM_TRANSFER_FRAME_SIZE - TM_PRIMARY_HEADER_SIZE - TM_SECONDARY_HEADER_SIZE - 4) // Ignore-MISRA
#elif TM_ERROR_CONTROL_FIELD_EXISTS
#define TM_FRAME_DATA_FIELD_SIZE (TM_TRANSFER_FRAME_SIZE - TM_PRIMARY_HEADER_SIZE - TM_SECONDARY_HEADER_SIZE - 2) // Ignore-MISRA
#else
#define TM_FRAME_DATA_FIELD_SIZE (TM_TRANSFER_FRAME_SIZE - TM_PRIMARY_HEADER_SIZE - TM_SECONDARY_HEADER_SIZE) // Ignore-MISRA
#endif

#define TM_MAX_PACKET_SIZE 32768U // TODO: Check whether this is defined in ECSS of OBC

/* TC Packet Definitions */

#define TC_PRIMARY_HEADER_SIZE 5U
#define TC_ERROR_CONTROL_FIELD_EXISTS 0
#define TC_SEGMENT_HEADER_EXISTS 0

#define TC_MAX_TRANSFER_FRAME_SIZE 1024U

#if TC_ERROR_CONTROL_FIELD_EXISTS
#define TC_MAX_DATA_FIELD_SIZE 1017
#else
#define TC_MAX_DATA_FIELD_SIZE 1019
#endif

#define MCID SPACECRAFT_IDENTIFIER

// TODO No idea what this should be
#define RECEIVED_TC_BUFFER_MAX_SIZE 16384U

// @todo Make this specific to each MAP/virtual channel. Probably requires some clever memory management
#define MAX_RECEIVED_TC_IN_MAP_BUFFER 5U

// Maximum received TCs in wait queue (before COP checks).
#define MAX_RECEIVED_TC_IN_WAIT_QUEUE 10U
// Maximum received TCs in sent queue (following COP checks).
#define MAX_RECEIVED_TC_IN_SENT_QUEUE 10U
// Maximum received TC in the master buffer, before being passed to the all frames generation service
#define MAX_RECEIVED_TC_IN_MASTER_BUFFER 100U
// Maximum received TC in the master buffer, ready to be transmitted to the lower procedures (doesn't include
// repetitions)
#define MAX_RECEIVED_TC_OUT_IN_MASTER_BUFFER 100U

#define MAX_VIRTUAL_CHANNELS 10U
#define MAX_MAP_CHANNELS 3U


// Raw packets stored directly in the virtual channel buffer. Set to 0 if VC processing service isn't used
#define MAX_RECEIVED_UNPROCESSED_TC_IN_VIRT_BUFFER 5U

#define FIELD_STATUS 2U

#define FOP_SLIDING_WINDOW_INITIAL 255
#define FOP_TIMER_INITIAL 60 // sec

#endif // CCSDS_TCCSDS_DEFINITIONS_HPP
