#ifndef GH_GAME_EVENT_ACTOR_CREATE_H
#define GH_GAME_EVENT_ACTOR_CREATE_H

#include "game_event_data_prefix.h"
#include "game_objects.h"
#include "game_room_pools.h"
#include "game_title_player.h"
#include "../platform/task_threads.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef gh_game_event_status (*gh_event_create_actor_view)(
    void *, uint32_t handle, gh_title_player_view *, uint32_t *frontier);

typedef struct {
    gh_event_components *components;
    const gh_event_data_prefix *data;
    gh_heap *heap;
    const gh_room_pools *pools;
    const gh_game_objects *objects;
    gh_task_threads *global_threads;
    gh_core_save_view save;
    gh_event_create_actor_view object_view;
    void *context;
} gh_event_actor_create_bindings;

typedef struct {
    const uint8_t *info;
    uint8_t *work;
    const gh_event_data_prefix *data;
    size_t actor_index;
    uint16_t source_kind, create_kind, resource_kind, create_argument;
    gh_object_view loaded_resource;
} gh_event_actor_create_request;

typedef struct {
    gh_event_actor_create_bindings bindings;
    gh_r207_allocation work;
    gh_event_actor_create_request pending;
    size_t object_cursor;
    int32_t task_slot;
    uint32_t frontier;
    gh_game_event_status task_error;
    uint8_t stage, task_entered, task_live;
} gh_event_actor_create;

/* After the actual Env allocation, run REL+0AC0, +0BA8 and +0D20.
 * Success here means the original create task was registered. Its first
 * actual object request remains strict at 312A80 until a factory is wired. */
gh_game_event_status gh_event_actor_create_start_002979f0(
    gh_event_actor_create *, const gh_event_actor_create_bindings *, uint32_t *frontier);
/* REL work+44 association as a bounded resource offset. */
gh_game_event_status gh_event_actor_create_work_at(const gh_event_actor_create *,
    size_t actor, const uint8_t **work, size_t *cut_offset);
/* Check the scheduled fiber's strict frontier before the original ready poll. */
gh_game_event_status gh_event_actor_create_task_status(const gh_event_actor_create *, uint32_t *frontier);
/* Host shutdown, after cancelling execution of the main Event controller. */
gh_game_event_status gh_event_actor_create_abort(gh_event_actor_create *);

#ifdef __cplusplus
}
#endif
#endif
