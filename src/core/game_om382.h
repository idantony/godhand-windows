#ifndef GH_GAME_OM382_H
#define GH_GAME_OM382_H

#include "game_om_frame.h"
#include "game_actor_motion_call.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    gh_om_init_state common;
    gh_hitarea_object hit[2];
    int shape_index[2];
    gh_actor_motion motion;
    gh_actor_motion_call select_motion;
    gh_bundle_entry clip[2];
    uint32_t frontier;
    uint8_t hit_owned,hit_phase;
} gh_om382_state;

gh_scene_placement_status gh_om382_construct_001c9140(void *,size_t,gh_title_player_view *);
gh_scene_placement_status gh_om382_initialize_001c9268(gh_om382_state *,gh_om_object *,
    const gh_om_initializer_bindings *,int *);
gh_scene_placement_status gh_om382_abort(gh_om382_state *);
gh_object_step_status gh_om382_move_prefix_001c9860(gh_om382_state *,const gh_om_frame_runtime *,
    const gh_object_opacity_environment *,uint8_t *phase,float *rate,
    gh_static_transform_policy,uint32_t *frontier);

#ifdef __cplusplus
}
#endif
#endif
