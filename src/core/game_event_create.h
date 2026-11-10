#ifndef GH_GAME_EVENT_CREATE_H
#define GH_GAME_EVENT_CREATE_H
#include "game_event_components.h"
#include "game_event_resource.h"
#include "se_voice.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef gh_game_event_status (*gh_event_create_leaf)(void *,uint32_t *frontier);
typedef struct {gh_event_create_leaf call;void *context;} gh_event_on_loaded_slot;
typedef enum {GH_EVENT_AUDIO_PREPARE_002D1900,GH_EVENT_AUDIO_READY_002D19D8,
 GH_EVENT_AUDIO_START_002D1A30,GH_EVENT_AUDIO_WAIT_002D1B70} gh_event_audio_operation;
/* SAME original BGM channel1, not room-BGM Set(mode5). ready is meaningful
 * only for READY/WAIT; these callbacks must perform actual native services. */
typedef gh_game_event_status (*gh_event_audio_service)(void *,gh_event_audio_operation,int32_t event_no,int32_t argument,int *ready,uint32_t *frontier);
typedef struct {gh_game_event *event;const gh_boot_work *work;gh_event_module *module;
 gh_event_components *components;gh_event_resource *resource;gh_dvd_jobs *dvd;
 gh_heap *heap;gh_se_voice_engine *voices;const void *boot_elf;size_t boot_elf_bytes;
 gh_event_on_loaded_slot *on_loaded;gh_event_audio_service audio_channel1;
 /* Link must really bind native SNR2 imports/constructors, not only markDVD
  * done. Component data callback executes actual2977E8 methods on SAMEbanks. */
 gh_event_create_leaf module_link_002be890,data_bind_002977e8,data_prepare_002979f0;
 gh_event_create_leaf actor_task_status;
 gh_event_create_leaf abnormal_fade_002d1e18;
 uint8_t *system_menu_enabled;
 void *context;} gh_event_create_bindings;
typedef struct {gh_game_event *event;uint32_t substep,bank,frontier;int detail;
 uint8_t finished;} gh_event_create_state;
/* Original 296958, including the two actor creation flags and the optional
 * phase 6 wait. The original return becomes one when Create enters Play.
 * Resource consumers and the menu flag refer to the current native owners. */
gh_game_event_status gh_game_event_create_00296958(gh_event_create_state *,const gh_event_create_bindings *,int *original_done);
#ifdef __cplusplus
}
#endif
#endif
