#ifndef GH_STATIC_MESH_PROGRAM10_H
#define GH_STATIC_MESH_PROGRAM10_H
#include "static_mesh_transform.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Independently recovered actual program10/MSCAL0 straight strip path.
 * Same typed raw-VIF/GS boundary and explicit binary32 policy as program14.
 * Requires every transformed clip coordinate inside |W|. Finite signed
 * screenW / exactzero DIV saturation are supported in the bounded domain. Any clipping returns UNSUPPORTED at VU0308 atomically; no clipped
 * polygon is silently suppressed or delegated to another program.
 * This is a bounded subset, not the full clipping/fan implementation. */
gh_static_mesh_status gh_static_mesh_transform_program10_batch(
 const gh_scene_mesh_stream *,uint32_t node_flags,size_t batch,
 const gh_static_transform_inputs *,gh_static_transform_policy,
 gh_static_gs_vertex *,size_t vertex_capacity,uint32_t *,size_t index_capacity,
 gh_static_geometry_result *,gh_static_transform_frontier *);
#ifdef __cplusplus
}
#endif
#endif
