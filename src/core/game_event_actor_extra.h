#ifndef GH_GAME_EVENT_ACTOR_EXTRA_H
#define GH_GAME_EVENT_ACTOR_EXTRA_H

#include "game_event_actor_factory.h"
#include "game_event_player_reset.h"
#include "game_player_child_attach.h"
#include "game_actor_external_parent.h"
#include "game_math_globals.h"
#include "game_player_springs.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef gh_game_event_status (*gh_event_actor_weapon_parent)(void *,
    const gh_event_weapon_attachment *, gh_actor_model *parent, unsigned part,
    const uint32_t position[3], const uint32_t rotation[3], uint32_t *frontier);

typedef struct {
    gh_event_actor_factory *factory;
    const gh_title_player_view *player;
    gh_game_player_child *child;
    gh_player_child_attachment *attachment;
    const gh_event_weapon_slot *weapon_slot;
    gh_core_save_view save;
    gh_event_actor_weapon_parent attach_weapon;
    void *weapon_context;
    gh_event_actor_roster *roster;
    const gh_event_actor_create *creation;
    const gh_game_math_globals *math;
    const void *boot_elf;
    size_t boot_elf_size;
} gh_event_actor_extra_bindings;

typedef struct {
    gh_event_actor_extra_bindings bindings;
    gh_player_child_attachment previous;
    uint32_t old_250, old_2f0, old_2088, old_20d4;
    const gh_event_weapon_attachment *weapon;
    uint8_t bound, stage, mesh_cursor, retargeted, weapon_started;
    gh_actor_external_parent_set row_parts[4];
    uint8_t row_stage[4], row_mesh_cursor[4];
    gh_event_actor_factory olivia_accessory;
    gh_actor_external_parent_set accessory_parts;
    uint32_t accessory_handle;
    uint8_t olivia_stage;
    gh_spring_profile_2a0 pendulum_profile;
    gh_actor_model *pendulum_parent;
    uint8_t one_body_mesh[2];
    uint8_t finish_stage;
} gh_event_actor_extra;

/* Borrow the same published player, jacket attachment and known weapon slot.
 * Keep the original player/model owners alive until after extra_abort. */
gh_game_event_status gh_event_actor_extra_bind(gh_event_actor_extra *, const gh_event_actor_extra_bindings *);
gh_game_event_status gh_event_actor_extra_run(void *, const gh_event_actor_create_request *,
    uint32_t handle, int *original_success, uint32_t *frontier);
gh_game_event_status gh_event_actor_extra_finish(void *, uint32_t *frontier);
/* Restore the previous jacket attachment before releasing the EL00 model.
 * A positive weapon provider must release its own external link first. */
gh_game_event_status gh_event_actor_extra_abort(gh_event_actor_extra *);

#ifdef __cplusplus
}
#endif
#endif
