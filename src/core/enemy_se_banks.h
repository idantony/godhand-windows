#ifndef GH_ENEMY_SE_BANKS_H
#define GH_ENEMY_SE_BANKS_H
#include "sound_control.h"
#include "dvd_jobs.h"
#include "boot.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct gh_enemy_se_banks gh_enemy_se_banks;
typedef enum {GH_ENEMY_SE_OK,GH_ENEMY_SE_ARGUMENT,GH_ENEMY_SE_BOUNDS,GH_ENEMY_SE_MEMORY,GH_ENEMY_SE_DISC,GH_ENEMY_SE_BUSY,GH_ENEMY_SE_UNSUPPORTED,GH_ENEMY_SE_FORMAT} gh_enemy_se_status;
typedef struct {gh_sound_control *control;gh_audio_registry *registry;gh_disc *disc;gh_dvd_jobs *dvd;gh_heap *default_heap,*room_heap;gh_se_voice_engine *voices;} gh_enemy_se_bindings;
typedef struct {int32_t bank,resource_id;uint32_t state,load_flags,source_heap_global,sample_heap_global,job;size_t file_bytes,capacity;gh_audio_token token;} gh_enemy_se_slot_view;
/* Attach ONCE to SAME control. Read actualELF52-slot ctor and15aliasgroups,
 * verify dynamic12slots are not already committed. Rawguest pointers remain IDs. */
gh_enemy_se_status gh_enemy_se_banks_from_startup(const gh_enemy_se_bindings *,gh_enemy_se_banks **);
gh_enemy_se_status gh_enemy_se_normalize_002cb3a8(const gh_enemy_se_banks *,int32_t resource_id,int32_t *);
/* Original 2CB418 scans the dynamic bank table in order. A known no-match
 * returns -1; an unbound or inconsistent registry reports a native error. */
gh_enemy_se_status gh_enemy_se_resolve_002cb418(const gh_enemy_se_banks *,int32_t dependency,int32_t *original_bank);
/* Actual2CB278: -1 only actualdisabled/outofrange/duplicate/full/not-found branches.
 * Positive result means a realDVD allocation queued, NOT ready or audible. */
gh_enemy_se_status gh_enemy_se_request_002cb278(gh_enemy_se_banks *,const gh_boot_work *,int32_t resource_id,int32_t *original_slot);
/* Per-original sound load update boundary. Native copies/registry publication
 * replace IOP/SPU transfers, with genuine external-index reads and staged ready.
 * Missing real voice owner remains375128 frontier. No SFX playback is claimed. */
gh_enemy_se_status gh_enemy_se_banks_update_002cf440(gh_enemy_se_banks *);
gh_enemy_se_status gh_enemy_se_ready_002cb778(const gh_enemy_se_banks *,int32_t slot,int *original_ready);
gh_enemy_se_status gh_enemy_se_slot_at(const gh_enemy_se_banks *,int32_t slot,gh_enemy_se_slot_view *);
gh_enemy_se_status gh_enemy_se_source_view(const gh_enemy_se_banks *,int32_t slot,const uint8_t **bytes,size_t *file_bytes);
/* Scoped realbank-lifetime cleanup; refuses same-bank EEvoices. Never releases
 * unrelated startup/room banks or resets registry generation. */
/* Original2CB7E8 node removal then state3. Bounded domain requires real
 * voice owner/no same-bank nodes; active-node removal remains2CBE68. */
gh_enemy_se_status gh_enemy_se_mark_failed_002cb7e8(gh_enemy_se_banks *,int32_t slot);
gh_enemy_se_status gh_enemy_se_release_002cf310(gh_enemy_se_banks *,int32_t slot);
gh_enemy_se_status gh_enemy_se_banks_destroy(gh_enemy_se_banks *);
uint32_t gh_enemy_se_frontier(const gh_enemy_se_banks *);
int gh_enemy_se_detail(const gh_enemy_se_banks *);
#ifdef __cplusplus
}
#endif
#endif
