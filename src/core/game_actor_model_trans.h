#ifndef GH_GAME_ACTOR_MODEL_TRANS_H
#define GH_GAME_ACTOR_MODEL_TRANS_H
#include "game_actor_model_nodes.h"
#include "game_player_shadow.h"
#include "game_world_light.h"
#include "game_camera.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct {uint32_t entry,parity;gh_actor_model *model;const gh_player_shadow *shadow;
 const uint8_t *first_node;int32_t depth;uint8_t far_mode;} gh_actor_trans_request;
typedef gh_actor_model_status (*gh_actor_trans_callee)(void *,const gh_actor_trans_request *,uint32_t *frontier);
typedef struct {const gh_boot_work *work;const gh_game_pl_camera *camera;
 const gh_game_world_light *world_light;const uint8_t *depth_shift;
 const void *elf;size_t elf_bytes;const gh_player_shadow *shadow;
 gh_actor_trans_callee callee;void *context;gh_static_transform_policy numeric_policy;} gh_actor_trans_bindings;
typedef struct {gh_actor_model *model;gh_actor_trans_bindings bindings;const uint8_t *root_identity,*alias_identity;
 const uint8_t *first_node;size_t first_mesh;uint32_t phase,parity;
 int32_t distance,depth;uint16_t near_limit;uint8_t active,complete;
 gh_actor_trans_request pending;} gh_actor_trans_call;
/* Native14A3E8 prefix with explicit finite binary32 reference and fullFEnv
 * restoration. New original invocation gets a zero cursor; error retry uses
 * the SAME cursor/model/bindings. Opaque native packet/body calls are required
 * callbacks at exact addresses, never guestcode or successful draw stubs. */
gh_actor_model_status gh_actor_model_trans_0014a3e8(gh_actor_trans_call *,gh_actor_model *,const gh_actor_trans_bindings *,uint32_t *frontier);
/* Actual1F8AD8 CLIP-history equivalent finite affine domain. */
gh_actor_model_status gh_actor_model_box_visible_001f8ad8(const gh_actor_model *,const gh_game_pl_camera *,int *visible,uint32_t *frontier);
#ifdef __cplusplus
}
#endif
#endif
