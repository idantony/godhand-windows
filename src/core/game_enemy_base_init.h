#ifndef GH_GAME_ENEMY_BASE_INIT_H
#define GH_GAME_ENEMY_BASE_INIT_H
#include "game_enemy_initialize.h"
#include "game_cockpit.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct {gh_id_base *cockpit_base;const gh_title_player_view *root;uint32_t stage,frontier;} gh_enemy_base_initialize;
gh_actor_model_status gh_enemy_base_initialize_0028ebf0(gh_enemy_base_initialize *,const gh_title_player_view *,const gh_enemy_request *,uint32_t argument,gh_game_cockpit *,gh_game_mt *,int *original_return);
#ifdef __cplusplus
}
#endif
#endif
