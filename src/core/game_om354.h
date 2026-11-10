#ifndef GH_GAME_OM354_H
#define GH_GAME_OM354_H
#include "game_om_initialize.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef gh_om_init_state gh_om354_state;
gh_scene_placement_status gh_om354_construct_001a8150(void *,size_t,gh_title_player_view *);
gh_scene_placement_status gh_om354_initialize_001a8200(gh_om354_state *,gh_om_object *,const gh_om_initializer_bindings *,int *original_return);
gh_scene_placement_status gh_om354_abort(gh_om354_state *);
#ifdef __cplusplus
}
#endif
#endif
