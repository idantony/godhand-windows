#ifndef GH_UI_FADE_RENDERER_H
#define GH_UI_FADE_RENDERER_H
#include "ui_renderer.h"
#include "../core/screen_fade.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct gh_ui_fade_renderer gh_ui_fade_renderer;
typedef struct {int32_t width,height,gs_y_origin;float gs_y_scale;uint32_t colclamp,prmodecont,pabe,fba,frame_mask;} gh_ui_fade_context;
/* Recover15C928->15B880 zero-anglequadpack foractualNTSC512x448fade.
 * ReturnsactualGSdecodedXYZlow16 XY (includingoriginalORpacking). Itmust
 * demonstrablycoverallviewportpixels; unknowngeometryreturnsUNSUPPORTED.
 * Global586B88 isexplicit, initializedby2A48A8 asheight/480. */
gh_ui_error gh_ui_fade_geometry(const gh_fade_draw*,const gh_ui_fade_context*,uint16_t xy[8]);
gh_ui_error gh_ui_fade_create(gh_ui_fade_renderer**);
void gh_ui_fade_destroy(gh_ui_fade_renderer*);
/* CachednativeD3D11 untexturedGS type0 ((Cs-Cd)*As>>7)+Cd, TESTalways,
 * rawRGBAalpha0..255 (noCSSalpha). CallerpreservesoriginalOTdraworder.
 * Actualinactive/gateddrawcopiesbackground. UnknownGSstatesfail, notgeneric
 * alphaapproximation. Output512x448rawGS RGBA, ownerthreadonly. */
gh_ui_error gh_ui_fade_apply(gh_ui_fade_renderer*,const gh_fade_draw*,const gh_ui_fade_context*,const unsigned char*background,unsigned char*output,size_t bytes);
/* Original kind0 completion packet: Z=FFFF, TEST=GREATER. Compare exact
 * CT32/Z16S depth from the same OT prefix. Caller must consume this as the
 * last packet, so the written Z has no further consumer in this frame. */
gh_ui_error gh_ui_fade_apply_with_depth(gh_ui_fade_renderer*,const gh_fade_draw*,const gh_ui_fade_context*,
 const unsigned char*background,unsigned char*output,size_t bytes,const uint32_t*z,size_t z_count);
#ifdef __cplusplus
}
#endif
#endif
