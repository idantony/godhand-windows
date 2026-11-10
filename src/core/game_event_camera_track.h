#ifndef GH_GAME_EVENT_CAMERA_TRACK_H
#define GH_GAME_EVENT_CAMERA_TRACK_H

#include "../assets/motion.h"
#include "game_event_data_prefix.h"
#include "game_event_link.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    gh_motion_view track;
    float frame;
    uint8_t apply_motion;
} gh_event_camera_track;

typedef struct {
    float eye[3], target[3], roll, fov;
} gh_motion_camera_sample;

/* REL+4C08 selects the borrowed resource track from the published EventEnv
 * frame. The motion camera owns the later SetMotion and projection calls. */
gh_game_event_status gh_event_camera_select_00004c08(gh_event_link *,
    const gh_event_data_prefix *, gh_event_camera_track *, uint32_t *frontier);

/* Main 139180 scalar sampling prefix, in the original eight dispatches. */
gh_game_event_status gh_motion_camera_sample_00139180(const gh_motion_view *,
    float frame, gh_motion_camera_sample *, uint32_t *frontier);

#ifdef __cplusplus
}
#endif
#endif
