#ifndef GH_GAME_ENEMY_RETIRE_H
#define GH_GAME_ENEMY_RETIRE_H
#include "game_enemy_ems.h"
#include "se_voice.h"
#include "game_actor_motion_call.h"
#include "game_actor_model_nodes.h"
#include "game_scene_dynamic.h"
#include "game_effects.h"
#ifdef __cplusplus
extern "C" {
#endif

/* Move an original marked +6EC weapon to its same EMS row's retirement owner. */
gh_actor_motion_status gh_enemy_ems_mark_held_weapon_00312708(gh_enemy_ems *,
    uint32_t enemy_handle,gh_se_voice_engine *,uint32_t *frontier);

typedef struct {
    const gh_enemy_ems *owner;
    gh_se_voice_engine *voices;
    uint32_t handle;
    uint8_t stage, hitarea_cursor;
} gh_enemy_death_mark;

/* Original first 312708 call for canonical R100 kind 0202 or 02A2.
 * Retains the cursor on strict attachment/resource boundaries. */
gh_actor_motion_status gh_enemy_death_mark_00312708(gh_enemy_death_mark *,
    gh_enemy_ems *,uint32_t handle,gh_se_voice_engine *,uint32_t *frontier);
typedef struct {
    gh_actor_motion_call motion;
    gh_enemy_death_mark virtual_mark;
    const gh_enemy_ems *owner;
    gh_motion_effect_consumer effect;
    void *effect_context;
    uint32_t handle, entry;
    unsigned hitarea_cursor;
    uint8_t stage;
} gh_enemy_destructor;
/* Original 202058 / 284A90 mode3 through 28EB88, stopping at resource
 * manager selection 1FE9B8. Registry/lifetime retirement remains with caller. */
gh_actor_motion_status gh_enemy_destroy_prefix_mode3(gh_enemy_destructor *,gh_enemy_ems *,
    uint32_t handle,gh_motion_effect_consumer,void *effect_context,uint32_t *frontier);
typedef struct {
    gh_enemy_death_mark mark;
    const gh_enemy_ems *owner;
    uint32_t handle;
    uint8_t stage, original_return;
} gh_enemy_manager_remove;
/* Original storeVital, ReleaseObj mark and manager-slot unlink. */
gh_actor_motion_status gh_enemy_ems_remove_00290770(gh_enemy_ems *,gh_enemy_manager_remove *,
    uint32_t handle,gh_se_voice_engine *,int *original_return,uint32_t *frontier);
typedef struct {
    gh_game_effects *effects;
    gh_scene_dynamic_owner *dynamic;
    gh_motion_effect_consumer release_effect;
    void *effect_context;
    gh_object_slot_release release_sound_slot;
    void *sound_context;
} gh_enemy_retire_bindings;
typedef struct {
    gh_enemy_destructor destructor;
    gh_actor_model_retirement model;
    gh_enemy_retire_bindings bindings;
    const gh_enemy_ems *owner;
    uint32_t handle;
    uint8_t stage;
} gh_enemy_full_retirement;
/* The weapon stays in the original primary list after its parent mark. */
gh_actor_motion_status gh_enemy_ems_retire_weapon_0030f8b0(gh_enemy_ems *,
    gh_enemy_full_retirement *,uint32_t handle,const gh_enemy_retire_bindings *,uint32_t *frontier);
/* Standalone mode3 registry owner after 290770 and original flag aging.
 * Owns both vector compactions. A Trans callback with a detached secondary
 * vector must use a distinct caller adapter. */
gh_actor_motion_status gh_enemy_ems_retire_0030f8b0(gh_enemy_ems *,gh_enemy_full_retirement *,
    uint32_t handle,const gh_enemy_retire_bindings *,uint32_t *frontier);
/* Ordinary Trans already removed the secondary entry. Finish removes the
 * primary registration before freeing its borrowed EMS owner. */
gh_actor_motion_status gh_enemy_ems_destroy_detached_mode3(gh_enemy_ems *,
    gh_enemy_full_retirement *,uint32_t,const gh_title_player_view *,
    const gh_enemy_retire_bindings *,uint32_t *);
gh_actor_motion_status gh_enemy_ems_finish_detached_mode3(gh_enemy_ems *,
    gh_enemy_full_retirement *,uint32_t,const void *registry_owner,uint32_t *);
#ifdef __cplusplus
}
#endif
#endif
