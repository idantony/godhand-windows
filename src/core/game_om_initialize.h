#ifndef GH_GAME_OM_INITIALIZE_H
#define GH_GAME_OM_INITIALIZE_H
#include "game_om_factory.h"
#include "game_actor_model.h"
#include "game_hitarea_objects.h"
#include "game_hitarea_shapes.h"
#include "game_om_door_geometry.h"
#include "game_collision_solid_units.h"
#include "game_effect_create.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct gh_om_initializer_bindings {gh_boot_work *work;gh_heap *packet_heap;const void *fog_rgb;
 gh_game_init_state *init;const gh_hitarea_shape_bindings *shapes;
 gh_scene_dynamic_owner *dynamic;const gh_om_door_shared *door_shared;
 const gh_title_player_view *player;gh_game_collision_solids *solids;
 const void *elf;size_t elf_bytes;gh_core_save_view save;} gh_om_initializer_bindings;
typedef struct gh_om_init_state {gh_actor_model model;gh_bundle_entry geometry,texture;
 gh_hitarea_object hitarea;gh_om_door_geometry geometry_state;uint8_t *door_segments;gh_om_object *object;const gh_om_initializer_bindings *bindings;
 uint32_t stage,texture_mode,frontier;int model_return;uint8_t hitarea_created;
 const gh_actor_model *item_attachment_owner;
 gh_hitarea_object catapult_attack;uint8_t catapult_attack_created;} gh_om_init_state;
/* Actual1B7118->door virtual1867E8->1B6FB8. Genuine model/source binding,
 * zeroanimation-source path, flags and direct semantic boundary at1FE218.
 * Future box/dynamicshape work continues SAME owner, nevermetadata-ready. */
/* Sharedactual1B6FB8 plus1B7118 sourcebinding; one modelowner peractor.
 * Runtime3EB has no OMS source; its constructor5E0==NULL genuinely skips
 * placement-action selection. No synthetic zero-filled OMS is supplied. */
gh_scene_placement_status gh_om_common_initialize_001b6fb8(gh_om_init_state *,gh_om_object *,const gh_om_initializer_bindings *,int *original_return);
gh_scene_placement_status gh_om_door_initialize_001b7118(gh_om_init_state *,gh_om_object *,const gh_om_initializer_bindings *,int *original_return);
gh_scene_placement_status gh_om_r101_item_construct_001c0068(void *,size_t,gh_title_player_view *);
gh_scene_placement_status gh_om_r101_item_initialize_001c0128(gh_om_init_state *,gh_om_object *,
 const gh_om_initializer_bindings *,int *);
gh_scene_placement_status gh_om_r101_item_collect_001c24e8(gh_om_init_state *);
gh_scene_placement_status gh_om_r101_item_save_001fb1c0(gh_om_init_state *,const uint8_t *);
gh_scene_placement_status gh_om_r101_item_position_001c2630(gh_om_init_state *,const uint8_t *);
gh_scene_placement_status gh_om_r101_item_follow_001c2378(gh_om_init_state *);
gh_scene_placement_status gh_om3bd_construct_001b87a8(void *,size_t,gh_title_player_view *);
gh_scene_placement_status gh_om3bd_initialize_001b8868(gh_om_init_state *,gh_om_object *,
 const gh_om_initializer_bindings *,int *);
gh_scene_placement_status gh_om_initialize_abort(gh_om_init_state *);

typedef struct {
 gh_om_init_state common;
 gh_hitarea_object attack,contact;
 gh_effect_create_call launch_effect;
 const gh_title_player_view *shooter;
 int attack_index,contact_index;
 uint8_t stage,attack_created,contact_created;
} gh_om3be_state;
gh_scene_placement_status gh_om3be_construct_001b8ff8(void *,size_t,gh_title_player_view *);
gh_scene_placement_status gh_om3be_initialize_001b90d0(gh_om3be_state *,gh_om_object *,
 const gh_om_initializer_bindings *);
gh_scene_placement_status gh_om3be_launch_001bb548(gh_om3be_state *,
 const gh_title_player_view *shooter,const float origin[3],const float velocity[3],
 unsigned heat,unsigned launch_type,const gh_effect_create_bindings *);
gh_scene_placement_status gh_om3be_launch_roulette_001bb548(gh_om3be_state *,
 const gh_title_player_view *shooter,const float origin[3],const float velocity[3],unsigned rank);
gh_scene_placement_status gh_om3be_abort(gh_om3be_state *);
typedef struct {
 gh_om_init_state common;
 gh_hitarea_object areas[2];
 gh_effect_create_call effect,blast;
 const gh_actor_model *parent;
 uint32_t parent_handle;
 int sphere_indices[2];
 uint8_t setup_stage,areas_created[2],fuse_sound_submitted;
} gh_om364_state;
gh_scene_placement_status gh_om364_construct_001b0ce8(void *,size_t,gh_title_player_view *);
gh_scene_placement_status gh_om364_initialize_001b0de8(gh_om364_state *,gh_om_object *,
 const gh_om_initializer_bindings *);
gh_scene_placement_status gh_om364_abort(gh_om364_state *);
#ifdef __cplusplus
}
#endif
#endif
