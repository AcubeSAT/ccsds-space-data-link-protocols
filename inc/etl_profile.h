/**
 * Configuration for the ETL standard library replacement
 */

#pragma once

#define ETL_THROW_EXCEPTIONS
#define ETL_VERBOSE_ERRORS
#define ETL_CHECK_PUSH_POP

// Only GCC is used as a compiler
#include "etl/profiles/gcc_linux_x86.h"
