#ifndef GH_GAME_EVENT_PENDULUM_H
#define GH_GAME_EVENT_PENDULUM_H
#include "game_event_actor_extra.h"
#include "game_cloth_finish.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef gh_cloth_frame gh_event_pendulum_call;
gh_game_event_status gh_game_event_pendulum_002df860(gh_event_pendulum_call *,
    gh_event_actor_factory *, const gh_event_actor_extra *, const gh_cloth_step_bindings *,
    gh_player_shared_vector *, uint32_t *frontier);
gh_game_event_status gh_game_event_accessory_move_0027a320(gh_event_pendulum_call *,
    gh_event_actor_factory *, const gh_event_spring_profile *, const gh_boot_work *,
    const gh_cloth_step_bindings *, gh_player_shared_vector *, uint32_t *frontier);
#ifdef __cplusplus
}
#endif
#endif
