#ifndef GH_GAME_OM_INITIALIZE_H
#define GH_GAME_OM_INITIALIZE_H
#include "game_om_factory.h"
#include "game_actor_model.h"
#include "game_hitarea_objects.h"
#include "game_hitarea_shapes.h"
#include "game_om_door_geometry.h"
#include "game_collision_solid_units.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct gh_om_initializer_bindings {gh_boot_work *work;gh_heap *packet_heap;const void *fog_rgb;
 gh_game_init_state *init;const gh_hitarea_shape_bindings *shapes;
 gh_scene_dynamic_owner *dynamic;const gh_om_door_shared *door_shared;
 const gh_title_player_view *player;gh_game_collision_solids *solids;} gh_om_initializer_bindings;
typedef struct gh_om_init_state {gh_actor_model model;gh_bundle_entry geometry,texture;
 gh_hitarea_object hitarea;gh_om_door_geometry geometry_state;uint8_t *door_segments;gh_om_object *object;const gh_om_initializer_bindings *bindings;
 uint32_t stage,texture_mode,frontier;int model_return;uint8_t hitarea_created;} gh_om_init_state;
/* Actual1B7118->door virtual1867E8->1B6FB8. Genuine model/source binding,
 * zeroanimation-source path, flags and direct semantic boundary at1FE218.
 * Future box/dynamicshape work continues SAME owner, nevermetadata-ready. */
/* Sharedactual1B6FB8 plus1B7118 sourcebinding; one modelowner peractor.
 * Runtime3EB has no OMS source; its constructor5E0==NULL genuinely skips
 * placement-action selection. No synthetic zero-filled OMS is supplied. */
gh_scene_placement_status gh_om_common_initialize_001b6fb8(gh_om_init_state *,gh_om_object *,const gh_om_initializer_bindings *,int *original_return);
gh_scene_placement_status gh_om_door_initialize_001b7118(gh_om_init_state *,gh_om_object *,const gh_om_initializer_bindings *,int *original_return);
gh_scene_placement_status gh_om_initialize_abort(gh_om_init_state *);
#ifdef __cplusplus
}
#endif
#endif
