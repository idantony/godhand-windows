#ifndef GH_GAME_OM35D_H
#define GH_GAME_OM35D_H
#include "game_om_initialize.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct {gh_om_init_state common;gh_collision_solid_unit *solid_unit;gh_collision_native_sphere *sphere;uint32_t stage,frontier;uint8_t solid_created;} gh_om35d_state;
gh_scene_placement_status gh_om35d_construct_001ae798(void *,size_t,gh_title_player_view *);
/* Explicit bounded PCSX2-default ChopZero multiplication reference, not an EE hardware capture. */
gh_scene_placement_status gh_om35d_hp_chopzero_reference(uint8_t level,uint16_t *hp,uint32_t *frontier);
gh_scene_placement_status gh_om35d_initialize_001ae928(gh_om35d_state *,gh_om_object *,const gh_om_initializer_bindings *,int *original_return);
gh_scene_placement_status gh_om35d_abort(gh_om35d_state *);
#ifdef __cplusplus
}
#endif
#endif
