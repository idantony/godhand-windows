#ifndef GH_SOUND_SCENE_H
#define GH_SOUND_SCENE_H
#include "sound_control.h"
#include "boot.h"
#include "game_room_pack.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Canonical additional cSnd scene fields. Borrow one instance in BGM requests;
 * no guest pointers, no second registry/controller. Firstboot R020/default
 * tables only. This is not an SPU2 reverb implementation or bank2 voice player. */
typedef struct {gh_sound_control *control;gh_audio_registry *registry;
 gh_bundle_entry seh,bmh,ts;gh_audio_token bank2;
 uint32_t sample_budget,fade_ratio_bits,default_ratio_bits;
 uint32_t ratios60[5],ratios74[5],ratios88[5];
 int32_t current_bank118,current_request11c,current_weight120;
 int sound_group130;uint8_t startup_bound,ready;
 gh_r207_allocation loading;uint32_t job;size_t file_bytes;
 uint32_t frontier;int detail;
} gh_sound_scene;
typedef enum {GH_SOUND_SCENE_OK,GH_SOUND_SCENE_ARGUMENT,GH_SOUND_SCENE_BUSY,
 GH_SOUND_SCENE_ASSET,GH_SOUND_SCENE_UNSUPPORTED} gh_sound_scene_status;
/* Requires actual loaded startup registry and the single initialized control.
 * Derives original default group0 from2C9F58->2CA390, only metadata/reference
 * DSP state; no RPC result or hardware reverb completion is fabricated. */
gh_sound_scene_status gh_sound_scene_from_startup(gh_sound_scene *,gh_sound_control *,gh_audio_registry *);
/* Actual2CABE8 firstboot path: bounded real roompack search, real queuedDVDread
 * and native fiber wait, owned bank2 commit. R020 has no SEH/BMH/TS. Nonempty
 * tables are explicit unsupported before loading. No voice is acknowledged.
 * Caller pumps sameDVDqueue after task traversal; allocator replacesSPU space
 * with real host-owned registry bytes under original default sample budget. */
gh_sound_scene_status gh_sound_scene_initialize_002cabe8(gh_sound_scene *,
 const gh_bundle *,gh_boot_work *,gh_dvd_jobs *,gh_heap *,gh_task_threads *);
/* Release only transient read allocation. Committed bank remains registry-owned. */
gh_sound_scene_status gh_sound_scene_release_loading(gh_sound_scene *,gh_dvd_jobs *,gh_heap *);
#ifdef __cplusplus
}
#endif
#endif
