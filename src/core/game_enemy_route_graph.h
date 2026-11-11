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

#ifdef __cplusplus
}
#endif
#endif
