#ifndef GH_MC_TITLE_LOAD_H
#define GH_MC_TITLE_LOAD_H
#include "mc_file_load.h"
#include "mc_error.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct {
    gh_mc_selected_load selected;
    uint8_t info_pending,choice,selected_slot,completion_display_pending;
} gh_mc_title_load;

typedef struct {
    gh_game_checkpoint *checkpoint;
    const gh_checkpoint_bindings *checkpoint_bindings;
    gh_game_door *door;
    gh_mc_output_mode_apply output_mode;
    void *audio_context;
    gh_mc_sound_dispatch sound;
    void *sound_context;
} gh_mc_title_load_providers;

/* Original 20B0/2490/2700/2968/2A58 and their SetDisp tails. Call once
 * per original Control visit. RESTORED and CANCELED set cMcLoad done.
 * The mode owns fade-out, cleanup and task exit. */
gh_mc_load_status gh_mc_control_title_select_00001590(gh_mc_title_load *,gh_mc_control_state *,
    gh_mc_load_scan *,gh_mc_task_state *,const gh_mc_task_bindings *,gh_mc_load_menu_memory *,
    const gh_mc_title_load_providers *,uint32_t *frontier);
/* Release queued native card work after suspending/cancelling the title
 * fiber and before freeing its McLoad payload. */
gh_mc_error gh_mc_control_title_abort_io(gh_mc_title_load *,gh_memory_card *);
#ifdef __cplusplus
}
#endif
#endif
