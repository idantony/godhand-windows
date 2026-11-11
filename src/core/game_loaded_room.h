#ifndef GH_GAME_LOADED_ROOM_H
#define GH_GAME_LOADED_ROOM_H
#include "game_scene_aux.h"
#include "game_door.h"
#include "game_checkpoint.h"
#include "bgm_requests.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct {
    uint16_t room,clear_count;
    uint32_t preserved_b90,output_mode,world_active,global_toggle;
    uint8_t stage,difficulty;
} gh_scene_loaded_room;

typedef struct {
    const gh_game_door_bindings *door_bindings;
    gh_game_door *door;
    gh_game_checkpoint *checkpoint;
    const gh_checkpoint_bindings *checkpoint_bindings;
    gh_bgm_requests *bgm;
    gh_sound_control *sound;
} gh_scene_loaded_room_bindings;

/* Run after 2BF540 or resume when loaded_room.stage is nonzero. Uses the
 * same restored player, checkpoint, sound, and Door owners. Door's ordinary
 * 1FFE10 remains responsible for publishing Main state5. */
gh_scene_aux_status gh_scene_cursor_loaded_room_002bf568(gh_game_scene_aux *,
    gh_scene_loaded_room *,const gh_scene_loaded_room_bindings *);
#ifdef __cplusplus
}
#endif
#endif
