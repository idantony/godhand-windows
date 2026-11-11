#ifndef GH_FRAME_STATE_H
#define GH_FRAME_STATE_H
#include "game_init.h"
#include "rng.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef enum {GH_FRAME_OK,GH_FRAME_ARGUMENT,GH_FRAME_RANDOM} gh_frame_error;
typedef enum {GH_FRAME_CHOP_REFERENCE=1} gh_frame_numeric_policy;
/* Original independent globals, not a second MT generator. The MT owner is
 * borrowed from GameInit. Policy is host metadata, not part of a PS2 ABI. */
typedef struct {uint32_t auxiliary_word;gh_rng_state lcg;gh_frame_numeric_policy policy;} gh_frame_random;
/* Actual31CEB8 writes two scalar seeds and seeds the shared MT. Calling this
 * does not establish that earlier boot or all pre-task constructors ran. */
/* Actual Rnd uses the auxiliary 747B10 word and returns its low byte. */
gh_frame_error gh_frame_random_byte_0031cef8(gh_frame_random *,uint8_t *);
gh_frame_error gh_frame_seed_0031ceb8(gh_frame_random *,gh_game_mt *,gh_frame_numeric_policy);
/* Actual finite32-bit domain, explicit pinned EE-chop reference. Pure integer
 * bit encoding, independent of host FEnv. Not a hardware FPU capture. */
uint32_t gh_frame_unit_bits_0030f518_word(uint32_t mt_word);
uint32_t gh_frame_jitter_bits_0031d020_word(uint32_t mt_word);
/* Recovered outer-loop leaves. The caller preserves their original relative
 * order: rate policy, Movie update, counter advance, later MT advance, tasks.
 * They do not replace the other required frame/render/platform operations. */
gh_frame_error gh_frame_rate_policy(gh_boot_work *);
gh_frame_error gh_frame_counter_advance(gh_boot_work *);
gh_frame_error gh_frame_random_before_tasks_0030b008(gh_frame_random *,gh_game_mt *,gh_boot_work *);
#ifdef __cplusplus
}
#endif
#endif
