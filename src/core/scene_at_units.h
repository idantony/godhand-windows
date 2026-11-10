#ifndef GH_SCENE_AT_UNITS_H
#define GH_SCENE_AT_UNITS_H
#include "game_scene_at.h"
#include "../platform/memory.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct {uint8_t *bytes;size_t size;uint32_t handle;} gh_at_native_unit;
/* Copies original mutable9C units into owned native state, typed links replace
 * taggedguest pointers. Reference arithmetic is explicit, saves fullFEnv. */
gh_scene_at_status gh_scene_at_initialize_units_002c0f90(gh_scene_at_manager *,const gh_bundle *,gh_heap *,uint16_t current_room,int binary32_reference);
gh_scene_at_status gh_scene_at_get_jump_002c2ae0(gh_scene_at_manager *,uint32_t,gh_at_native_unit *);
gh_scene_at_status gh_scene_at_set_enabled(gh_scene_at_manager *,gh_at_native_unit,int enabled,int *original_return);
gh_scene_at_status gh_scene_at_set_jump_info_002c2750(gh_scene_at_manager *,gh_at_native_unit,uint8_t state,uint32_t function_id,uint32_t arg,uint8_t trigger);
/* Native ownership cleanup at proven room lifetime boundary. Includes isolated
 * allocation made by originalself-destination JMP path; no destructor ack. */
gh_scene_at_status gh_scene_at_units_release(gh_scene_at_manager *);
#ifdef __cplusplus
}
#endif
#endif
