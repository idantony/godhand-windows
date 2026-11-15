#ifndef GH_GAME_ENEMY_SOLIDS_H
#define GH_GAME_ENEMY_SOLIDS_H
#include "game_enemy_motion.h"
#include "game_effect_create.h"
#include "game_enemy_shadow.h"
#include "game_enemy_random.h"
#include "game_collision_solid_units.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct {gh_enemy_shadow *shadow;gh_game_collision_solids *manager;gh_heap *heap;
 gh_collision_solid_unit *unit;gh_collision_native_sphere *sphere;uint32_t stage,frontier;int random_result;} gh_enemy_solids;
/* Same462FC0:20A unit(type5,bound.2), sphere(radius.7,pos0/.5/0),
 *273D28 byte1868 +actualtwo-MTword2949E8 flagdecision. Next14B730 strict. */
gh_actor_motion_status gh_enemy_solids_initialize_00204b4c(gh_enemy_solids *,gh_enemy_shadow *,gh_game_collision_solids *,gh_heap *);
/* Scopedoriginalunitrelease before EntryEmnativeabort/model/heapclear. */
gh_actor_motion_status gh_enemy_solids_abort_prefix(gh_enemy_solids *);
typedef struct {
 gh_enemy_devil_initialize *init;
 gh_motion_solve_bindings bindings;
 gh_heap *heap;
 gh_enemy_pose pose;
 gh_enemy_shadow shadow;
 gh_enemy_solids solids;
 gh_effect_create_call spawn_effect;
 uint32_t stage,frontier;
} gh_enemy_devil_pose;
/* Devil's solved first pose, shadow, and native solid. Pauses before its
 * attached bank58/index3 spawn effect. Release solids before entry abort. */
gh_actor_motion_status gh_enemy_devil_pose_00152a20(gh_enemy_devil_pose *,
 gh_enemy_devil_initialize *,const gh_motion_solve_bindings *,gh_heap *);
/* Original 0220 initializer's attached bank58/index3. Completes only this
 * call and its scalar tail, then reports the next 14B730 initializer. */
gh_actor_motion_status gh_enemy_devil_spawn_effect_00306528(gh_enemy_devil_pose *,
 const gh_effect_create_bindings *);
#ifdef __cplusplus
}
#endif
#endif
