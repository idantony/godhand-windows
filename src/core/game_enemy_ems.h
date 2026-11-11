#ifndef GH_GAME_ENEMY_EMS_H
#define GH_GAME_ENEMY_EMS_H
#include "game_enemy_actor.h"
#include "game_enemy_move.h"
#include "game_enemy_other_move.h"
#include "game_enemy_2a2_final.h"
#include "game_enemy_resources.h"
#include "game_enemy_request.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct gh_enemy_ems gh_enemy_ems;

typedef struct {
    gh_enemy_actor_bindings actor;
    const gh_enemy2a2_node_names *names_2a2;
} gh_enemy_ems_bindings;

/* The prefix already owns the stop gate, DVD/SE preload, and spawn cursor.
 * Keep these bindings, the prefix and the RoomSave owner alive until release.
 * Run only on the original owning task. */
gh_scene_placement_status gh_enemy_ems_spawn_00294ca0(gh_enemy_ems **,
    gh_em_resource_prefix *, const gh_room_state *, const gh_enemy_ems_bindings *,
    uint32_t *frontier, int *detail);
typedef struct {
    const gh_enemy_ems *owner;
    const uint8_t *page;
    gh_enemy_request request;
    uint8_t stage,index,stamp;
} gh_enemy_ems_set_call;
/* Original 2952D0 dynamic spawn through the same canonical EMS owner.
 * One zero-initialized cursor per invocation, retained across suspension. */
gh_scene_placement_status gh_enemy_ems_set_002952d0(gh_enemy_ems *,gh_enemy_ems_set_call *,
    uint8_t stamp,gh_game_title_player **original,uint32_t *frontier);
/* Resolve a completed actor only while the same canonical registry still owns it. */
gh_game_title_player *gh_enemy_ems_lookup(const gh_enemy_ems *, uint32_t handle);
/* Reverse lookup of a canonical entered enemy. A missing actor yields zero. */
gh_scene_placement_status gh_enemy_ems_handle_for_actor(const gh_enemy_ems *,
    const gh_game_title_player *,uint32_t *handle);
typedef struct {
    gh_game_title_player *actor;
    gh_actor_model *model;
    const gh_player_shadow *shadow;
    gh_enemy_actor *enemy202;
    gh_enemy202_move *move202;
    unsigned physical_row;
    uint8_t held_weapon;
    gh_enemy2a2_virtual_init *enemy2a2;
    gh_enemy_weapon_init *weapon;
    gh_enemy_other_move *other_move;
    gh_id_base **marker_id;
} gh_enemy_ems_live;
/* Borrow registered runtime storage; use one Move cursor per physical row. */
gh_scene_placement_status gh_enemy_ems_live_at(gh_enemy_ems *,uint32_t handle,gh_enemy_ems_live *);
/* Same live +6EC relation, discoverable by either original registry handle. */
const gh_enemy_weapon_init *gh_enemy_ems_held_weapon(const gh_enemy_ems *,
    uint32_t enemy_handle, uint32_t weapon_handle);
/* Original GetEm traverses the canonical manager list and reads the actor
 * entry stamp. FF is always null. SetEscPos is the separate 2A2 XYZ writer. */
gh_actor_motion_status gh_enemy_manager_get_00290868(const gh_game_init_state *,
    uint8_t entry_number, gh_game_title_player **);
gh_actor_motion_status gh_enemy2a2_set_escape_position_00289038(gh_game_title_player *,
    const float position[4]);
/* R100 AT3's original cEmWrap calls, on the same registered live actor. */
gh_actor_motion_status gh_enemy_ems_start_goto_at3(const gh_enemy_ems *,uint8_t stamp,
    const float destination[3],uint32_t *frontier);
/* Original soft-event suspend/resume through the canonical enemy manager list. */
gh_actor_motion_status gh_enemy_ems_set_suspend_002948e8(const gh_enemy_ems *,
    const gh_game_init_state *, int suspend, uint32_t *frontier);
gh_actor_motion_status gh_enemy_ems_unsuspend_002948e8(const gh_enemy_ems *,
    const gh_game_init_state *, uint32_t *frontier);
/* Native cancellation after task shutdown, before manager/pools/shape cleanup.
 * This does not execute the original gameplay escape or destructor. */
gh_actor_motion_status gh_enemy_ems_release_native(gh_enemy_ems **, uint32_t *frontier);
#ifdef __cplusplus
}
#endif
#endif
