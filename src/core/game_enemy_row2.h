#ifndef GH_GAME_ENEMY_ROW2_H
#define GH_GAME_ENEMY_ROW2_H
#include "game_enemy_initialize.h"
#ifdef __cplusplus
extern "C" {
#endif

/* Bounded actual R100 row2 profile for shared EM202 initialization. */
int gh_enemy202_is_row2_profile(const gh_enemy_request *);
int gh_enemy202_is_r100_appearance2_profile(const gh_enemy_request *);
int gh_enemy202_is_r100_supported_profile(const gh_enemy_request *);
/* Original 2070CC default body radius, explicit bounded ChopZero. */
int gh_enemy202_body_radius_002070cc(uint32_t scale,uint32_t nominal,uint32_t *bits);

/* Verified subset of original 2DDEA0. Unsupported pairs leave output alone. */
gh_actor_model_status gh_enemy_model_occurrence_002ddea0(uint32_t kind,
    uint32_t dependency, unsigned *occurrence);

typedef struct {
    size_t motion_offset;
    size_t sequence_offset;
    uint8_t main_state;
    uint8_t substate;
    uint8_t phase;
    uint8_t phase_detail;
} gh_enemy_initial_motion_selection;

/* Original appearance-2 branch 203C38 on unexchanged EM202/200, /202, /203. The caller
 * still owns the state writes and the actual 2A8578 Begin call. */
gh_actor_model_status gh_enemy202_appearance2_select_00203c38(
    const gh_enemy_request *, const gh_object_view *, gh_enemy_initial_motion_selection *);
/* Actual R100 appearance-5 rows and their shared motion directory. */
gh_actor_model_status gh_enemy202_appearance5_select_00203b3c(
    const gh_enemy_request *, const gh_object_view *, gh_enemy_initial_motion_selection *);
/* R100 appearance9 uses the dependency's already selected directory. */
gh_actor_model_status gh_enemy202_appearance9_select_0020393c(
    const gh_enemy_request *, const gh_object_view *, gh_enemy_initial_motion_selection *);
gh_actor_model_status gh_enemy202_appearance0_select_0020393c(
    const gh_enemy_request *, const gh_object_view *, gh_enemy_initial_motion_selection *);
#ifdef __cplusplus
}
#endif
#endif
