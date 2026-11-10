#ifndef GH_R100_ROOF_TASK_H
#define GH_R100_ROOF_TASK_H
#include "r100_scripts.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct {gh_r100_init *room_init;gh_room_state *room;gh_room_pools *pools;
 gh_game_scene_tables *actors;gh_scene_scenario_view scenario;gh_task_threads *threads;
 gh_r100_om_scalar_view object_view;void *context;} gh_r100_roof_bindings;
typedef struct {gh_r100_roof_bindings b;uint32_t frontier,scan_index;gh_r100_script_status status;int detail;
 int32_t slot;uint64_t sleep_calls;uint32_t changed[2];uint8_t bound,registered,entered,finished,enabled[2],pending_enable;} gh_r100_roof_task;
/* Actual2C5168→14FD98 freshcanonicalSearchOM→2C5110: missing=true;
 * otherwise SAME RoomSave256slots key/used==1, elseOM+5B0bit1.
 * Cached Init.roofs[] neverreplacefreshlookup orpointatfreedobjects. */
gh_r100_script_status gh_r100_roof_is_broken_002c5168(const gh_r100_roof_bindings *,const char *name,int *original_broken,uint32_t *frontier);
gh_r100_script_status gh_r100_roof_bind(gh_r100_roof_task *,const gh_r100_roof_bindings *);
/* REL2F30: checksrofF00/rofF01 everypass, firstbreak enablesactual
 *H03_Jfl2/H03_Jfl3 triangles through SAME Init.collision mutation, then
 *realSleep1 untilbothenabled; originalTask.exit onlyafterbothcallsfinish.
 * PollstatusafterScenario visits; cancelbeforeborrowedownersdestroyed. */
gh_r100_script_status gh_r100_roof_register_00002f30(gh_r100_roof_task *,int32_t original_request,int32_t *slot);
#ifdef __cplusplus
}
#endif
#endif
