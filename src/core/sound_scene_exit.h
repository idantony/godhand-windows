#ifndef GH_SOUND_SCENE_EXIT_H
#define GH_SOUND_SCENE_EXIT_H
#include "sound_scene.h"
#include "bgm_requests.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Borrows the SAME live owners, never copies sound lists or controllers. */
typedef struct {gh_sound_scene *scene;gh_bgm_requests *bgm;gh_se_voice_engine *voices;
 uint32_t frontier;int detail;uint8_t begun,final_stage;} gh_sound_scene_exit;
typedef enum {GH_SOUND_EXIT_OK,GH_SOUND_EXIT_ARGUMENT,GH_SOUND_EXIT_BUSY,
 GH_SOUND_EXIT_NATIVE,GH_SOUND_EXIT_UNSUPPORTED} gh_sound_exit_status;
/* Registry-backed native heap close is supported only once all mapped bank
 * allocations are genuinely released. Any residual owned bank is explicit
 * 2CD890 unsupported, never a made-up PS2 parent-block free. */
gh_sound_exit_status gh_sound_scene_exit_bind(gh_sound_scene_exit *,gh_sound_scene *,gh_bgm_requests *,gh_se_voice_engine *);
gh_sound_exit_status gh_sound_scene_exit_begin_002caa00(gh_sound_scene_exit *);
/* Exact original boolean; no elapsed frames, SE/BGM pumping or forced EOS. */
gh_sound_exit_status gh_sound_scene_exit_wait_002cab58(gh_sound_scene_exit *,int *original_return);
gh_sound_exit_status gh_sound_scene_exit_finalize_002ca910(gh_sound_scene_exit *);
#ifdef __cplusplus
}
#endif
#endif
