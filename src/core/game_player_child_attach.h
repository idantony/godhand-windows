#ifndef GH_GAME_PLAYER_CHILD_ATTACH_H
#define GH_GAME_PLAYER_CHILD_ATTACH_H
#include "game_player_child.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Cross-model cParts parent is not a local int16 index. This companion owns
 * no storage and borrows SAME parent/child wrappers and their real part arrays.
 * Release/clear before either model or factory lifetime ends. */
typedef struct {gh_game_player_child *child;gh_game_player_factory *parent;
 gh_actor_model *parent_model;const gh_actor_part *child_part,*parent_part;
 int32_t part_index;uint8_t attached;} gh_player_child_attachment;
/* Exact100ED8: absentparent/part is original no-op. The successfulbranch
 * replaces raw148/20A8/20D0 pointerfields with this typed cross-owner link.
 * Does not initialize either model, update matrices, or publishWork5F8. */
gh_title_player_status gh_game_player_child_attach_00100ed8(gh_player_child_attachment *,gh_game_player_child *,gh_game_player_factory *,gh_actor_model *,int32_t part_index,gh_core_save_view);
void gh_game_player_child_attachment_clear(gh_player_child_attachment *);
#ifdef __cplusplus
}
#endif
#endif
