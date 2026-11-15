#ifndef GH_R105_ROOM_H
#define GH_R105_ROOM_H

#include "../core/game_room_module.h"
#include "../core/game_room_state.h"
#include "../core/scene_at_units.h"
#include "../core/game_init.h"
#include "../core/game_title_player.h"
#include "r003_flags.h"

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
    void *context;
} gh_r105_room_bindings;

typedef struct {
    gh_r105_room_bindings bindings;
    gh_r207_allocation work;
    int32_t watcher_slot[2];
    uint32_t watcher_frontier[2];
    uint32_t frontier;
    uint8_t stage, ready;
} gh_r105_room;

/* Original R105Init prefix. Once scheduled, the original AT0 and battle
 * watchers sleep on the same Scenario backend. Positive unfinished leaves
 * report their own address before any unrecovered behavior. */
gh_scene_status gh_r105_initialize_000001a8(gh_r105_room *,const gh_r105_room_bindings *);
gh_scene_status gh_r105_move_00000398(gh_r105_room *,const gh_om_factory *,gh_game_mt *,
    gh_frame_random *,gh_r100_om_scalar_view,void *other_context,uint32_t *frontier);
gh_scene_status gh_r105_room_release(gh_r105_room *);

#ifdef __cplusplus
}
#endif
#endif
