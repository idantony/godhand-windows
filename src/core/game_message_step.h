#ifndef GH_GAME_MESSAGE_STEP_H
#define GH_GAME_MESSAGE_STEP_H
#include "game_scene_ui.h"
#include "boot.h"
#include "game_event_timing.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Actual 2AEFA8 Work gate and the EV01 window's first two 2B2438 updates.
 * The text timer/commands and recycle paths retain their exact frontiers. */
gh_scene_ui_status gh_scene_message_update_002aefa8(gh_game_scene_ui *,const gh_boot_work *,uint32_t *frontier);
/* Lend the original current EventEnv timing allocation for 297BF8 after
 * a real A301 confirmation. No timing owner is used while waiting. */
gh_scene_ui_status gh_scene_message_update_with_timing_002aefa8(gh_game_scene_ui *,
 const gh_boot_work *,gh_event_timing *,uint32_t *frontier);
/* Original lookup by generation at block+0, then immediate recycle on a match.
 * An already recycled work ID returns original false with native success. */
gh_scene_ui_status gh_scene_message_delete_002aebf8(gh_game_scene_ui *,
 uint16_t work_id, int *original_found, uint32_t *frontier);
/* Actual 2AF028 -> 2B16F0/2B17A8 pre-font path. The same owner receives
 * its geometry fields even when 2B1C80's text visibility bit is clear. */
gh_scene_ui_status gh_scene_message_prepare_draw_002af028(gh_game_scene_ui *,
 const gh_boot_work *,uint32_t *frontier);
#ifdef __cplusplus
}
#endif
#endif
