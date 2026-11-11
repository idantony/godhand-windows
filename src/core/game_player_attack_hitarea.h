#ifndef GH_GAME_PLAYER_ATTACK_HITAREA_H
#define GH_GAME_PLAYER_ATTACK_HITAREA_H
#include "game_hitarea_shapes.h"

#ifdef __cplusplus
extern "C" {
#endif
/* Original 10A438 after the motion sequence publishes player+3AC.
 * Class 0 uses the same five Give DamageUnits created by player Init. */
gh_hitarea_status gh_player_attack_hitarea_0010a438(
    const gh_title_player_view *, const gh_hitarea_object give[5], uint32_t *frontier);
#ifdef __cplusplus
}
#endif
#endif
