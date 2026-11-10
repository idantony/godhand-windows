#ifndef GH_GAME_ESP_CREATE_H
#define GH_GAME_ESP_CREATE_H
#include "game_effect_create.h"
#ifdef __cplusplus
extern "C" {
#endif
struct gh_effects_frame_globals;
typedef struct {gh_effect_release_bindings storage;gh_game_mt *random;const gh_boot_work *work;gh_static_transform_policy policy;struct gh_effects_frame_globals *globals;} gh_esp_create_bindings;
typedef struct {const void *identity;gh_esp_create_bindings b;unsigned emitter_slot,slot;uint32_t emitter_serial,serial,stage,frontier;uint8_t active,complete,allocated;} gh_esp_create_call;
/* One original3029C0 emission iteration:2FAA88 ->300E30 ->virtualInit.
 * Entry spelling keeps the original producer address. Only continuation state
 * is private; original bitmap/payload are SAME1024 pool. Ready is not inferred. */
gh_effect_status gh_esp_create_begin_00300e30(gh_esp_create_call *,unsigned emitter_slot,uint32_t emitter_serial);
gh_effect_status gh_game_esp_create_00300e30(gh_esp_create_call *,const gh_esp_create_bindings *);
#ifdef __cplusplus
}
#endif
#endif
