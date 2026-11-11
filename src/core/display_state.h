#ifndef GH_DISPLAY_STATE_H
#define GH_DISPLAY_STATE_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef enum {GH_DISPLAY_OK,GH_DISPLAY_ARGUMENT,GH_DISPLAY_BOUNDS,GH_DISPLAY_FORMAT,GH_DISPLAY_NUMERIC} gh_display_status;
/* Known scalar globals only, not a whole GS/display/font object. Canonical
 * native owner shared by UI/projection; numeric policy is binary32 nearest
 * reference, independent of caller FEnv, not console hardware validation. */
typedef struct {
 int32_t screen_width,screen_lines,gs_y_origin;
 uint32_t screen_height_bits,gs_y_origin_bits,pixel_aspect_bits;
 uint32_t font_batch_count[2];
 uint8_t loaded,initialized;
} gh_display_state;
gh_display_status gh_display_state_from_elf(gh_display_state *,const void *,size_t);
/* Actual static initializer2A48A8: conditional five writes, including both
 * font batch counters. The complete earlier static-constructor sequence is
 * still outside the developer startup host's claimed execution scope. */
gh_display_status gh_display_static_initialize_002a48a8(gh_display_state *,int initialize,int priority);
/* Default NTSC display copy 30B870, at destination (0,0). RGB from the
 * previous complete frame's first two rows at x=0. TEX0=0 uses TCC0. */
uint32_t gh_display_copy_base0_0030b870(const uint8_t top_rgb[3],const uint8_t next_rgb[3]);
#ifdef __cplusplus
}
#endif
#endif
