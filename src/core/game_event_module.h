#ifndef GH_GAME_EVENT_MODULE_H
#define GH_GAME_EVENT_MODULE_H
#include "game_event.h"
#include "boot.h"
#include "dvd_jobs.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct {gh_game_event *event;gh_dvd_jobs *dvd;gh_heap *heap;
 gh_r207_allocation allocation;size_t file_bytes;uint32_t allocated_bytes,job,frontier;int detail;
 uint8_t requested,loaded;} gh_event_module;
/* Actual297378 nonblocking200CB0. Original Release directory resolves to
 * debugoptimize in this disc's proven FST policy. Development host0 requests
 * remain unsupported; no generic basename fallback. Rawresourcepointer stays
 * zero; allocation above carries the original owned resource identity. */
gh_game_event_status gh_game_event_module_begin_00297378(gh_event_module *,gh_game_event *,
 const gh_boot_work *,gh_dvd_jobs *,gh_heap *);
/* Actual297428 returns original pending==0, including handle0. Native I/O
 * errors are a separate channel, not a successful module link/constructor. */
gh_game_event_status gh_game_event_module_poll_00297428(gh_event_module *,int *original_done);
/* Native abort/shutdown only; does not acknowledge original REL unlink. */
gh_game_event_status gh_game_event_module_abort(gh_event_module *);
#ifdef __cplusplus
}
#endif
#endif
