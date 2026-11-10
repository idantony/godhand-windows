#ifndef GH_GAME_PLAYER_CHILD_TAIL_H
#define GH_GAME_PLAYER_CHILD_TAIL_H
#include "game_player_springs.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Canonical persistent mutable .data3BD880 companion. Initialize ONCE from
 * actual ELF, retain across room transitions. Future original writers must
 * borrow these same bytes; this is not an immutable constant assumption. */
typedef struct {uint8_t bytes[16];uint8_t initialized;} gh_player_shared_vector;
gh_title_player_status gh_player_shared_vector_from_elf(gh_player_shared_vector *,const void *,size_t);
typedef struct {gh_game_player_child *child;gh_player_shared_vector *shared_vector;
 gh_game_player_child **work5f8;gh_game_player_springs springs;uint32_t stage,frontier;
 uint8_t complete;} gh_player_child_tail;
/* Actual100378 internal100420 suffix. Requires successful SAME149738 owner.
 * Writes firstpart E0/D0, calls real149EB8 then100870, and only THEN publishes
 * typed Work5F8 and original254/2F0 flags. Never truncates hostpointer intoWork. */
gh_title_player_status gh_game_player_child_finish_00100420(gh_player_child_tail *,gh_game_player_child *,
 gh_player_shared_vector *,gh_game_player_child **work5f8,gh_core_save_view,const void *elf,size_t,
 gh_static_transform_policy,int *original_return);
#ifdef __cplusplus
}
#endif
#endif
