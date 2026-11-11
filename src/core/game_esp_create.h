#ifndef GH_GAME_ESP_CREATE_H
#define GH_GAME_ESP_CREATE_H
#include "game_effect_create.h"
#include "game_effect_node.h"
#include "game_camera.h"
#ifdef __cplusplus
extern "C" {
#endif
struct gh_effects_frame_globals;
struct gh_esp14_provider;
typedef struct {gh_effect_release_bindings storage;gh_game_mt *random;const gh_boot_work *work;gh_static_transform_policy policy;struct gh_effects_frame_globals *globals;gh_game_scene_tables *actors;const float *atan_coeff;
 const struct gh_esp14_provider *model_effect;gh_game_pl_camera *camera;const gh_game_camera *active_camera;
} gh_esp_create_bindings;
typedef struct {const void *identity;gh_esp_create_bindings b;unsigned emitter_slot,slot;uint32_t emitter_serial,serial,stage,frontier;uint8_t active,complete,allocated;
 float ground_position[4],ground_from[4],ground_to[4],ground_hit[4];int ground_ray_hit;gh_effect_node_detach_call detach;} gh_esp_create_call;
gh_effect_status gh_esp18_shake_range_002f3e68(gh_game_effects *,unsigned slot,uint32_t serial,
 const gh_game_pl_camera *,float radius,gh_static_transform_policy,int *near,uint32_t *frontier);
/* One original3029C0 emission iteration:2FAA88 ->300E30 ->virtualInit.
 * Entry spelling keeps the original producer address. Only continuation state
 * is private; original bitmap/payload are SAME1024 pool. Ready is not inferred. */
gh_effect_status gh_esp_create_begin_00300e30(gh_esp_create_call *,unsigned emitter_slot,uint32_t emitter_serial);
gh_effect_status gh_game_esp_create_00300e30(gh_esp_create_call *,const gh_esp_create_bindings *);
#ifdef __cplusplus
}
#endif
#endif
