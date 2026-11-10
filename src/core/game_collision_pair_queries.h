#ifndef GH_GAME_COLLISION_PAIR_QUERIES_H
#define GH_GAME_COLLISION_PAIR_QUERIES_H
#include "../assets/static_mesh_transform.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef enum {GH_PAIR_QUERY_OK,GH_PAIR_QUERY_ARGUMENT,GH_PAIR_QUERY_UNSUPPORTED} gh_pair_query_status;
/* Actual2BB670 ->1566C0/1565A0, explicit binary32 reference for bounded
 * finite normal/zero input domain. Uses original endpoint selection and <=,
 * not a replacement raycast. Returns no hit only after the real predicate. */
gh_pair_query_status gh_collision_query_sphere_cylinder_002bb670(const uint8_t sphere_position[12],uint32_t sphere_radius,const uint8_t cylinder_matrix[64],uint32_t cylinder_radius,uint32_t height,gh_static_transform_policy,int *hit,uint32_t *distance_bits,uint32_t *frontier);
/* Actual2BBBA0 ->2BB760 ->2BB868: six normalized face-plane tests on
 * transformed eight corners. Preserves original expanded-plane behavior;
 * this is deliberately not replaced with nearest-point sphere/OBB math. */
gh_pair_query_status gh_collision_query_sphere_box_002bbba0(const uint8_t sphere_position[12],uint32_t sphere_radius,const uint8_t box_matrix[64],const uint32_t full_size_xyz[3],gh_static_transform_policy,int *hit,uint32_t *frontier);
/* Actual2BC470/2BC0F8: first cylinder's endpoint trim and original
 * radius-selected sampled-line predicate, including short-height behavior.
 * Bounded count<=4096; unsupported count/numerics never becomes a miss. */
gh_pair_query_status gh_collision_query_cylinder_cylinder_002bc470(const uint8_t matrix_a[64],uint32_t radius_a,uint32_t height_a,const uint8_t matrix_b[64],uint32_t radius_b,uint32_t height_b,gh_static_transform_policy,int *hit,uint32_t *frontier);
/* Actual2BCB80/2BC6C8/2BC950, original trimmed line and sampled calls
 * to the same six-face predicate. Same explicit count/numeric reference. */
gh_pair_query_status gh_collision_query_cylinder_box_002bcb80(const uint8_t cylinder_matrix[64],uint32_t radius,uint32_t height,const uint8_t box_matrix[64],const uint32_t full_size_xyz[3],gh_static_transform_policy,int *hit,uint32_t *frontier);
/* Actual native kind2 x kind2 case0E:2BCF28 builds FULL endpoints then
 * shared2BC0F8; unlike kind1x2's2BC470 it never trims either endpoint. */
gh_pair_query_status gh_collision_query_kind2_pair_002bcf28(const uint8_t matrix_a[64],uint32_t radius_a,uint32_t height_a,const uint8_t matrix_b[64],uint32_t radius_b,uint32_t height_b,gh_static_transform_policy,int *hit,uint32_t *frontier);
/* Actual native kind2 x kind4 case10:2BD200 ->2BD078 fullsegment,
 * then shared2BC950. It does not call kind1x4's trim wrapper2BCB80. */
gh_pair_query_status gh_collision_query_kind2_box_002bd200(const uint8_t kind2_matrix[64],uint32_t radius,uint32_t height,const uint8_t box_matrix[64],const uint32_t full_size_xyz[3],gh_static_transform_policy,int *hit,uint32_t *frontier);
/* Actualnative kind4x4 case17:2BD828 centers/fullsizehalf/copyaxes ->
 *136A88 normalized-axis15SAT with original .001 padding and operation order. */
gh_pair_query_status gh_collision_query_box_box_002bd828(const uint8_t matrix_a[64],const uint32_t full_size_a[3],const uint8_t matrix_b[64],const uint32_t full_size_b[3],gh_static_transform_policy,int *hit,uint32_t *frontier);
#ifdef __cplusplus
}
#endif
#endif
