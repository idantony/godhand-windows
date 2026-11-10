#ifndef GH_GAME_PLAYER_CHILD_MOVE_H
#define GH_GAME_PLAYER_CHILD_MOVE_H
#include "game_player_child_step.h"
#include "game_cloth_runtime.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct {gh_cloth_step_bindings cloth;gh_boot_work *work;const gh_title_player_view *work5f4;} gh_child_move_bindings;
typedef struct {gh_child_move_bindings bindings;gh_cloth_step_prefix simulation;uint32_t stage,frontier;uint8_t complete,skipped;} gh_child_move;
/* Real100530 caller activeprefix->149EB8->100CE0->104010. Same attachment
 * parent/part, canonicalmodels and springrecords. Current normal active path
 * reaches10430C precisely; force integration incomplete. Wind3AC800 path stops
 * strict147988, not silently clearing wind or claiming a finished frame. */
gh_title_player_status gh_game_player_child_move_00100530(gh_child_move *,const gh_child_move_bindings *);
#ifdef __cplusplus
}
#endif
#endif
