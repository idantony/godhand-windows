#ifndef GODHAND_CORE_MC_ERROR_H
#define GODHAND_CORE_MC_ERROR_H
#include "mc_control.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct gh_mc_sound_request {
    int32_t bank,sound_id;
    int16_t parameter3,parameter4; /* original two unknown short args, both-1 */
    uint32_t parameter5;int32_t parameter6; /* original zero */
} gh_mc_sound_request;
/* Return0 only after the real native SeCall consumer ran (its original return
 * value is ignored). Missing/unsupported voice execution must return nonzero,
 * not enqueue an acknowledged empty sound. */
typedef int (*gh_mc_sound_dispatch)(void *,const gh_mc_sound_request *);
typedef struct gh_mc_error_state {
    uint8_t choice; /* original McBoot+0x0C */
    uint32_t first_unported;
    int sound_error;
} gh_mc_error_state;
/* Actual ErrorProc@mcport+BD8 only; original Control invokes SetDisp and then
 * increments frame afterward. Reads original Sys button words+1A0/+1B0 from
 * work. No key is invented and no card check is skipped. Sound callbacks are
 * required only on branches that really emit one; missing callback stops
 * before changing confirm/choice/done state at actual SeCall@2CB8A0. */
gh_mc_boot_error gh_mc_error_process_00000bd8(gh_mc_error_state *,gh_mc_control_state *,
    gh_mc_task_state *,const gh_mc_task_bindings *,gh_mc_sound_dispatch,void *);
/* Recovered error SetDisp modes12/14/19 and confirmation17 (hex), including
 * actual message IDs, visibility and setChoise@7E08 cursor position copying. */
gh_id_error gh_mc_error_set_display(gh_mc_error_state *,gh_mc_control_state *,
    gh_mc_task_state *,const gh_mc_task_bindings *);
#ifdef __cplusplus
}
#endif
#endif
