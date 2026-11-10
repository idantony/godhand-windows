#ifndef GH_GAME_PLAYER_POST_INIT_H
#define GH_GAME_PLAYER_POST_INIT_H
#include "game_player_init.h"
#include "game_actor_model.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Only the actualmode0 callfrom107F58. SamecanonicalHeat owner, writesbyte10
 * and delta8=max(current4-drainC,0), preservesallotherfields/FEnv.
 * Nonfinite values remain unsupported ratherthan being replacedwithzero. */
gh_player_init_status gh_game_heat_clear_mode_002a9cc8(gh_game_init_state *,gh_static_transform_policy,uint32_t *frontier);
typedef struct {gh_game_title_player *player;gh_actor_model *model;gh_game_init_state *init;
 gh_boot_work *work;gh_core_save_view save;uint32_t stage,frontier,pending_object_id;
 uint8_t bound,complete;} gh_game_player_post_init;
/* Exact107F58 suffix AFTER real152A20 returned. 14A038's flags250&20 clear
 * branch is genuineempty; positive branch remains strictactualcallee frontier.
 * Then realHeat/position/statewrites; save157 true requires12C8D8 Obj376 and
 * never fabricatesitsfactory. Ready meansonlythiscaller suffixcompleted. */
gh_player_init_status gh_game_player_post_initialize_00107f58(gh_game_player_post_init *,gh_game_title_player *,gh_actor_model *,gh_game_init_state *,gh_boot_work *,gh_core_save_view,gh_static_transform_policy,int *original_return);
#ifdef __cplusplus
}
#endif
#endif
