#ifndef GH_GAME_ENEMY_2A2_INIT_H
#define GH_GAME_ENEMY_2A2_INIT_H
#include "game_enemy_2a2.h"
#include "game_enemy_base_init.h"
#include "game_actor_motion.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    gh_boot_work *work;
    gh_room_pools *pools;
    gh_heap *packet_heap;
    const void *fog;
    gh_game_cockpit *cockpit;
    gh_game_mt *random;
    gh_static_transform_policy policy;
    const struct gh_motion_ik_globals *ik;
} gh_enemy2a2_init_bindings;

typedef struct {
    gh_enemy_model model;
    gh_enemy_base_initialize base;
    gh_actor_motion motion;
    gh_enemy2a2_init_bindings bindings;
    gh_enemy_request request;
    gh_object_view resource;
    gh_title_player_view *root;
    size_t motion_offset, sequence_offset;
    uint32_t argument, frontier;
    uint8_t stage;
} gh_enemy2a2_init;

/* Bounded original 284AF8 through appearance 0's direct 157568 call. Stops at first 157C18 sample.
 * original_return remains zero until the virtual initializer has returned. */
gh_actor_model_status gh_enemy2a2_initialize_prefix_00284af8(gh_enemy2a2_init *,
    gh_title_player_view *, const gh_enemy_request *, uint32_t argument,
    const gh_object_view *, const gh_enemy2a2_init_bindings *, int *original_return);
gh_actor_model_status gh_enemy2a2_init_release_prefix(gh_enemy2a2_init *);
#ifdef __cplusplus
}
#endif
#endif
