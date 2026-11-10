#ifndef GH_GAME_OM322_H
#define GH_GAME_OM322_H
#include "game_om_initialize.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct {gh_om_init_state common;gh_hitarea_object box_a;uint32_t stage,frontier;uint8_t a_created;} gh_om322_state;
gh_scene_placement_status gh_om322_construct_00196058(void *,size_t,gh_title_player_view *);
gh_scene_placement_status gh_om322_initialize_00196188(gh_om322_state *,gh_om_object *,const gh_om_initializer_bindings *,int *original_return);
gh_scene_placement_status gh_om322_abort(gh_om322_state *);
#ifdef __cplusplus
}
#endif
#endif
