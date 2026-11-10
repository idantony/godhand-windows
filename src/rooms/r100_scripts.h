#ifndef GH_R100_SCRIPTS_H
#define GH_R100_SCRIPTS_H
#include "r100_init.h"
#include "../core/game_scenario_exec.h"
#include "../core/game_scenario_soft_event.h"
#include "../core/game_event_environment.h"
#include "../platform/disc.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef enum {GH_R100_SCRIPT_OK,GH_R100_SCRIPT_ARGUMENT,GH_R100_SCRIPT_BOUNDS,GH_R100_SCRIPT_UNSUPPORTED,GH_R100_SCRIPT_CALLEE,GH_R100_SCRIPT_THREAD,GH_R100_SCRIPT_DISC} gh_r100_script_status;
typedef gh_r100_script_status (*gh_r100_script_leaf)(void *,uint32_t *frontier);
/* Mustregisterreal recoveredRELbody, notanemptyunknownscript. */
typedef gh_r100_script_status (*gh_r100_script_register_other)(void *,uint32_t relative,int32_t request,uint32_t *frontier);
typedef gh_r100_script_status (*gh_r100_script_get_enemy)(void *,uint8_t entry,const void **identity,uint32_t *frontier);
typedef gh_r100_script_status (*gh_r100_script_enemy_escape)(void *,const void *,const float position[4],uint32_t *frontier);
/* operation0=setLock(arg0),1=releaseObjCollision,2=setOpen(arg0,arg1). */
typedef gh_r100_script_status (*gh_r100_script_door)(void *,const void *,unsigned operation,int arg0,int arg1,uint32_t *frontier);
typedef struct {gh_scene_scenario_view scenario;gh_task_threads *threads;gh_room_state *room;
 gh_boot_work *work;gh_disc *disc;gh_game_init_state *init;gh_room_pools *pools;
 gh_soft_event_bindings soft;gh_event_play_start_bindings play;
 gh_r100_script_leaf end_soft_002c3c10,checkpoint_002c05b0;
 gh_r100_script_register_other register_other;gh_r100_om_scalar_view object_view;
 gh_r100_script_get_enemy get_enemy;gh_r100_script_enemy_escape set_escape;
 gh_r100_script_door door;gh_r100_collision_enable collision;void *context;} gh_r100_script_bindings;
typedef struct {gh_r100_script_bindings b;gh_soft_event_start soft_start;uint32_t stage,setup_stage,frontier;
 gh_r100_script_status status;int detail;int32_t slot;uint64_t sleep_calls;
 const void *enemies[2];gh_r100_om_view gates[2];uint8_t gate_stage,bound,registered,entered,finished,frontier_relative;} gh_r100_scripts;
/* SameCoreSave owner input, actualsigned+10 andWork59C40000 override. */
gh_r100_script_status gh_r100_get_clear_num_001fa6c0(gh_core_save_view,const gh_boot_work *,int16_t *);
gh_r100_script_status gh_r100_scripts_bind(gh_r100_scripts *,const gh_r100_script_bindings *);
/* Registersnative9B0 only; originalslotselection/attrinherit/8byteclear come
 * fromSAMEgh_scenario_exec. Doesnotexecuteonregistration. Rootmustpoll
 * Ownerandallborrowedviews mustoutlive itslivefiber; originalroomreset must
 * cancelScenario tasks beforedestroying/resettingthisowner.
 * Rootmustinspectowner.status/frontier afteractualScenario visit: missinglaterprovider blocks
 * itsfiberwithrealSleep1, neverpretendsEventcomplete/normaltaskexit. */
gh_r100_script_status gh_r100_scripts_register_000009b0(gh_r100_scripts *,int32_t request,int32_t *slot);
/* ActualBC0 orderedtasks/enemypositions/doorflags; unknownREL bodies require
 * typedregister_other andkeepRELfrontier. MaycalledfromoriginalInitdirectpath. */
gh_r100_script_status gh_r100_script_setup_00000bc0(gh_r100_scripts *);
#ifdef __cplusplus
}
#endif
#endif
