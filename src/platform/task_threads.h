#ifndef GODHAND_PLATFORM_TASK_THREADS_H
#define GODHAND_PLATFORM_TASK_THREADS_H
#include "../core/tasks.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct gh_task_threads gh_task_threads;
typedef void (*gh_native_task_entry)(void *context, int32_t argument);
typedef enum gh_thread_status {
    GH_THREAD_OK, GH_THREAD_INVALID, GH_THREAD_NO_MEMORY,
    GH_THREAD_WRONG_THREAD, GH_THREAD_BUSY, GH_THREAD_UNSUPPORTED
} gh_thread_status;
/* Windows fiber adapter for the observed serial task sleep/exit handshake.
 * Same OS thread only. Task code must yield through this API or return.
 * No arbitrary EE syscalls, preemption, interrupts, kernel priority scheduling,
 * or state4 external wakeups are implemented. A task that never yields blocks
 * its caller, just as the original manager waits for its semaphore signal.
 * Creating a backend attaches its step callback to an otherwise unbound manager.
 * The caller owns records and must keep their base/capacity/count valid. */
gh_thread_status gh_task_threads_create(gh_task_manager *, gh_task_threads **);
gh_thread_status gh_task_threads_destroy(gh_task_threads *);
/* Select original slot, record host entry/arg, set original state2; slot is -1
 * on pool full with OK. Active slots cannot be overwritten. Inherits attr. */
gh_thread_status gh_task_threads_start(gh_task_threads *, int32_t request,
    gh_native_task_entry, void *context, int32_t argument, int32_t *slot);
/* Original uint16 sleep semantics: 1 resumes on the next selected visit;
 * 0 wraps and requires 65536 selected visits. Valid only inside its active task. */
gh_thread_status gh_task_threads_sleep(gh_task_threads *, int32_t frames);
/* Cancels a suspended/unstarted task; does not unwind its C stack. Cancelling
 * the currently executing task or from another OS thread is rejected. */
gh_thread_status gh_task_threads_cancel(gh_task_threads *, int32_t slot);
/* Bound gh_task_step_fn; drive only from the creating caller fiber on the
 * same OS thread. Errors propagate through GH_TASK_STEP_FAILED. */
int gh_task_threads_step(void *, gh_task_manager *, gh_task_record32 *);
#ifdef __cplusplus
}
#endif
#endif
