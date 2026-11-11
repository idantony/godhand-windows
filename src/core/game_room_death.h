#ifndef GH_GAME_ROOM_DEATH_H
#define GH_GAME_ROOM_DEATH_H
#include "game_scene_meshes.h"
#include "game_player_factory.h"
#include "game_actor_model_nodes.h"
#include "se_voice.h"
#include "game_player_child_attach.h"
#include "game_player_child_tail.h"
#include "game_collision_solid_units.h"
#include "game_collision_pairs.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
 const gh_scene_meshes *owner;
 uint32_t cursor, blocked_handle, frontier;
 uint32_t retiring_handle;
 uint8_t retire_stage;
} gh_room_death_pass;

/* One ordered 30F8B0 marking pass for the original static scroll object prefix.
 * A blocked object stays at the current index for a later typed consumer.
 * A complete pass resets the cursor for the next original invocation. */
gh_room_pool_status gh_room_death_mark_scroll_0030f8b0(gh_room_death_pass *,gh_scene_meshes *);
/* Ordered type0 retirement after both original mark passes. Foreign actors
 * remain strict and keep their typed owner and registry membership. */
gh_room_pool_status gh_room_death_scroll_0030f8b0(gh_room_death_pass *,gh_scene_meshes *);
/* Original first nonstatic R100 actor 312708. The original player virtual
 * leaf is 14E928 (return); the real shared SE detach runs before flags change. */
gh_room_pool_status gh_room_death_mark_player_00312708(gh_game_player_factory *,
 gh_actor_model *,gh_se_voice_engine *,uint32_t *frontier);
typedef struct {
 gh_game_player_factory *factory;
 gh_actor_model *model;
 gh_game_collision_solids *solids;
 gh_player_child_attachment *attachment;
 const gh_object_resource_bindings *resources;
} gh_player_death_bindings;
typedef struct {
 gh_player_death_bindings bound;
 uint32_t frontier, handle;
 uint8_t stage;
} gh_player_death_prefix;
/* Original 107EC8 mode3 through both solid removals and pinned resource
 * release. Removes the host attachment borrow without destroying the child.
 * This partial routine reaches 1496C0 with the parent model still live. */
gh_room_pool_status gh_room_death_player_prefix_00107ec8(gh_player_death_prefix *,
 const gh_player_death_bindings *);
typedef struct {
 gh_player_death_prefix prefix;
 gh_actor_model_retirement model;
 gh_game_init_state *hitarea;
 gh_collision_pairs *pairs;
 uint32_t handle,frontier;
 uint8_t stage;
} gh_player_retirement;
/* 30F8B0 mode3 for the first nonstatic player entry. Native HitArea views
 * lose only retiring references; their original phase5 rows stay live. */
gh_room_pool_status gh_room_death_retire_player_0030f8b0(gh_player_retirement *,
 const gh_player_death_bindings *,gh_game_init_state *,gh_collision_pairs *);
typedef struct {
 gh_game_player_child *child;
 gh_player_child_tail *tail;
 gh_actor_model_retirement model;
 gh_game_init_state *hitarea;
 gh_collision_pairs *pairs;
 gh_room_pools *pools;
 uint32_t handle,frontier;
 uint8_t stage;
} gh_child_retirement;
gh_room_pool_status gh_room_death_mark_child_00312708(gh_game_player_child *,
 gh_se_voice_engine *,uint32_t *frontier);
/* 30F8B0 dispatch of original 100508 mode3. Call only once child attachment
 * and prior effect/physics consumers have ended, and PL01 leads the roster. */
gh_room_pool_status gh_room_death_retire_child_0030f8b0(gh_child_retirement *,
 gh_game_player_child *,gh_player_child_tail *,gh_game_init_state *,gh_collision_pairs *);
#ifdef __cplusplus
}
#endif
#endif
