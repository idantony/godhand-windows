#ifndef GH_GAME_PLAYER_NODE_CACHE_H
#define GH_GAME_PLAYER_NODE_CACHE_H
#include "game_player_factory.h"
#include "game_actor_model_nodes.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef enum {GH_PLAYER_NODES_OK,GH_PLAYER_NODES_ARGUMENT,GH_PLAYER_NODES_BOUNDS,GH_PLAYER_NODES_UNSUPPORTED} gh_player_nodes_status;
typedef struct {uint8_t names[15][8];uint8_t initialized;} gh_player_node_names;
typedef struct {gh_actor_model *owner;const uint8_t *bytes;size_t mesh_index;} gh_player_mesh_reference;
typedef struct {gh_game_title_player *player;gh_actor_model *model;const gh_player_node_names *names;
 gh_player_mesh_reference nodes[10];uint32_t frontier;uint8_t costume,bound,ready;} gh_game_player_node_cache;
/* ExactLB/signextension+DSLLV/OR key: stopatNUL or8bytes, neverstrcmp onmesh.
 * Non-ASCII signedchars preserveoriginalquirk; boundedreadfornativeinput. */
gh_player_nodes_status gh_actor_mesh_name_key_0014b730(const uint8_t *,size_t,uint64_t *);
gh_player_nodes_status gh_actor_model_find_mesh_0014b730(gh_actor_model *,const uint8_t *,size_t,gh_player_mesh_reference *,int *original_nonnull);
/* Ownedcopy of caller's actual15 eight-byte staticnames41B828..41B898.
 * No gamebytes embeddedinsource, no ELF lifetime neededafterthiscopy. */
gh_player_nodes_status gh_player_node_names_from_elf(gh_player_node_names *,const void *,size_t);
/* Actual107F58 namecaches andcostume380/410 gates afterallhitareas.
 * BorrowsSAMEreadyActorModel/native480byte nodes, usesnarrowowner mutator.
 * Raw1550..1574 pointertokens0 are replacedbynodes[]. Readyisonlythisleaf. */
gh_player_nodes_status gh_game_player_node_cache_initialize_00107f58(gh_game_player_node_cache *,gh_game_title_player *,gh_actor_model *,const gh_player_node_names *,gh_core_save_view);
#ifdef __cplusplus
}
#endif
#endif
