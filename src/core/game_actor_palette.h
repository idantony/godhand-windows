#ifndef GH_GAME_ACTOR_PALETTE_H
#define GH_GAME_ACTOR_PALETTE_H
#include "game_actor_model_nodes.h"
#ifdef __cplusplus
extern "C" {
#endif

/* Actual 1503B8, called by the final part loop in 14C7E8. The second
 * matrix is identity with translation XYZ from the SAME part's C0;
 * C0.W is not copied. Output is part.world80 * translation(C0.xyz).
 * This is the palette producer only, not camera/light/clip/draw execution. */
gh_actor_model_status gh_actor_part_palette_001503b8(const gh_actor_part *,
 gh_static_transform_policy,float out[16],uint32_t *frontier);

/* Preserve original part-table order and borrow the canonical model's parts.
 * Output belongs to the frame caller, not a second skeleton or pose owner.
 * Failure leaves all caller output and count unchanged. */
gh_actor_model_status gh_actor_palette_from_parts(const gh_actor_model *,
 gh_static_transform_policy,float (*out)[16],size_t capacity,size_t *count,
 uint32_t *frontier);
#ifdef __cplusplus
}
#endif
#endif
