#ifndef GH_GAME_SCENARIO_SOFT_EVENT_H
#define GH_GAME_SCENARIO_SOFT_EVENT_H
#include "game_scene_setup.h"
#include "game_event_start.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef enum {GH_SOFT_EVENT_OK,GH_SOFT_EVENT_ARGUMENT,GH_SOFT_EVENT_UNSUPPORTED,GH_SOFT_EVENT_CALLEE} gh_soft_event_status;
typedef gh_soft_event_status (*gh_soft_player_abort)(void *,uint32_t *frontier);
typedef gh_soft_event_status (*gh_soft_event_leaf)(void *,uint32_t *frontier);
typedef struct {
 const gh_event_start_bindings *room_objects;
 gh_soft_event_leaf suspend_enemies,suspend_effects,suspend_player;
} gh_soft_event_kind0_bindings;
typedef struct {gh_scene_scenario_view scenario;gh_boot_work *work;
 gh_soft_player_abort player_abort_00126770;void *context;const gh_soft_event_kind0_bindings *kind0;} gh_soft_event_bindings;
typedef struct {uint32_t stage,frontier;uint8_t *scenario_bytes;gh_task_manager *manager;gh_boot_work *work;uint8_t kind;} gh_soft_event_start;
/* Originalleaf31AFA0->31B410, SAMEWork input bytes. No native padowner. */
gh_soft_event_status gh_scenario_stop_input_0031afa0(gh_boot_work *);
/* Kind4 event start and kind0 room soft event. A required provider failure
 * keeps the continuation at that call. The next invocation is fresh after
 * this original call completes. Other kinds retain their strict frontier. */
gh_soft_event_status gh_scenario_start_soft_event_002c39b0(gh_soft_event_start *,const gh_soft_event_bindings *,uint8_t kind);
/* Original kind0 uses argument one. Related actor slots are ordered
 * Work+5F8, player+698, +6A0 and +6A8; known_mask distinguishes NULL. */
gh_soft_event_status gh_soft_event_suspend_player_0012c7d0(const gh_title_player_view *,
 const gh_title_player_view *const related[4],uint8_t known_mask,uint32_t *frontier);
/* Actual 2FA470 only visits the original 754C30 object registry. */
gh_soft_event_status gh_soft_event_suspend_effects_002fa470(const gh_event_start_bindings *,
 uint32_t *frontier);

#ifdef __cplusplus
}
#endif
#endif
