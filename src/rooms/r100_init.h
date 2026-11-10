#ifndef GH_R100_INIT_H
#define GH_R100_INIT_H
#include "../core/game_room_state.h"
#include "../core/scene_at_units.h"
#include "../core/game_scene_meshes.h"
#include "../core/game_collision_scroll.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef enum {GH_R100_INIT_OK,GH_R100_INIT_ARGUMENT,GH_R100_INIT_MEMORY,GH_R100_INIT_BOUNDS,GH_R100_INIT_UNSUPPORTED,GH_R100_INIT_CALLEE} gh_r100_init_status;
typedef struct {uint8_t *bytes;size_t size;const void *identity;} gh_r100_om_view;
typedef gh_r100_init_status (*gh_r100_om_scalar_view)(void *,uint32_t handle,gh_r100_om_view *,uint32_t *frontier);
typedef gh_r100_init_status (*gh_r100_collision_enable)(void *,const char *name,int enabled,uint32_t *frontier);
/* Must really register originalScenario task atitsoriginalscheduler position;
 * outputfrontier onerror namesa mainELFcallee (modulebodyerrorsstaywithits
 * own taskowner). Never acknowledge an
 * unresolvedRELentry. No callback => preciseR100 relativefrontier. */
typedef gh_r100_init_status (*gh_r100_task_exec)(void *,uint32_t relative_entry,int32_t argument,uint32_t *frontier);
typedef struct {gh_boot_work *work;const gh_room_state *room_state;gh_scene_at_manager *at;
 gh_room_pools *pools;gh_scene_meshes *meshes;gh_heap *default_heap;
 gh_r100_om_scalar_view object_view;gh_r100_collision_enable collision_enable;gh_r100_task_exec task_exec;void *context;
 gh_game_scene_tables *actor_tables; /*optionalborrow SAMEcollisiontable owner*/
} gh_r100_init_bindings;
typedef struct {gh_r207_allocation work;gh_heap *heap;uint16_t known_work_bytes;
 uint32_t stage,frontier,pending_script;uint8_t frontier_relative,ready;
 gh_r100_om_view roofs[2];gh_collision_scroll_mutation collision;
 uint16_t jump_callback_module;uint32_t jump_callback_relative;} gh_r100_init;
gh_r100_init_status gh_r100_scroll_layer_002db2b0(gh_room_pools *,gh_scene_meshes *,uint32_t layer,int enabled,uint32_t *frontier);
/* Ordered recoveredprefix. CurrentunconnectedScript9B0/BC0 andtutorial
 * remainprecisefrontiers; ready is neverinferredfromallocation/metadata. */
gh_r100_init_status gh_r100_initialize_00000208(gh_r100_init *,const gh_r100_init_bindings *);
/* Native owner cleanup only; originalREL destructorlist is empty and not a
 * claim that its allocation was freed at everyroomexit. */
gh_r100_init_status gh_r100_init_release(gh_r100_init *);
#ifdef __cplusplus
}
#endif
#endif
