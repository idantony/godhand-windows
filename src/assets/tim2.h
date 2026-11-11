#ifndef GH_TIM2_H
#define GH_TIM2_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef enum {GH_TIM2_OK,GH_TIM2_ARGUMENT,GH_TIM2_FORMAT,GH_TIM2_BOUNDS,GH_TIM2_UNSUPPORTED} gh_tim2_error;
typedef struct {
 uint16_t width,height,colors;
 uint8_t image_type,clut_type;
 uint64_t tex0;
 const unsigned char *image,*clut;
 size_t image_size,clut_size;
} gh_tim2_picture;
/* Validated views into caller-owned immutable bytes. Supports first picture
 * indexed4/indexed8 with one mip level; no swizzle/CLUT order guessed. */
gh_tim2_error gh_tim2_first(const void *bytes,size_t size,gh_tim2_picture *out);
/* Asset QA conversion: indexed4, even-width linear PSMT4 upload, 16 RGBA32
 * colors, CSM1/CSA0. GS alpha 0..128 maps to host 0..255 by min(2*a,255).
 * This is texture sampling preview, not final game blend/shader output. */
gh_tim2_error gh_tim2_rgba4(const gh_tim2_picture *,void *rgba,size_t capacity);
/* Linear PSMT8 upload with the physical 16x16 RGBA32 CSM1 palette.
 * Alpha uses the same GS 0..128 to host 0..255 convention as indexed4. */
gh_tim2_error gh_tim2_rgba8(const gh_tim2_picture *,void *rgba,size_t capacity);
#ifdef __cplusplus
}
#endif
#endif
