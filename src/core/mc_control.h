#ifndef GODHAND_CORE_MC_CONTROL_H
#define GODHAND_CORE_MC_CONTROL_H
#include "mc_task.h"
#include "id_jobs.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct gh_mc_control_state {
    unsigned char progress[0x18]; /* original this+98..AF */
    uint32_t zero_b0,zero_b4,zero_b8;
    uint32_t first_unported;
    gh_id_frontier id_frontier;
} gh_mc_control_state;
/* Original Control slot/file checks and cMcSys.SetDisp for current boot mode2.
 * NEEDS_SLEEP is an in-Control cMcSys.Sleep request; perform actual frames then
 * finish_delay. phase2 fileLoad/errorProc remain explicit frontiers. */
gh_mc_boot_error gh_mc_control_advance(gh_mc_control_state *,gh_mc_task_state *,const gh_mc_task_bindings *);
gh_mc_boot_error gh_mc_control_finish_delay(gh_mc_control_state *,gh_mc_task_state *,const gh_mc_task_bindings *);
/* Must perform the real original draw consumer represented by this job.
 * Unknown consumers return UNSUPPORTED; no successful empty draw adapter.
 * consumer_address is diagnostic only, never called as a host function ptr. */
typedef gh_id_error (*gh_mc_draw_consumer)(void *,const gh_id_draw_job *);
/* Actual cMcSys.Sleep(this,1) default-MC frame: IDBase.Move, Trans jobs in
 * original order through REQUIRED draw consumer, then genuine native task sleep.
 * On first unimplemented draw, returns its address and does NOT sleep/claim a
 * completed frame. The WorldMove/Trans path (flag10000000) is unsupported. */
gh_id_error gh_mc_control_sleep_frame(gh_mc_control_state *,gh_mc_task_state *,
    const gh_mc_task_bindings *,gh_id_post_message,void *post_context,
    gh_mc_draw_consumer,void *draw_context);
#ifdef __cplusplus
}
#endif
#endif
