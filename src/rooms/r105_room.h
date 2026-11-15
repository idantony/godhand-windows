#ifndef GH_R105_ROOM_H
#define GH_R105_ROOM_H

#include "../core/game_room_module.h"
#include "../core/game_room_state.h"
#include "../core/scene_at_units.h"
#include "../core/game_init.h"
#include "../core/game_title_player.h"
#include "r003_flags.h"
#include "r100_scripts.h"
#include "../core/game_enemy_ems.h"
#include "../core/game_scenario_soft_event.h"
#include "../core/game_scenario_end_soft_event.h"
#include "../core/game_motion_camera.h"
#include "../core/game_scene_at_step.h"
#include "../core/game_action_button_step.h"
#include "../core/game_door.h"
#include "../core/game_event.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    const gh_room_module *module;
    gh_heap *heap;
    gh_boot_work *work;
    gh_room_state *room;
    gh_scene_scenario_view scenario;
    gh_task_threads *threads;
    gh_scene_at_manager *at;
    const gh_game_init_state *init;
    const gh_title_player_view *player;
    gh_core_save_view save;
    gh_scene_room_hook paper_0012c8d8;
    gh_scene_status (*omsub_000005f8)(void *,uint8_t *room_work,uint32_t *frontier);
    gh_scene_room_hook tutorial_000003b8;
    gh_scene_status (*goto_player_00295f80)(void *,uint8_t stamp,const float position[3],uint32_t *frontier);
    void *context;
    gh_enemy_ems **enemies;
    gh_game_mt *random;
    const gh_om_factory *gate_factory;
    gh_r100_om_scalar_view object_view;
    gh_r100_script_status (*program_se)(void *,int16_t,uint32_t *);
    gh_r100_script_status (*start_soft_event)(void *,gh_soft_event_start *,unsigned,uint32_t *);
    gh_r100_script_status (*set_room_camera)(void *,gh_room_camera_start *,const char *,uint32_t *);
    gh_r100_script_status (*camera_ended)(void *,int *,uint32_t *);
    gh_r100_script_status (*end_soft_event)(void *,gh_soft_event_end *,uint32_t *);
    gh_r100_init_status (*collision)(void *,const char *,int,uint32_t *);
    gh_game_action_buttons *buttons;
    gh_scene_room_hook jump_unsuspend_player;
    gh_scene_status (*place_mounted_player)(void *,const uint32_t position[3],uint32_t *);
    gh_scene_room_hook refresh_mounted_player;
    gh_screen_fade *fade;
    const gh_display_state *display;
    gh_event_reserve_work *reserve;
    gh_door_status (*request_room)(void *,uint16_t,const float[3],float,uint8_t,uint32_t,uint8_t,uint32_t *);
} gh_r105_room_bindings;

typedef struct {
    gh_r105_room_bindings bindings;
    gh_r207_allocation work;
    const gh_om_object *gate;
    int32_t watcher_slot[2];
    uint32_t watcher_frontier[2];
    uint32_t frontier;
    uint8_t stage, ready,gate_stage;
    gh_soft_event_start battle_soft_start;
    gh_soft_event_end battle_soft_end;
    gh_room_camera_start battle_camera;
    gh_enemy_ems_set_call battle_spawn[2];
    uint32_t gate_mount[8];
    gh_soft_event_start jump_soft_start;
    gh_room_camera_start jump_camera;
    gh_soft_event_end jump_soft_end;
    uint32_t jump_frontier;
    int32_t jump_slot,jump_gate_slot;
    uint8_t jump_pending,jump_gate_pending,gate_player_mounted;
} gh_r105_room;

/* Original R105Init prefix. Once scheduled, the original AT0 and battle
 * watchers sleep on the same Scenario backend. Positive unfinished leaves
 * report their own address before any unrecovered behavior. */
gh_scene_status gh_r105_initialize_000001a8(gh_r105_room *,const gh_r105_room_bindings *);
/* REL 05F8 uses the registered 0355 gate and fields of the existing
 * cOmSub work. Its native gate identity lives with that work. */
gh_scene_status gh_r105_gate_000005f8(gh_r105_room *,const gh_om_factory *,
    gh_r100_om_scalar_view,
    gh_r100_init_status (*collision)(void *,const char *,int,uint32_t *),
    void *,uint32_t *frontier);
gh_scene_status gh_r105_move_00000398(gh_r105_room *,const gh_om_factory *,gh_game_mt *,
    gh_frame_random *,gh_r100_om_scalar_view,void *other_context,uint32_t *frontier);
gh_scene_at_status gh_r105_jump_offer_001f7858(gh_r105_room *,
    const gh_scene_at_task_request *,uint32_t *frontier);
gh_action_step_status gh_r105_jump_action_002c2138(gh_r105_room *,
    void *unit_argument,uint32_t *frontier);
gh_scene_status gh_r105_room_release(gh_r105_room *);

#ifdef __cplusplus
}
#endif
#endif
