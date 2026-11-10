#ifndef GH_GAME_EVENT_AUDIO_H
#define GH_GAME_EVENT_AUDIO_H

#include "game_event_create.h"
#include "sound_control.h"

#ifdef __cplusplus
extern "C" {
#endif

struct gh_bgm_requests;

typedef struct gh_event_audio_prefetch_request {
    gh_audio_registry *registry;
    const void *archive_owner;
    gh_disc *disc;
    size_t archive_index;
    uint64_t member_offset,member_size;
    uint32_t archive_id,row_index;
    uint8_t afd_row[32];
} gh_event_audio_prefetch_request;

/* Borrow cSnd's original common request pool. The provider performs 2D22B0,
 * 2CCB10, 2D2228 and the mode-4/flags-1 request, in that order when applicable.
 * The request is valid for this call. A provider retaining data must copy it. */
typedef gh_game_event_status (*gh_event_audio_prefetch_provider)(void *,
    const gh_event_audio_prefetch_request *,uint32_t *frontier);

/* Original cSnd+40 channel 1 (005FFD34). The host borrows the same sound
 * controller, DVD queue and default heap. AFD pointers are bounded offsets. */
typedef struct {
    gh_sound_control *control;
    gh_dvd_jobs *dvd;
    gh_heap *heap;
    gh_r207_allocation afd,work;
    size_t afd_bytes,afs_index,afs_bytes;
    uint32_t flags,job,frontier,row_offset;
    int32_t mode,event_no;
    int detail;
    uint8_t register_pending;
    gh_audio_registry *registry;
    gh_event_audio_prefetch_provider prefetch;
    void *prefetch_context;
    struct gh_bgm_requests *requests;
} gh_event_audio_channel1;

gh_game_event_status gh_event_audio_channel1_bind(gh_event_audio_channel1 *,
    gh_sound_control *,gh_dvd_jobs *,gh_heap *);
gh_game_event_status gh_event_audio_channel1_bind_registry(gh_event_audio_channel1 *,gh_audio_registry *);
gh_game_event_status gh_event_audio_channel1_bind_prefetch(gh_event_audio_channel1 *,
    gh_event_audio_prefetch_provider,void *context);
gh_game_event_status gh_event_audio_channel1_bind_lifecycle(gh_event_audio_channel1 *,struct gh_bgm_requests *);
gh_game_event_status gh_event_audio_channel1_service(void *,gh_event_audio_operation,
    int32_t event_no,int32_t argument,int *ready,uint32_t *frontier);
/* Call in 2CA558 after SE bank updates. DVD is pumped by the ordinary host. */
gh_game_event_status gh_event_audio_channel1_tick_002d0068(gh_event_audio_channel1 *);
gh_game_event_status gh_event_audio_channel1_shutdown(gh_event_audio_channel1 *);

#ifdef __cplusplus
}
#endif
#endif
