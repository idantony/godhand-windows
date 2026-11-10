#ifndef GH_GAME_PLAYER_CHILD_UPDATE_H
#define GH_GAME_PLAYER_CHILD_UPDATE_H
#include "game_player_child_move.h"
#include "game_cloth_finish.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct {gh_child_move move;gh_cloth_forces forces;gh_cloth_angles angles;
 gh_cloth_distances distances;gh_cloth_finish finish;gh_player_shared_vector *shared_pivot;
 uint32_t stage,frontier;uint8_t original_returned;} gh_child_update;
/* ONE original100530 invocation, all supported104010 blocks in originalorder.
 * Resume SAMEstate on strictfrontier; start a NEWzeroedstate for a newframe.
 * Only complete105A94 suffix permits original parent650 clear and return.
 * Requires caller alreadyexecuted realparentframe/matrix update. */
gh_title_player_status gh_game_player_child_update_00100530(gh_child_update *,const gh_child_move_bindings *,gh_player_shared_vector *canonical3bd880);
#ifdef __cplusplus
}
#endif
#endif
