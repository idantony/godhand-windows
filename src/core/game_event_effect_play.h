#ifndef GH_GAME_EVENT_EFFECT_PLAY_H
#define GH_GAME_EVENT_EFFECT_PLAY_H

#include "game_effect_create.h"
#include "game_event_data_prefix.h"
#include "game_event_environment.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    const gh_event_data_prefix *data;
    const void *allocation;
    gh_game_effects *effects;
    gh_boot_work *work;
    gh_effect_create_call create;
    const uint8_t *esp, *emitters;
    uint32_t cut;
    unsigned next_esp;
    uint8_t stage;
} gh_event_effect_play;

/* The bound sequence is genuinely absent in EV01. Cut-start effects still
 * run through the exact bank registered by the same 2978B8 owner. */
gh_game_event_status gh_event_effect_init_00005000(gh_event_effect_play *,
    const gh_event_data_prefix *, const gh_game_event_config *, gh_boot_work *,
    gh_game_effects *, uint32_t *frontier);
gh_game_event_status gh_event_effect_move_00005050(gh_event_effect_play *,
    const gh_effect_create_bindings *, uint32_t *frontier, int *relative);

/* The original actor lookup has neither a previous nor a next key for any
 * actor in EV01's absent Vertex sequence, so the loop publishes no edits. */
gh_game_event_status gh_event_vertex_init_absent_0000a200(const gh_event_data_prefix *, uint32_t *);
gh_game_event_status gh_event_vertex_move_absent_0000a220(const gh_event_data_prefix *, uint32_t *);

#ifdef __cplusplus
}
#endif
#endif
