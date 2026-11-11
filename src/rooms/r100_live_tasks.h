#ifndef GH_R100_LIVE_TASKS_H
#define GH_R100_LIVE_TASKS_H
#include "r100_counter_task.h"
#include "../core/scene_at_units.h"
#include "../core/game_enemy_ems.h"
#include "../core/game_math_globals.h"
#include "../core/game_scene_tables.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct {
    gh_scene_scenario_view scenario;
    gh_task_threads *threads;
    gh_r100_init *room_init;
    gh_room_state *room;
    gh_scene_at_manager *at;
    gh_game_init_state *init;
    const gh_boot_work *work;
    const gh_title_player_view *player;
    gh_enemy_ems **enemies;
    gh_r100_script_leaf checkpoint;
    void *context;
    const gh_game_math_globals *math;
    const void *elf;
    size_t elf_size;
    gh_game_scene_tables *scene_tables;
    gh_r100_script_status (*start_soft_event)(void *,gh_soft_event_start *,unsigned,uint32_t *);
} gh_r100_live_bindings;
typedef struct {
    gh_r100_live_bindings b;
    uint32_t entry,frontier;
    int32_t slot;
    gh_r100_script_status status;
    uint8_t registered,entered,finished,frontier_relative,at3_stage;
    gh_enemy_ems_set_call at3_set;
    gh_soft_event_start soft_start;
} gh_r100_live_task;
typedef struct {gh_r100_live_task tasks[8];} gh_r100_live_tasks;
/* Original BC0 registrations. The supported wait paths run real scenario
 * fibers and native gates. The first unrecovered positive branch is explicit. */
int gh_r100_live_entry(uint32_t entry);
/* Original EmWrap GetPos resolves the current stamp owner, using the shared
 * initialized 747B00 vector when the manager has no matching actor. */
gh_r100_script_status gh_r100_enemy_position_00295ab8(const gh_game_init_state *,
    uint8_t stamp,const gh_game_math_globals *,float out[4],uint32_t *frontier);
gh_r100_script_status gh_r100_live_register(gh_r100_live_tasks *,const gh_r100_live_bindings *,
    uint32_t entry,int32_t request,uint32_t *frontier);
#ifdef __cplusplus
}
#endif
#endif
