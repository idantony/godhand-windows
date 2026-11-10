#ifndef GH_GAME_OM30E_H
#define GH_GAME_OM30E_H
#include "game_om303.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef gh_om303_state gh_om30e_state;
gh_scene_placement_status gh_om30e_construct_0018ec98(void *,size_t,gh_title_player_view *);
gh_scene_placement_status gh_om30e_initialize_0018ed30(gh_om30e_state *,gh_om_object *,const gh_om_initializer_bindings *,int *original_return);
gh_scene_placement_status gh_om30e_abort(gh_om30e_state *);
#ifdef __cplusplus
}
#endif
#endif
