#ifndef GH_GAME_ACTOR_MODEL_H
#define GH_GAME_ACTOR_MODEL_H
#include "game_title_player.h"
#include "../assets/model.h"
#include "../assets/core_bundle.h"
#include "../assets/static_mesh_transform.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Original cParts storage comes from SAMEroompool0, not another skeleton.
 * Guest144/148/14C pointers are hosttyped links below, never x64casts. */
typedef struct {uint32_t handle;uint8_t *bytes,*alias_d0,*alias_f0;
 int16_t parent,mirror;uint8_t parent_is_model,mirror_is_self;} gh_actor_part;
typedef enum {GH_ACTOR_MODEL_OK,GH_ACTOR_MODEL_ARGUMENT,GH_ACTOR_MODEL_BOUNDS,
 GH_ACTOR_MODEL_MEMORY,GH_ACTOR_MODEL_BUSY,GH_ACTOR_MODEL_UNSUPPORTED} gh_actor_model_status;
struct gh_actor_model_nodes;
struct gh_motion_ik_globals;
struct gh_actor_external_parent_set;
typedef struct gh_actor_model {gh_title_player_view root;gh_boot_work *work;gh_room_pools *pools;
 gh_model_view script;gh_bundle_entry texture;gh_actor_part parts[255];
 uint32_t part_count,parts_allocated,stage,texture_flags,packet_argument,texture_mode,frontier,detail;
 struct gh_actor_model_nodes *nodes;gh_heap *packet_heap;const void *borrowed_fog_rgb;
 uint32_t first_node_handle;uint8_t *first_node;uint8_t bound,parts_ready,ready;
 /* Native lifetime identity and actual cross-model part148 link (100ED8). */
 struct gh_actor_model *native_identity,*external_parent;const void *external_owner;
 const uint8_t *external_parent_bytes;uint32_t external_parent_index,external_parent_handle,external_child_handle,external_borrowers;
 const struct gh_motion_ik_globals *blend_globals; /* borrowed SAME motion setup */
 uint32_t sound_borrowers; /* hostlifetime only, notguestfield */
 struct gh_actor_external_parent_set *external_parts;
} gh_actor_model;
/* Real149738 prefix and149C20 skeletalbind initialization. Current finite
 * zero-rotation/unit-scale reference domain reproduces initialPL00/OM00 pose.
 * Partallocations, typedlinks, local/world matrices and inversebindXYZ are
 * genuine. Optional explicit packet inputs extend through the complete ordered node
 * loop and14BFE0 palette construction. ready means149738 initialization only,
 * neverGPU/skinning execution or animation readiness. MissingMD follows
 * originalreturn0 after mandatoryinitialwrites. Retry sameoperation isstable. */
gh_actor_model_status gh_actor_model_initialize_00149738(gh_actor_model *,const gh_title_player_view *,gh_boot_work *,
 gh_room_pools *,const gh_bundle_entry *md,const gh_bundle_entry *texture,uint32_t packet_argument,
 uint32_t texture_mode,gh_static_transform_policy,int *original_return);
/* Explicit borrowed platform inputs for original default/specified packet heap
 * and canonical604704 fog snapshot. Bind before first initialize; no copyowner. */
gh_actor_model_status gh_actor_model_bind_packet_inputs(gh_actor_model *,gh_heap *,const void *fog_rgb);
gh_actor_model_status gh_actor_model_node_at(const gh_actor_model *,unsigned,const uint8_t **,size_t *mesh_index);
/* Actual149EB8 bounded generalEuler/scale reference hierarchy update; arbitrary finite
 * stored local matrices allowed only when original154 bit8 skips rebuild.
 * Original SDKorders0..5/defaultidentity and XYZEulerwrap restored.
 * Does not recompute inversebindC0 or pretend animation/blend is restored. */
gh_actor_model_status gh_actor_model_update_hierarchy_00149eb8(gh_actor_model *,gh_static_transform_policy);
gh_actor_model_status gh_actor_model_part_at(const gh_actor_model *,unsigned,const gh_actor_part **);
/* Nativeabort constructorprefix only. Releases ownblock handles and clears
 * associated modelcount/list metadata; no virtualmodel destructoracknowledged. */
gh_actor_model_status gh_actor_model_release(gh_actor_model *);
/* Native strongborrow for actualattachedEEvoice references. Return before
 * modelrelease via originalDetach/EOS or explicit hostabort. */
gh_actor_model_status gh_actor_model_sound_borrow(gh_actor_model *);
gh_actor_model_status gh_actor_model_sound_return(gh_actor_model *);
#ifdef __cplusplus
}
#endif
#endif
