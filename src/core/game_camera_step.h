#ifndef GH_GAME_CAMERA_STEP_H
#define GH_GAME_CAMERA_STEP_H
#include "game_camera.h"
#include "game_scene_placements.h"
#include "game_title_player.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Borrow the SAME manager+FF4 ACD owner bound by137AE8 during room setup.
 * No duplicate path owner or guest pointer is created. field30 is only an
 * opaque diagnostic token; positive regions need a recovered typed selection. */
void gh_game_camera_path_construct_00137a48(gh_acd_scene_binding *);
gh_camera_status gh_game_camera_path_select_00137bb8(gh_acd_scene_binding *,
 const gh_title_player_view *,int *original_return,uint32_t *frontier);
/* Recovered zero-region path blend, exact operation order with explicit
 * existing CAMERA_NATIVE_BINARY32_REFERENCE policy. Not EE/VU bit identity. */
gh_camera_status gh_game_camera_path_step_00137f38(gh_game_pl_camera *,
 gh_acd_scene_binding *,const gh_title_player_view *,uint32_t *frontier);
/* Actual138FC8: path step THEN mode4 gate THEN activecamera138600.
 * Implemented prefix preserves real writes and returns the first unported
 * follow/projection consumer; an error must not be acknowledged as a frame. */
gh_camera_status gh_game_camera_step_00138fc8(gh_game_pl_camera *,
 gh_game_init_state *,gh_acd_scene_binding *,const gh_title_player_view *,
 const gh_boot_work *,uint32_t *frontier);
#ifdef __cplusplus
}
#endif
#endif
