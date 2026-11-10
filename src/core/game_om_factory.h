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
 gh_om_placement_record source;gh_object_view resource;gh_object_resource_load resource_load;gh_game_objects *resource_manager;uint32_t handle,stage;uint8_t acquired,registered;struct gh_om_init_state *initialization;} gh_om_object;
typedef struct gh_om_factory {gh_om_factory_bindings bindings;gh_om_object *objects;size_t capacity,count;
 uint32_t frontier,detail;uint8_t bound;} gh_om_factory;
gh_scene_placement_status gh_om_factory_bind(gh_om_factory *,const gh_om_factory_bindings *,size_t capacity);
/* Real300 door allocation/constructor and originalfactory resourceprefix.
 * Unknown classes stop attheiractualcreator; virtual1B7118 remainsunported.
 * Retries resume partialobject and neverrepeatserial/acquire/register. */
gh_scene_placement_status gh_om_factory_create_record(gh_om_factory *,const gh_om_placement_record *,int *original_nonnull);
gh_scene_placement_status gh_om_factory_object_view(const gh_om_factory *,uint32_t handle,gh_title_player_view *);
/* Native abort of this constructor-only prefix. No originalvirtualInit ran;
 * releases acquiredreference and SAMEarena, unregistering ONLYownhandles. */
gh_scene_placement_status gh_om_factory_release(gh_om_factory *);
#ifdef __cplusplus
}
#endif
#endif
