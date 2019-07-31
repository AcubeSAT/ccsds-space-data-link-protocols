/**
 * Configuration for the ETL standard library replacement
 */

#ifndef CCSDS_TM_PACKETS_ETL_PROFILE_H
#define CCSDS_TM_PACKETS_ETL_PROFILE_H

#define ETL_THROW_EXCEPTIONS
#define ETL_VERBOSE_ERRORS
#define ETL_CHECK_PUSH_POP

// Only GCC is used as a compiler
#include "etl/profiles/gcc_linux_x86.h"

#endif // CCSDS_TM_PACKETS_PROFILE_H
