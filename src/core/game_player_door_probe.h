#ifndef GH_GAME_PLAYER_DOOR_PROBE_H
#define GH_GAME_PLAYER_DOOR_PROBE_H
#include "game_scene_at_jump.h"
#include "game_init.h"
#include "game_camera.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef enum {GH_PLAYER_DOOR_OK,GH_PLAYER_DOOR_ARGUMENT,GH_PLAYER_DOOR_NUMERIC,GH_PLAYER_DOOR_UNSUPPORTED,GH_PLAYER_DOOR_CALLEE} gh_player_door_status;
typedef struct {const gh_title_player_view *player;int32_t priority,life,unknown20,kind;uint32_t callback_entry;const void *argument;} gh_player_door_action;
typedef gh_player_door_status (*gh_player_door_publish)(void *,const gh_player_door_action *,uint32_t *frontier);
typedef struct {gh_title_player_view *player;const gh_title_player_view *current_player;gh_game_init_state *init;
 gh_scene_at_manager *scene_at;const gh_bundle *room;gh_boot_work *work;gh_game_scene_tables *actors;
 gh_camera_numeric_policy numeric_policy;gh_player_door_publish publish;void *context;} gh_player_door_bindings;
typedef struct {const void *identity;const gh_player_door_bindings *bindings;uint32_t stage,frontier;uint8_t active,complete;
 float position[3],yaw;gh_player_door_action pending;} gh_player_door_probe;
/* Original12BFB8 callees2DC920 THEN2DCAD8, with prefix retry cursor. Doesnot
 * set caller15F4|100; caller only commits that after BOTH really completed. */
gh_player_door_status gh_player_door_probe_begin(gh_player_door_probe *);
gh_player_door_status gh_player_door_probe_002dc920_002dcad8(gh_player_door_probe *,const gh_player_door_bindings *);
#ifdef __cplusplus
}
#endif
#endif
