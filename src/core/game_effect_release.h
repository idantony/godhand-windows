#ifndef GH_GAME_EFFECT_RELEASE_H
#define GH_GAME_EFFECT_RELEASE_H
#include "game_effects.h"
#include "game_title_player.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Native pointer identity resolver for live slots. No guest token (including0)
 * is reinterpreted as a native player pointer. matchingkind with unknownactor
 * retains the actual3039E8/2FEB80 frontier. */
typedef gh_effect_status (*gh_effect_actor_match)(void *,int emitter,unsigned slot,const gh_title_player_view *actor,int *match,uint32_t *frontier);
typedef struct {gh_game_effects *effects;uint8_t *esp,*emitters;size_t esp_bytes,emitter_bytes;
 gh_effect_actor_match actor_match;void *context;} gh_effect_release_bindings;
/* Actual306200 dispatch,2FB5C0/3009F8 MSB-first occupancy/paused/hidden scans
 * and kind/actor matching. Matchinglive destructors remain strict2FBE50 or
 *302918; no handle0 shortcut, no fake deletion or pool reset. */
gh_effect_status gh_game_effect_release_00306200(const gh_effect_release_bindings *,const gh_title_player_view *actor,int32_t kind,uint32_t mode,uint32_t *frontier,unsigned *slot);
#ifdef __cplusplus
}
#endif
#endif
