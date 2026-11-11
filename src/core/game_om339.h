#ifndef GH_GAME_OM339_H
#define GH_GAME_OM339_H

#include "game_om303.h"

#ifdef __cplusplus
extern "C" {
#endif

enum {GH_OM339_BYTES=0x7a0};
gh_scene_placement_status gh_om339_construct_0019e1c0(void *,size_t,gh_title_player_view *);
gh_scene_placement_status gh_om339_initialize_0019e2a8(gh_om303_state *,gh_om_object *,
    const gh_om_initializer_bindings *,int *);

#ifdef __cplusplus
}
#endif
#endif
