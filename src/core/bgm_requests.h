#ifndef GH_BGM_REQUESTS_H
#define GH_BGM_REQUESTS_H
#include "sound_control.h"
#include "boot.h"
#include "../platform/memory.h"
#include "../platform/adx_stream.h"
#include "game_children.h"
#include "game_init.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct gh_bgm_requests gh_bgm_requests;
typedef enum {GH_BGM_OK,GH_BGM_ARGUMENT,GH_BGM_MEMORY,GH_BGM_NOT_READY,
 GH_BGM_BOUNDS,GH_BGM_DISC,GH_BGM_BUSY,GH_BGM_UNSUPPORTED,GH_BGM_NATIVE} gh_bgm_status;
typedef enum {GH_BGM_GAIN_NATIVE_DB_REFERENCE=1} gh_bgm_gain_policy;
typedef struct {
 uint32_t slot,event,logical_id,mode,request_flags,stream_flags;
 uint32_t rate,original_buffer_bytes;uint16_t member;
 uint8_t afd_channels,adx_channels,adx_version,adx_flags;
 uint64_t member_offset,member_bytes;
 const uint8_t *original_scalars;size_t original_scalar_bytes;
 gh_sound_span afd_row,encoded; /*owned, stable untilabort/destroy*/
 uint32_t frontier;int linked,prepared;
 gh_adx_stream_status transport; /*actualnativequeue/status,notSDKnumericstatus*/
} gh_bgm_request_view;
/* Borrows unique startupregistry/controller/disc/heap. Does not add another
 * soundcontroller or a second BGM table. No output device is opened here. */
gh_bgm_status gh_bgm_requests_create(gh_audio_registry *,gh_sound_control *,gh_disc *,gh_heap *,gh_bgm_requests **);
/* Original2D15A8 is void; this port status exposes an unimplemented callee.
 * Recover its actual firstfree16/reset/mode5/373118/373158 prefix plus real
 * native-owned AFS source and original373BA8 workbuffer allocation. Stops at
 * SDK constructor329438, before a native decoder/player exists. A partial
 * request is NOT linked at2CC5C0, not ready/playing, and cannot complete Init.
 * Failure retains partialstate for inspection and explicitabort. */
gh_bgm_status gh_bgm_event_set_prefix_002d15a8(gh_bgm_requests *,uint32_t event,int16_t arg3,int16_t arg4);
/* Explicit native transport/DSP adaptation. Key comes from actualbootELF.
 * Converts original quantized tenthdB table to nativePCM gain using a pinned
 * binary32 nearest pow10 reference; not a claim of originalSPU2 DSP equality.
 * Configure before requests; masterMute is player-local, before firststart. */
gh_bgm_status gh_bgm_requests_enable_native(gh_bgm_requests *,const gh_adx_key *,int muted,gh_bgm_gain_policy);
/* Full immediate mode5 R020event0 path, including realnativeprepare,original
 * update andlistinsert. Doesnotstartplayback untiloriginalflagsrequest it. */
gh_bgm_status gh_bgm_event_set_002d15a8(gh_bgm_requests *,uint32_t event,int16_t arg3,int16_t arg4);
/* Existing prepared Start and missing-request2CC900/373298 path for verified
 * R020 event0/zeroargs. Fresh requests own a separate encoded/workbuffer and
 * actually start native PCM after the original immediate gain update (explicit
 * asynchronous SDK-to-native timing adaptation). Set itself never autoplays. */
gh_bgm_status gh_bgm_event_start_002d1610(gh_bgm_requests *,uint32_t event,int16_t arg3,int16_t arg4);
/* Event-specific2D1768: current R020 negative initial-factor branch, without
 * Stop's controller flag changes. Missing event is the original void no-op. */
gh_bgm_status gh_bgm_event_fade_002d1768(gh_bgm_requests *,uint32_t event,float frames,float target,float initial_factor);
gh_bgm_status gh_bgm_event_stop_002d1800(gh_bgm_requests *,int preserve_mode5);
/* Original2D0938 activeBGM update only, not the entire2CAF28 soundsystemtick.
 * Unknownactivebranches stop at exactcallee; no discardedBGM request. */
gh_bgm_status gh_bgm_requests_update_002d0938(gh_bgm_requests *,const gh_boot_work *);
/* Actualcase3tail2D0850/2D0D70. Readsoriginal44rowroommap fromuserELF;
 * borrows canonicalchildren/Heat/enemyhead, no defaultroom->trackguess.
 * Activeenemy/specialdoor override consumers remainpreciseunsupported. */
gh_bgm_status gh_bgm_requests_scene_begin_002d0850(gh_bgm_requests *,const gh_boot_work *,const gh_game_children *,const gh_game_init_state *);
/* Actual2CAA00 BGM pass and2CAB58 BGM predicate. Does not advance audio
 * time; caller retains ordinary2D0938/frame pump. Preserved requests fade to
 * .01 then pause after original20->...->-1 countdown. */
int gh_bgm_requests_uses_owners(const gh_bgm_requests *,const gh_audio_registry *,const gh_sound_control *);
gh_bgm_status gh_bgm_requests_scene_exit_begin(gh_bgm_requests *);
gh_bgm_status gh_bgm_requests_scene_exit_ready(const gh_bgm_requests *,int *);
gh_bgm_status gh_bgm_requests_scene_exit_finalize(gh_bgm_requests *);
int gh_bgm_requests_scene_event(const gh_bgm_requests *,int32_t *);
/* Borrowed scene context is captured by actual scene_begin and must outlive
 * ordinary audio ticks. This is the SAME children/Heat/enemy owner, no copies.
 *2D0EA0 countdown/select/crossfade uses original16 request pool, mode1 source
 * preparation and native transport. Native scene starts queue at next audio
 * tick, after original constructor zero-gain/fade writes have completed. */
typedef struct {int32_t current128;uint32_t delay134,countdown138;const gh_game_children *children;const gh_game_init_state *init;} gh_bgm_automatic_state;
int gh_bgm_requests_automatic_state(const gh_bgm_requests *,gh_bgm_automatic_state *);
unsigned gh_bgm_requests_pending(const gh_bgm_requests *);
unsigned gh_bgm_requests_active(const gh_bgm_requests *);
uint32_t gh_bgm_requests_frontier(const gh_bgm_requests *);
gh_bgm_status gh_bgm_requests_inspect(const gh_bgm_requests *,unsigned,gh_bgm_request_view *);
/* Port teardown for unlinked, never-started native requests. This does not
 * pretend to implement originalStop/BgmEventFade or audio completion. */
gh_bgm_status gh_bgm_requests_abort_pending(gh_bgm_requests *);
void gh_bgm_requests_destroy(gh_bgm_requests *);
#ifdef __cplusplus
}
#endif
#endif
