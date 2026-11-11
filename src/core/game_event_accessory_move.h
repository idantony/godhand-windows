#ifndef GH_GAME_EVENT_ACCESSORY_MOVE_H
#define GH_GAME_EVENT_ACCESSORY_MOVE_H

#include "game_event_actor_factory.h"
#include "game_player_child_tail.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    const gh_event_actor_factory *actor;
    unsigned texture_node;
    uint8_t stage;
} gh_event_accessory_move;

typedef gh_game_event_status (*gh_event_simple_shadow)(void *, gh_event_actor_factory *, uint32_t *);
typedef gh_game_event_status (*gh_event_simple_tail)(void *, gh_event_actor_factory *,
    uint32_t original_entry, uint32_t *frontier);

/* Completed original late callees remain completed across a downstream retry. */
gh_game_event_status gh_game_event_simple_move_with_tails_002b6930(gh_event_accessory_move *,
    gh_event_actor_factory *, const gh_player_shared_vector *, gh_event_simple_shadow,
    void *shadow_context, gh_event_simple_tail, void *tail_context, uint32_t *frontier);


/* cObjSimple::Update for the original 44B528 class. The shadow producer runs
 * after hierarchy update and before the remaining mode-dependent calls. */
gh_game_event_status gh_game_event_simple_move_002b6930(gh_event_accessory_move *,
    gh_event_actor_factory *, const gh_player_shared_vector *, gh_event_simple_shadow,
    void *shadow_context, uint32_t *frontier);

/* cObjSimple::Update on the original EL0D auxiliary actor, including its
 * mode-4 FollowParent before the canonical part hierarchy update. */
gh_game_event_status gh_game_event_accessory_move_002b6930(gh_event_accessory_move *,
    gh_event_actor_factory *, const gh_player_shared_vector *, uint32_t *frontier);

#ifdef __cplusplus
}
#endif
#endif
