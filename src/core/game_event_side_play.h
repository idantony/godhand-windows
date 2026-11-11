#ifndef GH_GAME_EVENT_SIDE_PLAY_H
#define GH_GAME_EVENT_SIDE_PLAY_H

#include "bgm_requests.h"
#include "game_event_data_prefix.h"
#include "game_event_environment.h"
#include "game_scene_meshes.h"
#include "game_scene_ui.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    const gh_event_data_prefix *data;
    gh_game_world_light *world;
    gh_game_init_state *init;
    gh_core_save_view save;
    uint8_t stage;
} gh_event_scr_light;

/* Main 296D38 / 296E48. The absent EV01 track still installs and writes
 * its origin key in the same WorldLight, scroll counter and CoreSave. */
gh_game_event_status gh_event_scr_light_init_00008b20(gh_event_scr_light *,
    const gh_event_data_prefix *, gh_game_world_light *, gh_game_init_state *,
    gh_core_save_view, const gh_light_player_parts *, uint32_t *, int *relative);
gh_game_event_status gh_event_scr_light_move_00008ef8(gh_event_scr_light *, uint32_t *);

/* Main 296D5C / 296E6C. The Init still stops scene BGM while preserving
 * mode 5. EV01's absent track has no Move command, even on cut changes. */
gh_game_event_status gh_event_bgm_init_000046d8(const gh_event_data_prefix *,
    gh_bgm_requests *, uint32_t *, int *relative);
gh_game_event_status gh_event_bgm_move_00004708(const gh_event_data_prefix *, uint32_t *);

/* Main 296D74 / 296E84. Borrow the actual 754C38 registry and the one
 * owning scroll-mesh manager. The init snapshots visibility then applies
 * the EV01 origin. Move applies a mask only when it differs from the cache. */
gh_game_event_status gh_event_scr_layer_init_00008948(const gh_event_data_prefix *,
    const gh_room_pools *, const gh_scene_meshes *, uint32_t *, int *relative);
gh_game_event_status gh_event_scr_layer_move_00008a40(const gh_event_data_prefix *,
    const gh_room_pools *, const gh_scene_meshes *, uint32_t *, int *relative);

typedef gh_game_event_status (*gh_event_message_create)(
    void *, gh_game_scene_ui *, uint16_t message_id, uint16_t *work_id, uint32_t *frontier);
typedef struct {
    const gh_event_data_prefix *data;
    gh_game_scene_ui *ui;
    uint16_t message_id, work_id;
    uint8_t stage;
} gh_event_message_play;

/* Main 296D80 / 296E90. These inputs are the live cFilter_tag +08 flag
 * word and +3C alternate message id. The callback is the actual 2AEB68
 * operation with actor NULL, group 0 and priority 0. */
gh_game_event_status gh_event_message_init_00008100(gh_event_message_play *,
    const gh_event_data_prefix *, const gh_game_event_config *, gh_game_scene_ui *,
    uint32_t filter_flags, uint16_t filter_message, gh_event_message_create,
    void *context, uint32_t *, int *relative);
void gh_event_message_move_000081a8(void);

#ifdef __cplusplus
}
#endif
#endif
