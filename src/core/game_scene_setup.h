/* God Hand original Scenario setup leaves; MIT LICENSE.upstream. */
#ifndef GH_GAME_SCENE_SETUP_H
#define GH_GAME_SCENE_SETUP_H
#include "boot.h"
#include "tasks.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Borrow the canonical original Scenario scalar storage. Manager bytes1C..4F
 * are NOT read/written here: its native owner is manager. No constructor or
 * cold-zero assumption is supplied by attaching this view. */
typedef struct {uint8_t *bytes;size_t size;gh_task_manager *manager;} gh_scene_scenario_view;
typedef enum {GH_SCENE_OK,GH_SCENE_ARGUMENT,GH_SCENE_UNSUPPORTED,GH_SCENE_CALLEE} gh_scene_status;
typedef gh_scene_status (*gh_scene_room_hook)(void *,uint32_t *frontier);
typedef enum {GH_SCENE_HOOK_ABSENT,GH_SCENE_HOOK_NATIVE,GH_SCENE_HOOK_UNRESOLVED} gh_scene_hook_kind;
/* ABSENT means original record+offset was verified zero after relocation.
 * UNRESOLVED is distinct, never an empty successful native callback. */
typedef struct {gh_scene_hook_kind kind;gh_scene_room_hook call;void *context;} gh_scene_hook;
/*2C33D8: caller lends actual cEmSetParam base00586AB0 first byte. */
gh_scene_status gh_scene_before_objects_002c33d8(gh_scene_scenario_view,
 gh_boot_work *,uint8_t *em_set_first_byte,gh_scene_hook record_plus4,uint32_t *frontier);
/*2C3440: requires original selected-record+8 export mapping (R020Init forroom20).
 * Reset prefix is observable even when that export is unsupported. Callback
 * executes after all writes. The two bracket functions2D9BD0/2D9BD8 are actual
 * jrra/nop in this patched ELF. No guest function address is called. */
gh_scene_status gh_scene_initialize_room_002c3440(gh_scene_scenario_view,
 gh_boot_work *,gh_scene_hook record_plus8,uint32_t *frontier);
#ifdef __cplusplus
}
#endif
#endif
