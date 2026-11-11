#ifndef GH_GAME_ENEMY_NODE_CACHE_H
#define GH_GAME_ENEMY_NODE_CACHE_H
#include "game_enemy_solids.h"
#include "game_player_node_cache.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct {uint8_t names[11][8];uint8_t initialized;} gh_enemy_node_names;
typedef struct {gh_enemy_solids *solids;const gh_enemy_node_names *names;gh_player_mesh_reference nodes[11];uint32_t stage,cursor,frontier;} gh_enemy_node_cache;
/* Immutableoriginalrodata42C9E0...; initializeONEownedcopy, thenborrow. */
gh_player_nodes_status gh_enemy_node_names_from_elf(gh_enemy_node_names *,const void *,size_t);
gh_player_nodes_status gh_enemy_node_cache_initialize_002055b8(gh_enemy_node_cache *,gh_enemy_solids *,const gh_enemy_node_names *);
#ifdef __cplusplus
}
#endif
#endif
