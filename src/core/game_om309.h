#ifndef GH_GAME_OM309_H
#define GH_GAME_OM309_H
#include "game_om303.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef gh_om303_state gh_om309_state;
gh_scene_placement_status gh_om309_construct_0018b290(void *,size_t,gh_title_player_view *);
gh_scene_placement_status gh_om309_initialize_0018b378(gh_om309_state *,gh_om_object *,const gh_om_initializer_bindings *,int *original_return);
gh_scene_placement_status gh_om309_abort(gh_om309_state *);
#ifdef __cplusplus
}
#endif
#endif
