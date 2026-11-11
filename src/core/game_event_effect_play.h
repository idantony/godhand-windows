#ifndef GH_GAME_EVENT_EFFECT_PLAY_H
#define GH_GAME_EVENT_EFFECT_PLAY_H

#include "game_effects_frame.h"
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

typedef struct {
    gh_game_effects_frame region;
    const gh_event_effect_play *play;
    const uint8_t *esp, *emitters;
    unsigned next_esp, resource_slot;
    uint8_t stage, completed;
} gh_event_effect_quit;

/* REL5478: 2FA310 queues original retirement, 2FA0E8 removes the bank
 * registry reference, then Work+5E4 bit 16 clears. Call before releasing the
 * event allocation. Existing live effects still borrow its views. */
gh_game_event_status gh_event_effect_quit_00005478(gh_event_effect_quit *,
    const gh_event_effect_play *, const gh_effects_frame_bindings *, uint32_t *frontier);

/* The original actor lookup has neither a previous nor a next key for any
 * actor in EV01's absent Vertex sequence, so the loop publishes no edits. */
gh_game_event_status gh_event_vertex_init_absent_0000a200(const gh_event_data_prefix *, uint32_t *);
gh_game_event_status gh_event_vertex_move_absent_0000a220(const gh_event_data_prefix *, uint32_t *);

#ifdef __cplusplus
}
#endif
#endif
