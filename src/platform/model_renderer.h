#ifndef GH_MODEL_RENDERER_H
#define GH_MODEL_RENDERER_H
#include "../assets/static_mesh_transform.h"
#include "../core/game_stage.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct gh_model_renderer gh_model_renderer;
typedef enum {GH_MODEL_GPU_OK,GH_MODEL_GPU_ARGUMENT,GH_MODEL_GPU_UNSUPPORTED,GH_MODEL_GPU_MEMORY,GH_MODEL_GPU_FAILURE} gh_model_gpu_error;
/* Explicit bounded native raster reference. It consumes recovered post-VU
 * PACKED XYZF2 values, not a guest DMA stream or a PS2 instruction runtime.
 * Float interpolation is a native reference, not a hardware-bitexact claim. */
typedef struct {
 uint64_t test,alpha,tex1,clamp;
 uint32_t xy_offset_x16,xy_offset_y16,fog_rgb;
 uint16_t primitive;
 uint8_t frame_psm,zbuf_psm,z_write,colclamp,dthe,fba,pabe;
 uint8_t native_float_reference;
} gh_model_draw_state;
gh_model_gpu_error gh_model_renderer_create(unsigned width,unsigned height,gh_model_renderer **);
void gh_model_renderer_destroy(gh_model_renderer *);
/* Actual decoded TM3 material, raw GS alpha. A successful call creates a real
 * D3D texture/view; cache_id is the same stage owner's nonrecycled identity. */
gh_model_gpu_error gh_model_renderer_upload(gh_model_renderer *,const gh_stage_material *,uint32_t *texture);
void gh_model_renderer_reset_resources(gh_model_renderer *);
gh_model_gpu_error gh_model_renderer_begin(gh_model_renderer *,uint32_t clear_rgb,uint32_t clear_z);
/* Triangle-list indices are the original post-VU strip/fan equivalent, in
 * original kick order. Caller preserves original OT group/setup traversal.
 * All inputs validate before GPU mutation; failures never acknowledge draws.
 * Known CT32 / Z16S surface, no dither, CLAMP0 repeat, TFX0/TCC1, MXL0 and
 * matching base min/mag filtering (including mip modes clamped to level0).
 * XYZF2 extracts low16XY, bits4..27 Z and bits4..11 fog. */
gh_model_gpu_error gh_model_renderer_draw(gh_model_renderer *,const gh_model_draw_state *,uint32_t texture,
 const gh_static_gs_vertex *,size_t vertex_count,const uint32_t *indices,size_t index_count);
gh_model_gpu_error gh_model_renderer_readback(gh_model_renderer *,uint8_t *rgba,size_t rgba_bytes,uint32_t *z,size_t z_count);
#ifdef __cplusplus
}
#endif
#endif
