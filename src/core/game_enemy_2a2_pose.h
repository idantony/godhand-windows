#ifndef GH_GAME_ENEMY_2A2_POSE_H
#define GH_GAME_ENEMY_2A2_POSE_H
#include "game_enemy_2a2_init.h"
#include "game_actor_motion_solve.h"
#include "game_actor_finish_model.h"
#include "game_player_shadow.h"
#include "game_hitarea_shapes.h"
#include "game_collision_solid_units.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    const float *rate;
    gh_game_scene_tables *actors;
    const float *atan_coeff;
    gh_game_init_state *init;
    gh_hitarea_shape_bindings shapes;
    gh_game_collision_solids *solids;
    gh_heap *solid_heap;
} gh_enemy2a2_pose_bindings;

typedef struct {
    gh_enemy2a2_init *prefix;
    gh_enemy2a2_pose_bindings bindings;
    gh_motion_solve solve;
    gh_actor_finish_model_call finish;
    gh_player_shadow shadow;
    gh_hitarea_object hitarea;
    gh_collision_solid_unit *unit;
    gh_collision_native_sphere *sphere;
    int hit_indices[4];
    uint32_t frontier;
    uint8_t stage, hit_cursor, hitarea_created;
} gh_enemy2a2_pose;

/* Continue 284AF8 after direct 157568. Success of the whole virtual call is
 * still pending at 31D000; use this owner's scoped release before model free. */
gh_actor_model_status gh_enemy2a2_pose_continue_002851b8(gh_enemy2a2_pose *,
    gh_enemy2a2_init *, const gh_enemy2a2_pose_bindings *);
gh_actor_model_status gh_enemy2a2_pose_release_prefix(gh_enemy2a2_pose *);
#ifdef __cplusplus
}
#endif
#endif
