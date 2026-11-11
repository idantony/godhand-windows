#ifndef GH_GAME_ENEMY_ACTOR_H
#define GH_GAME_ENEMY_ACTOR_H
#include "game_enemy_finalize.h"
#include "game_actor_motion_ik.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct {gh_enemy_entry_bindings entry;gh_game_cockpit *cockpit;const gh_motion_ik_globals *ik;
 gh_game_scene_tables *actors;const float *atan_coeff,*canonical_rate;gh_motion_solve_ray ray;void *ray_context;
 gh_motion_effect_consumer effect;void *effect_context;gh_game_collision_solids *solids;gh_heap *default_heap;
 const gh_enemy_node_names *node_names;gh_hitarea_shape_bindings shapes;gh_em_numeric_policy numeric_policy;gh_collision_scroll_mutation *weapon_scroll;} gh_enemy_actor_bindings;
/* ONEoriginalEntryEm invocation anditsownedactor metadata. NOT a second
 * enemylist: canonicalinit64slots/top/last andpools remain authoritative.
 * Keepthisownerandallborrowedbindings alive after originalactor return. */
typedef struct {gh_enemy_entry entry;gh_enemy_entry_tail tail;gh_enemy_hp hp;gh_enemy_motion motion;gh_enemy_pose pose;
 gh_enemy_shadow shadow;gh_enemy_solids solids;gh_enemy_node_cache nodes;gh_enemy_timers timers;gh_enemy_behavior behavior;
 gh_enemy_hitareas hitareas;gh_enemy_finalize final;gh_enemy_actor_bindings b;gh_enemy_request request;
 int32_t manager_kind;uint32_t argument,stage,frontier;uint8_t bound,null_request,original_null;} gh_enemy_actor;
/* Supportedoriginal202/20A weaponlessappearance0/3 initialization. Genuine
 * originalactor isreturned ONLYafter2022D0 originalbool1. Everyunknowncallee
 * retains frontier/prefix; noAI/frame/renderer readiness isacknowledged. */
gh_actor_motion_status gh_enemy_actor_enter_002905d0(gh_enemy_actor *,const gh_enemy_actor_bindings *,const gh_enemy_request *,int32_t manager_kind,uint32_t argument,gh_game_title_player **original_actor);
/* Nativecancel/shutdown only, after task/consumer cancellation, BEFORE
 * canonicalmanager reset/pools/solids/HitArea globalcleanup. Not290770 ack. */
gh_actor_motion_status gh_enemy_actor_shutdown_native(gh_enemy_actor *);
#ifdef __cplusplus
}
#endif
#endif
