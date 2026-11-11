#ifndef GH_GAME_OM300_RETIRE_H
#define GH_GAME_OM300_RETIRE_H
#include "game_om_initialize.h"
#include "game_collision_scroll.h"
#include "se_voice.h"
#include "game_actor_model_nodes.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
 gh_om_factory *factory;
 gh_game_scene_tables *tables;
 gh_collision_scroll_mutation *scroll;
 gh_game_collision_solids *solids;
 gh_object_slot_release release_sound_slot;
 void *sound_context;
} gh_om300_retire_bindings;
typedef struct {
 gh_om300_retire_bindings bindings;
 gh_om_object *object;
 uint32_t handle,frontier,hitarea_cursor;
 uint8_t stage;
} gh_om300_retirement;

gh_room_pool_status gh_om300_mark_00312708(gh_om_factory *,uint32_t handle,
 gh_se_voice_engine *,uint32_t *frontier);
/* Original mode3 destructor prefix for the first R100 door class. Completes
 * owned HitArea, dynamic box, named AKG, solid and resource operations.
 * Leaves 1496C0, physical allocation and registry retirement to the caller. */
gh_room_pool_status gh_om300_retire_prefix_001867c0(gh_om300_retirement *,
 const gh_om300_retire_bindings *,uint32_t handle);

typedef struct {
 gh_om300_retirement prefix;
 gh_actor_model_retirement model;
 gh_om_factory *factory;
 gh_om_object *object;
 uint32_t handle,frontier;
 uint8_t stage;
} gh_om300_full_retirement;
/* Original 30F8B0 mode3: erase secondary first, run virtual destructor,
 * then erase primary. Prior effect/script borrowers must be quiescent. */
gh_room_pool_status gh_om300_retire_0030f8b0(gh_om300_full_retirement *,
 const gh_om300_retire_bindings *,uint32_t handle);
/* Verified shared R100 OM class dispatch. The class-specific prefixes retain
 * their original differences before calling the common OM/base destructor. */
uint32_t gh_om_retire_entry(uint32_t kind);
gh_room_pool_status gh_om_retire_mark_00312708(gh_om_factory *,uint32_t,gh_se_voice_engine *,uint32_t *);
/* Ordinary 30F650 already removed 754C58 and retains ownership of the
 * primary registry compaction after original 198CE0 mode 3 returns. */
gh_room_pool_status gh_om324_destroy_detached_00198ce0(gh_om300_full_retirement *,
 const gh_om300_retire_bindings *,uint32_t handle,const gh_title_player_view *);
gh_room_pool_status gh_om_retire_0030f8b0(gh_om300_full_retirement *,const gh_om300_retire_bindings *,uint32_t);
#ifdef __cplusplus
}
#endif
#endif
