#ifndef GODHAND_CORE_DVD_JOBS_H
#define GODHAND_CORE_DVD_JOBS_H
#include "../platform/disc.h"
#include "../platform/memory.h"
#include "../platform/task_threads.h"
#ifdef __cplusplus
extern "C" {
#endif
#define GH_DVD_JOB_COUNT 32
typedef enum gh_dvd_error {
    GH_DVD_OK,GH_DVD_ARGUMENT,GH_DVD_NOT_FOUND,GH_DVD_MEMORY,
    GH_DVD_IO,GH_DVD_BOUNDS,GH_DVD_UNSUPPORTED,GH_DVD_TASK_ERROR
} gh_dvd_error;
/* Decoded original fields plus separate native ownership. No guest pointers
 * are cast into host pointers; this struct is not the 0x88-byte guest ABI. */
typedef struct gh_dvd_job {
    uint32_t state,id,file_number,blocks,byte_size,priority,arg70;
    size_t disc_index;
    gh_r207_allocation allocation;
} gh_dvd_job;
typedef struct gh_dvd_jobs {
    gh_disc *disc;
    gh_dvd_job jobs[GH_DVD_JOB_COUNT];
    int current; /* native index, -1 when original cur==NULL */
    uint32_t id_counter,queue_state;
    gh_dvd_error last_error;
    uint32_t failed_id;
} gh_dvd_jobs;
void gh_dvd_jobs_init(gh_dvd_jobs *,gh_disc *);
/* 200CB0/200DF8: caller owns successful allocation until gh_heap_release,
 * including after cancel/error. Override bytes rounds to 0x800; zero means
 * actual file length. arg70!=0 is unsupported CRI seek/preload configuration.
 * Explicit nonzero IDs retain original duplicate-ID semantics. Zero on failure,
 * separate last_error describes host failure. No data read occurs at enqueue. */
uint32_t gh_dvd_read_alloc(gh_dvd_jobs *,const char *,gh_r207_allocation *,
    gh_heap *,uint32_t id,uint32_t override_bytes,uint32_t arg70,uint32_t priority);
uint32_t gh_dvd_read_alloc_sized(gh_dvd_jobs *,const char *,gh_r207_allocation *,
    gh_heap *,uint32_t id,uint32_t *allocated_bytes,uint32_t arg70,uint32_t priority);
/* 201098: 2=current,1=queued,0=not pending, including failed/cancelled. */
int gh_dvd_check(const gh_dvd_jobs *,uint32_t id);
/* One original queue step: idle discovers queued state; state1 starts next;
 * state2 does one synchronous native sector read, completes then starts next.
 * Host read replaces CRI DMA, not its hardware timing. */
gh_dvd_error gh_dvd_pump(gh_dvd_jobs *);
/* 201108: while pending sleep1; after completion unconditionally sleep1 again.
 * Must run inside a native fiber; the frame owner pumps disc separately. */
gh_dvd_error gh_dvd_check_wait_frame(gh_dvd_jobs *,uint32_t,gh_task_threads *);
/* Original cancel subset: cancel current or matching queued IDs. Output buffer
 * remains caller-owned. No in-flight DMA because native reads are synchronous. */
/* Actual201018 clears all32 job records/current/queue-state1108. Native reads
 * are synchronous: no in-flight DMA/CRI handle exists to fake-cancel. Allocations
 * remain caller-owned, disc/id_counter/diagnostics remain the same owner. */
gh_dvd_error gh_dvd_jobs_reset_00201018(gh_dvd_jobs *);
int gh_dvd_cancel(gh_dvd_jobs *,uint32_t id);
#ifdef __cplusplus
}
#endif
#endif
