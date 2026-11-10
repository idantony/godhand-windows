#ifndef GH_GAME_OM310_H
#define GH_GAME_OM310_H
#include "game_om303.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct {gh_om303_state base;const gh_title_player_view *player;} gh_om310_state;
/* Explicit pinnedPCSX2-default ChopZero reference, NOT PS2 hardwareproof.
 * For |exactF32factor*80|<2^24, RZbinary32thenTRUNC equalsTRUNCexactproduct. */
gh_scene_placement_status gh_om310_damage_rz_reference(uint32_t factor_bits,int32_t *damage,uint32_t *frontier);
gh_scene_placement_status gh_om310_construct_0018f110(void *,size_t,gh_title_player_view *);
gh_scene_placement_status gh_om310_initialize_0018f218(gh_om310_state *,gh_om_object *,const gh_om_initializer_bindings *,int *original_return);
gh_scene_placement_status gh_om310_abort(gh_om310_state *);
#ifdef __cplusplus
}
#endif
#endif
