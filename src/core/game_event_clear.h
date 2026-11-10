#ifndef GH_GAME_EVENT_CLEAR_H
#define GH_GAME_EVENT_CLEAR_H
#include "game_event_start.h"
#include "game_player_abort.h"
#include "game_objects.h"
#include "game_init.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Consumers complete exactly their named original calls. Camera callback is
 * the TWO calls after124F60 has cleared player15F4 bit2, not124F60 itself. */
typedef gh_game_event_status (*gh_event_clear_consumer)(void *,uint32_t *frontier);
typedef struct {gh_game_event *event;const gh_game_event_config *config;gh_game_init_state *init;
 gh_room_pools *pools;gh_game_objects *global_objects;const gh_player_abort_bindings *player_abort;
 gh_event_object_view object_view;gh_event_clear_consumer player_clear_0012c418,camera_reset_0013b9b8_then_00138d40,special_remove_00312708;
 gh_object_slot_release release_sound_slot;void *context;} gh_event_clear_bindings;
typedef struct {gh_game_player_abort abort;uint8_t *player;uint32_t stage,frontier;} gh_event_player_clear;
/* Genuine12C418 prefix: conditional124F60, actual126770, 2F4..7 byte clears.
 * Stops at animation2A8578; does not claim an initialized pose was sampled. */
gh_game_event_status gh_event_player_clear_0012c418(gh_event_player_clear *,const gh_event_clear_bindings *);
typedef struct {gh_game_event *event;gh_event_player_clear player;uint32_t stage,index,frontier;int detail;} gh_event_clear_state;
/* Each completed invocation re-runs actual12C418, including config1 bypass.
 * Required fullplayer provider must complete real animation/model processing.
 * NULL invokes recovered strict prefix above, never acknowledges deletion. */
gh_game_event_status gh_event_clear_00296818(gh_event_clear_state *,const gh_event_clear_bindings *,int *done);
#ifdef __cplusplus
}
#endif
#endif
