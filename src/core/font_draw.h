#ifndef GH_FONT_DRAW_H
#define GH_FONT_DRAW_H
#include "font_state.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef enum {GH_FONT_DRAW_OK,GH_FONT_DRAW_ARGUMENT,GH_FONT_DRAW_UNSUPPORTED,GH_FONT_DRAW_BOUNDS} gh_font_draw_error;
typedef struct {
 float scale_x,scale_y; /* original object+2A068/+2A06C */
 uint32_t alignment_flags; /* +2A070 */
 int32_t screen_width,screen_height,gs_y_origin; /* globals3C1180/118C/586B80 */
 uint32_t rgba; /* raw original RGBAQ input, GS neutral RGB is128; not RGBA8 */
} gh_font_draw_state;
typedef struct {
 uint16_t x0,y0,x1,y1; /* GS12.4 coordinate payload, origin already added */
 uint16_t u0,v0,u1,v1; /* original12.4 UV with halftexel+8 */
 uint32_t z,rgba;
 uint16_t advance;
 uint8_t slot,palette_parity;
} gh_font_quad;
/* Recovered screen/texture quad stage of original2A53A8 and color2A52E8.
 * No target submission, texture tint approximation, or guessed GS blend.
 * Original code0 special layout remains unsupported. State buffers stay owned
 * by caller. Safe finite range only; no claim of complete EE FPU modeling. */
gh_font_draw_error gh_font_draw_quad(const gh_font_state *,const gh_font_draw_state *,
 uint16_t code,float x,float y,float z,gh_font_quad *);
#ifdef __cplusplus
}
#endif
#endif
