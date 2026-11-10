#ifndef GH_UI_SPRITE_RENDERER_H
#define GH_UI_SPRITE_RENDERER_H
#include "../core/id_jobs.h"
#include "ui_renderer.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct gh_ui_sprite_renderer gh_ui_sprite_renderer;
typedef struct {uint64_t alpha,test;uint32_t prim,tex1,clamp,colclamp,prmodecont,pabe,fba,frame_mask;} gh_ui_sprite_state;
typedef struct {uint32_t texture;gh_id_texture_quad quad;} gh_ui_sprite_item;
/* Hardware device/shader/target lifetime persists across batches. Texture
 * uploads are explicit and cached by returned handle until destroy. */
gh_ui_error gh_ui_sprite_create(unsigned width,unsigned height,gh_ui_sprite_renderer **);
void gh_ui_sprite_destroy(gh_ui_sprite_renderer *);
/* Original T32/PSMT8 512x256/512, T32/PSMT4 512x512titlelogo,
 * andverified32x32TIM2/PSMT4cursor. QA alpha must be even
 * or255 and is losslessly reversed to GS0..128 before integer filtering. */
gh_ui_error gh_ui_sprite_upload(gh_ui_sprite_renderer *,const gh_id_texture *,
 const unsigned char *rgba,size_t bytes,uint32_t *handle);
/* Original centeredzeroangle quads, 448-line UI coordinates, GS vftoi4 edges,
 * normalized ST interpolated16.16, wrap, 4-bit bilinear, integer GS modulation/
 * test/blend. Preserves supplied item order. Background/output rawGS RGBA32.
 * Caller provides required globalcontext; this is not completeGS emulation. */
gh_ui_error gh_ui_sprite_batch(gh_ui_sprite_renderer *,const gh_ui_sprite_state *,
 const gh_ui_sprite_item *,size_t count,const unsigned char *background,
 unsigned char *output,size_t output_bytes);
gh_ui_error gh_ui_sprite_batch_with_depth(gh_ui_sprite_renderer *,const gh_ui_sprite_state *,
 const gh_ui_sprite_item *,size_t,const unsigned char *,unsigned char *,size_t,const gh_ui_depth_write *);
#ifdef __cplusplus
}
#endif
#endif
