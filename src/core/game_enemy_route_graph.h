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

#ifdef __cplusplus
}
#endif
#endif
