#ifndef GH_GAME_OM_REACTION_H
#define GH_GAME_OM_REACTION_H
#include "game_object_step.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Actual1C2C40 shared fragment reaction. Positive effect/SE call boundaries
 * remain explicit until their canonical providers are restored. */
gh_object_step_status gh_om_reaction_001c2c40(const gh_title_player_view *,uint32_t *frontier);
#ifdef __cplusplus
}
#endif
#endif
