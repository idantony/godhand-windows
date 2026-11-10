#ifndef GH_GAME_RENDER_PROJECTION_H
#define GH_GAME_RENDER_PROJECTION_H
#include "game_camera_projection.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct {
 float camera_clip[16]; /* original004A6900, copied activecamera+40 */
 float projection[16]; /* original004A6940 */
 uint8_t ready;
} gh_game_render_projection;
/* Actual1538F0, explicit native binary32 reference; borrows canonical active
 * PlCamera and display owner. No draw/clear/OT acknowledgement. The intermediate
 * inverse is overwritten by original30A5A0 identity before final field writes. */
gh_camera_status gh_game_render_projection_001538f0(gh_game_render_projection *,
 const gh_game_pl_camera *,const gh_camera_projection_display *,uint32_t *frontier);
#ifdef __cplusplus
}
#endif
#endif
