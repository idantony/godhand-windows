#ifndef GH_GAME_SCENARIO_SOFT_EVENT_H
#define GH_GAME_SCENARIO_SOFT_EVENT_H
#include "game_scene_setup.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef enum {GH_SOFT_EVENT_OK,GH_SOFT_EVENT_ARGUMENT,GH_SOFT_EVENT_UNSUPPORTED,GH_SOFT_EVENT_CALLEE} gh_soft_event_status;
typedef gh_soft_event_status (*gh_soft_player_abort)(void *,uint32_t *frontier);
typedef struct {gh_scene_scenario_view scenario;gh_boot_work *work;
 gh_soft_player_abort player_abort_00126770;void *context;} gh_soft_event_bindings;
typedef struct {uint32_t stage,frontier;uint8_t *scenario_bytes;gh_task_manager *manager;gh_boot_work *work;uint8_t kind;} gh_soft_event_start;
/* Originalleaf31AFA0->31B410, SAMEWork input bytes. No native padowner. */
gh_soft_event_status gh_scenario_stop_input_0031afa0(gh_boot_work *);
/* Recovered kind4 (original Event-playStartWait). A returned calleeerror keeps
 * continuationcursor so thedepth/inputprefix isnotreplayed. Otherkinds
 * have additional distinct player/effect/camera operations andremainpending. */
gh_soft_event_status gh_scenario_start_soft_event_002c39b0(gh_soft_event_start *,const gh_soft_event_bindings *,uint8_t kind);
#ifdef __cplusplus
}
#endif
#endif
