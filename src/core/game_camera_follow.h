#ifndef GH_GAME_CAMERA_FOLLOW_H
#define GH_GAME_CAMERA_FOLLOW_H
#include "game_camera_step.h"
#include "game_ray_query.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef gh_scene_ray_status (*gh_camera_follow_ray)(void *,gh_game_scene_tables *,const gh_scene_ray_request *,int *,uint32_t *);
/* One native continuation companion, not another camera/player/actor owner.
 * Borrowed player identity is installed only by the actual139DB8 call site.
 * Scratch persists across a failed ray so prefix writes are not repeated. */
typedef struct {gh_game_pl_camera *camera;const gh_title_player_view *player;
 uint8_t bound,stage;float weight,target[4],hit[4],up_global[3];
 gh_scene_ray_request ray;uint32_t frontier;} gh_camera_follow;
/* Recovered default(NULL,0)139DB8 including actual13B9B8 initial pose.
 * Invoke ONLY at the original caller (31C5C0 final call), never early boot.
 * Existing active-camera identity must be canonical. Raw pointer fields430/
 * 3F0 remain zero; the typed borrowed identity above replaces those links.
 * Other explicit target/copy-active-camera modes are outside this API. */
gh_camera_status gh_game_camera_bind_default_00139db8(gh_camera_follow *,gh_game_pl_camera *,const gh_game_init_state *,const gh_title_player_view *,const gh_boot_work *,const void *elf,size_t elf_bytes,uint32_t *frontier);
/* Actual ordinary139FF8 prefix through terrain rays and smoothed target/eye.
 * NULL ray callback invokes SAME canonical game_ray_query, never a fake miss.
 * Remaining collision-envelope/FOV/mode blend is a precise internal frontier,
 * not a completed camera update or permission to project/draw a frame. */
gh_camera_status gh_game_camera_follow_00139ff8(gh_camera_follow *,gh_game_scene_tables *,gh_camera_follow_ray,void *,uint32_t *frontier);
/* Existing strict API is preserved. Extended variant selects bound follow
 * only at its real139FF8 branch; other existing frontiers remain unchanged. */
gh_camera_status gh_game_camera_step_follow_00138fc8(gh_game_pl_camera *,gh_game_init_state *,gh_acd_scene_binding *,const gh_title_player_view *,const gh_boot_work *,gh_camera_follow *,gh_game_scene_tables *,gh_camera_follow_ray,void *,uint32_t *frontier);
#ifdef __cplusplus
}
#endif
#endif
