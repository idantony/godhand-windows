#ifndef GH_GAME_ENEMY_MODEL_H
#define GH_GAME_ENEMY_MODEL_H
#include "game_actor_model.h"
#include "game_objects.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Bounded original208268 EM202/20A path. Resource is SAME acquired holder;
 * file_extent is distinct from allocated capacity. No resource copy/registry. */
typedef struct {gh_actor_model model;gh_bundle_entry geometry,texture;const uint8_t *resource;
 uint32_t occurrence,stage,frontier;} gh_enemy_model;
gh_actor_model_status gh_enemy_model_initialize_00208268(gh_enemy_model *,const gh_title_player_view *,
 const gh_object_view *,gh_boot_work *,gh_room_pools *,gh_heap *,const void *fog_rgb,
 gh_static_transform_policy,int *original_return);
gh_actor_model_status gh_enemy_model_release_prefix(gh_enemy_model *);
#ifdef __cplusplus
}
#endif
#endif
