#ifndef GH_GAME_ESP_UPDATE_H
#define GH_GAME_ESP_UPDATE_H
#include "game_esp_create.h"
#include "game_camera.h"
#include "display_state.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct {gh_effect_release_bindings storage;const gh_boot_work *work;struct gh_effects_frame_globals *globals;const float *atan_coeff;gh_static_transform_policy policy;
 gh_game_pl_camera *camera;const gh_game_camera *active_camera;const gh_display_state *display;
} gh_esp_update_bindings;
typedef struct {const void *identity;gh_esp_update_bindings b;unsigned slot;uint32_t serial,stage,frontier;uint8_t complete,retired;gh_effect_node_detach_call detach;} gh_esp_update_call;
/* One actual type6/type1A update. Retain this cursor through failed callees. */
gh_effect_status gh_game_esp_update_002fbed0(gh_esp_update_call *,const gh_esp_update_bindings *,unsigned,uint32_t);
#ifdef __cplusplus
}
#endif
#endif
