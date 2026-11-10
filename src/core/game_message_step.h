#ifndef GH_GAME_MESSAGE_STEP_H
#define GH_GAME_MESSAGE_STEP_H
#include "game_scene_ui.h"
#include "boot.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Actual2AEFA8: global5D8bit80000 precedes activehead traversal. Emptyhead
 * is the actual2AEA18 NULL state, not a second message-owner or fake update.
 * Active2B2438/update and2AEF10/recycle remain explicit frontier. */
gh_scene_ui_status gh_scene_message_update_002aefa8(const gh_game_scene_ui *,const gh_boot_work *,uint32_t *frontier);
#ifdef __cplusplus
}
#endif
#endif
