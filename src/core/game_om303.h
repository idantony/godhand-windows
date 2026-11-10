#ifndef GH_GAME_OM303_H
#define GH_GAME_OM303_H
#include "game_om_initialize.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct {gh_om_init_state common;gh_hitarea_object sphere_unit;uint8_t *segments;uint32_t stage,frontier;uint8_t sphere_created;} gh_om303_state;
/* Actual313BB0 zeros9E0 before1879C8. This constructor itself preserves all
 * fields notwritten by originalchain; factoryowns itsrealzeroallocation. */
gh_scene_placement_status gh_om303_construct_001879c8(void *,size_t,gh_title_player_view *);
gh_scene_placement_status gh_om303_initialize_00187a78(gh_om303_state *,gh_om_object *,const gh_om_initializer_bindings *,int *original_return);
gh_scene_placement_status gh_om303_set_position_001c3778(gh_om303_state *,const uint8_t source_xyz[12]);
gh_scene_placement_status gh_om303_set_euler_001c3988(gh_om303_state *,const uint8_t source_xyz[12]);
gh_scene_placement_status gh_om303_abort(gh_om303_state *);
#ifdef __cplusplus
}
#endif
#endif
