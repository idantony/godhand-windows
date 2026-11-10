#ifndef GH_GAME_WORLD_LIGHT_STEP_H
#define GH_GAME_WORLD_LIGHT_STEP_H
#include "game_world_light.h"
#include "game_room_pools.h"
#include "game_camera.h"
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
#ifdef __cplusplus
}
#endif
#endif
