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
/* Original 2C2728 writes only the type-1 unit's door callback and argument.
 * A native module-relative ID is kept in the canonical unit's sidecar. */
gh_scene_at_status gh_scene_at_set_door_func_002c2728(gh_scene_at_manager *,gh_at_native_unit,
 uint32_t relative_entry,uint32_t argument);
gh_scene_at_status gh_scene_at_door_func(gh_scene_at_manager *,gh_at_native_unit,
 uint32_t *relative_entry,uint32_t *argument);
/* Originalslot15 (owner+5C) and taggednext traversal, preserving sourceunit
 * handles/identity. Invalid links are errors, never an empty-list success. */
gh_scene_at_status gh_scene_at_first_unit_002c30c0(gh_scene_at_manager *,gh_at_native_unit *);
gh_scene_at_status gh_scene_at_next_unit_002c30c8(gh_scene_at_manager *,gh_at_native_unit,gh_at_native_unit *);
/* Original indexed search, through the canonical linked AT units. Missing
 * IDs return a null unit. The original selector FFFFF bypasses the search. */
gh_scene_at_status gh_scene_at_find_id_002c3118(gh_scene_at_manager *,uint32_t id,gh_at_native_unit *);
size_t gh_scene_at_native_unit_count(const gh_scene_at_manager *);
/* Native ownership cleanup at proven room lifetime boundary. Includes isolated
 * allocation made by originalself-destination JMP path; no destructor ack. */
gh_scene_at_status gh_scene_at_units_release(gh_scene_at_manager *);
/* Actual 2C1270: release known JMP allocations in eight-slot order. Retire
 * the native list companion at this room-exit lifetime boundary. */
gh_scene_at_status gh_scene_at_quit_002c1270(gh_scene_at_manager *,uint32_t *frontier);
#ifdef __cplusplus
}
#endif
#endif
