#ifndef GH_GAME_WORLD_LIGHT_STEP_H
#define GH_GAME_WORLD_LIGHT_STEP_H
#include "game_world_light.h"
#include "game_room_pools.h"
#include "game_camera.h"
#include "game_scene_at.h"
#include "game_title_player.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Actual30E5C0 state writes to SAME Work270/3E0 GS context words and60C RGB.
 * No draw/present is acknowledged. Unknown modes are original genuine no-op. */
gh_light_status gh_game_set_background_0030e5c0(gh_boot_work *,unsigned mode,uint8_t red,uint8_t green,uint8_t blue);
/* Actual2D7560 finite nonfading domain: noFAR area, canonical registry3 empty,
 * no light expiry timers or animated100000 extras. Real13R100 light records
 * are retained, not discarded. Fog0.96/0.04 and background writes are real;
 * active unknown branches return precise consumer before native commits. */
gh_light_status gh_game_world_light_move_002d7560(gh_game_world_light *,gh_boot_work *,
 const gh_room_pools *,gh_camera_numeric_policy);
typedef gh_light_status (*gh_light_actor_view)(void *,uint32_t handle,gh_title_player_view *,uint32_t *frontier);
typedef struct {const gh_scene_at_manager *scene_at;gh_light_actor_view actor_view;void *context;} gh_light_move_bindings;
/* Same model registry3 and actor bytes, plus original LAR region selection.
 * Missing typed resolution/visibility-mask backing is an explicit frontier.
 * Native preflight commits no fog or actor writes on an unsupported branch. */
gh_light_status gh_game_world_light_move_native_002d7560(gh_game_world_light *,gh_boot_work *,
 const gh_room_pools *,const gh_light_move_bindings *,gh_camera_numeric_policy);
int gh_game_world_light_class_002d74b8(uint16_t original_kind);
#ifdef __cplusplus
}
#endif
#endif
