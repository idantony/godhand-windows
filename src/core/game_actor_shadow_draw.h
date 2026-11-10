#ifndef GH_GAME_ACTOR_SHADOW_DRAW_H
#define GH_GAME_ACTOR_SHADOW_DRAW_H
#include "game_actor_model_trans.h"
#include "game_camera_follow.h"
#include "game_player_child_tail.h"
#include "game_ray_query.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct {uint8_t player_pattern[64],initialized;const void *identity;} gh_shadow_templates;
gh_actor_model_status gh_shadow_templates_from_elf(gh_shadow_templates *,const void *,size_t);
typedef struct {float size,position[3],quaternion[4];int32_t xy_fixed4[4][2],z_integer[4];uint8_t alpha,enabled;} gh_shadow_quad;
typedef struct {const gh_player_shadow *owner;uint32_t parity,bank,depth;uint64_t tex0,tex1,alpha,test,clamp;uint16_t primitive;gh_shadow_quad quads[12];uint8_t ready;} gh_shadow_group;
typedef gh_actor_model_status (*gh_shadow_emit)(void *,const gh_shadow_group *,uint32_t *);
typedef gh_scene_ray_status (*gh_shadow_ray)(void *,gh_game_scene_tables *,const gh_scene_ray_request *,int *,uint32_t *);
typedef struct {gh_actor_model *model;const gh_player_shadow *shadow;const gh_boot_work *work;
 const gh_game_pl_camera *camera;const gh_game_world_light *world_light;gh_game_scene_tables *actors;
 const gh_camera_follow *up_owner;const gh_player_shared_vector *pivot_owner;const gh_shadow_templates *templates;
 gh_shadow_emit emit;void *emit_context;gh_shadow_ray ray;void *ray_context;gh_static_transform_policy policy;} gh_shadow_bindings;
typedef struct {gh_shadow_bindings bindings;gh_shadow_group group;gh_scene_ray_request ray;
 float scale,ray_length,direction[3],current_point[3],previous_point[3],from[3],normal[3];
 float current_quat[4],previous_quat[4],current_alpha,previous_alpha,current_size,previous_size;
 uint32_t phase,slot,row,remaining,row_count;uint8_t active,complete,far_mode,current_valid,previous_valid,invalid_part;
 gh_shadow_quad pending;const void *identity,*root_identity,*alias_identity;} gh_shadow_call;
/* Source15EC60: actualquaternion+scale, all-fourclip gate, projected12.4XY/
 * integerZ. Camera matrices areborrowed; nativebinary32 reference, noGPUack. */
gh_actor_model_status gh_shadow_quad_project_0015ec60(gh_shadow_quad *,const gh_game_pl_camera *,gh_static_transform_policy,uint32_t *);
/* Oneoriginal14EFA8. Alloriginal ray/part/packetprefixes persist onfailure;
 * samebindings required forretry. Complete requires the realONEgroup emit. */
gh_actor_model_status gh_actor_shadow_draw_0014efa8(gh_shadow_call *,const gh_shadow_bindings *,uint32_t *);
#ifdef __cplusplus
}
#endif
#endif
