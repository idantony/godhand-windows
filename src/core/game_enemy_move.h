#ifndef GH_GAME_ENEMY_MOVE_H
#define GH_GAME_ENEMY_MOVE_H
#include "game_enemy_damage.h"
#include "game_enemy_near.h"
#include "game_player_targets.h"
#include "game_effect_create.h"
#include "game_actor_motion_call.h"
#include "game_object_opacity.h"
#include "game_collision_solid_step.h"
#include "game_terrain_sphere.h"
#include "game_scene_step.h"
#include "game_room_state.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    const gh_scene_nearcheck *owner;
    float point[4],distance[8];
    uint32_t vertices[8];
    uint8_t initialized,visibility,index,complete;
} gh_enemy_route_query;

typedef gh_actor_motion_status (*gh_enemy202_death_release)(void *,const gh_enemy_actor *,uint32_t *);

typedef struct {
    gh_enemy_actor *enemy;
    const gh_title_player_view *player;
    const void *elf;
    size_t elf_bytes;
    unsigned stage, texture_node, hit_index;
    uint32_t frontier, dispatch_pc, pending_ai_pc;
    float path_target[3];
    const gh_scene_nearcheck *navigation;
    gh_enemy_route_query target_query, origin_query;
    gh_enemy202_hit_call damage;
    gh_motion_sound_request hit_sound;
    gh_effect_create_call impact_create;
    gh_effect_create_bindings impact_bindings;
    uint32_t target_vertex, origin_vertex;
    uint8_t visibility_done, path_target_ready, visibility_hit, route_stage;
    uint8_t original_returned, substate67_stage, post_body_index, post_node_index, substate67_phase;
    gh_actor_motion_call substate67_motion;
    const gh_object_opacity_environment *opacity;
    gh_motion_solve post_solve;
    gh_actor_finish_model_call post_finish;
    gh_collision_solid_frame post_solid;
    gh_player_terrain_call wall_call, floor_call;
    gh_terrain_sphere_call sphere_call;
    gh_scene_ray_report floor_report;
    float terrain_start[3], terrain_point[3], terrain_normal[3];
    uint8_t floor_hit, water_effect;
    const gh_title_player_view *player_aux_698;
    uint8_t player_aux_698_known, state1_phase, state71_phase, state71_exit;
    uint16_t reaction_resource_offset;
    gh_motion_sound_request reaction_sound;
    gh_enemy_ema2_damage_call near_damage;
    gh_enemy_ema2_hit_sound near_sound;
    void *near_sound_context;
    const gh_room_state *room_state;
    gh_player_target_bindings targets;
    uint8_t death_phase,death_effect_cursor,death_ground_cursor;
    float death_ground[2][4];
    gh_enemy202_death_release death_release;
    void *death_release_context;
    uint16_t death_item;
    uint8_t death_fade_phase;
    gh_effect_create_call death_effect_create;
} gh_enemy202_move;

/* Borrow original Main+18C bound PFD2 owner before entering Move. */
gh_actor_motion_status gh_enemy202_move_bind_navigation(gh_enemy202_move *,const gh_scene_nearcheck *);
gh_actor_motion_status gh_enemy202_move_bind_opacity(gh_enemy202_move *,const gh_object_opacity_environment *);
/* Borrow the canonical player auxiliary; NULL is an explicit known-empty binding. */
gh_actor_motion_status gh_enemy202_move_bind_player_aux_698(gh_enemy202_move *,const gh_title_player_view *);
/* Borrows the existing scene effect owner for the original impact call. */
gh_actor_motion_status gh_enemy202_move_bind_impact(gh_enemy202_move *,const gh_effect_create_bindings *);
/* Canonical room actor registry, borrowed by the periodic weapon search. */
gh_actor_motion_status gh_enemy202_move_bind_targets(gh_enemy202_move *,const gh_player_target_bindings *);
/* Same actor-attached sound owner for original 292D18 -> 288F00. */
gh_actor_motion_status gh_enemy202_move_bind_near_sound(gh_enemy202_move *,gh_enemy_ema2_hit_sound,void *);
/* Borrow the selected RoomSave for original 2736F8 -> 295498. */
gh_actor_motion_status gh_enemy202_move_bind_room_state(gh_enemy202_move *,const gh_room_state *);
gh_actor_motion_status gh_enemy202_move_bind_death_release(gh_enemy202_move *,
    gh_enemy202_death_release,void *);
/* One invocation of original 208BE0. Keep the cursor on a suspended result;
 * clear it only before a new Move after an original return. The borrowed
 * player is the canonical Work.playerObj, never a raw guest address. */
gh_actor_motion_status gh_enemy202_move_00208be0(gh_enemy202_move *,
    gh_enemy_actor *, const gh_title_player_view *player, const void *elf, size_t elf_bytes);
#ifdef __cplusplus
}
#endif
#endif
