#ifndef GH_MC_CONTROL_FLOW_H
#define GH_MC_CONTROL_FLOW_H
#include "mc_error.h"
#include "mc_file_load.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct {gh_mc_error_state error;uint32_t first_unported,delay_remaining;uint64_t completed_controls,delay_frames_completed;gh_mc_boot_error failure;uint8_t original_f8;} gh_mc_control_flow;
/* Zero-initflow(originalworkInit self+F8=0). OriginalControl2E8 dispatches
 * phase0/1/2/3, SetDisp72C8 then incrementsframeE8, inthatorder. Actualcard
 * andSFX errorsstopattheirfrontier; failurelatched, no repeatedsideeffects.
 * OK stillrequiresMain'sordinarySleep1; ifdone usemc_exit_final_frameinstead.
 * NEEDS_SLEEP isinternalSlotCheckcMcSys.Sleep(n), notordinaryMainSleep. */
gh_mc_boot_error gh_mc_control_flow_advance(gh_mc_control_flow*,gh_mc_control_state*,
 gh_mc_task_state*,const gh_mc_task_bindings*,gh_mc_sound_dispatch,void*);
/* ExecuteoneactualMove/Trans/nativeSleep ofinternaldelay. CalluntilOK;
 * thenSetDisp+Controlframeincrementcompleteonce, stillfollowedbyMainSleep1.
 * No delaycounterisconsumed whenrender/taskSleepfails. */
gh_mc_boot_error gh_mc_control_flow_delay_frame(gh_mc_control_flow*,gh_mc_control_state*,
 gh_mc_task_state*,const gh_mc_task_bindings*,gh_id_post_message,void*,gh_mc_draw_consumer,void*);
#ifdef __cplusplus
}
#endif
#endif
