#ifndef GH_GAME_ENEMY_OTHER_MOVE_H
#define GH_GAME_ENEMY_OTHER_MOVE_H
#include "game_enemy_2a2_final.h"
#include "game_enemy_weapon.h"
#include "game_player_targets.h"
#include "game_object_opacity.h"
#include "game_actor_motion_call.h"
#include "game_collision_solid_step.h"
#include "game_terrain_sphere.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    const void *owner;
    const gh_title_player_view *player;
    const gh_player_target_bindings *target_bindings;
    const gh_object_opacity_environment *opacity;
    gh_motion_effect_consumer effect;
    void *effect_context;
    gh_motion_sound_consumer sound;
    void *sound_context;
    gh_actor_motion_call motion;
    gh_motion_solve solve;
    gh_actor_finish_model_call finish;
    gh_collision_solid_frame solid;
    gh_terrain_sphere_call sphere;
    gh_player_terrain_call wall, floor;
    gh_scene_ray_report report;
    float old[3];
    uint32_t frontier;
    unsigned texture_cursor;
    uint8_t stage, original_returned, nearby;
    uint16_t reaction_resource_offset;
    gh_motion_sound_request reaction_sound;
} gh_enemy_other_move;

/* Borrow the same room/player resolver and ELF as ordinary player target queries. */
gh_actor_motion_status gh_enemy_other_move_bind_targets(gh_enemy_other_move *,
    const gh_player_target_bindings *);
gh_actor_motion_status gh_enemy_other_move_bind_2a2_services(gh_enemy_other_move *,
    const gh_object_opacity_environment *,gh_motion_effect_consumer,void *,
    gh_motion_sound_consumer,void *);
/* Stable per-invocation cursors. Borrow existing EMS factory owners. */
gh_actor_motion_status gh_enemy2a2_move_002856c0(gh_enemy_other_move *,
    gh_enemy2a2_virtual_init *,const gh_title_player_view *);
gh_actor_motion_status gh_enemy363_move_001c9860(gh_enemy_other_move *,
    gh_enemy_weapon_init *,gh_motion_effect_consumer effect,void *effect_context);
#ifdef __cplusplus
}
#endif
#endif
