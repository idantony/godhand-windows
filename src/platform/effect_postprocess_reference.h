#ifndef GH_EFFECT_POSTPROCESS_REFERENCE_H
#define GH_EFFECT_POSTPROCESS_REFERENCE_H
#include "../core/game_effects_trans.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef enum {GH_EFFECT_POST_OK,GH_EFFECT_POST_ARGUMENT,GH_EFFECT_POST_BOUNDS,
 GH_EFFECT_POST_UNSUPPORTED,GH_EFFECT_POST_MEMORY} gh_effect_post_status;
typedef enum {GH_EFFECT_SAMPLE_NATIVE_SEPARABLE_4BIT=1} gh_effect_sample_policy;
/* Raw GS RGBA bytes, NOT normalized/expanded alpha. Allocated rows may be
 * fewer than TEX0's declared texture rows; positive-weight missing texels
 * remain BOUNDS. Padding and unused rows are not fabricated or overwritten. */
typedef struct {uint8_t *rgba;size_t bytes,stride;uint32_t width,height;} gh_effect_post_surface;
typedef struct {uint64_t tex1,clamp;uint8_t known;gh_effect_sample_policy policy;} gh_effect_post_sampler;
typedef struct {gh_effect_post_surface surfaces[4];uint32_t xy_offset_x16,xy_offset_y16;
 uint32_t visible_width,visible_height;uint8_t frame_psm,colclamp,dthe,fba;} gh_effect_post_context;
typedef struct {uint64_t covered_fragments,color_writes;uint32_t quads;
 gh_effect_surface destination;uint8_t restores_scene_target,texture_flush_after;
 uint32_t restore_frame_mask;uint64_t restore_zbuf;} gh_effect_post_receipt;
/* Pixel centers match the existing native world profile: integer pixel*16,
 * no extra +8. Quantize interpolated FST UV to1/16, then nearest or separable
 * four-bit bilinear with X/Y truncation. This is an explicit native reference,
 * not a hardware-bitexact GS sampler claim. Zero-weight neighbors are unread. */
gh_effect_post_status gh_effect_post_sample(const gh_effect_post_surface *,uint32_t declared_width,
 uint32_t declared_height,const gh_effect_post_sampler *,int64_t u12_4,int64_t v12_4,uint8_t rgba[4]);
/* Consume ONE already OT-selected type6 packet. All surfaces stay caller-owned;
 * source/destination overlap is unsupported. Uses only invocation-local target
 * storage and commits atomically after all25 original-order quads succeed.
 * Tail restore metadata is returned for the ONE frontend state owner. */
gh_effect_post_status gh_effect_postprocess_apply(const gh_effect_trans_packet *,
 const gh_effect_post_sampler *,gh_effect_post_context *,gh_effect_post_receipt *,uint32_t *frontier);
/* Original 2FA620 copies eight source strips into the 256x448 warp surface.
 * A linear top sample can borrow an unavailable GS row. Leave that target
 * row untouched and let the warp consumer prove its footprint avoids it. */
gh_effect_post_status gh_effect_postprocess_capture_scene(const gh_effect_trans_packet *,
 const gh_effect_post_sampler *, const gh_effect_post_surface *scene,
 gh_effect_post_surface *capture, uint32_t x_origin, uint32_t y_origin, int *first_row_known);
/* Validate source-known rows only for raster-covered, source-dependent
 * samples of the original mode1 fan. Uses the shared native raster profile. */
gh_effect_post_status gh_effect_postprocess_warp_source_known(const gh_effect_trans_packet *,
 const gh_effect_post_sampler *,uint32_t width,uint32_t height,uint32_t x_origin,uint32_t y_origin,
 int first_row_known);
#ifdef __cplusplus
}
#endif
#endif
