/*
 * All necessary definitions used throughout the program
 */

#ifndef CCSDS_TM_PACKETS_CCSDS_DEFINITIONS_HPP
#define CCSDS_TM_PACKETS_CCSDS_DEFINITIONS_HPP

#define SPACECRAFT_IDENTIFIER 567U // A 10-bit unique identifier, assigned by the SANA
#define TRANSFER_FRAME_SIZE 128U
#define PRIMARY_HEADER_SIZE 6U

#define SECONDARY_HEADER_SIZE 0U // Size set to zero if the secondary header is not used
#define FRAME_DATA_FIELD_MAX_SIZE 64U

#endif // CCSDS_TM_PACKETS_CCSDS_DEFINITIONS_HPP
