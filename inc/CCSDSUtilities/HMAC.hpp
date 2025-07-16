/**
 * @file HMAC.hpp
 *
 * @brief Software implementation of HMAC using tinycrypt, a library offering cryptography primitives implementations,
 *        for devices with constrained resources
 */

#pragma once
#include <cstdint>
#include "tinycrypt/hmac.h"
#include "etl/span.h"

namespace CCSDSDataLinkLayer {
    /**
     * @brief Compute HMAC of payload and authentication key. Ensure that the length of the macOutput buffer
     *        is at least 32 bytes in length. Returns true if operation is successful.
     */
    inline bool computeHMAC(etl::span<const uint8_t> payload,
                     etl::span<const uint8_t> authenticationKey,
                     uint8_t* macOutputBuffer) {
        // According to tiny crypt documentation, successful operation returns 1
        tc_hmac_state_struct hmacStruct;
    	if (tc_hmac_set_key(&hmacStruct, authenticationKey.data(),
							  authenticationKey.size()) != 1) {
		    return false;
	    }

        if (tc_hmac_init(&hmacStruct) != 1) {
		    return false;
	    }

    	if (tc_hmac_update(&hmacStruct, payload.data(),
							 payload.size()) != 1) {
		    return false;
	    }

        if (tc_hmac_final(macOutputBuffer, TC_SHA256_DIGEST_SIZE, &hmacStruct) != 1) {
		    return false;
	    }
    	return true;
    }
} // namespace CCSDSDataLinkLayer
