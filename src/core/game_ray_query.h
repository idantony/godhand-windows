#ifndef GH_GAME_RAY_QUERY_H
#define GH_GAME_RAY_QUERY_H
#include "game_scene_tables.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct {
 float from[4],to[4];uint32_t mask,flags;
 float *hit_position,*hit_normal;int32_t report_limit;void *reports;
 uint8_t segmented;
} gh_scene_ray_request;
typedef enum {GH_RAY_OK,GH_RAY_ARGUMENT,GH_RAY_NUMERIC,GH_RAY_UNSUPPORTED} gh_scene_ray_status;
/* Original straight131E10->12ED60->12F288 outside-grid domain. Uses SAME actor
 * table owner, actual query scratch and sorted inclusive AABB tests. A miss
 * leaves caller output vectors/reports untouched. Positive grid overlap and
 * segmented queries retain precise frontiers; this is not a fake no-hit ray.
 * Numeric policy is explicit native binary32 nearest, not EE/VU hardware bits. */
gh_scene_ray_status gh_scene_ray_query_00131e10(gh_game_scene_tables *,const gh_scene_ray_request *,int *original_hit,uint32_t *frontier);
#ifdef __cplusplus
}
#endif
#endif
