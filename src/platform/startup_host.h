#ifndef GH_STARTUP_HOST_H
#define GH_STARTUP_HOST_H
#include "audio_startup.h"
#include "task_threads.h"
#include "../core/startup_task.h"
#include "../core/font_state.h"
#include "../core/menu_state.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct gh_startup_host gh_startup_host;
typedef enum {
    GH_HOST_RUNNING, GH_HOST_FRONTIER, GH_HOST_COMPLETE, GH_HOST_FAILED
} gh_host_phase;
typedef enum {
    GH_HOST_OK, GH_HOST_ARGUMENT, GH_HOST_MEMORY, GH_HOST_DISC,
    GH_HOST_THREADS, GH_HOST_ASSET, GH_HOST_AUDIO, GH_HOST_ENTRY_UNAVAILABLE
} gh_host_error;
typedef struct gh_startup_snapshot {
    gh_host_phase phase;
    gh_host_error error;
    int subsystem_error;
    unsigned failed_operation;
    uint32_t unavailable_entry;
    uint64_t ticks;
    uint32_t flags;
    unsigned loaded_bundles;
    int startup_returned;
    const gh_font_state *font;
    const gh_menu_state *menu;
    const gh_id_state *id;
    const gh_audio_registry *audio;
} gh_startup_snapshot;
/* Developer execution slice beginning at original task0030D358 in slot3 of
 * the four-slot manager. Does not run the earlier boot/GS/IOP initialization
 * or claim the whole game has booted. Language0..6, flags598 and heap capacity
 * are explicit host inputs, not recovered default cHeap policies.
 * Original disc stays read-only; resource views live until host destruction.
 * Same OS thread and creating caller fiber only, as task_threads requires. */
gh_host_error gh_startup_host_create(const char *iso,int32_t language,
    uint32_t flags598,size_t heap_capacity,gh_startup_host **);
/* Advances selected tasks, then the actual DVD queue in original relative
 * order. A tick is a scheduler visit, not a timed/video frame. Audio startup
 * performs actual synchronous native commits; no fake per-frame RPC polling.
 * Unrecovered next entries stop at FRONTIER instead of successful empty tasks.
 * COMPLETE would mean only task0030D358, never a playable-game claim.
 * A call from the wrong owner returns FAILED without mutating host state;
 * snapshot still reports the session's actual execution phase. */
gh_host_phase gh_startup_host_tick(gh_startup_host *);
gh_host_error gh_startup_host_snapshot(const gh_startup_host *,gh_startup_snapshot *);
gh_host_error gh_startup_host_destroy(gh_startup_host *);
#ifdef __cplusplus
}
#endif
#endif
