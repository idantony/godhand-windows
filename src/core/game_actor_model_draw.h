#ifndef GH_GAME_ACTOR_MODEL_DRAW_H
#define GH_GAME_ACTOR_MODEL_DRAW_H
#include "game_actor_model_trans.h"
#include "game_actor_model_material.h"
#include "game_actor_model_program2.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Actual14C7E8/1503B8: camera constants and world*inverse-bind-translation
 * palette in the SAME model object packet allocation. No skin/GPU execution. */
gh_actor_model_status gh_actor_palette_0014c7e8(gh_actor_model *,const gh_game_pl_camera *,
 gh_static_transform_policy,uint32_t *frontier);
gh_actor_model_status gh_actor_node_light_00154240(gh_actor_model *,unsigned,const gh_game_pl_camera *,const gh_game_world_light *,gh_static_transform_policy,uint32_t *frontier);
typedef struct {uint32_t entry,parity;size_t node_index;gh_actor_model *model;
 const gh_game_pl_camera *camera;const gh_game_world_light *light;uint16_t depth;uint8_t far_mode;const gh_actor_materials *materials;} gh_actor_draw_request;
typedef gh_actor_model_status (*gh_actor_draw_callee)(void *,const gh_actor_draw_request *,uint32_t *);
typedef enum {GH_ACTOR_DRAW_GEOMETRY,GH_ACTOR_DRAW_SETUP} gh_actor_draw_kind;
/* One original OT node. Geometry preserves stream then batch order. Setup
 * writes ONLY FOGCOL and object VU/palette references, not FRAME/ZBUF/etc.
 * Same-frame borrowed packet/palette/cache lifetime; no guest DMA execution. */
typedef struct {gh_actor_draw_kind kind;gh_actor_model *model;size_t node_index;
 uint32_t parity,bank,depth,selected_program;uint64_t test,alpha,tex1,clamp;
 const gh_actor_node_packets *packet;const gh_actor_object_packets *object;
 gh_actor_materials materials;gh_skinned_program2_inputs inputs;
 uint32_t fog_rgb;uint8_t packet_group;} gh_actor_draw_command;
typedef gh_actor_model_status (*gh_actor_draw_emit)(void *,const gh_actor_draw_command *,uint32_t *);
typedef struct {const gh_game_pl_camera *camera;const gh_game_world_light *light;
 gh_actor_draw_callee callee;void *context;gh_static_transform_policy policy;gh_stage_textures *textures;gh_actor_draw_emit emit;void *emit_context;} gh_actor_draw_bindings;
typedef struct {gh_actor_model *model;gh_actor_draw_bindings bindings;const void *identity,*root_identity;
 size_t node_index;uint32_t phase,parity;uint16_t depth;uint8_t active,complete,far_mode;gh_actor_materials materials;size_t previous_node;uint32_t previous_program,selected_program;uint8_t has_previous;gh_actor_draw_command pending;const void *work_identity;uint32_t frame_counter;} gh_actor_draw_call;
/* Bounded original14AC90 program2 body. Complete means original OT enqueue
 * finished, never GPU/frame success. Required emit queues one whole group;
 * later native consumer must execute every stream/batch. Retry preserves prefix. */
gh_actor_model_status gh_actor_model_draw_0014ac90(gh_actor_draw_call *,gh_actor_model *,uint16_t depth,
 uint8_t far_mode,const gh_actor_draw_bindings *,uint32_t *frontier);
#ifdef __cplusplus
}
#endif
#endif
