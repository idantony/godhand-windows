#ifndef GH_R100_COUNTER_TASK_H
#define GH_R100_COUNTER_TASK_H
#include "r100_scripts.h"
#ifdef __cplusplus
extern "C" {
#endif
/* MustimplementactualREL1028 positiveEM linkedlist traversal/checkDeadFlag.
 * Outputs count aliveEM200..29F (excludes2A0..2FF) andwhetheralive614==1.
 * NULL usesonlytheverifiedemptycanonicalhead branch, neverdefaultalive0. */
typedef gh_r100_script_status (*gh_r100_count_alive)(void *,uint32_t *alive,int *has614,uint32_t *frontier);
typedef struct {gh_r100_init *room_init;gh_room_state *room;gh_game_init_state *init;
 gh_scene_scenario_view scenario;gh_task_threads *threads;gh_r100_count_alive count_alive;void *context;} gh_r100_counter_bindings;
typedef struct {gh_r100_counter_bindings b;uint32_t stage,frontier;gh_r100_script_status status;
 int32_t slot;uint64_t sleep_calls;uint8_t bound,registered,entered,frontier_relative;} gh_r100_counter_task;
gh_r100_script_status gh_r100_counter_bind(gh_r100_counter_task *,const gh_r100_counter_bindings *);
/* ActualREL10F8, originalScenario slot/callback andrealSleep1. Borrows SAME
 *12-byte R100allocation; DOESNOT allocate anotherroomwork orsavebank.
 * PollstatusafterScenario visits. Cancelbeforefreeinganyborrowedowner. */
gh_r100_script_status gh_r100_counter_register_000010f8(gh_r100_counter_task *,int32_t original_request,int32_t *slot);
#ifdef __cplusplus
}
#endif
#endif
