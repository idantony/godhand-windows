#ifndef GH_UI_POLYGON_RENDERER_H
#define GH_UI_POLYGON_RENDERER_H
#include "ui_sprite_renderer.h"
#include "../core/id_texture_polygon.h"
#include "../core/cockpit_fan.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct gh_ui_polygon_renderer gh_ui_polygon_renderer;
typedef struct {uint32_t texture;gh_id_texture_polygon polygon;} gh_ui_polygon_item;
/* Only the proven NTSC512x448 UI surface/origin/scissor profile is accepted. */
gh_ui_error gh_ui_polygon_create(unsigned width,unsigned height,gh_ui_polygon_renderer**);
void gh_ui_polygon_destroy(gh_ui_polygon_renderer*);
/* Restricted actual64x64 HUD TIM2/PSMT4; alpha preview must losslessly map
 * back to GS0..128 (even or255). Cached native device/shader/texture lifetime. */
gh_ui_error gh_ui_polygon_upload(gh_ui_polygon_renderer*,const gh_id_texture*,
 const unsigned char*,size_t,uint32_t*);
/* Native reference raster: original GS12.4 truncation, integer sample centers,
 * top-left triangle strip coverage, rational16.16 ST, manual4bitbilinear and
 * explicit GS integer modulation/test/blend. No generic D3D alpha/filter.
 * Visibledepth unsupported; same proven UI state as15BCF0/2C64EC. All items
 * validated before any submission; ordered composition onto actual background. */
gh_ui_error gh_ui_polygon_batch(gh_ui_polygon_renderer*,const gh_ui_sprite_state*,
 const gh_ui_polygon_item*,size_t,const unsigned char*,unsigned char*,size_t);
/* Separate actual1430E8/15C410 profile:16x16indexed4TIM2 and one33vertex
 * triangle-fan packet. Bank/depth group ordering remains caller's OT job.
 * Same manual GS fragment arithmetic, PRIM55 (fan), no generic D3D blend. */
gh_ui_error gh_ui_polygon_upload_fan16(gh_ui_polygon_renderer*,const gh_id_texture*,
 const unsigned char*,size_t,uint32_t*);
/* Actual numeric4x4 glyph atlas,128x128 TIM2/PSMT4 or T32/PSMT8 only. */
gh_ui_error gh_ui_polygon_upload_number128(gh_ui_polygon_renderer*,const gh_id_texture*,
 const unsigned char*,size_t,uint32_t*);
gh_ui_error gh_ui_polygon_fan(gh_ui_polygon_renderer*,const gh_ui_sprite_state*,
 uint32_t texture,const gh_cockpit_fan*,const unsigned char*,unsigned char*,size_t);
#ifdef __cplusplus
}
#endif
#endif
