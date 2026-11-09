#ifndef GH_ID_TEXTURE_POLYGON_H
#define GH_ID_TEXTURE_POLYGON_H
#include "id_jobs.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct {
 gh_id_texture texture;float x[4],y[4],u[4],v[4];
 uint32_t color;uint8_t color_mode,alignment;int16_t font_mode;uint16_t clip;
} gh_id_texture_polygon;
/* Actual2AAEF0->2C5EA8 alignment0/3, f80=1 and drawInit scale/shift1.
 * Explicit original9th-degree nativebinary32 reference, no libc sin/cos.
 * Triangle strip order0,1,2,3 retained. Does not submit a draw. */
gh_id_error gh_id_texture_polygon_prepare(const gh_id_state*,const gh_id_base*,
 const gh_id_entry*,gh_id_texture_polygon*);
/* Existing verifiedformats plus actual64x64 indexed4 TIM2 HUD icon at
 * language TBL1 item1. Old quad/texture ABI and wrappers are untouched. */
gh_id_error gh_id_polygon_texture_open(const gh_id_state*,uint32_t,uint32_t,gh_id_texture*);
gh_id_error gh_id_polygon_texture_rgba(const gh_id_texture*,void*,size_t);
#ifdef __cplusplus
}
#endif
#endif
