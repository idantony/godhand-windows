#ifndef GH_GAME_SCENARIO_END_SOFT_EVENT_H
#define GH_GAME_SCENARIO_END_SOFT_EVENT_H

#include "game_scenario_soft_event.h"
#include "game_event_start.h"
#include "game_init.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef gh_game_event_status (*gh_soft_end_leaf)(void *, uint32_t *frontier);
typedef gh_game_event_status (*gh_soft_end_player_leaf)(
    void *, const gh_title_player_view *, uint32_t *frontier);

typedef struct {
    gh_scene_scenario_view scenario;
    gh_boot_work *work;
    const gh_title_player_view *current_player;
    const gh_event_start_bindings *room_objects;
    gh_soft_end_player_leaf player_clear_0012c418;
    gh_soft_end_leaf camera_pose_0013b9b8, camera_switch_00138d40;
    gh_soft_end_leaf effect_unsuspend_002fa470, enemies_unsuspend_002948e8;
    gh_soft_end_player_leaf player_unsuspend_0012c7d0;
    void *context;
} gh_soft_end_bindings;

typedef struct {
    gh_scene_scenario_view scenario;
    gh_boot_work *work;
    const gh_title_player_view *player;
    uint32_t kind;
    uint8_t stage;
} gh_soft_event_end;

/* One original call per zeroed cursor. Missing operations retain their
 * exact original addresses. Retry the same invocation until it completes. */
gh_game_event_status gh_scenario_end_soft_event_002c3c10(
    gh_soft_event_end *, const gh_soft_end_bindings *, uint32_t *frontier);

/* Original list head +500 empty path. Positive entries need their real
 * actor virtual setSuspend provider; raw guest tokens are never dereferenced. */
gh_game_event_status gh_soft_enemies_unsuspend_empty_002948e8(
    const gh_game_init_state *, uint32_t *frontier);
typedef struct {
    const gh_title_player_view *child_5f8, *object_698, *object_6a0, *weapon_6a8;
    uint8_t known_mask; /* bits 0..3 respectively; known NULL is allowed */
} gh_soft_player_suspend_links;

/* Actual PL00 VT+64 with argument zero. The generic cObjBase VT slot is
 * supported for typed attachments whose descriptor resolves to 14E930. */
gh_game_event_status gh_soft_player_unsuspend_0012c7d0(
    const gh_title_player_view *, const gh_soft_player_suspend_links *,
    uint32_t *frontier);
#ifdef __cplusplus
}
#endif
#endif
