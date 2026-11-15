#ifndef GH_GAME_OM311_H
#define GH_GAME_OM311_H

#include "game_om_initialize.h"

#ifdef __cplusplus
extern "C" {
#endif

enum { GH_OM311_BYTES=0xfa0, GH_OM311_FRAGMENTS=25 };

typedef struct {
    gh_om_init_state common;
    gh_hitarea_object anchor_box;
    gh_hitarea_object boxes[4];
    uint32_t stage, cursor, frontier;
    uint8_t anchor_created, created[4];
} gh_om311_state;

gh_scene_placement_status gh_om311_construct_001901c8(void *,size_t,gh_title_player_view *);
gh_scene_placement_status gh_om311_initialize_001902f8(gh_om311_state *,gh_om_object *,
    const gh_om_initializer_bindings *,int *original_return);
gh_scene_placement_status gh_om311_abort(gh_om311_state *);

#ifdef __cplusplus
}
#endif
#endif
