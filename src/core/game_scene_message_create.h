#ifndef GH_GAME_SCENE_MESSAGE_CREATE_H
#define GH_GAME_SCENE_MESSAGE_CREATE_H

#include "boot.h"
#include "game_scene_ui.h"
#include "game_event.h"

#ifdef __cplusplus
extern "C" {
#endif

/* The original EventMessage call uses NULL actor, group 0 and priority 0.
 * Return FFFF for the original full-pool branch through work_id. The owner
 * retains borrowed bounded MSD references until window release/reset. */
gh_scene_ui_status gh_scene_message_create_002aeb68(gh_game_scene_ui *,
    const gh_boot_work *, uint16_t message_id, uint16_t *work_id, uint32_t *frontier);

/* Direct adapter for gh_event_message_init_00008100. Context is canonical Work. */
gh_game_event_status gh_scene_event_message_create_002aeb68(void *work, gh_game_scene_ui *,
    uint16_t message_id, uint16_t *work_id, uint32_t *frontier);

#ifdef __cplusplus
}
#endif
#endif
