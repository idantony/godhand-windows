#ifndef GODHAND_CORE_MC_BOOT_ID_H
#define GODHAND_CORE_MC_BOOT_ID_H
#include "mc_boot.h"
#include "dvd_jobs.h"
#include "menu_state.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct gh_mc_id_resource {
    gh_r207_allocation allocation;
    gh_bundle_entry image;
    char path[128];
    uint32_t job_id;
    int preferred_path_found;
    uint8_t pair_bound;
} gh_mc_id_resource;
/* Real resource half of mcport SetIdData@82F0 after DispInit@8468 resolves
 * idsavecore via original2ACFA8 language/fallback rules. Loads through DVD,
 * yields while pending, binds actual nested TBL0/1 to ID manager slot0xB.
 * Stops BEFORE cIDBase_initialize@2A9EA0 and setPackedMessData@2A9F30; does
 * not claim DispInit/UI success. Zero-initialize resource before first call.
 * Caller owns allocation on success or a later parsing/binding error. */
gh_mc_boot_error gh_mc_boot_load_id_resource(gh_mc_id_resource *,gh_dvd_jobs *,
    gh_heap *,gh_task_threads *,gh_id_state *,int8_t requested_language,int8_t system_language);
/* Resource-half cleanup only; call before handing views to an active IDBase.
 * Original full release additionally releases IDBase; not supplied here. */
gh_mc_boot_error gh_mc_boot_release_id_resource(gh_mc_id_resource *,gh_heap *,gh_id_state *);
#ifdef __cplusplus
}
#endif
#endif
