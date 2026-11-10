#ifndef GH_GAME_OM304_H
#define GH_GAME_OM304_H
#include "game_om303.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Same nativeBbox+Asphere lifetimes; originalraw layouts/classes differ. */
typedef gh_om303_state gh_om304_state;
gh_scene_placement_status gh_om304_construct_00187c88(void *,size_t,gh_title_player_view *);
gh_scene_placement_status gh_om304_initialize_00187d88(gh_om304_state *,gh_om_object *,const gh_om_initializer_bindings *,int *original_return);
gh_scene_placement_status gh_om304_abort(gh_om304_state *);
#ifdef __cplusplus
}
#endif
#endif
