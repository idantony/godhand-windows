/* Copyright (c) 2026 Lucas Picoli and god-hand-decomp contributors.
 * Derived from src/cod/0031cf78.c and src/cod/0031cf30.c at
 * LucasPicoli/god-hand-decomp eaae18d0dfab1f9a0df276ae5ca1581ad326faf3.
 * MIT: full notice in LICENSE.upstream. Native adaptation: explicit state
 * and defined unsigned modulo-2^32 arithmetic; algorithm unchanged.
 */
#include "rng.h"
#include <float.h>
#include <limits.h>
#include <string.h>

_Static_assert(CHAR_BIT == 8 && sizeof(uint32_t) == 4, "32-bit RNG word required");
_Static_assert(sizeof(float) == 4 && FLT_RADIX == 2 && FLT_MANT_DIG == 24 &&
               FLT_MAX_EXP == 128, "IEEE binary32 float required");

float gh_rng_0031cf78(gh_rng_state *state)
{
    /* Upstream uses signed int multiplication; wrapping is defined here. */
    const uint32_t v = (uint32_t)(state->word * UINT32_C(0x10DCD));
    uint32_t scan = v, top = 0, significand, encoded;
    float result;
    state->word = v;
    if (v == 0)
        return 0.0f;
    while (scan >>= 1)
        ++top;
    /* EE integer conversion chops low bits instead of nearest rounding.
     * Then the original two power-of-two scales are exact. Encode the
     * final positive normal binary32 directly, independent of host FEnv.
     * top - 31 + exponent_bias(127) = top + 96. */
    significand = top > 23 ? v >> (top - 23) : v << (23 - top);
    encoded = ((top + 96) << 23) | (significand & UINT32_C(0x7fffff));
    memcpy(&result, &encoded, sizeof(result));
    return result;
}

float gh_rng_0031cf30(gh_rng_state *state)
{
    float result = gh_rng_0031cf78(state);
    if (result <= 0.0f)
        result = result + 1.0f;
    return result;
}
