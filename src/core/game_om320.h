#ifndef GH_GAME_OM320_H
#define GH_GAME_OM320_H
#include "game_om303.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef gh_om303_state gh_om320_state;
gh_scene_placement_status gh_om320_construct_00195c20(void *,size_t,gh_title_player_view *);
gh_scene_placement_status gh_om320_initialize_00195d08(gh_om320_state *,gh_om_object *,const gh_om_initializer_bindings *,int *original_return);
gh_scene_placement_status gh_om320_abort(gh_om320_state *);
#ifdef __cplusplus
}
#endif
#endif
