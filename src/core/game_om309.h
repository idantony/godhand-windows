#ifndef GH_GAME_OM309_H
#define GH_GAME_OM309_H
#include "game_om303.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef gh_om303_state gh_om309_state;
gh_scene_placement_status gh_om308_construct_0018a598(void *,size_t,gh_title_player_view *);
gh_scene_placement_status gh_om308_initialize_0018a680(gh_om309_state *,gh_om_object *,const gh_om_initializer_bindings *,int *);
gh_scene_placement_status gh_om309_construct_0018b290(void *,size_t,gh_title_player_view *);
gh_scene_placement_status gh_om309_initialize_0018b378(gh_om309_state *,gh_om_object *,const gh_om_initializer_bindings *,int *original_return);
gh_scene_placement_status gh_om30b_construct_0018cea0(void *,size_t,gh_title_player_view *);
gh_scene_placement_status gh_om30c_construct_0018dc58(void *,size_t,gh_title_player_view *);
gh_scene_placement_status gh_om30b_initialize_0018cf88(gh_om309_state *,gh_om_object *,const gh_om_initializer_bindings *,int *);
gh_scene_placement_status gh_om30c_initialize_0018dd40(gh_om309_state *,gh_om_object *,const gh_om_initializer_bindings *,int *);
gh_scene_placement_status gh_om331_construct_0019c588(void *,size_t,gh_title_player_view *);
gh_scene_placement_status gh_om331_initialize_0019c670(gh_om309_state *,gh_om_object *,const gh_om_initializer_bindings *,int *);
gh_scene_placement_status gh_om332_initialize_0019c670(gh_om309_state *,gh_om_object *,const gh_om_initializer_bindings *,int *);
gh_scene_placement_status gh_om309_abort(gh_om309_state *);
#ifdef __cplusplus
}
#endif
#endif
