#ifndef GH_GAME_PLAYER_TERRAIN_H
#define GH_GAME_PLAYER_TERRAIN_H
#include "game_ray_query.h"
#include "game_title_player.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct {
 const gh_title_player_view *actor;float center[3],radius;uint32_t mask,flags;
 gh_scene_ray_report *reports;uint32_t report_capacity;uint32_t *report_count;
} gh_terrain_sphere_request;
typedef gh_scene_ray_status (*gh_terrain_sphere_query)(void *,gh_game_scene_tables *,
 const gh_terrain_sphere_request *,float resolved[3],int *,uint32_t *);
typedef struct {
 gh_game_scene_tables *tables;const gh_title_player_view *actor;
 gh_terrain_sphere_query sphere_query;void *sphere_context;
} gh_player_terrain_bindings;
/* One cursor per original invocation. Zero initialize before a new call.
 * Successful earlier queries are never replayed on a later error. Caller
 * outputs are committed only at the original successful output branches.
 * Finite binary32-nearest reference; every exit restores the caller FEnv. */
typedef struct {
 gh_player_terrain_bindings bindings;const uint8_t *actor_bytes,*alias_identity,*akg_identity;
 float radius_or_depth,old[3],current[3],candidate[3],result[3],normal[3];
 float direction[3],distance,next_step,height;
 gh_scene_ray_report *reports;uint32_t report_capacity,*report_count,flags;
 uint32_t phase;uint8_t active,kind,complete,hit;
} gh_player_terrain_call;
gh_scene_ray_status gh_player_terrain_wall_00132da0(gh_player_terrain_call *,
 const gh_player_terrain_bindings *,float radius,uint32_t flags,
 float output[3],gh_scene_ray_report *,uint32_t capacity,uint32_t *count,
 int *original_hit,uint32_t *frontier);
gh_scene_ray_status gh_player_terrain_floor_001329c0(gh_player_terrain_call *,
 const gh_player_terrain_bindings *,float depth,float position[3],float normal[3],
 gh_scene_ray_report *,int *original_hit,uint32_t *frontier);
#ifdef __cplusplus
}
#endif
#endif
