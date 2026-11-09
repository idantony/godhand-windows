#ifndef GH_STARTUP_HOST_H
#define GH_STARTUP_HOST_H
#include "audio_startup.h"
#include "task_threads.h"
#include "../core/startup_task.h"
#include "../core/font_state.h"
#include "../core/menu_state.h"
#include "../core/mc_task.h"
#include "../core/mc_control.h"
#include "video_player.h"
#include "native_input.h"
#include "../core/se_voice.h"
#include "../core/sound_control.h"
#include "../core/game_movie_pool.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct gh_startup_host gh_startup_host;
typedef gh_id_error (*gh_host_mc_frame_begin)(void *);
typedef struct gh_startup_continuation {
    int enabled;
    int muted; /* QA/user master mute, never simulated audio completion */
    gh_se_dsp_policy audio_dsp_policy; /* explicit native reference profile */
    const char *ffmpeg_path; /* copied; missing codec/presenter is a frontier */
    gh_video_present movie_present;
    void *movie_present_context;
} gh_startup_continuation;
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
    uint8_t game_state;
    int game_assets_loaded;
    float game_rate;
    const char *frontier_module; /* NULL=main ELF, otherwise module-relative entry */
    const gh_id_base *mc_ui;
    const gh_packed_message_state *mc_messages;
    uint32_t game_texture_banks;
    uint64_t mc_ui_epoch,mc_frame_serial;
    int mc_ui_live,mc_frame_ready;
    uint32_t mc_phase,mc_subphase;
    int mc_done,normal_continuation;
    const gh_boot_work *work; /* borrowed read-only, exact input fields */
    const gh_sound_control *sound_control;
    const gh_movie_pool *movie_pool;
    gh_video_status movie_video;
} gh_startup_snapshot;
/* Developer execution slice beginning at original task0030D358 in slot3 of
 * the four-slot manager. Does not run the earlier boot/GS/IOP initialization
 * or claim the whole game has booted. Language0..6, flags598 and heap capacity
 * are explicit host inputs, not recovered default cHeap policies.
 * Original disc stays read-only; resource views live until host destruction.
 * Same OS thread and creating caller fiber only, as task_threads requires. */
gh_host_error gh_startup_host_create(const char *iso,int32_t language,
    uint32_t flags598,size_t heap_capacity,gh_startup_host **);
/* Explicit native save-profile location. Opening never creates or modifies
 * it. The shorter create uses relative userdata/memory_card for this developer
 * host; absent directory is genuinely no card when Control later queries it. */
gh_host_error gh_startup_host_create_with_profile(const char *iso,const char *profile,
    int32_t language,uint32_t flags598,size_t heap_capacity,gh_startup_host **);
/* Configure actual consumers before the first visit. Without one, the host
 * returns an explicit draw frontier in original order. Callback success must
 * mean the actual consumer ran; collecting a job is not rendered success. */
gh_host_error gh_startup_host_set_mc_consumers(gh_startup_host *,gh_mc_draw_consumer,
    void *draw_context,gh_id_post_message,void *post_context);
/* Explicit opt-in before tick0. Default retains first-frame prefix mode.
 * Full mode requires the named DSP profile and real device; rendering/video
 * consumers are never replaced with empty successful callbacks. */
gh_host_error gh_startup_host_set_continuation(gh_startup_host *,const gh_startup_continuation *);
/* Optional actual renderer frame setup, before Move/Trans. No yielding allowed
 * inside consumers. Snapshot frame_serial advances only after a successful
 * complete draw reaches its actual task Sleep, so present once per serial.
 * UI epoch changes on creation/normal release; borrowed UI views can expire. */
gh_host_error gh_startup_host_set_mc_frame_begin(gh_startup_host *,gh_host_mc_frame_begin,void *);
/* Owner-fiber only, normally once before each timed engine tick. Writes raw
 * 32-bit and logical64-bit original work fields via memcpy, no host-long casts.
 * Does not invent physical input, replay, analog or pad-lock policy. */
gh_host_error gh_startup_host_set_input(gh_startup_host *,const gh_native_input_snapshot *);
/* Advances selected tasks, then the actual DVD queue in original relative
 * order. A tick is a scheduler visit, not a timed/video frame. Audio startup
 * performs actual synchronous native commits; no fake per-frame RPC polling.
 * Unrecovered next entries stop at FRONTIER instead of successful empty tasks.
 * Recovered next tasks may run too; independent recovered tasks drain before
 * reporting another task's frontier. COMPLETE never means a playable game.
 * A call from the wrong owner returns FAILED without mutating host state;
 * snapshot still reports the session's actual execution phase. */
gh_host_phase gh_startup_host_tick(gh_startup_host *);
gh_host_error gh_startup_host_snapshot(const gh_startup_host *,gh_startup_snapshot *);
gh_host_error gh_startup_host_destroy(gh_startup_host *);
#ifdef __cplusplus
}
#endif
#endif
