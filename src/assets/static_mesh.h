#ifndef GH_STATIC_MESH_H
#define GH_STATIC_MESH_H
#include "../core/game_scene_meshes.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Actual155800 semantic input, after VIF signed/unsigned UNPACK, before VU
 * ITOF/matrices/clip/GS conversion. These are NOT world-space vertices.
 * RawW is control data, never substituted for homogeneous position W. */
typedef struct {int32_t position[4],uv[2];uint32_t color[4];uint16_t raw_w;} gh_static_mesh_vertex;
typedef struct {size_t first_vertex,logical_vertices,fetched_vertices;
 size_t position_offset,uv_offset,color_offset;
 uint32_t position_ref_qwords,uv_ref_qwords,color_ref_qwords;
 uint32_t position_unpack,uv_unpack,color_unpack;
 uint64_t gif_strip[2],gif_fan[2];uint8_t material;} gh_static_mesh_batch;
typedef enum {GH_STATIC_OK,GH_STATIC_ARGUMENT,GH_STATIC_BOUNDS,GH_STATIC_UNSUPPORTED} gh_static_mesh_status;
/* Original batch max38, nextstart+36 (two vertex overlap), fetch rounded8.
 * Full rounded DMA spans are checked even where padding crosses logical
 * attribute arrays. Original bytes are preserved; no inventedzero padding. */
gh_static_mesh_status gh_static_mesh_batch_count(const gh_scene_mesh_stream *,size_t *);
gh_static_mesh_status gh_static_mesh_batch_at(const gh_scene_mesh_stream *,uint32_t node_flags,size_t batch,gh_static_mesh_batch *,uint32_t *frontier);
gh_static_mesh_status gh_static_mesh_vertex_at(const gh_scene_mesh_stream *,uint32_t node_flags,size_t vertex,gh_static_mesh_vertex *,uint32_t *frontier);
#ifdef __cplusplus
}
#endif
#endif
