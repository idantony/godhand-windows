#ifndef GH_GAME_OM041C_H
#define GH_GAME_OM041C_H

#include "game_om_initialize.h"
#include "game_actor_motion_ik.h"

#ifdef __cplusplus
extern "C" {
#endif

enum {GH_OM041C_BYTES=0x1db0};
typedef struct {
    gh_om_init_state common;
    const uint8_t *part_bases,*point_setup,*spring_setup;
    uint32_t spring_coefficients[2],frontier;
    float gravity,drag,force_cap;
    uint8_t initialized;
} gh_om041c_state;

gh_scene_placement_status gh_om041c_construct_0017b870(void *,size_t,gh_title_player_view *);
gh_scene_placement_status gh_om041c_initialize_0017b940(gh_om041c_state *,gh_om_object *,
    const gh_om_initializer_bindings *,int *);
gh_scene_placement_status gh_om041c_set_position_0017c3a0(gh_om041c_state *,const void *);
gh_scene_placement_status gh_om041c_set_euler_0017c408(gh_om041c_state *,const void *);
gh_scene_placement_status gh_om041c_add_wind_002ae218(gh_om041c_state *,const float center[3],
    const float direction[3],float radius,float strength);
/* The caller owns the invocation phase. Phase 1 resumes bone publication
 * after the once-per-frame original cloth integration has committed. */
gh_scene_placement_status gh_om041c_move_0017baf0(gh_om041c_state *,const gh_motion_ik_globals *,uint8_t *phase);
gh_scene_placement_status gh_om041c_abort(gh_om041c_state *);

#ifdef __cplusplus
}
#endif
#endif
