#ifndef GH_R100_COUNTER_TASK_H
#define GH_R100_COUNTER_TASK_H
#include "r100_scripts.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Optional alternative provider for the same REL1028 contract. NULL uses
 * the canonical native manager and RoomSave bindings below. */
typedef gh_r100_script_status (*gh_r100_count_alive)(void *,uint32_t *alive,int *has614,uint32_t *frontier);
typedef struct {gh_r100_init *room_init;gh_room_state *room;gh_game_init_state *init;
 gh_scene_scenario_view scenario;gh_task_threads *threads;gh_r100_count_alive count_alive;void *context;} gh_r100_counter_bindings;
typedef struct {gh_r100_counter_bindings b;uint32_t stage,frontier;gh_r100_script_status status;
 int32_t slot;uint64_t sleep_calls;uint8_t bound,registered,entered,frontier_relative;} gh_r100_counter_task;
/* Original REL1028 and 28FB38. Counts live kind 200..29F from the
 * original manager list and reports whether one has byte 614 equal to 1. */
gh_r100_script_status gh_r100_count_alive_00001028(const gh_game_init_state *,
    const gh_room_state *,uint32_t *alive,int *has614);
gh_r100_script_status gh_r100_enemy_dead_0028fb38(const gh_room_state *,
    const gh_game_title_player *,int *dead);
gh_r100_script_status gh_r100_first_flagged_00002cb0(const gh_game_init_state *,
    const gh_room_state *,const gh_game_title_player **);
gh_r100_script_status gh_r100_counter_bind(gh_r100_counter_task *,const gh_r100_counter_bindings *);
/* ActualREL10F8, originalScenario slot/callback andrealSleep1. Borrows SAME
 *12-byte R100allocation; DOESNOT allocate anotherroomwork orsavebank.
 * PollstatusafterScenario visits. Cancelbeforefreeinganyborrowedowner. */
gh_r100_script_status gh_r100_counter_register_000010f8(gh_r100_counter_task *,int32_t original_request,int32_t *slot);
#ifdef __cplusplus
}
#endif
#endif
