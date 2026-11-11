#ifndef GH_GAME_OM_FACTORY_H
#define GH_GAME_OM_FACTORY_H
#include "game_scene_placements.h"
#include "game_objects.h"
#include "game_title_player.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Nativehandle kind distinct from scrollmesh1/player2. No guestvtable call. */
enum {GH_ROOM_OBJECT_OM=3};
struct gh_om_initializer_bindings;struct gh_om_init_state;
typedef struct {gh_room_pools *pools;const gh_boot_work *work;const gh_room_state *room_state;
 gh_game_objects *objects,*room_objects;uint8_t *factory_serial;
 const gh_object_resource_bindings *resources;const struct gh_om_initializer_bindings *initializer;} gh_om_factory_bindings;
typedef struct {gh_room_arena_allocation allocation;gh_title_player_view view;
 gh_om_placement_record source;gh_object_view resource;gh_object_resource_load resource_load;gh_game_objects *resource_manager;uint32_t handle,stage;uint8_t acquired,registered;struct gh_om_init_state *initialization;
 /* Real runtime CreateObj has no OMS record. These are call arguments,
  * never a fabricated placement source or a copied second actor owner. */
 uint16_t runtime_kind,runtime_resource_id;uint8_t runtime_created;
} gh_om_object;
typedef struct gh_om_factory {gh_om_factory_bindings bindings;gh_om_object *objects;size_t capacity,count;
 uint32_t frontier,detail;uint8_t bound;void *runtime_pending;} gh_om_factory;
gh_scene_placement_status gh_om_factory_bind(gh_om_factory *,const gh_om_factory_bindings *,size_t capacity);
/* Real300 door allocation/constructor and originalfactory resourceprefix.
 * Unknown classes stop attheiractualcreator; virtual1B7118 remainsunported.
 * Retries resume partialobject and neverrepeatserial/acquire/register. */
gh_scene_placement_status gh_om_factory_create_record(gh_om_factory *,const gh_om_placement_record *,int *original_nonnull);
typedef struct {gh_om_factory *factory;gh_om_object *object;uint16_t kind,resource_id;
 uint8_t bound,complete;} gh_om_runtime_create;
/* One original30FA98 invocation, initially zero companion. A repeated call
 * retries/completes the SAME invocation; a new original call uses a fresh
 * companion. Returns constructed+registered object BEFORE virtualInit44.
 * Bounded to observed3EB/idFFFF; unsupported kinds never become NULL success.
 * Factory storage must already reserve native capacity for runtime objects. */
gh_scene_placement_status gh_om_factory_create_0030fa98(gh_om_factory *,gh_om_runtime_create *,uint16_t kind,uint16_t resource_id,gh_om_object **original_object);
gh_scene_placement_status gh_om_factory_initialize_runtime_3eb(gh_om_factory *,gh_om_object *,int *original_return);
gh_scene_placement_status gh_om_factory_object_view(const gh_om_factory *,uint32_t handle,gh_title_player_view *);
/* Native abort of this constructor-only prefix. No originalvirtualInit ran;
 * releases acquiredreference and SAMEarena, unregistering ONLYownhandles. */
gh_scene_placement_status gh_om_factory_release(gh_om_factory *);
/* Discard descriptors only after the ordinary room retirement has released
 * every object, resource reference and allocation. */
gh_scene_placement_status gh_om_factory_release_retired(gh_om_factory *);
#ifdef __cplusplus
}
#endif
#endif
