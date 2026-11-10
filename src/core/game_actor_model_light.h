#ifndef GH_GAME_ACTOR_MODEL_LIGHT_H
#define GH_GAME_ACTOR_MODEL_LIGHT_H
#include "game_actor_model.h"
#include "game_world_light.h"
#include "game_player_child_tail.h"
#include "game_camera.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Borrow the canonical mutable 3BD880 XYZ vector, shared with IK/shadow.
 * No ELF reload or private copy is used to reset this runtime global. */
typedef struct {const gh_player_shared_vector *pivot_owner;const gh_game_pl_camera *camera;} gh_actor_light_bindings;
/* 14D828 per-model light selection. Supports absent/ineligible candidates and
 * ordinary kind0/1 distance attenuation and kind2 directional records.
 * Current-model part0 / camera-relative attachments use the borrowed camera.
 * Animated colors and unresolved attachment identities remain strict.
 * Native validation is atomic; successful writes target SAME model/WorldLight.
 * This prepares lighting constants, and submits no rendering commands. */
gh_actor_model_status gh_actor_model_light_0014d828(gh_actor_model *,gh_game_world_light *,
 const gh_actor_light_bindings *,gh_static_transform_policy,uint32_t *frontier);
#ifdef __cplusplus
}
#endif
#endif
