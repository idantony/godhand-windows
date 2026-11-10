#ifndef GH_STATIC_MESH_PROGRAM10_PACKETS_H
#define GH_STATIC_MESH_PROGRAM10_PACKETS_H
#include "static_mesh_program10.h"
#include "static_mesh_clip10.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Original XGKICK ordering: zero or more clipped fan packets in encountered
 * triangle order, then the original strip packet. Index values are LOCAL to
 * each range's vertex span. No crosspacket reordering or state coalescing. */
typedef struct {size_t vertex_offset,vertices,index_offset,triangles;uint16_t primitive;uint8_t material;} gh_static_packet_range;
typedef struct {size_t vertices,triangles,packets;} gh_static_packet_result;
/* Maximum onebatch:362 vertices,864 indices,37 packets. Actual outputcounts
 * determine capacity checks. Borrows canonical153B00 projection separately.
 * Existing binary32/nonfinite/projectionW limits retained; atomic outputs. */
gh_static_mesh_status gh_static_mesh_program10_packets(const gh_scene_mesh_stream *,uint32_t node_flags,size_t batch,
 const gh_static_transform_inputs *,const float projection[16],gh_static_transform_policy,
 gh_static_gs_vertex *,size_t,uint32_t *,size_t,gh_static_packet_range *,size_t,
 gh_static_packet_result *,gh_static_transform_frontier *);
#ifdef __cplusplus
}
#endif
#endif
