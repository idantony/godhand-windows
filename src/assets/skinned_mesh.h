#ifndef GH_SKINNED_MESH_H
#define GH_SKINNED_MESH_H
#include "model.h"
#include "static_mesh.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Actual154E98 float-position skeletalMDB input; rawcontrolW isNOT floatW.
 * Remainingrawstreams retain VIF U8/S16 semantics, not guessed skinresults. */
typedef struct {const uint8_t *bytes;size_t size,data_offset,header_offset;
 size_t position_offset,normal_offset,uv_offset,color_offset,skin_offset;
 uint16_t vertices,parts;uint8_t material;uint32_t parity_packet_bytes;} gh_skinned_mesh_stream;
typedef struct {size_t first_vertex,logical_vertices,fetched_vertices;
 size_t position_offset,normal_offset,uv_offset,color_offset,skin_offset;
 uint32_t position_qwords,normal_qwords,uv_qwords,color_qwords,skin_qwords;
 uint32_t position_unpack,normal_unpack,uv_unpack,color_unpack,skin_unpack;
 uint64_t gif_strip[2],gif_fan[2];} gh_skinned_mesh_batch;
typedef struct {float position[3];uint32_t raw_control;uint8_t normal[4],color[4],bone_indices[4],weights[4];int32_t uv[2];} gh_skinned_mesh_vertex;
gh_static_mesh_status gh_skinned_mesh_stream_count(const gh_model_view *,size_t mesh,size_t *);
gh_static_mesh_status gh_skinned_mesh_stream_at(const gh_model_view *,size_t mesh,size_t stream,gh_skinned_mesh_stream *);
/*1551E0 ceil(vertices/24)*90+10,1551C0 adds40. Batchmin26/start+24,
 * fetchrounded8. Physicalfetchspans mayoverlapadjacentattribute bytes;
 * boundedwholeMD source isrequired, not synthesizedzero padding. */
gh_static_mesh_status gh_skinned_mesh_batch_count(const gh_skinned_mesh_stream *,size_t *);
gh_static_mesh_status gh_skinned_mesh_batch_at(const gh_skinned_mesh_stream *,size_t,gh_skinned_mesh_batch *);
gh_static_mesh_status gh_skinned_mesh_vertex_at(const gh_skinned_mesh_stream *,size_t,gh_skinned_mesh_vertex *);
#ifdef __cplusplus
}
#endif
#endif
