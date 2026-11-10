#ifndef GH_GAME_OM33F_H
#define GH_GAME_OM33F_H
#include "game_om303.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef gh_om303_state gh_om33f_state;
gh_scene_placement_status gh_om33f_construct_0019f0a8(void *,size_t,gh_title_player_view *);
gh_scene_placement_status gh_om33f_initialize_0019f190(gh_om33f_state *,gh_om_object *,const gh_om_initializer_bindings *,int *original_return);
gh_scene_placement_status gh_om33f_abort(gh_om33f_state *);
#ifdef __cplusplus
}
#endif
#endif
