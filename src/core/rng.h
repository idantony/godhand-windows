/* Reconstructed God Hand RNG. See rng.c and PROVENANCE.md. */
#ifndef GODHAND_CORE_RNG_H
#define GODHAND_CORE_RNG_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* The word originally stored at PS2 EE virtual address 0x00747B14.
 * The caller owns the state; do not replace zero with an invented seed. */
typedef struct gh_rng_state {
    uint32_t word;
} gh_rng_state;

/* func_0031CF78: one modular multiply, then the original float expression.
 * Requires IEEE binary32. EE chop conversion is explicit and independent
 * of the host rounding mode. Result is in [0, 2), with a zero absorbing state.
 * State must be non-null. Separate states may be advanced independently. */
float gh_rng_0031cf78(gh_rng_state *state);

/* func_0031CF30: calls 0031CF78 and adds 1 when its result <= 0. */
float gh_rng_0031cf30(gh_rng_state *state);

#ifdef __cplusplus
}
#endif
#endif
