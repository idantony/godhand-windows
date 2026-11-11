#ifndef GH_MC_EXIT_H
#define GH_MC_EXIT_H
#include "mc_title_load.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef enum {GH_MC_EXIT_FINAL_FRAME,GH_MC_EXIT_ICONS,GH_MC_EXIT_SUSPEND,GH_MC_EXIT_ID_BASE,GH_MC_EXIT_ID_PAIR,GH_MC_EXIT_ID_RESOURCE,GH_MC_EXIT_ID_FLAGS,GH_MC_EXIT_SYS_FLAGS,GH_MC_EXIT_CARD_END,GH_MC_EXIT_UNLINK,GH_MC_EXIT_MODULE_FREE,GH_MC_EXIT_BUSY_CLEAR,GH_MC_EXIT_FINISHED,GH_MC_EXIT_LOAD_PAYLOAD} gh_mc_exit_stage;
typedef struct {gh_mc_exit_stage stage;uint32_t tail_entry,frontier_address;gh_id_error id_error;gh_mc_error card_error;gh_mem_error memory_error;int pool_reclaimed,body_finished,frame_attempted,frame_completed;} gh_mc_exit_state;
/* CallafterControlsetsdone instead of thatiteration's sleep. Executes thelast
 * real Move/Trans/frameSleep itself, then originalnormalDeInit/DllFree prefix.
 * Mustruninsidecurrentnativefiber. BUSY/error retains exactcommittedprefix;
 * cleanupretrycontinuesatthatstage withoutrepeatSleep/releasedobjects.
 * A failedpartialdraw islatched; itcannotbesilentlyreplayedasanewframe.
 * Successfulfinish returnsoriginaltailentry0030DA08; ownerthenperformsreal
 * replace-current-entry-and-exit (doesnotreturn). Thishelperdoesnotcallabort,
 * executeunknownsuccessor, freecallerownedcardobject, orfakeEE IOPruntime.
 * Zero-initializeexitstate. Successivefinishedcalls returnBUSY, notdoublefree. */
gh_mc_task_status gh_mc_exit_run_final_frame(gh_mc_exit_state*,gh_mc_control_state*,
 gh_mc_task_state*,const gh_mc_task_bindings*,gh_id_post_message,void*,gh_mc_draw_consumer,void*);
/* Call after successful title Control. FRAME requires the original Sleep
 * for that visit: shared Move/Trans before End, bare fiber sleep after End.
 * OK means Main returned, the module/card endpoint is released, and the
 * current title task can return to its native scheduler slot. */
gh_mc_task_status gh_mc_title_exit_step_00001458(gh_mc_exit_state *,gh_mc_title_load *,
    gh_mc_task_state *,const gh_mc_task_bindings *,gh_screen_fade *,
    const gh_screen_fade_context *);
#ifdef __cplusplus
}
#endif
#endif
