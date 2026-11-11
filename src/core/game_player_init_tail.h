#ifndef GH_GAME_PLAYER_INIT_TAIL_H
#define GH_GAME_PLAYER_INIT_TAIL_H
#include "game_player_init.h"
#include "game_player_spawn.h"
#include "game_player_child_attach.h"
#include "game_player_shadow.h"
#include "game_collision_solid_units.h"
#include "game_hitarea_shapes.h"
#include "game_player_node_cache.h"
#include "game_player_post_init.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Optional full100378 provider for realsharedmodel/pose/spring integration.
 * Nativeerrors distinctfromoriginalbool. NULL uses strictrecovered100378,
 * currently148420/100420 untiltheiractualdependenciesarebound. ReturningOK mustrepresentactualfullmethodcompletion. */
typedef gh_title_player_status (*gh_player_child_initialize)(void *,gh_game_player_child *,int *original_return,uint32_t *frontier);
/* Exact107F58 callsite: sameMTB3 offset for motion andrawa2candidate.
 * 2C7F88 inside originalcallee decideswhethercandidateisSEQ; never choose+2C.
 * start_frame_bits=f12(0),blend_frames=a3(0),flags=t0(0),release_previous
 * istruebecause t1==0. Typedrate isRootONE747A14 owner, not Workpadding. */
typedef struct {gh_game_title_player *player;gh_actor_model *model;gh_boot_work *work;
 const uint8_t *resource;size_t resource_bytes,motion_offset,sequence_candidate_offset;
 uint32_t start_frame_bits,blend_frames;uint16_t flags;uint8_t release_previous_sequence;
 const float *rate;} gh_player_motion_request;
typedef gh_player_init_status (*gh_player_motion_call)(void *,const gh_player_motion_request *,uint32_t *frontier);
typedef struct {gh_game_player_init *prefix;gh_actor_model *parent_model;
 gh_game_player_child *child;gh_player_child_attachment *attachment;
 gh_game_collision_solids *solids;gh_heap *default_heap;gh_core_save_view save;
 gh_object_resource_bindings resources;gh_player_child_initialize initialize_child;void *child_context;
 gh_game_init_state *init;gh_collision_pairs *pairs;const gh_game_math_globals *math_globals;gh_static_transform_policy shape_policy;const gh_player_node_names *node_names;
 gh_player_motion_call initialize_motion,advance_motion;void *motion_context;const float *rate;gh_player_motion_call solve_motion;
} gh_player_init_tail_bindings;
typedef struct {gh_player_init_tail_bindings b;gh_game_title_player *player;
 gh_player_shadow shadow;gh_collision_solid_unit *solid_unit;gh_collision_native_sphere *sphere;
 uint32_t stage,frontier;int child_original_return;uint8_t bound,active_child;
 gh_hitarea_object take_object,give_objects[5];int take_sphere_indices[4],give_shape_indices[5];
 unsigned take_step,give_step,give_phase;uint8_t take_present;uint32_t pending_lookup_name;
 gh_game_player_node_cache node_cache;gh_bundle_entry motion_entry;
 gh_game_player_post_init post;int original_return;uint8_t complete;
} gh_game_player_init_tail;
/* Continuesactual107F58 onlyaftertheexistingprefix's realmodelcall completed.
 * Samechildfactory/virtualInit/attachment→shadow→partflags→solidUnit/Sphere→HP.
 * Actualsamepool hitareas/4spheres/4cylinders/box anddisable gates continue throughnodecache/scalars, optionalrealmotion begin/advance, shared149EB8, real152A20 provider andexactpost-init suffix;
 * Successrequiresallactualconsumers andoriginalreturn1;31C5C0 remains the originalcaller nextorderedoperation. */
gh_player_init_status gh_game_player_initialize_tail_00107f58(gh_game_player_init_tail *,const gh_player_init_tail_bindings *);
/* Drop native constructor cursors only after the room actors have retired. */
gh_player_init_status gh_game_player_reset_room_cursors(
 gh_game_player_init *,gh_game_player_init_tail *,gh_game_player_spawn *,
 const gh_game_title_player *,const gh_game_player_child *);
#ifdef __cplusplus
}
#endif
#endif
