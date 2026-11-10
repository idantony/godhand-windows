#ifndef GH_GAME_SCENARIO_EXEC_H
#define GH_GAME_SCENARIO_EXEC_H
#include "game_scene_setup.h"
#include "../platform/task_threads.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef enum {GH_SCENARIO_EXEC_OK,GH_SCENARIO_EXEC_ARGUMENT,GH_SCENARIO_EXEC_MISSING,GH_SCENARIO_EXEC_THREAD} gh_scenario_exec_status;
/* Actual2C3890->2D5470->2C3968: originalrequestselects slot, nativeargument
 * belongs onlyto explicit Centryadapter (originalRELscripts takevoid). No task
 * runs duringregistration. SameScenario currentattribute is inherited by the
 * realbackend; then exactlyeightbytes at54+slot*8 are cleared. */
gh_scenario_exec_status gh_scenario_exec_002c3890(gh_scene_scenario_view,gh_task_threads *,int32_t original_request,
 gh_native_task_entry,void *context,int32_t native_argument,int32_t *original_slot,uint32_t *frontier);
#ifdef __cplusplus
}
#endif
#endif
