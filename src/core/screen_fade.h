#ifndef GH_SCREEN_FADE_H
#define GH_SCREEN_FADE_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef enum {GH_FADE_OK,GH_FADE_ARGUMENT,GH_FADE_UNSUPPORTED} gh_fade_error;
/* OriginalclassFADE hasnoforeignpointers; explicitPS2scalarwidths retain
 * exact0x28layout without transplantingPS2long intoWindowslong. */
typedef struct {uint64_t alpha;uint16_t x,y,w,h;uint32_t color_from,color_to;uint16_t time,count;uint8_t state,kind;uint16_t unknown_1e;uint32_t ot_min,ot_max;} gh_screen_fade;
typedef struct {int32_t screen_width,gs_y_origin;uint32_t frame_shift,input_mask_5e0;} gh_screen_fade_context;
typedef struct {uint64_t alpha,test;uint32_t rgba,ot_min,ot_max;uint16_t x,y,w,h;uint8_t kind,visible;} gh_fade_draw;
gh_fade_error gh_screen_fade_start_00307208(gh_screen_fade*,const gh_screen_fade_context*,uint8_t kind,uint8_t state,uint32_t type,uint32_t color_from,uint32_t color_to,uint16_t frames);
void gh_screen_fade_kill_003076e0(gh_screen_fade*);
uint8_t gh_screen_fade_status_00307718(const gh_screen_fade*);
void gh_screen_fade_set_ot_00307760(gh_screen_fade*,uint32_t,uint32_t);
/* Actual307388: independentintegerbyteinterpolation, count/stateupdates,
 * sourceALPHA/TEST/rectangle/OTdrawarguments. Inactive/gatedemitsnovisible
 * draw, notafabricatedcompletedfade. Finalcolorisdrawnevenifstatebecomes0.
 * Hardwarepacketallocation/renderingbelongsrequirednativeconsumer. */
gh_fade_error gh_screen_fade_step_00307388(gh_screen_fade*,const gh_screen_fade_context*,gh_fade_draw*);
#ifdef __cplusplus
}
#endif
#endif
