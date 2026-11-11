#ifndef GODHAND_CORE_MC_FILE_LOAD_H
#define GODHAND_CORE_MC_FILE_LOAD_H
#include "mc_task.h"
#include "game_checkpoint.h"
#include "game_door.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Original fileLoad@REL730. Opens, reads and closes the original 0x400-byte
 * boot record through the same native card endpoint, then aggregates flags.
 * The caller owns Control's frame increment and display. */
gh_mc_boot_error gh_mc_file_load_00000730(gh_mc_boot_checks *,gh_memory_card *,gh_boot_work *,
    uint8_t original_f8,uint32_t *first_unported);
/* XOR8C08 reads the original first 0xB108 bytes of a full save record. */
uint64_t gh_mc_save_checksum_00008c08(const uint8_t *record);
typedef struct {int32_t first_slot,selected_row;} gh_mc_load_menu_memory;
typedef struct {
    int32_t first_slot,selected_row;
    uint8_t menu_ready;
} gh_mc_load_scan;
/* Title Control phases 0..2. The caller executes the common SetDisp72C8
 * after each successful visit. Mc+0C/+10 must start at -1 on a fresh owner. */
gh_mc_boot_error gh_mc_title_load_scan_00001590(gh_mc_load_scan *,gh_mc_boot_checks *,
    gh_memory_card *,gh_mc_load_menu_memory *,uint32_t *frontier);
/* Device-free selected-file phases of cMcLoad@REL2700. A verified file
 * does not imply Control/UI completion. Keep cursor and endpoint alive
 * until a terminal result; use one endpoint owner while it is pending. */
typedef enum {
    GH_MC_LOAD_PENDING, GH_MC_LOAD_VERIFIED, GH_MC_LOAD_ARGUMENT,
    GH_MC_LOAD_IO, GH_MC_LOAD_BUSY, GH_MC_LOAD_NOT_FOUND, GH_MC_LOAD_SHORT,
    GH_MC_LOAD_CHECKSUM, GH_MC_LOAD_VERSION, GH_MC_LOAD_UNSUPPORTED,
    GH_MC_LOAD_RESTORED, GH_MC_LOAD_CANCELED
} gh_mc_load_status;
typedef struct {
    uint8_t record[GH_MC_SAVE_RECORD_BYTES];
    uint8_t *borrowed_record;
    gh_memory_card *card;
    gh_mc_load_status result;
    uint32_t frontier;
    uint8_t phase,applied;
} gh_mc_selected_load;
gh_mc_load_status gh_mc_selected_load_begin(gh_mc_selected_load *,gh_memory_card *,unsigned slot);
gh_mc_load_status gh_mc_selected_load_step(gh_mc_selected_load *,uint32_t *frontier);
/* cMcLoad uses its original B400 work allocation and defers CloseCheck
 * until its Control counter reaches twenty. Legacy callers use record[]. */
gh_mc_load_status gh_mc_selected_load_begin_using(gh_mc_selected_load *,gh_memory_card *,unsigned slot,
    uint8_t *record,size_t bytes);
gh_mc_load_status gh_mc_selected_load_step_wait_close(gh_mc_selected_load *,int allow_close,
    uint32_t *frontier);
/* Host cancellation after the task stops, before releasing its record. */
gh_mc_error gh_mc_selected_load_abort_io(gh_mc_selected_load *);
/* REL2C88 and Main2C06D0 through Heat restore and the 1F9CB0 next-room writes.
 * The canonical checkpoint bindings supply the existing CoreSave, room pages,
 * player, Heat, and stage work area. Returns UNSUPPORTED at 1F9F50.
 * A retry resumes at the first uncompleted phase, without replaying writes. */
gh_mc_load_status gh_mc_selected_load_apply_00002c88(gh_mc_selected_load *,
    gh_game_checkpoint *,const gh_checkpoint_bindings *,uint32_t *frontier);
/* Same restore cursor with the canonical Door owner. Stops at the original
 * audio mode setter after applying the door, clock and vitality progression. */
gh_mc_load_status gh_mc_selected_load_apply_with_door_00002c88(gh_mc_selected_load *,
    gh_game_checkpoint *,const gh_checkpoint_bindings *,gh_game_door *,uint32_t *frontier);
/* The provider owns all original active ADX players and applies 375700's
 * mode before the subsequent Work and cheat reset. Report a lower frontier
 * when an active player or mode cannot be applied. */
typedef gh_mc_load_status (*gh_mc_output_mode_apply)(void *,uint32_t mode,uint32_t *frontier);
gh_mc_load_status gh_mc_selected_load_restore_00002c88(gh_mc_selected_load *,
    gh_game_checkpoint *,const gh_checkpoint_bindings *,gh_game_door *,
    gh_mc_output_mode_apply,void *audio_context,uint32_t *frontier);
#ifdef __cplusplus
}
#endif
#endif
