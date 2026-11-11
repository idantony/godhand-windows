#ifndef GH_GAME_ENEMY_MARKER_QUAD_H
#define GH_GAME_ENEMY_MARKER_QUAD_H
#include "game_effects_trans.h"
#include "id_jobs.h"
#include "game_player_child_tail.h"

#ifdef __cplusplus
extern "C" {
#endif
/* Semantic 2AB360/15BCF0 quad. GS FRAME and ZBUF remain inherited.
 * The texture view borrows the same ID manager language bundle. */
typedef struct {
 gh_ui_order_group order;
 gh_effect_trans_quad quad;
 gh_id_texture texture;
 uint64_t test, alpha, tex1, clamp;
 uint16_t prim;
 uint8_t visible;
} gh_enemy_marker_quad;

gh_actor_model_status gh_enemy_marker_quad_002ab360(const gh_game_pl_camera *,
 const gh_player_shared_vector *,const gh_id_state *,const gh_id_base *,gh_id_entry *,
 gh_enemy_marker_quad *,uint32_t *);

#ifdef __cplusplus
}
#endif
#endif
