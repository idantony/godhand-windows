#ifndef GH_UI_TEXT_RENDERER_H
#define GH_UI_TEXT_RENDERER_H
#include "../core/id_text.h"
#include "ui_renderer.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct gh_ui_text_renderer gh_ui_text_renderer;
/* Caches actualfont0 twoCBP palette atlases, hardwaredevice/shader/targets.
 * Nofont substitution. Unsupported original fontformats fail explicitly. */
gh_ui_error gh_ui_text_create(const gh_font_slot *,unsigned width,unsigned height,gh_ui_text_renderer **);
void gh_ui_text_destroy(gh_ui_text_renderer *);
/* Actualnearest font GSstate, originalsubpixelquads, ordered per-glyph integer
 * modulation/test/blend. Output/background rawGS RGBA32; origin12.4 explicit.
 * No spritebilinear or genericD3D alpha approximation. */
gh_ui_error gh_ui_text_batch(gh_ui_text_renderer *,const gh_ui_gs_state *,
 int32_t origin_x16,int32_t origin_y16,const gh_id_text_glyph *,size_t count,
 const unsigned char *background,unsigned char *output,size_t output_bytes);
#ifdef __cplusplus
}
#endif
#endif
