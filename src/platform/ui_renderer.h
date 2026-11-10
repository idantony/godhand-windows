#ifndef GH_UI_RENDERER_H
#define GH_UI_RENDERER_H
#include "../core/font_draw.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef enum {GH_UI_OK,GH_UI_ARGUMENT,GH_UI_UNSUPPORTED,GH_UI_GPU,GH_UI_MEMORY} gh_ui_error;
/* Original TEST=ALWAYS/AFAIL=KEEP, write enabled, same Z16S OT plane.
 * The backend writes z only where its original draw passes alpha test. */
typedef struct {uint32_t *pixels;size_t count;uint32_t z;} gh_ui_depth_write;
typedef struct {uint64_t alpha,test,tex0;uint32_t prim,tex1,clamp,colclamp;
 uint32_t prmodecont,pabe,fba,frame_mask;} gh_ui_gs_state;
/* Restricted single original-font sprite fragment pipeline on hardware D3D11.
 * Integer pixel edges, 1:1 nearest atlas samples only. atlas is QA RGBA8 with
 * alpha min(GSalpha*2,255), background/output are raw GS RGBA32 bytes.
 * Requires source-supported GS state below; no depth scene or guessed state.
 * Origin arguments convert original GS12.4 screen coordinates to target pixels.
 * GPU implementation uses compute fragments for explicit GS integer blending;
 * no generic D3D alpha blend approximates the recovered equation. */
gh_ui_error gh_ui_font_quad(const gh_font_quad *,const gh_ui_gs_state *,
 int32_t origin_x16,int32_t origin_y16,unsigned width,unsigned height,
 const unsigned char *atlas512x256,const unsigned char *background,
 unsigned char *output,size_t output_capacity);
#ifdef __cplusplus
}
#endif
#endif
