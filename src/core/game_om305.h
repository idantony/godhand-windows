#ifndef GH_GAME_OM305_H
#define GH_GAME_OM305_H
#include "game_om303.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef gh_om303_state gh_om305_state;
gh_scene_placement_status gh_om305_construct_00188a68(void *,size_t,gh_title_player_view *);
gh_scene_placement_status gh_om305_initialize_00188b50(gh_om305_state *,gh_om_object *,const gh_om_initializer_bindings *,int *original_return);
gh_scene_placement_status gh_om305_abort(gh_om305_state *);
#ifdef __cplusplus
}
#endif
#endif
