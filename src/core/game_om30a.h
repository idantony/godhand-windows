#ifndef GH_GAME_OM30A_H
#define GH_GAME_OM30A_H
#include "game_om303.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef gh_om303_state gh_om30a_state;
gh_scene_placement_status gh_om30a_construct_0018c098(void *,size_t,gh_title_player_view *);
gh_scene_placement_status gh_om30a_initialize_0018c180(gh_om30a_state *,gh_om_object *,const gh_om_initializer_bindings *,int *original_return);
gh_scene_placement_status gh_om30a_abort(gh_om30a_state *);
#ifdef __cplusplus
}
#endif
#endif
