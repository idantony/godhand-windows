#ifndef GH_GAME_OM307_H
#define GH_GAME_OM307_H
#include "game_om_initialize.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct {gh_om_init_state common;gh_hitarea_object box_a,box_b2;
 uint32_t stage,frontier;uint8_t a_created,b2_created;} gh_om307_state;
gh_scene_placement_status gh_om307_construct_00188f80(void *,size_t,gh_title_player_view *);
gh_scene_placement_status gh_om307_initialize_001890b0(gh_om307_state *,gh_om_object *,const gh_om_initializer_bindings *,int *original_return);
gh_scene_placement_status gh_om307_abort(gh_om307_state *);
#ifdef __cplusplus
}
#endif
#endif
