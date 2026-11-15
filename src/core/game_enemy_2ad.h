#ifndef GH_GAME_ENEMY_2AD_H
#define GH_GAME_ENEMY_2AD_H
#include "game_enemy_factory.h"
#include "game_effects.h"
#include "game_enemy_model.h"
#include "game_enemy_base_init.h"
#include "game_actor_motion_solve.h"
#include "game_actor_finish_model.h"
#include "game_actor_motion_call.h"
#include "game_effect_create.h"
#include "game_player_shadow.h"
#include "game_object_opacity.h"
#include "game_room_state.h"
#include "game_action_button_set.h"
#include "game_scenario_exec.h"
#include "game_scenario_soft_event.h"
#include "se_voice.h"
#ifdef __cplusplus
extern "C" {
#endif
#define GH_EM2AD_BYTES 0x1560u
gh_title_player_status gh_enemy2ad_construct_0028e018(void *,size_t,gh_title_player_view *);
gh_title_player_status gh_enemy2ad_allocate_00313970(gh_game_title_player *,gh_heap *room_heap,const gh_boot_work *,int *original_nonnull);
/* Original 28E0E0 imports D7 through the resource-root relative pointer at
 * +10 before inspecting the room save bit. Borrow the bounded original data. */
gh_effect_status gh_enemy2ad_effect_data(const gh_object_view *,gh_effect_view *);
gh_actor_model_status gh_enemy2ad_model_0028ea38(gh_enemy_model *,const gh_title_player_view *,
    const gh_object_view *,gh_boot_work *,gh_room_pools *,gh_heap *,const void *,
    gh_static_transform_policy,int *original_return);
typedef struct {
    gh_enemy_model model;
    gh_enemy_base_initialize base;
    const gh_title_player_view *actor;
    const gh_game_effects *effects_owner;
    const uint8_t *resource;
    uint32_t stage,frontier;
} gh_enemy2ad_init;
/* Original room and stamp profiles for native 02AD entry. */
int gh_enemy2ad_is_room_supported_profile(const gh_enemy_request *,uint16_t room);

/* Staged 28E0E0 through D7 registration, room save gate and common
 * model/base initialization. Actual motion/effect tail retains its PC. */
gh_actor_model_status gh_enemy2ad_init_prefix_0028e0e0(gh_enemy2ad_init *,
    const gh_title_player_view *,const gh_enemy_request *,const gh_object_view *,
    gh_boot_work *,gh_core_save_view,gh_game_effects *,gh_room_pools *,gh_heap *,
    const void *,gh_game_cockpit *,gh_game_mt *,gh_static_transform_policy,int *);
typedef struct gh_enemy2ad_init_tail {
    const gh_enemy2ad_init *prefix;
    gh_actor_motion motion;
    gh_motion_solve solve;
    gh_actor_finish_model_call finish;
    gh_player_shadow shadow;
    gh_effect_create_call effect;
    uint32_t stage,frontier;
} gh_enemy2ad_init_tail;
typedef struct {
    const float *rate;
    gh_motion_solve_bindings solve;
    gh_actor_motion_bindings motion;
    gh_effect_create_bindings effect;
} gh_enemy2ad_tail_bindings;
gh_actor_model_status gh_enemy2ad_init_tail_0028e320(gh_enemy2ad_init_tail *,gh_enemy2ad_init *,
    gh_title_player_view *,const gh_object_view *,const gh_enemy2ad_tail_bindings *,
    gh_static_transform_policy,int *);
typedef struct {
    gh_actor_motion_call begin;
    gh_motion_solve solve;
    gh_actor_finish_model_call finish;
    gh_motion_sound_request sound;
    uint8_t stage;
} gh_enemy2ad_move;
typedef struct {
    const gh_room_state *room;
    const uint8_t *scenario14;
    const void *elf;
    size_t elf_bytes;
    const gh_object_opacity_environment *opacity;
    gh_motion_solve_bindings solve;
    gh_motion_effect_consumer release_effect;
    void *effect_context;
    const float *rate;
    const gh_se_voice_engine *voices;
    gh_motion_sound_consumer play_sound;
    void *sound_context;
    gh_game_action_buttons *buttons;
} gh_enemy2ad_move_bindings;
/* A single original update. Retries resume after the last completed callee. */
gh_actor_motion_status gh_enemy2ad_move_0028e420(gh_enemy2ad_move *,gh_enemy2ad_init_tail *,
    gh_game_title_player *,const gh_enemy2ad_move_bindings *,uint32_t *frontier);
typedef struct {
    gh_soft_event_start soft;
    const gh_game_title_player *actor;
    int32_t slot;
    uint8_t stage;
} gh_enemy2ad_action;
/* Scenario owns the scheduled fiber and borrows the original actor context.
 * A busy fixed slot means the original call starts no task. */
gh_soft_event_status gh_enemy2ad_action_0028e7c8(gh_enemy2ad_action *,
    const gh_game_title_player *,const gh_soft_event_bindings *,gh_task_threads *,
    gh_native_task_entry,void *,uint32_t *frontier);
/* Original derived-destructor motion release only. D7 bank stop follows. */
gh_actor_motion_status gh_enemy2ad_dtor_motion_0028e068(gh_actor_motion_call *,
    gh_enemy2ad_init_tail *,gh_motion_effect_consumer,void *,uint32_t *frontier);
#ifdef __cplusplus
}
#endif
#endif
