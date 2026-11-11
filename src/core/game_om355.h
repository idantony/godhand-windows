#ifndef GH_GAME_OM355_H
#define GH_GAME_OM355_H

#include "game_om_initialize.h"

#ifdef __cplusplus
extern "C" {
#endif

enum {GH_OM355_BYTES=0x610};
gh_scene_placement_status gh_om355_construct_001a8348(void *,size_t,gh_title_player_view *);
gh_scene_placement_status gh_om355_initialize_001a83e0(gh_om_init_state *,gh_om_object *,
    const gh_om_initializer_bindings *,int *);

#ifdef __cplusplus
}
#endif
#endif
