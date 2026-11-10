#ifndef GH_GAME_RAY_QUERY_H
#define GH_GAME_RAY_QUERY_H
#include "game_scene_tables.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct {
 const uint8_t *attributes;size_t attribute_bytes;uint16_t triangle_index,attribute_index;
} gh_scene_ray_report;
/* Original130700 extra arguments are OUTPUT identities/capacity/count.
 * Its source is always the canonical actor owner's live190/194 list. */
typedef struct {const void **object_identities;uint32_t capacity;uint32_t *count;} gh_ray_dynamic_reports;
typedef struct {
 float from[4],to[4];uint32_t mask,flags;
 float *hit_position,*hit_normal;int32_t report_limit;void *reports;
 uint8_t segmented;
 gh_scene_ray_report *native_reports; /*typed replacement for original pointer outputs*/
 gh_ray_dynamic_reports *dynamic_reports;
} gh_scene_ray_request;
typedef enum {GH_RAY_OK,GH_RAY_ARGUMENT,GH_RAY_NUMERIC,GH_RAY_UNSUPPORTED,GH_RAY_BOUNDS,GH_RAY_MEMORY} gh_scene_ray_status;
/* Original straight and 8-unit/1%-overlap segmented131E10->12ED60 paths.
 * Uses SAME actor
 * table owner, mutable flags, original grid/group traversal and nearest hit.
 * A miss leaves caller outputs untouched. Native failures defer caller output
 * commits, while canonical scratch/visited prefix remains observable. Positive
 * live dynamic actor shapes retain130700 frontier. Opaque legacy
 * report storage is unsupported only when an actual write would be required;
 * native_reports carries real bounded attribute identities instead.
 * Numeric policy is explicit native binary32 nearest, not EE/VU hardware bits. */
/* Original direct12ED60 entry, including its mode181 nearest-hit shortening. */
gh_scene_ray_status gh_scene_ray_query_direct_0012ed60(gh_game_scene_tables *,const gh_scene_ray_request *,uint8_t mode181,int *original_hit,uint32_t *frontier);
gh_scene_ray_status gh_scene_ray_query_00131e10(gh_game_scene_tables *,const gh_scene_ray_request *,int *original_hit,uint32_t *frontier);
#ifdef __cplusplus
}
#endif
#endif
