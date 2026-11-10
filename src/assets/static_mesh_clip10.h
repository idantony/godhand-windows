#ifndef GH_STATIC_MESH_CLIP10_H
#define GH_STATIC_MESH_CLIP10_H
#include "static_mesh_transform.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Actual0368..0FC8 program10 polygon path. Borrowed post-CLIP matrix values,
 * already FTOI0-quantized color, and pre-Q UV. Caller supplies original
 * triangle order selected by rawcontrol, not guessed strip parity. */
typedef struct {float clip[4],uv[2];uint32_t rgba[4];} gh_static_clip10_vertex;
/* Projection is node153B00+90 / VUmem16..19, distinct from M0/M1. */
typedef struct {float projection[16],fog[2];} gh_static_clip10_inputs;
/* Full sixplane convex clipping + original firstthree projected MACZ cull.
 * On success result.primitive=fan(7D/5D), triangles indexed 0,i-1,i.
 * Capacity9 vertices/21 indices is sufficient for a triangle in sixplanes.
 * Zero polygon/backface gives zero vertices/triangles. Explicit reference,
 * finite signed projectionW / exactzero DIV saturation in boundeddomain;
 * no EE/VU-bitexact claim; atomic outputs. */
gh_static_mesh_status gh_static_clip10_triangle(const gh_static_clip10_vertex triangle[3],
 const gh_static_clip10_inputs *,uint16_t primitive,gh_static_transform_policy,
 gh_static_gs_vertex *,size_t,uint32_t *,size_t,gh_static_geometry_result *,
 gh_static_transform_frontier *);
#ifdef __cplusplus
}
#endif
#endif
