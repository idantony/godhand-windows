#ifndef GODHAND_CORE_MC_FILE_LOAD_H
#define GODHAND_CORE_MC_FILE_LOAD_H
#include "mc_task.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Original fileLoad@REL730. Opens, reads and closes the original 0x400-byte
 * boot record through the same native card endpoint, then aggregates flags.
 * The caller owns Control's frame increment and display. */
gh_mc_boot_error gh_mc_file_load_00000730(gh_mc_boot_checks *,gh_memory_card *,gh_boot_work *,
    uint8_t original_f8,uint32_t *first_unported);
#ifdef __cplusplus
}
#endif
#endif
