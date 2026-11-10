#ifndef GH_GAME_CAMERA_PROJECTION_H
#define GH_GAME_CAMERA_PROJECTION_H
#include "game_camera_step.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Borrow scalars from ONE initialized native display_state owner. Projection
 * never derives another private screen/aspect state or assumes BSS0 is ready. */
typedef struct {const int32_t *screen_width;const uint32_t *screen_height_bits,*pixel_aspect_bits;
 const uint8_t *initialized;gh_camera_numeric_policy policy;} gh_camera_projection_display;
/* Actual138740 projection/view/composite/threeclip matrices. Nativebinary32
 * separately rounded arithmetic and original smallangle tan polynomial, not
 * VU accumulator/hardwareQ bitidentity. Active shake is explicit unsupported.
 * This computes genuine camera state; it neither draws nor acknowledges GPU. */
gh_camera_status gh_game_camera_project_00138740(gh_game_pl_camera *,
 const gh_camera_projection_display *,uint32_t *frontier);
/* Ordered138FC8 path/gates then projection at the actual138740 boundary. */
gh_camera_status gh_game_camera_step_projected_00138fc8(gh_game_pl_camera *,
 gh_game_init_state *,gh_acd_scene_binding *,const gh_title_player_view *,
 const gh_boot_work *,const gh_camera_projection_display *,uint32_t *frontier);
#ifdef __cplusplus
}
#endif
#endif
