#ifndef GH_GAME_EVENT_ACTOR_BUST_H
#define GH_GAME_EVENT_ACTOR_BUST_H

#include "game_event_actor_factory.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Original EL0A post-hierarchy update. The current EventEnv frame is read
 * from the same component the PLAY timing code publishes. */
gh_game_event_status gh_event_actor_bust_002b6a40(gh_event_actor_factory *,
    gh_event_components *, uint32_t *frontier);

#ifdef __cplusplus
}
#endif
#endif
