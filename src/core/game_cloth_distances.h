#ifndef GH_GAME_CLOTH_DISTANCES_H
#define GH_GAME_CLOTH_DISTANCES_H
#include "game_cloth_angles.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct {gh_cloth_angles *angles;uint32_t frontier,iterations,pair_queries,point_hits;uint8_t complete;} gh_cloth_distances;
/* Actual104EC8..105A90 orderediterations. Same spring records and live shared
 * bodygeometry; original neighbor3/4contacts thenneighbor1/2 halfdistance,
 * then anchored/mainlinkdistance. Floor100 domain only; flags80 rootresponse
 * remains exact106BF8. Commit records only after wholeblock succeeds. */
gh_cloth_status gh_cloth_constrain_distances_00104ec8(gh_cloth_distances *,gh_cloth_angles *);
#ifdef __cplusplus
}
#endif
#endif
