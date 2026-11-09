#ifndef GODHAND_CORE_MC_TASK_H
#define GODHAND_CORE_MC_TASK_H
#include "mc_boot_id.h"
#include "id_base.h"
#include "packed_message.h"
#include "boot.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef enum gh_mc_task_status {
    GH_MC_TASK_OK,GH_MC_TASK_CONTROL_FRONTIER,GH_MC_TASK_ARGUMENT,GH_MC_TASK_BUSY,
    GH_MC_TASK_DISC_ERROR,GH_MC_TASK_MODULE_VERSION,GH_MC_TASK_PLATFORM_ERROR,
    GH_MC_TASK_MEMORY_ERROR,GH_MC_TASK_ID_ERROR,GH_MC_TASK_UNSUPPORTED
} gh_mc_task_status;
typedef struct gh_mc_task_bindings {
    gh_dvd_jobs *dvd;
    gh_heap *module_heap,*subscreen_heap;
    gh_task_threads *threads;
    gh_memory_card *card; /* already-created real native service, not fake IOP replies */
    gh_id_state *id_manager;
    gh_id_runtime *id_runtime;
    gh_font_state *fonts;
    gh_packed_message_state *packed;
    const gh_bundle *language;
    gh_boot_work *work;
    int32_t *game_suspend_word; /* decoded original Game+0x10 */
} gh_mc_task_bindings;
typedef struct gh_mc_task_state {
    gh_r207_allocation module;
    uint32_t module_job,flags,stage,frontier_offset;
    uint32_t zero_bc,zero_c0,zero_c8,zero_cc,zero_d0,progress_e0,progress_e4;
    uint8_t system_phase_dc,module_loaded,mc_sys_initialized,ui_initialized;
    gh_mc_boot_checks checks;
    gh_mc_id_resource id_resource;
    gh_id_base id_base;
    gh_id_error id_error;
    const char *frontier_name;
} gh_mc_task_state;
/* Actual original body through mcport Main/Initialize/workInit/DispInit(mode2).
 * Runs inside a real native fiber; frame owner pumps DVD separately. Loads and
 * SHA256-validates the actual mcport resource, binds native card service, then
 * truly allocates icon work, loadsIDsavecore and initializes67 entries/packed
 * environment/initial visibility/scale/positions. CONTROL_FRONTIER means these
 * steps completed, NOT MC Main/task completion. State/assets remain live.
 * First unexecuted call is overlay Control@0x2E8. No IOP guest code is run.
 * Explicitly supports current USA resource version only; debug host0 path fails.
 * Zero-initialize state before first call; this preparation is one-shot. */
gh_mc_task_status gh_mc_task_prepare_002ae8a8(gh_mc_task_state *,const gh_mc_task_bindings *);
/* Explicit abort/window-close cleanup of live partial state, not a fabricated
 * normal MC exit or tail-restart. Cancels pending reads, releases real owners,
 * and restores only globals owned by this MC prefix. */
gh_mc_task_status gh_mc_task_abort(gh_mc_task_state *,const gh_mc_task_bindings *);
#ifdef __cplusplus
}
#endif
#endif
