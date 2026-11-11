#ifndef GH_GAME_ENEMY_2A2_FINAL_H
#define GH_GAME_ENEMY_2A2_FINAL_H
#include "game_enemy_2a2_pose.h"
#include "game_enemy_timers.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint8_t names[8][8];
    uint8_t initialized;
} gh_enemy2a2_node_names;

typedef struct {
    gh_enemy2a2_pose *pose;
    const gh_enemy2a2_node_names *names;
    gh_player_mesh_reference nodes[8];
    uint32_t frontier, timer_word, loot_words[2];
    uint8_t stage, cursor, complete;
} gh_enemy2a2_final;

gh_player_nodes_status gh_enemy2a2_node_names_from_elf(gh_enemy2a2_node_names *,const void *,size_t);
/* Original 284AF8 tail. Its true return is separate from native status.
 * Keeps references in the same model. Abort via pose release, then Init release. */
gh_actor_model_status gh_enemy2a2_finalize_0028540c(gh_enemy2a2_final *,gh_enemy2a2_pose *,
    const gh_enemy2a2_node_names *,gh_em_numeric_policy,int *original_return);

typedef struct {
    gh_enemy2a2_init prefix;
    gh_enemy2a2_pose pose;
    gh_enemy2a2_final final;
    uint32_t frontier;
} gh_enemy2a2_virtual_init;

/* One stable virtual-call owner, kept alive with the actor after return 1. */
gh_actor_model_status gh_enemy2a2_initialize_00284af8(gh_enemy2a2_virtual_init *,
    gh_title_player_view *,const gh_enemy_request *,uint32_t argument,const gh_object_view *,
    const gh_enemy2a2_init_bindings *,const gh_enemy2a2_pose_bindings *,
    const gh_enemy2a2_node_names *,gh_em_numeric_policy,int *original_return);
/* Scoped failure/shutdown release. Caller releases the actor allocation last. */
gh_actor_model_status gh_enemy2a2_release_native(gh_enemy2a2_virtual_init *);
#ifdef __cplusplus
}
#endif
#endif
