#ifndef GH_GAME_CLOTH_FINISH_H
#define GH_GAME_CLOTH_FINISH_H
#include "game_cloth_distances.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct {gh_cloth_distances *distances;uint32_t frontier,rotated_parts,point_hits;uint8_t complete;} gh_cloth_finish;
/* Actual105A94..1061C0 finalrestlength/bodyresponse/velocity/boneworldrotation.
 * Samechildparts80/B0 and originalrecords, sequentialXYZaliases. Domain: floor
 * disabled100/flags80clear/NULL2074dampingtable/NULL20A4override. */
gh_cloth_status gh_cloth_finish_00105a94(gh_cloth_finish *,gh_cloth_distances *);
#ifdef __cplusplus
}
#endif
#endif
