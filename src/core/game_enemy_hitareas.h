#ifndef GH_GAME_ENEMY_HITAREAS_H
#define GH_GAME_ENEMY_HITAREAS_H
#include "game_enemy_behavior.h"
#include "game_hitarea_shapes.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct {gh_enemy_behavior *behavior;gh_hitarea_shape_bindings b;gh_hitarea_object units[6];uint8_t created[6];
 int indices[9];uint32_t stage,body_cursor,aux_cursor,aux_stage,frontier;} gh_enemy_hitareas;
/* Actual2022D0 ordinary20A sixunits+nine shapes from SAMEcold574380 pool,
 * SAME400pairs/math andSAME31parts. Constructoronly; nocollisionqueryack. */
gh_actor_motion_status gh_enemy_hitareas_initialize_00207018(gh_enemy_hitareas *,gh_enemy_behavior *,const gh_hitarea_shape_bindings *);
/* Nativepausedprefix cleanup inreverseownunitorder; no globalpoolreset. */
gh_actor_motion_status gh_enemy_hitareas_abort_prefix(gh_enemy_hitareas *);
#ifdef __cplusplus
}
#endif
#endif
