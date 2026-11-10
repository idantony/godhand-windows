#ifndef GH_GAME_SCENE_POST_H
#define GH_GAME_SCENE_POST_H
#include "game_room_pools.h"
#include "game_scene_tables.h"
#include "game_camera.h"
#include "game_title_player.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef enum {GH_POST_OK,GH_POST_ARGUMENT,GH_POST_POOLS,GH_POST_NUMERIC,GH_POST_UNSUPPORTED,GH_POST_QUERY} gh_scene_post_status;
typedef struct {float from[4],to[4];} gh_scene_model_ray;
typedef struct {gh_boot_work *work;const gh_room_pools *pools;gh_game_scene_tables *tables;
 const gh_game_init_state *init;const gh_game_pl_camera *camera;const gh_title_player_view *player;
 gh_camera_numeric_policy policy;} gh_scene_post_bindings;
/* Actual754C50 list forwarding, with its original5D8 freeze gate. */
gh_scene_post_status gh_scene_scroll_forward_002c5470(const gh_room_pools *,const gh_boot_work *,uint32_t *frontier);
/* SAME747B20 owner alreadybound/reset bycase3 307A60: tables.scroll_link_words.
 * Nonzero originaltokens require realmodel resolution, never hostptrcasts. */
gh_scene_post_status gh_scene_hidden_reset_00308bc0(gh_game_scene_tables *,const gh_room_pools *,uint32_t *frontier);
void gh_scene_hidden_clear_00308d40(gh_game_scene_tables *);
/* Pure recovered307B28 rayconstruction. Explicitbinary32reference, original
 * smallangleFDlibmsine + existingverifiedVUcos/quaternion operations. */
gh_scene_post_status gh_scene_model_rays_00307b28(const gh_scene_post_bindings *,gh_scene_model_ray rays[5],size_t *count,uint32_t *frontier);
/* Actualreset/clear/rays andTWO real131E10 queriesperray. UsesONEcanonical
 * actorqueryowner via game_ray_query. Onlymisses andemptytargetregistries
 * currentlycomplete; hitmodel consumers remainprecisefrontiers. */
gh_scene_post_status gh_scene_model_hide_00307b28(const gh_scene_post_bindings *,uint32_t *frontier);
#ifdef __cplusplus
}
#endif
#endif
