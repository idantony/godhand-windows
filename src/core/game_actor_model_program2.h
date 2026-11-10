#ifndef GH_GAME_ACTOR_MODEL_PROGRAM2_H
#define GH_GAME_ACTOR_MODEL_PROGRAM2_H
#include "game_actor_model_nodes.h"
#include "../assets/skinned_mesh_transform2.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Borrow SAME14C7E8 palette and154240 constants. This merely exposes data;
 * caller must have completed producers and selected original program2.
 * No initialization/render/upload readiness is fabricated by this view. */
gh_actor_model_status gh_actor_model_program2_inputs(gh_actor_model *,unsigned node_index,gh_skinned_program2_inputs *,uint32_t *frontier);
#ifdef __cplusplus
}
#endif
#endif
