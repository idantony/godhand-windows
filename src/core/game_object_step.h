#ifndef GH_GAME_OBJECT_STEP_H
#define GH_GAME_OBJECT_STEP_H
#include "game_init.h"
#include "game_room_pools.h"
#include "game_scene_at.h"
#include "game_title_player.h"
#include "../assets/static_model_constants.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Existing init.side_counter[16] is the canonical original754C38+0..F
 * scalar/opaque span. Registries belong to room_pools, never this companion.
 * This also owns754C48 ratio/profiling752CF8 and the recovered5CAFFC scalar;
 * OT depthshift is the explicit31A0E8 constructor leaf, not another OT. */
typedef enum {GH_OBJSTEP_OK,GH_OBJSTEP_ARGUMENT,GH_OBJSTEP_POOLS,GH_OBJSTEP_UNSUPPORTED,GH_OBJSTEP_CALLEE} gh_object_step_status;
typedef enum {GH_OBJECT_NODE_GEOMETRY,GH_OBJECT_SETUP} gh_object_static_command_kind;
typedef struct {
 gh_object_static_command_kind kind;size_t object_index;uint32_t handle,bank,depth,selected_program;
 uint64_t test,tex1,alpha,clamp;const gh_scene_mesh_object *object;
 gh_static_node_packet packet;const gh_stage_material *materials[16];unsigned material_count;
} gh_object_static_command;
/* Queue geometry14B810 first and setup14C6B8 second as distinct groups.
 * OT head insertion later runs setup before geometry. Copy retained values. */
typedef gh_object_step_status (*gh_object_static_emit)(void *,const gh_object_static_command *,uint32_t *);
typedef struct {
 gh_scene_meshes *meshes;const gh_scene_at_manager *scene_at;
 const gh_game_pl_camera *camera;const gh_title_player_view *player;
 gh_stage_textures *textures;const gh_game_world_light *world_light;
 const gh_game_render_projection *projection;gh_object_static_emit emit;void *context;
} gh_object_static_bindings;
/* Original descriptor identity is read from the user's ELF; the address is
 * diagnostic and dispatches only through an explicitly recovered native body.
 * actor aliases borrow the SAME scalar allocation, never a copied actor. */
typedef struct {uint32_t handle,original_entry;int16_t this_delta;gh_title_player_view actor;} gh_object_move_request;
typedef gh_object_step_status (*gh_object_move_resolve)(void *,uint32_t,gh_title_player_view *,uint32_t *);
typedef gh_object_step_status (*gh_object_move_invoke)(void *,const gh_object_move_request *,const gh_boot_work *,uint32_t *);
typedef struct {gh_object_move_resolve resolve;gh_object_move_invoke invoke;void *context;const void *elf;size_t elf_bytes;} gh_object_move_bindings;
typedef enum {GH_OBJECT_TRANS_NORMAL,GH_OBJECT_TRANS_ALTERNATE,GH_OBJECT_TRANS_DESTROY} gh_object_trans_kind;
typedef struct {gh_object_trans_kind kind;uint32_t handle,original_entry,original_vtable,argument;
 int16_t this_delta;gh_title_player_view actor;} gh_object_trans_request;
typedef gh_object_step_status (*gh_object_trans_invoke)(void *,const gh_object_trans_request *,const gh_boot_work *,uint32_t *);
typedef gh_object_step_status (*gh_object_trans_layer)(void *,const gh_scene_at_manager *,uint8_t,int *visible,uint32_t *);
/* Optional canonical unregister for actor owners which must outlive the
 * registry binding. An unhandled request falls through to generic erase. */
typedef gh_object_step_status (*gh_object_trans_erase)(void *,const gh_object_trans_request *,
 const void *registry_owner,int *handled,uint32_t *);
typedef struct {gh_object_trans_invoke invoke;gh_object_trans_layer layer;void *context;
 const gh_scene_at_manager *scene_at;gh_object_trans_erase erase;} gh_object_trans_bindings;

typedef struct {gh_game_init_state *init;uint32_t ratio10_bits,profile_752cf8;uint8_t constructed;
 gh_object_static_bindings static_objects;uint32_t moved_objects_5caffc;
 uint64_t alpha_presets[8];
 uint8_t globals_loaded,depth_shift,static_bound,move_stage,move_object_stage,trans_stage,trans_phase;
 size_t move_index,move_count,trans_index,trans_material;
 const gh_room_pools *move_pools,*trans_pools;const gh_boot_work *move_work,*trans_work;
 gh_object_static_command pending;
 gh_object_move_bindings live_objects;gh_object_move_request move_request;
 gh_scene_mesh_object *move_static_object;
 uint8_t live_bound,move_native_phase,move_skip;
 gh_object_trans_bindings trans_live;gh_object_trans_request trans_live_request;
 uint32_t trans_live_phase,trans_live_handle;const void *trans_live_owner;
} gh_game_object_step;

typedef uint32_t (*gh_object_profile_counter)(void *);
/* Actual2C53A8 constructor precedes roompool construction. Clearsdiagnostic
 * pointertoken/on/side, setsratio1; preservescounter/progress/padding/profile.
 * Must not be reused to reset a live typed registry/list owner. */
gh_object_step_status gh_game_object_step_construct_002c53a8(gh_game_object_step *,gh_game_init_state *);
/* Actualentry2C53C8;2C53E0 is an interiorblock. Modulo32 arithmetic andsigned
 * count/progress comparisons, no traversal of the separate754C38 registry. */
gh_object_step_status gh_game_scroll_counter_move_002c53c8(gh_game_object_step *);
/* Actual30F090 empty/nativegated path. Timerreads alwaysrequired andprofiling
 * remains observable. Positive unskipped objects needtypedactors; precise
 * frontier beforeopaqueguestobjectaccess. No objectmove callback is faked. */
gh_object_step_status gh_game_move_objects_0030f090(gh_game_object_step *,
 const gh_room_pools *,const gh_boot_work *,gh_object_profile_counter,void *,uint32_t *frontier);
/* Actual30F650 registry0 empty earlyreturn, not a graphicsacknowledgment. */
gh_object_step_status gh_game_objects_trans_0030f650(const gh_room_pools *,uint32_t *frontier);
/* Cold5CAFFC via actualELF PT_LOAD/BSS once; OT+B58 via actual31A0E8
 * SBzero leaf. Rebinding never resets either scalar. Borrow canonicalowners. */
gh_object_step_status gh_game_object_step_bind_static(gh_game_object_step *,const gh_object_static_bindings *,const void *elf,size_t);
gh_object_step_status gh_game_object_step_bind_live(gh_game_object_step *,const gh_object_move_bindings *);
/* Extended entry retains the strict empty-list compatibility API above. */
gh_object_step_status gh_game_objects_trans_static_0030f650(gh_game_object_step *,const gh_room_pools *,const gh_boot_work *,uint32_t *);
#ifdef __cplusplus
}
#endif
#endif
