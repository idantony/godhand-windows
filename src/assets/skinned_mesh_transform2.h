#ifndef GH_SKINNED_MESH_TRANSFORM2_H
#define GH_SKINNED_MESH_TRANSFORM2_H
#include "skinned_mesh.h"
#include "static_mesh_transform.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Actual dynamic program2 VU0. Palette matrices are borrowed from the SAME
 * model's14C7E8 output; screen/clip and lights are original packet constants.
 * This is not the static10/14 pipeline and does not submit GPU work.
 * Finite signed screenW follows original DIV; zero gives signedfiniteMax in
 * the pinned numerical reference. CLIP/rawADC never alter logical counts;
 * unresolved UV or surviving-cross arithmetic retains precise frontiers. */
typedef struct {const uint8_t *palette;size_t palette_bytes,part_count;
 float screen[16],clip[16],normal_light[16],light_color[16],uv[2],fog;} gh_skinned_program2_inputs;
gh_static_mesh_status gh_skinned_transform_program2_batch(const gh_skinned_mesh_stream *,size_t batch,
 const gh_skinned_program2_inputs *,gh_static_transform_policy,gh_static_gs_vertex *,size_t,
 uint32_t *,size_t,gh_static_geometry_result *,gh_static_transform_frontier *);
#ifdef __cplusplus
}
#endif
#endif
