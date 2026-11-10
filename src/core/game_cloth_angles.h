#ifndef GH_GAME_CLOTH_ANGLES_H
#define GH_GAME_CLOTH_ANGLES_H
#include "game_cloth_forces.h"
#include "game_player_child_tail.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct {gh_cloth_forces *forces;gh_player_shared_vector *shared_pivot;uint32_t frontier,corrections;uint8_t complete;} gh_cloth_angles;
/* Actual10497C..104EC4 angular constraint. SAME bone aliasF0 and spring40
 * positions, original sequential dependencies. Native atomic commit of the
 * block on supported finite reference input; no clamp of out-of-domainacos. */
gh_cloth_status gh_cloth_constrain_angles_0010497c(gh_cloth_angles *,gh_cloth_forces *,gh_player_shared_vector *);
#ifdef __cplusplus
}
#endif
#endif
