#ifndef GH_GAME_EVENT_ACTOR_RELEASE_H
#define GH_GAME_EVENT_ACTOR_RELEASE_H

#include "game_event_actor_play.h"
#include "game_effects.h"
#include "sound_control.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    gh_event_actor_create *creation;
    gh_event_actor_roster *roster;
    gh_event_actor_extra *extra;
    gh_event_actor_play *play;
    gh_sound_control *sound;
    const gh_game_effects *effects;
} gh_event_actor_release_bindings;

typedef struct {
    gh_event_actor_release_bindings bindings;
    size_t row, unsuspend_cursor;
    uint8_t bound, quit, release_started, waited, work_released, complete;
} gh_event_actor_release;

gh_game_event_status gh_event_actor_quit_00001588(gh_event_actor_release *,
    const gh_event_actor_release_bindings *, uint32_t *frontier);
/* Main297A88 -> REL12C0. Original ordered actor and associated-object deletion. */
gh_game_event_status gh_event_actor_start_release_00297a88(gh_event_actor_release *, uint32_t *frontier);
/* Main297AA8 -> REL1390. Poll actual global DataManager holder state. */
gh_game_event_status gh_event_actor_wait_release_00297aa8(gh_event_actor_release *, int *done, uint32_t *frontier);
/* Main297AD0 actor leaves: releaseWork1428 before unsuspendAllObj0C60. */
gh_game_event_status gh_event_actor_release_work_00297ad0(gh_event_actor_release *, uint32_t *frontier);
/* Original invalid -1 sound slot path has a real alias into cSnd recent[10]. */
gh_object_status gh_event_actor_release_slot_002cb340(void *, int32_t slot, uint32_t *frontier);

#ifdef __cplusplus
}
#endif
#endif
