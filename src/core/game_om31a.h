#ifndef GH_GAME_OM31A_H
#define GH_GAME_OM31A_H

#include "game_om303.h"

#ifdef __cplusplus
extern "C" {
#endif

enum { GH_OM31A_BYTES = 0x7a0 };
gh_scene_placement_status gh_om31a_construct_001931f0(void *,size_t,gh_title_player_view *);
gh_scene_placement_status gh_om31a_initialize_001932d8(gh_om303_state *,gh_om_object *,
 const gh_om_initializer_bindings *,int *original_return);

#ifdef __cplusplus
}
#endif

#endif
