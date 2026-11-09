#ifndef GH_BGM_REQUESTS_H
#define GH_BGM_REQUESTS_H
#include "sound_control.h"
#include "../platform/memory.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct gh_bgm_requests gh_bgm_requests;
typedef enum {GH_BGM_OK,GH_BGM_ARGUMENT,GH_BGM_MEMORY,GH_BGM_NOT_READY,
 GH_BGM_BOUNDS,GH_BGM_DISC,GH_BGM_BUSY,GH_BGM_UNSUPPORTED} gh_bgm_status;
typedef struct {
 uint32_t slot,event,logical_id,mode,request_flags,stream_flags;
 uint32_t rate,original_buffer_bytes;uint16_t member;
 uint8_t afd_channels,adx_channels,adx_version,adx_flags;
 uint64_t member_offset,member_bytes;
 const uint8_t *original_scalars;size_t original_scalar_bytes;
 gh_sound_span afd_row,encoded; /*owned, stable untilabort/destroy*/
 uint32_t frontier;int linked,prepared;
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
