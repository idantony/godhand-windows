#ifndef GH_GAME_EFFECT_SPRITE_H
#define GH_GAME_EFFECT_SPRITE_H
#include "game_esp_create.h"
#include "display_state.h"
#include "frame_state.h"
#include "game_camera.h"
#include "game_stage.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct {const gh_game_pl_camera *camera;const gh_game_init_state *init;const gh_display_state *display;
 gh_static_transform_policy policy;gh_frame_numeric_policy cvt_policy;} gh_effect_sprite_camera;
typedef struct {int32_t xyz_fixed4[4][3],original_depth;float q[4];uint8_t visible;
 int32_t center_fixed4[3];float center_q;} gh_effect_sprite_projection;
/* Original2FE0D8 bounded projectedquad domain. SAME CNode cache/identity,
 * no renderer/material acknowledgment. Clipped quads genuinely returnvisible0. */
gh_effect_status gh_effect_sprite_project_002fe0d8(gh_game_effects *,unsigned,uint32_t,const gh_effect_sprite_camera *,gh_effect_sprite_projection *,uint32_t *frontier);
/* Existing SINGLE cTexture owner handles the actual customcount1 format2
 * source; returns its own material handle/pixels. Never creates a new cache. */
gh_effect_status gh_effect_sprite_material_00151398(gh_stage_textures *,gh_effect_view,unsigned original_mode,const gh_stage_material **,uint32_t *frontier);
#ifdef __cplusplus
}
#endif
#endif
