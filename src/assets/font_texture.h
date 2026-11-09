#ifndef GH_FONT_TEXTURE_H
#define GH_FONT_TEXTURE_H
#include "../core/font_state.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct {uint16_t width,height;uint8_t psm;uint16_t destination_base;uint8_t buffer_width;const unsigned char *pixels;size_t pixel_bytes;} gh_font_transfer;
/* Original font MST contains CT32 uploads, later sampled as PSMT4.
 * This extracts actual transfer pixels, not a falsely linear final atlas. */
gh_font_error gh_font_transfer_at(const gh_font_slot *,unsigned table,unsigned index,gh_font_transfer *);
/* Restricted original USA atlas, PSMCT32 upload -> PSMT4 512x256 sampling.
 * CSA0 first CLUT load only; produces texture QA RGBA, not glyph draw output. */
gh_font_error gh_font_atlas_rgba(const gh_font_slot *,void *,size_t);
gh_font_error gh_font_atlas_bank_rgba(const gh_font_slot *,unsigned parity,void *,size_t);
/* Original glyph code parity chooses CBP+0/+1; paired codes share a cell.
 * Returns cropped unscaled texture primitive, without game tint/blending. */
gh_font_error gh_font_glyph_rgba(const gh_font_state *,uint16_t code,void *,size_t,unsigned *width,unsigned *height);
#ifdef __cplusplus
}
#endif
#endif
