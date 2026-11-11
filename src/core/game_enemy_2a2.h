#ifndef GH_GAME_ENEMY_2A2_H
#define GH_GAME_ENEMY_2A2_H
#include "game_enemy_factory.h"
#include "game_enemy_model.h"
#ifdef __cplusplus
extern "C" {
#endif

#define GH_EM2A2_BYTES 0x1610u
/* 284A28 is the shared constructor for factory kinds 2A2, 2A3 and 2A4. */
gh_title_player_status gh_enemy2a2_construct_00284a28(void *, size_t, gh_title_player_view *);
/* Borrow the already initialized native owner of original heap 754220. */
gh_title_player_status gh_enemy2a2_allocate_00313820(gh_game_title_player *, gh_heap *,
    const gh_boot_work *, int *original_nonnull);
/* 288DA8 model setup for object kind 2A2 with dependency 2A2, 2A3 or 2A4. */
gh_actor_model_status gh_enemy2a2_model_00288da8(gh_enemy_model *, gh_title_player_view *,
    const gh_object_view *, gh_boot_work *, gh_room_pools *, gh_heap *, const void *fog,
    gh_static_transform_policy, int *original_return);
#ifdef __cplusplus
}
#endif
#endif
