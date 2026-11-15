#ifndef GH_R102_ROOM_H
#define GH_R102_ROOM_H

#include "../core/game_om_factory.h"
#include "../core/game_room_module.h"
#include "r100_init.h"
#include "../core/scene_at_units.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    gh_scene_scenario_view scenario;
    gh_task_threads *threads;
    gh_room_state *room;
    gh_r100_om_scalar_view object_view;
    gh_r100_collision_enable set_collision;
    void *context;
} gh_r102_om046_bindings;

typedef struct {
    gh_scene_scenario_view scenario;
    gh_task_threads *threads;
    gh_room_state *room;
    gh_game_init_state *init;
    gh_scene_at_manager *areas;
    gh_native_task_entry saved_route;
    void *saved_context;
} gh_r102_area11_bindings;

typedef struct {
    const gh_room_module *module;
    gh_heap *heap;
    gh_om_factory *objects;
    gh_r207_allocation work;
    gh_om_runtime_create weapon, mount;
    gh_object_resource_load item3bc_load;
    gh_r102_om046_bindings task;
    gh_r102_area11_bindings area11;
    uint32_t task_frontier;
    int32_t task_slot, area11_slot;
    uint8_t prefix_complete, om046_stage, task_registered, task_finished, area11_registered;
    uint8_t followup_stage, om094_complete, mount_complete, item3bc_complete, tail_stage;
    uint8_t saved_intro_registered;
} gh_r102_room;

/* The first original R102 Init call sequence, through 0250. The remaining
 * Init instructions begin with SearchOM("om046") at 02B8. */
gh_scene_status gh_r102_room_init_prefix_000001a8(gh_r102_room *,const gh_room_module *,
    gh_heap *,gh_om_factory *,uint32_t *frontier);
/* Execute the original SearchOM and SetCollEnable prefix and stop at the
 * next unresolved REL call. The original missing-OM path begins at 0348. */
gh_scene_status gh_r102_room_prepare_om046_000002b8(gh_r102_room *,gh_r100_om_scalar_view,
    gh_r100_collision_enable,void *context,uint32_t *frontier);
/* Original 02F8 registers REL0898 with task request -1. The fiber first
 * sleeps, then checks the canonical RoomSave and live OM once per visit. */
gh_scene_status gh_r102_room_register_om046_00000898(gh_r102_room *,
    const gh_r102_om046_bindings *,uint32_t *frontier);
/* Original 0348 finds om095, hides layer 2 through 1B7280 if present,
 * and continues to the next original task registration at 0370. */
gh_scene_status gh_r102_room_hide_om095_00000348(gh_r102_room *,gh_r100_om_scalar_view,
    void *context,uint32_t *frontier);
/* Original 0370 registers the 2508 story task. The original first path
 * disables lock-on for stamp 7 and waits until Gene contacts AT 0B. */
gh_scene_status gh_r102_room_register_area11_00002508(gh_r102_room *,
    const gh_r102_area11_bindings *,uint32_t *frontier);
/* Create and initialize the original 0353 at REL0488. */
gh_scene_status gh_r102_room_create_mount_00000488(gh_r102_room *,uint32_t *frontier);

/* Main SNR symbol 434 (g_DataManager) and symbol 331 (RoomHeap)
 * feed the original referenced loadWait(kind3BC) at REL0520. */
gh_scene_status gh_r102_room_preload_00000520(gh_r102_room *,uint32_t *frontier);
/* Cancel owned scene tasks before releasing the room's state. */
gh_scene_status gh_r102_room_release_prefix(gh_r102_room *);

#ifdef __cplusplus
}
#endif
#endif
