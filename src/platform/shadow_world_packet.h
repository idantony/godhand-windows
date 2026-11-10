#ifndef GH_SHADOW_WORLD_PACKET_H
#define GH_SHADOW_WORLD_PACKET_H
#include "../core/game_actor_shadow_draw.h"
#include "../assets/static_mesh_transform.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef enum {GH_SHADOW_PACKET_OK,GH_SHADOW_PACKET_ARGUMENT,GH_SHADOW_PACKET_UNSUPPORTED} gh_shadow_packet_status;
typedef struct {gh_shadow_group group;gh_static_gs_vertex vertices[48];uint32_t indices[72];
 size_t vertex_count,triangles;} gh_shadow_world_packet;
/* Capture the original PACKED XYZ2/ST/RGBAQ group, not XYZF2. Integer Z is
 * explicitly converted to the existing raster reference's fixed4 carrier;
 * only its supported0..65535 domain is admitted. Proved zero-area triangles
 * have no color/depth samples and are omitted from raster input, while the
 * ONE original OT group/state is retained even when all12 slots are disabled.
 * Output is atomic on failure. No GPU execution is implied. */
gh_shadow_packet_status gh_shadow_world_packet_capture(const gh_shadow_group *,gh_shadow_world_packet *,uint32_t *frontier);
#ifdef __cplusplus
}
#endif
#endif
