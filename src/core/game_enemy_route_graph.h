#ifndef GH_GAME_ENEMY_ROUTE_GRAPH_H
#define GH_GAME_ENEMY_ROUTE_GRAPH_H

#include "game_scene_step.h"
#include "game_actor_motion.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Search the bound PFD2 graph in the original edge order. A missing route
 * returns FLT_MAX and leaves output unchanged. The original sentinel is FFFF. */
gh_actor_motion_status gh_enemy_route_graph_0031be88(
    const gh_scene_nearcheck *navigation, uint32_t origin, uint32_t destination,
    unsigned max_depth, uint32_t *output, unsigned capacity, float *cost);

typedef struct {
    uint32_t pair;
    uint16_t vertices[2];
    float projection[4];
} gh_enemy_route_edge;

typedef struct {
    gh_enemy_route_edge edge;
    float distance, ray_target[4];
} gh_enemy_route_edge_candidate;

typedef struct {
    const gh_scene_nearcheck *navigation;
    const gh_game_scene_tables *actors;
    float origin[4];
    gh_enemy_route_edge_candidate candidates[4];
    unsigned ray_cursor;
} gh_enemy_route_edge_query;

/* Original nearest-edge search with the caller's visibility test enabled.
 * A completed search with no visible edge returns UINT32_MAX in edge->pair. */
gh_actor_motion_status gh_enemy_route_edge_002a8038(
    gh_enemy_route_edge_query *query, const gh_scene_nearcheck *navigation,
    gh_game_scene_tables *actors, const float origin[4],
    gh_enemy_route_edge *edge, uint32_t *frontier);

typedef struct {
    const gh_scene_nearcheck *owner;
    float point[4],distance[8];
    uint32_t vertices[8];
    uint8_t initialized,visibility,index,complete;
} gh_enemy_route_query;

typedef struct {
    const gh_scene_nearcheck *navigation;
    gh_game_scene_tables *actors;
    gh_enemy_route_query target_query, origin_query;
    gh_enemy_route_edge_query origin_edge_query;
    uint32_t target_vertex, origin_vertex, frontier;
    uint8_t route_stage;
} gh_enemy_route_call;

/* Original common route helper, called by ordinary and cEmA2 enemies.
 * Reuse the cursor on a suspended call. Clear it before computing a new route. */
struct gh_game_init_state;
gh_actor_motion_status gh_enemy_route_clear_002a74e0(gh_enemy_route_call *, uint8_t *actor_work,
    const float actor[3], const float target[3], const gh_scene_nearcheck *,
    gh_game_scene_tables *, const struct gh_game_init_state *, uint16_t room);
uint8_t gh_enemy_route_waypoint_002a7ca0(const uint8_t *actor_work, uint8_t *destination);

#ifdef __cplusplus
}
#endif
#endif
