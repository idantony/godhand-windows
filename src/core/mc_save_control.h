#ifndef GH_MC_SAVE_CONTROL_H
#define GH_MC_SAVE_CONTROL_H
#include "mc_control.h"
#include "mc_error.h"
#include "mc_file_save.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct {
    gh_mc_load_scan scan;
    int32_t free_clusters;
    uint8_t choice,selected_slot,info_pending,complete;
} gh_mc_save_control;
typedef struct {
    gh_game_checkpoint *checkpoint;
    const gh_checkpoint_bindings *checkpoint_bindings;
    const gh_game_door *door;
    const gh_mc_save_metadata *metadata;
    gh_mc_sound_dispatch sound;
    void *sound_context;
} gh_mc_save_providers;
typedef enum {
    GH_MC_SAVE_PENDING,GH_MC_SAVE_COMPLETE,GH_MC_SAVE_ARGUMENT,
    GH_MC_SAVE_UNSUPPORTED,GH_MC_SAVE_IO
} gh_mc_save_status;
/* Original formatted card mode0 Control. Caller owns cMcSys.Sleep(1) and
 * native checkpoint/clock/sound providers. Completion follows readback and
 * icon close, then the original completion notice. */
gh_mc_save_status gh_mc_control_save_00003f78(gh_mc_save_control *,gh_mc_control_state *,
    gh_mc_save_task_state *,const gh_mc_task_bindings *,gh_mc_load_menu_memory *,
    const gh_mc_save_providers *,uint32_t *frontier);
#ifdef __cplusplus
}
#endif
#endif
