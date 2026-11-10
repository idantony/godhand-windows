#ifndef GH_GAME_TERRAIN_SPHERE_H
#define GH_GAME_TERRAIN_SPHERE_H
#include "game_player_terrain.h"
#include "game_scene_dynamic.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Host continuation only; all original scratch/visited bits stay in tables.ray.
 * Zero initialize for each new original1321D8 invocation. */
typedef struct {
 const gh_boot_work *work; /*required canonical Work594 debuggate after traversal*/
 gh_game_scene_tables *tables;const uint8_t *akg;gh_terrain_sphere_request request;
 const void *dynamic_identity;uint32_t phase,x,y,z,group,item,triangle_flags,dynamic_face;
 float result[3];uint32_t result_count;
 uint16_t triangle;uint8_t active,complete,pending_triangle,hit;
 const struct gh_scene_dynamic_box *dynamic_cursor;
} gh_terrain_sphere_call;
/* Callback-compatible bounded1321D8 prefix. Native errors preserve original
 * traversal position. Static135348/135B18/135D50 closestpoint and XZ push are native.
 * Actualkind2 box faces and originalsourceactor registration are native.
 * Actualkind1 quad faces share the same closestpoint/contact semantics.
 * Unknowncontacts retain1311A0; zero-XZ push retains30A468. Never a ray substitute. */
gh_scene_ray_status gh_terrain_sphere_query_001321d8(void *call,gh_game_scene_tables *,
 const gh_terrain_sphere_request *,float resolved[3],int *hit,uint32_t *frontier);
#ifdef __cplusplus
}
#endif
#endif
