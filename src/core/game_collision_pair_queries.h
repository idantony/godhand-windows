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
#ifdef __cplusplus
}
#endif
#endif
