#ifndef GH_GAME_ROOM_DEATH_DISPATCH_H
#define GH_GAME_ROOM_DEATH_DISPATCH_H
#include "game_room_death.h"
#include "game_om300_retire.h"
#include "game_effect_create.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
 gh_scene_meshes *scroll_meshes;
 gh_player_death_bindings player;
 gh_game_player_child *child;
 gh_player_child_tail *child_tail;
 gh_om300_retire_bindings door;
 gh_game_init_state *hitarea;
 gh_collision_pairs *pairs;
 gh_scene_dynamic_owner *dynamic;
 const gh_game_effects *effects;
 gh_se_voice_engine *voices;
} gh_room_death_bindings;

typedef struct {
 const gh_room_death_bindings *bound;
 gh_room_death_pass scan;
 gh_player_retirement player;
 gh_child_retirement child;
 gh_om300_full_retirement door;
 uint32_t frontier,blocked_handle,retiring_kind,retiring_handle;
} gh_room_death_dispatch;

/* Executes exactly one 30F8B0 visit. Keep the same bindings alive through all
 * visits and through the deferred holder sound callback. Stops at an unknown
 * actor with its exact cursor. A successful visit resets only the scan. */
gh_room_pool_status gh_room_death_dispatch_0030f8b0(gh_room_death_dispatch *,
 const gh_room_death_bindings *);
#ifdef __cplusplus
}
#endif
#endif
