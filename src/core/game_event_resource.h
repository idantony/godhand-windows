#ifndef GH_GAME_EVENT_RESOURCE_H
#define GH_GAME_EVENT_RESOURCE_H
#include "game_event_module.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct {gh_game_event *event;gh_dvd_jobs *dvd;gh_heap *heap;
 gh_r207_allocation allocation;size_t file_bytes;uint32_t allocated_bytes,job,frontier;
 int32_t data_number;int detail;uint8_t requested,loaded;} gh_event_resource;

/* Actual44A960 format is hexadecimal event/ev%02x.dat, not decimal.
 * Width2 is a minimum; retain original full32-bit event-number formatting. */
gh_game_event_status gh_game_event_resource_path_00297540(int32_t,char out[32]);
/* Actual297540: independent DVD load of Event data, not Event.rel or a movie.
 * A missing file retains the original handle0 return; host I/O/allocation
 * failures have a separate error. Typed allocation replaces guest field14.
 * Completion proves only the bytes were read, never parsing or playback. */
gh_game_event_status gh_game_event_resource_begin_00297540(gh_event_resource *,
 gh_game_event *,gh_dvd_jobs *,gh_heap *);
gh_game_event_status gh_game_event_resource_poll_002975d0(gh_event_resource *,int *original_done);
/* Native abort only. Original event-release/flag bookkeeping is separate. */
gh_game_event_status gh_game_event_resource_abort(gh_event_resource *);
#ifdef __cplusplus
}
#endif
#endif
