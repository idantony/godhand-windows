#ifndef GH_SKINNED_MESH_BLEND_H
#define GH_SKINNED_MESH_BLEND_H
#include "skinned_mesh.h"
#include "static_mesh_transform.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct {float position[3],normal[3];uint32_t raw_control;} gh_skinned_blend_result;
/* Actual kernel2 VU0140..0230/0368..03C0 geometry only. Palette entries are
 * the original four-qword matrices, not inferred model/part transforms.
 * Skin Y/Z/W encode palette qword addresses; weight X/Y/Z are unsigned percent
 * bytes. Normal unpack is signed8 with original1/127 scaling. Neither weights
 * nor blended normals are normalized. Projection/light/clip/GIF remain outside
 * this leaf. Explicit finite nativebinary32 reference, not VU hardware proof. */
gh_static_mesh_status gh_skinned_mesh_blend_program2(const gh_skinned_mesh_vertex *,
 const float (*palette)[16],size_t matrix_count,gh_static_transform_policy,
 gh_skinned_blend_result *,uint32_t *vu_frontier);
#ifdef __cplusplus
}
#endif
#endif
