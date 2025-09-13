/**
* @file HMAC.hpp
 *
 * @brief Software implementation of idle data generator. The galois implementation is used, as it is considered more
 *        computationally efficient.
 * @see Annex D of CCSDS TM Space Data Link Protocol
 */

#include <cstdint>

/**
 * Polynomial: x^32 + x^22 + x^2 + x + 1
 * Annex-D Galois seed: 0x003FFFFD
 * Galois mask = poly >> 1  = 0x80200003
 *
 * This function keeps a static 32-bit state (seeded once) and returns one
 * idle byte per call. Bits are packed MSB-first into each returned octet.
 */
__attribute__((weak)) inline uint8_t getNextOidByte(void)
{
    static uint32_t state = 0x003FFFFDu;   /* Annex-D Galois seed */
    const uint32_t MASK = 0x80200003u;     /* (1<<31)|(1<<21)|(1<<1)|(1<<0) */

    uint8_t out = 0;
    for (int i = 0; i < 8; ++i) {
        uint8_t bit = (uint8_t)(state & 1u);
        out = (uint8_t)((out << 1) | (bit & 1u));   /* MSB-first packing */

        uint32_t lsb = state & 1u;
        state >>= 1;
        if (lsb) state ^= MASK;
    }
    return out;
}