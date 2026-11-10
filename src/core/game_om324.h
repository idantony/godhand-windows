#ifndef GH_GAME_OM324_H
#define GH_GAME_OM324_H
#include "game_om303.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef gh_om303_state gh_om324_state;
gh_scene_placement_status gh_om324_construct_00198ca8(void *,size_t,gh_title_player_view *);
gh_scene_placement_status gh_om324_initialize_00198d40(gh_om324_state *,gh_om_object *,const gh_om_initializer_bindings *,int *original_return);
gh_scene_placement_status gh_om324_abort(gh_om324_state *);
/* Explicit native binary32-nearest reference; original division at198ECC/E4/F4. */
gh_scene_placement_status gh_om324_scale_reference(uint8_t selector,uint32_t out_xyz[3],uint32_t *frontier);
#ifdef __cplusplus
}
#endif
#endif
