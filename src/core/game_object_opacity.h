#ifndef GH_GAME_OBJECT_OPACITY_H
#define GH_GAME_OBJECT_OPACITY_H
#include "game_camera.h"
#include "game_title_player.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct {const gh_game_init_state *init;const gh_game_pl_camera *camera;
 const gh_title_player_view *current_player;uint8_t player_binding_known;} gh_object_opacity_environment;
/* Actual2A87E8, originalposition=NULL uses selfF0. Distinct from player123160.
 * Caller borrows canonical Work/currentplayer/activecamera; no rawpointer
 * casts or assumedNULLs. Explicit finite nearestbinary32 reference and FEnv
 * preservation, not full EE hardwarefloat equivalence. */
gh_camera_status gh_game_object_opacity_002a87e8(const gh_title_player_view *,const float override_xyz[3],const gh_boot_work *,const gh_object_opacity_environment *,gh_camera_numeric_policy,uint32_t *frontier);
#ifdef __cplusplus
}
#endif
#endif
