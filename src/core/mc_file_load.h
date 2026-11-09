#ifndef GODHAND_CORE_MC_FILE_LOAD_H
#define GODHAND_CORE_MC_FILE_LOAD_H
#include "mc_task.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Original fileLoad@REL730: fully executes the no-pending-selected-file path,
 * including per-record flags aggregation. The original self+F8 byte is explicit;
 * workInit sets it0. Selected files stop at actual Open@3350, never fake-read.
 * Does not increment Control frame or draw; caller preserves those operations. */
gh_mc_boot_error gh_mc_file_load_00000730(gh_mc_boot_checks *,gh_boot_work *,
    uint8_t original_f8,uint32_t *first_unported);
#ifdef __cplusplus
}
#endif
#endif
