#ifndef GH_STATIC_MESH_TRANSFORM_H
#define GH_STATIC_MESH_TRANSFORM_H
#include "static_mesh.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Explicit numerical replacement for this original VU program. Zero/default
 * policy never silently selects a host model. Not PS2 hardware-bitexact. */
typedef enum {GH_STATIC_BINARY32_REFERENCE=1} gh_static_transform_policy;
/* Borrow the actual packet constants, not a second camera/model owner:
 * screen_matrix = VUmem0..3; clip_matrix =4..7, column-major;
 * uv_offset = VUmem14.xy (node+70); color_scale =VUmem15(node+80);
 * fog = VUmem8.w/9.w(node+1C/+2C), computing A+B*screenW. */
typedef struct {
 float screen_matrix[16],clip_matrix[16];
 float uv_offset[2],color_scale[4],fog[2];
} gh_static_transform_inputs;
typedef struct {
 float stq[3];uint32_t rgba[4];int32_t xyz_fixed4[3];uint16_t fog_fixed4;
 uint16_t raw_control;uint8_t clip_flags,adc,triangle_tested;
} gh_static_gs_vertex;
typedef struct {size_t vertices,triangles;uint8_t material;uint16_t primitive;} gh_static_geometry_result;
typedef struct {uint32_t ee_entry,vu_byte_pc;} gh_static_transform_frontier;
/* Actual firstR100 layer14,155DC8->403BB0 MPG/403F40 MSCAL400 (VUbyte2000).
 * Bounded static V4_16 input only. Coordinates are original GS integer12.4
 * outputs, UV is original STQ, colors remain original0..255 GS values; no
 * D3D viewport/depth/color conversion is invented. Indices are batch-local
 * triangle-list equivalents of the original strip, already ADC/clip/backface
 * filtered. Original packet's first2 vertices cannot kick a GS triangle, so
 * triangle_tested=0; their unknown previous-packet flag history is not claimed.
 * Positive finite screenW only; unsupported numerics return exact VU PC.
 * Output buffers/result are committed only after complete success. */
gh_static_mesh_status gh_static_mesh_transform_batch(const gh_scene_mesh_stream *,
 uint32_t node_flags,size_t batch,const gh_static_transform_inputs *,gh_static_transform_policy,
 gh_static_gs_vertex *vertices,size_t vertex_capacity,uint32_t *indices,size_t index_capacity,
 gh_static_geometry_result *,gh_static_transform_frontier *);
#ifdef __cplusplus
}
#endif
#endif
