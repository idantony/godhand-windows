#ifndef GH_GAME_ESP_TRAIL_DRAW_H
#define GH_GAME_ESP_TRAIL_DRAW_H
#include "game_effects_trans.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Original world-space, stride-0x40 ESP09 draw. Each emitted segment
 * contains an explicit triangle list (3 or 6 vertices), in original order. */
gh_effect_status gh_esp09_draw_002ec398(const gh_effects_trans_bindings *,unsigned,
 const gh_effect_esp_view *,gh_effect_trans_packet *,uint32_t *);
#ifdef __cplusplus
}
#endif
#endif
