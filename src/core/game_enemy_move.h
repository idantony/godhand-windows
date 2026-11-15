#ifndef GH_GAME_ENEMY_MOVE_H
#define GH_GAME_ENEMY_MOVE_H
#include "game_enemy_damage.h"
#include "game_action_button_set.h"
#include "game_enemy_near.h"
#include "game_player_targets.h"
#include "game_effect_create.h"
#include "game_actor_motion_call.h"
#include "game_object_opacity.h"
#include "game_collision_solid_step.h"
#include "game_terrain_sphere.h"
#include "game_enemy_route_graph.h"
#include "game_room_state.h"
#include "game_om_factory.h"
#include "game_om_frame.h"
#include "game_cloth_finish.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef gh_actor_motion_status (*gh_enemy202_player_grab)(void *,const gh_enemy_actor *,unsigned,uint32_t *);

typedef gh_actor_motion_status (*gh_enemy202_devil_spawn)(void *,const gh_enemy_actor *,
    const gh_enemy_request *,uint32_t *);
typedef gh_actor_motion_status (*gh_enemy202_death_release)(void *,const gh_enemy_actor *,uint32_t *);
typedef gh_actor_motion_status (*gh_enemy202_item_drop)(void *,gh_om_box_item_drop *,const gh_enemy_actor *,uint16_t,uint32_t *);

typedef struct {
    gh_enemy_entry *entry;
    const gh_enemy_actor_bindings *bindings;
    gh_enemy_node_cache *nodes;
    gh_enemy_hitareas *hitareas;
    gh_actor_motion *motion;
    gh_enemy_timers *timers;
} gh_enemy_move_actor;
typedef gh_actor_motion_status (*gh_enemy_devil_selector)(void *,uint32_t *);
gh_actor_motion_status gh_enemy_devil_choose_far_002705d8(const gh_title_player_view *,
    const gh_boot_work *,uint32_t *frontier);

typedef struct {
    gh_enemy_actor *enemy;
    gh_enemy_move_actor runtime;
    gh_enemy_devil_selector devil_selector;
    void *devil_selector_context;
    gh_om_factory *projectile_factory;
    gh_om_runtime_create projectile_create[2];
    const gh_title_player_view *player;
    const void *elf;
    size_t elf_bytes;
    unsigned stage, texture_node, hit_index;
    uint32_t frontier, dispatch_pc, pending_ai_pc;
    float path_target[3];
    const gh_scene_nearcheck *navigation;
    gh_enemy_route_call route;
    gh_enemy202_hit_call damage;
    gh_motion_sound_request hit_sound;
    gh_effect_create_call impact_create;
    gh_effect_create_bindings impact_bindings;
    uint32_t pickup_weapon_handle;
    uint8_t visibility_done, path_target_ready, visibility_hit;
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
    gh_enemy_request death_transform_request;
    gh_enemy202_player_grab player_grab;
    void *player_grab_context;
    gh_game_action_buttons *grab_buttons;
    gh_enemy202_devil_spawn devil_spawn;
    void *devil_spawn_context;
    uint8_t death_phase,death_effect_cursor,death_ground_cursor;
    float death_ground[2][4];
    gh_enemy202_death_release death_release;
    void *death_release_context;
    uint16_t death_item;
    uint8_t death_fade_phase;
    gh_om_box_item_drop death_drop;
    gh_enemy202_item_drop death_item_spawn;
    void *death_item_context;
    gh_effect_create_call death_effect_create, ambient_light_effect, stun_effect_create;
    gh_scene_at_manager *scene_jump_at;
    gh_bundle scene_jump_room;
    gh_cloth_step_bindings spring_bindings;
    gh_player_shared_vector *spring_pivot;
    gh_cloth_frame spring_frame;
} gh_enemy202_move;

/* Borrow original Main+18C bound PFD2 owner before entering Move. */
gh_actor_motion_status gh_enemy204_move_bind_springs(gh_enemy202_move *,const gh_cloth_step_bindings *,
    gh_player_shared_vector *);
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
/* Canonical room AT and bundle for original 24BDA0 same-room jumps. */
gh_actor_motion_status gh_enemy202_move_bind_scene_jump(gh_enemy202_move *,gh_scene_at_manager *,const gh_bundle *);
gh_actor_motion_status gh_enemy202_move_bind_devil_spawn(gh_enemy202_move *,
    gh_enemy202_devil_spawn,void *);
gh_actor_motion_status gh_enemy202_move_bind_death_release(gh_enemy202_move *,
    gh_enemy202_death_release,void *);
gh_actor_motion_status gh_enemy202_move_bind_player_grab(gh_enemy202_move *,gh_enemy202_player_grab,void *,gh_game_action_buttons *);
/* One invocation of original 208BE0. Keep the cursor on a suspended result;
 * clear it only before a new Move after an original return. The borrowed
 * player is the canonical Work.playerObj, never a raw guest address. */
gh_actor_motion_status gh_enemy202_move_00208be0(gh_enemy202_move *,
    gh_enemy_actor *, const gh_title_player_view *player, const void *elf, size_t elf_bytes);
gh_actor_motion_status gh_enemy_devil_move_00208be0(gh_enemy202_move *,
    const gh_enemy_move_actor *,const gh_title_player_view *,const void *,size_t);
#ifdef __cplusplus
}
#endif
#endif
