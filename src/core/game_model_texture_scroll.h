#ifndef GH_GAME_MODEL_TEXTURE_SCROLL_H
#define GH_GAME_MODEL_TEXTURE_SCROLL_H

#include "game_actor_model.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Original 14E4D0 visits the canonical nodes in link order. The cursor is
 * the next node in this call, so a later failure preserves earlier updates. */
gh_actor_model_status gh_game_model_texture_scroll_0014e4d0(const gh_actor_model *,
    float rate, unsigned *cursor, uint32_t *frontier);

#ifdef __cplusplus
}
#endif
#endif
