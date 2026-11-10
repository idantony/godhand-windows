#ifndef GH_SKINNED_MESH_TRANSFORM30_H
#define GH_SKINNED_MESH_TRANSFORM30_H
#include "skinned_mesh_transform2.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct {gh_skinned_program2_inputs base;float half_vector[3];uint16_t squarings;} gh_skinned_program30_inputs;
/* Actual program30 shares skin/projection/diffuse geometry with2 but computes
 * highlight alpha BEFORE integer conversion. It preserves original repeated
 * squaring, not pow(x, squarings). TFX3 GPU consumption is outside this API. */
gh_static_mesh_status gh_skinned_transform_program30_batch(const gh_skinned_mesh_stream *,size_t,
 const gh_skinned_program30_inputs *,gh_static_transform_policy,gh_static_gs_vertex *,size_t,
 uint32_t *,size_t,gh_static_geometry_result *,gh_static_transform_frontier *);
#ifdef __cplusplus
}
#endif
#endif
