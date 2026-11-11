#ifndef GH_GAME_ENEMY_INITIALIZE_H
#define GH_GAME_ENEMY_INITIALIZE_H
#include "game_enemy_model.h"
#include "game_enemy_factory.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Semantic actual2022D0 request fields. Unread gueststack holes omitted. */
typedef struct {uint32_t object_kind,position_xyz[3],yaw_bits,flags,dependency_kind,dependency_flags;uint8_t appearance,entry_number;} gh_enemy_request;
typedef struct {gh_enemy_model model;const gh_title_player_view *root;gh_enemy_request request;uint32_t stage,frontier;} gh_enemy_initialize;
gh_actor_model_status gh_enemy202_initialize_prefix_002022d0(gh_enemy_initialize *,const gh_title_player_view *,
 const gh_enemy_request *,const gh_object_view *,gh_boot_work *,gh_core_save_view,const uint8_t *scenario14,
 const uint8_t *borrowed586aa4,gh_room_pools *,gh_heap *,const void *fog,gh_static_transform_policy);
gh_actor_model_status gh_enemy202_initialize_abort_prefix(gh_enemy_initialize *);
#ifdef __cplusplus
}
#endif
#endif
