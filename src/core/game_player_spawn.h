#ifndef GH_GAME_PLAYER_SPAWN_H
#define GH_GAME_PLAYER_SPAWN_H
#include "game_camera_follow.h"
#include "game_actor_model.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef enum {GH_PLAYER_SPAWN_OK,GH_PLAYER_SPAWN_ARGUMENT,GH_PLAYER_SPAWN_NUMERIC,GH_PLAYER_SPAWN_UNSUPPORTED,GH_PLAYER_SPAWN_CALLEE,GH_PLAYER_SPAWN_BUSY} gh_player_spawn_status;
typedef struct {gh_game_title_player *player;gh_actor_model *model;gh_game_scene_tables *actors;
 gh_boot_work *work;gh_game_init_state *init;gh_game_pl_camera *camera;gh_camera_follow *follow;
 const void *elf;size_t elf_bytes;gh_camera_follow_ray ray;void *ray_context;} gh_player_spawn_bindings;
typedef struct {gh_player_spawn_bindings b;float position[4],ray_to[4],yaw;
 uint32_t stage,frontier;int detail;uint8_t *player_bytes;uint16_t room;uint8_t begun,complete;} gh_game_player_spawn;
/* Actual31C5C0, called ONLY after the original player's virtualInit finished.
 * Does not create a player/model or substitute successful Init. Callerzeroes
 * companion for the next original spawn operation after prior lifetime ends.
 * One sameplayer/actors/model/follow binding, retries preserve sourceprefix;
 * real149EB8 and139DB8 mustfinish beforecomplete. No rendering acknowledgment. */
gh_player_spawn_status gh_game_player_spawn_0031c5c0(gh_game_player_spawn *,const gh_player_spawn_bindings *);
#ifdef __cplusplus
}
#endif
#endif
