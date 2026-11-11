#ifndef GH_GAME_EVENT_ACTOR_FACTORY_H
#define GH_GAME_EVENT_ACTOR_FACTORY_H

#include "game_event_actor_create.h"
#include "game_player_shadow.h"

#ifdef __cplusplus
extern "C" {
#endif

enum { GH_ROOM_OBJECT_EVENT_ACTOR = 0x600, GH_EVENT_ACTOR_BYTES = 0x30e0 };
/* EV02 is the largest admitted roster. One extra actor is bound separately. */
enum { GH_EVENT_ACTOR_ROSTER_CAPACITY = 22 };

typedef struct {
    gh_room_pools *pools;
    gh_game_objects *global_objects, *room_objects;
    gh_boot_work *work;
    uint8_t *factory_serial;
    gh_object_resource_bindings resources;
    gh_heap *packet_heap;
    const void *fog_rgb;
} gh_event_actor_factory_bindings;

typedef struct {
    gh_event_actor_factory_bindings bindings;
    gh_game_title_player object;
    gh_object_resource_load load;
    gh_object_view resource;
    uint32_t handle;
    uint16_t kind;
    uint8_t bound, stage, room_reference, global_reference;
    gh_actor_model model;
    gh_player_shadow shadow;
    gh_bundle_entry geometry, texture;
    uint8_t init_stage;
    uint8_t normal_release_stage;
    size_t actor_index;
} gh_event_actor_factory;

/* The caller lends the native owner already used for original heap 754220
 * in resources.heap. Bind before starting the GlobalTask actor fiber. */
gh_game_event_status gh_event_actor_factory_bind(gh_event_actor_factory *,
    const gh_event_actor_factory_bindings *);
gh_game_event_status gh_event_actor_factory_run(void *, const gh_event_actor_create_request *,
    int *original_nonnull, uint32_t *frontier);
/* Virtual 2B6768, then the original first EL00's zero-pose R0_Init/calcParts.
 * A positive result identifies the same registry handle, before ExtraInitialize. */
gh_game_event_status gh_event_actor_factory_initialize(void *, const gh_event_actor_create_request *,
    uint32_t *handle, uint32_t *frontier);
/* Virtual Init only, used by original auxiliary factory calls from ExtraInitialize. */
gh_game_event_status gh_event_actor_factory_initialize_model(gh_event_actor_factory *,
    const gh_event_actor_create_request *, int *original_nonnull, uint32_t *frontier);
gh_game_event_status gh_event_actor_factory_view(const gh_event_actor_factory *, uint32_t handle,
    gh_title_player_view *);
/* Native owner shutdown for this actor prefix. Cancel the actor task first.
 * Any pending DVD allocation belongs to the canonical holder manager. */
gh_game_event_status gh_event_actor_factory_abort(gh_event_actor_factory *);

/* Fixed caller-owned storage keeps registered object/model addresses stable. */
typedef struct {
    gh_event_actor_factory_bindings bindings;
    gh_event_actor_factory *rows;
    size_t capacity;
} gh_event_actor_roster;
gh_game_event_status gh_event_actor_roster_bind(gh_event_actor_roster *, const gh_event_actor_factory_bindings *,
    gh_event_actor_factory *rows, size_t capacity);
gh_game_event_status gh_event_actor_roster_run(void *, const gh_event_actor_create_request *, int *, uint32_t *);
gh_game_event_status gh_event_actor_roster_initialize(void *, const gh_event_actor_create_request *, uint32_t *, uint32_t *);
gh_game_event_status gh_event_actor_roster_abort(gh_event_actor_roster *);

#ifdef __cplusplus
}
#endif
#endif
