#ifndef GODHAND_CORE_MC_BOOT_H
#define GODHAND_CORE_MC_BOOT_H
#include "../platform/memory_card.h"
#include "../platform/memory.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef enum gh_mc_boot_error {GH_MC_BOOT_OK,GH_MC_BOOT_ARGUMENT,GH_MC_BOOT_MEMORY,GH_MC_BOOT_IO,GH_MC_BOOT_UNSUPPORTED,GH_MC_BOOT_NEEDS_SLEEP} gh_mc_boot_error;
/* Decoded cMcBoot fields used by the verified slot/file check slice. */
typedef struct gh_mc_boot_checks {
    uint8_t done,has_min_free_space;
    int32_t file_index,load_index,error,mode,retry;
    uint32_t present_mask,frame,phase,subphase;
    uint32_t delay_frames; /* original request to cMcSys_sleep, to be honored */
    uint32_t phase_after_delay;
    gh_r207_allocation icon_work; /* original+0x80,20*0x400 bytes */
} gh_mc_boot_checks;
/* Allocates/initializes the original20KiB work buffer and check fields.
 * This is NOT the full Initialize/Main entry: idsavecore/cIDBase setup is a
 * separate unresolved prerequisite. No full boot success is returned here. */
gh_mc_boot_error gh_mc_boot_checks_create(gh_mc_boot_checks *,gh_heap *);
gh_mc_boot_error gh_mc_boot_checks_release(gh_mc_boot_checks *,gh_heap *);
/* SlotCheck/fileCheck portion of a Control visit, against real native card operations.
 * phase0/1 recovered; phase2 fileLoad and phase3 error UI explicitly unsupported.
 * NEEDS_SLEEP retains original phase until caller honors delay_frames through
 * its recovered frame pump/task sleep and calls delay_completed. Not a full
 * Control replacement: cMcSys UI update is still a separate prerequisite. Frame
 * increments once per control visit, matching original Control, not wall time.
 * game_flags is Sys+0x584; slotCheck clears bit2 as original. */
gh_mc_boot_error gh_mc_boot_check_control(gh_mc_boot_checks *,gh_memory_card *,uint32_t *game_flags);
gh_mc_boot_error gh_mc_boot_delay_completed(gh_mc_boot_checks *);
#ifdef __cplusplus
}
#endif
#endif
