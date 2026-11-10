#ifndef GH_GAME_SCENE_FINISH_H
#define GH_GAME_SCENE_FINISH_H
#include "game_world_light.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Actual2D8408 zero-lightpreset domain. Copies originalfixedscalarfields from
 * canonicalpreset15D00+index80, preservesVF0.W/otherfields. No GPUack.
 * Active/nonzero-light paths remainunsupported instead ofdiscardinglights. */
gh_light_status gh_scene_world_light_select_002d8408(gh_game_world_light *,unsigned preset);
/* Actual2B65F0 storesone into original3C2555; suppliedcanonicalfieldowner. */
void gh_scene_system_menu_enable_002b65f0(uint8_t *);
#ifdef __cplusplus
}
#endif
#endif
