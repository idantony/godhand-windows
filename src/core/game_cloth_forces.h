#ifndef GH_GAME_CLOTH_FORCES_H
#define GH_GAME_CLOTH_FORCES_H
#include "game_cloth_runtime.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct {gh_cloth_step_prefix *prefix;uint32_t frontier;uint8_t complete;} gh_cloth_forces;
/* Actual10430C..104978 force integration, sourceflags20 skip supported.
 * Same original spring/history records. Domain: nativeNULL optional2070 gravity
 * table, floor-disabled100, player radialforce674<=0. Other live branches strict;
 * data is committed only after bounded wholeblock numerical validation. */
gh_cloth_status gh_cloth_integrate_forces_0010430c(gh_cloth_forces *,gh_cloth_step_prefix *);
#ifdef __cplusplus
}
#endif
#endif
