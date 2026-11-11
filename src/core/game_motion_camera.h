#ifndef GH_GAME_MOTION_CAMERA_H
#define GH_GAME_MOTION_CAMERA_H

#include "game_camera_projection.h"
#include "game_event_camera_track.h"
#include "game_event_environment.h"
#include "game_scene_setup.h"
#include "../assets/core_bundle.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    gh_game_camera identity;
    gh_game_pl_camera camera;
    gh_motion_view motion;
    uint8_t move_stage;
} gh_game_motion_camera;

gh_camera_status gh_game_motion_camera_construct_00138be8(gh_game_motion_camera *, gh_camera_numeric_policy);
/* Original REL+4BD8 with the EV01 zero-copy argument to CamManager. */
gh_game_event_status gh_event_camera_init_play_00004bd8(gh_event_components *,
    gh_game_init_state *, gh_game_motion_camera *, uint32_t *frontier);
/* Original REL+4D2C SetMotion call and its subsequent frame write. */
gh_camera_status gh_game_motion_camera_set_001390b8(gh_game_motion_camera *, const gh_event_camera_track *);
/* 138600 with MotionCamera vtable slot 139180, then shake and projection.
 * The EVT caller supplies the original zero blend/parent/offset setup. */
gh_camera_status gh_game_motion_camera_move_00138600(gh_game_motion_camera *,
    const gh_game_init_state *, const gh_boot_work *, const gh_camera_projection_display *, uint32_t *frontier);

typedef struct {
    gh_bundle room;
    gh_scene_scenario_view scenario;
    gh_game_init_state *init;
    gh_game_motion_camera *motion;
    gh_boot_work *work;
    gh_camera_projection_display display;
} gh_room_camera_bindings;

typedef struct {
    gh_room_camera_bindings bindings;
    char name[4];
    uint8_t stage,started;
} gh_room_camera_start;

/* Original 2C3ED8 names a motion in the loaded room's CMP directory. */
gh_camera_status gh_game_room_camera_start_002c3ed8(gh_room_camera_start *,
    const gh_room_camera_bindings *,const char *name,int *started,uint32_t *frontier);
/* The caller performs the original one-tick fiber sleeps while ended is false. */
gh_camera_status gh_game_room_camera_ended_002c4040(gh_scene_scenario_view,
    const gh_game_motion_camera *,int *ended);

typedef struct {
    gh_event_components *components;
    const gh_event_data_prefix *data;
    const gh_game_event_config *config;
    gh_game_init_state *init;
    gh_game_motion_camera *motion;
    gh_game_pl_camera *player_camera;
    const gh_boot_work *work;
    gh_camera_projection_display display;
} gh_event_camera_quit_bindings;

typedef struct {
    gh_event_camera_quit_bindings bindings;
    uint8_t stage, frontier_relative;
} gh_event_camera_quit;

/* Main 296F3C -> REL4D50. Final actual camera motion sample, canonical
 * switch, then shake reset on the camera that is active after the switch. */
gh_game_event_status gh_event_camera_quit_play_00004d50(gh_event_camera_quit *,
    const gh_event_camera_quit_bindings *, uint32_t *frontier);

#ifdef __cplusplus
}
#endif
#endif
