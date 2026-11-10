#ifndef GH_GAME_EFFECT_NODE_H
#define GH_GAME_EFFECT_NODE_H
#include "game_effects.h"
#include "game_effect_create.h"
#include "../assets/static_mesh_transform.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Typed samepool context. Supported topology is original parentless ambient
 * emitters with ESPchildren; no guestpointer cast or duplicate tree manager. */
gh_effect_status gh_effect_node_world_matrix_native(gh_game_effects *,int emitter,unsigned,uint32_t,float out[16],gh_static_transform_policy,uint32_t *);
gh_effect_status gh_effect_node_world_position_native(gh_game_effects *,int emitter,unsigned,uint32_t,float out[4],gh_static_transform_policy,uint32_t *);
gh_effect_status gh_effect_node_set_euler_native(gh_game_effects *,int emitter,unsigned,uint32_t,const float xyz[3],uint32_t *);
gh_effect_status gh_effect_node_set_position_native(gh_game_effects *,int emitter,unsigned,uint32_t,const float xyz[3],uint32_t *);
gh_effect_status gh_effect_node_set_scale_native(gh_game_effects *,int emitter,unsigned,uint32_t,const float xyz[3],uint32_t *);
typedef struct {const void *identity;gh_game_effects *effects;unsigned slot;uint32_t serial;int32_t parent;uint32_t parent_serial;
 uint8_t stage,complete;float local_euler[3],world_scale[3],world_euler[3],world_position[3];} gh_effect_node_detach_call;
/* One actualvirtual2FCB88 invocation; retry must retain this cursor. */
gh_effect_status gh_effect_node_detach_update_002fcb88(gh_effect_node_detach_call *,gh_game_effects *,unsigned,uint32_t,const float atan_coeff[8],gh_static_transform_policy,uint32_t *);
#ifdef __cplusplus
}
#endif
#endif
