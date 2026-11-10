#include "game_motion_camera.h"

#include "game_actor_matrix.h"
#include "game_camera_shake.h"
#include "game_scene_at_geometry.h"

#include <fenv.h>
#include <math.h>
#include <string.h>

static float read_float(const uint8_t *address) {
    float value;
    memcpy(&value, address, sizeof(value));
    return value;
}

static uint32_t read_word(const uint8_t *address) {
    uint32_t value;
    memcpy(&value, address, sizeof(value));
    return value;
}

static void write_float(uint8_t *address, float value) {
    memcpy(address, &value, sizeof(value));
}

static void write_word(uint8_t *address, uint32_t value) {
    memcpy(address, &value, sizeof(value));
}

static float multiply(float a, float b) {
    const volatile float result = a * b;
    return result;
}

static float sum(float a, float b) {
    const volatile float result = a + b;
    return result;
}

static gh_camera_status stop(uint32_t *frontier, uint32_t address, gh_camera_status status) {
    *frontier = address;
    return status;
}

gh_camera_status gh_game_motion_camera_construct_00138be8(gh_game_motion_camera *s, gh_camera_numeric_policy policy) {
    if (!s) return GH_CAMERA_ARGUMENT;
    return gh_game_camera_construct_motion_00138be8(&s->camera, &s->identity, policy);
}

gh_game_event_status gh_event_camera_init_play_00004bd8(gh_event_components *components,
    gh_game_init_state *init, gh_game_motion_camera *s, uint32_t *frontier) {
    if (!frontier) return GH_EVENT_ARGUMENT;
    *frontier = 0x4bd8;
    if (!components || !init || !s || !s->camera.constructed || s->camera.identity != &s->identity) {
        return GH_EVENT_ARGUMENT;
    }
    gh_event_component_view camera;
    if (gh_event_component_at(components, 1, &camera) || camera.original_offset != 0xd1c8 || camera.size != 0x18) {
        return GH_EVENT_ARGUMENT;
    }
    if (init->camera.mode == 4 && init->active_camera != &s->identity) return GH_EVENT_ARGUMENT;
    /* Component +0 borrows the typed owner supplied to the move callback. */
    write_word(camera.bytes, 0);
    if (init->camera.mode != 4 && init->camera.mode != 16) {
        init->camera.previous_mode = init->camera.mode;
        init->camera.previous = init->active_camera;
        init->active_camera = &s->identity;
        init->camera.mode = 4;
    }
    *frontier = 0;
    return GH_EVENT_OK;
}

gh_camera_status gh_game_motion_camera_set_001390b8(gh_game_motion_camera *s, const gh_event_camera_track *selected) {
    if (!s || !s->camera.constructed || !selected || !selected->track.bytes || !selected->track.frames ||
        !isfinite(selected->frame) || selected->frame < 0 || selected->frame >= selected->track.frames) {
        return GH_CAMERA_ARGUMENT;
    }
    if (s->move_stage) return GH_CAMERA_BUSY;
    if (!selected->apply_motion) return GH_CAMERA_OK;
    uint8_t *p = s->camera.fields;
    p[0x27a] = p[0x27b] = 0;
    write_float(p + 0x2b4, 1);
    write_word(p + 0x3a4, 0); /* Typed motion view below. */
    write_float(p + 0x3a8, selected->frame);
    write_word(p + 0x3ac, 0);
    memset(p + 0x3b0, 0, 12);
    memset(p + 0x3c0, 0, 12);
    write_word(p + 0x3e4, 0);
    s->motion = selected->track;
    return GH_CAMERA_OK;
}

static gh_camera_status sample_rotation(const gh_motion_camera_sample *sample,
    float euler[3], float up[3], uint32_t *frontier) {
    float direction[3], rotation[16], local[4];
    for (unsigned i = 0; i < 3; ++i) direction[i] = sum(sample->eye[i], -sample->target[i]);
    gh_camera_status status = gh_game_angle_001f8180(direction[0], direction[2], euler + 1,
        GH_CAMERA_NATIVE_BINARY32_REFERENCE, frontier);
    if (status) return status;

    const float inverse_yaw[3] = {0, -euler[1], 0};
    if (gh_actor_matrix_rotation_00147728(inverse_yaw, 0, rotation, GH_STATIC_BINARY32_REFERENCE, frontier) ||
        gh_scene_at_apply_world_001508d8(rotation, direction, local)) {
        return stop(frontier, *frontier ? *frontier : 0x138358, GH_CAMERA_UNSUPPORTED);
    }
    status = gh_game_angle_001f8180(local[1], local[2], euler, GH_CAMERA_NATIVE_BINARY32_REFERENCE, frontier);
    if (status) return status;
    euler[0] = -euler[0];
    euler[2] = 0;

    const float roll[3] = {0, 0, sample->roll};
    const float y_axis[3] = {0, 1, 0};
    if (gh_actor_matrix_rotation_00147728(roll, 0, rotation, GH_STATIC_BINARY32_REFERENCE, frontier) ||
        gh_scene_at_apply_world_001508d8(rotation, y_axis, local) ||
        gh_actor_matrix_rotation_00147728(euler, 0, rotation, GH_STATIC_BINARY32_REFERENCE, frontier) ||
        gh_scene_at_apply_world_001508d8(rotation, local, local)) {
        return stop(frontier, *frontier ? *frontier : 0x150830, GH_CAMERA_UNSUPPORTED);
    }
    const float length = sqrtf(sum(sum(multiply(local[0], local[0]), multiply(local[1], local[1])),
        multiply(local[2], local[2])));
    if (!isfinite(length) || length <= 0) return stop(frontier, 0x101e28, GH_CAMERA_NUMERIC);
    const volatile float inverse = 1.0f / length;
    for (unsigned i = 0; i < 3; ++i) up[i] = multiply(local[i], inverse);
    return GH_CAMERA_OK;
}

static gh_camera_status motion_step(gh_game_motion_camera *s, const gh_boot_work *work,
    const gh_camera_projection_display *display, uint32_t *frontier) {
    gh_game_pl_camera next = s->camera;
    uint8_t *p = next.fields;
    const uint32_t flags = read_word(p + 0x3e4);
    if (!s->motion.bytes || p[0x27a] || p[0x27b] || read_word(p + 0x3ac) ||
        read_word(p + 0x3b0) || read_word(p + 0x3b4) || read_word(p + 0x3b8) ||
        read_word(p + 0x3c0) || read_word(p + 0x3c4) || read_word(p + 0x3c8) || (flags & ~10u)) {
        return stop(frontier, 0x139180, GH_CAMERA_UNSUPPORTED);
    }

    const float frame = read_float(p + 0x3a8);
    gh_motion_camera_sample sample;
    if (gh_motion_camera_sample_00139180(&s->motion, frame, &sample, frontier)) return GH_CAMERA_UNSUPPORTED;
    float euler[3], up[3];
    gh_camera_status status = sample_rotation(&sample, euler, up, frontier);
    if (status) return status;
    memcpy(p + 0x200, sample.target, sizeof(sample.target));
    memcpy(p + 0x210, sample.eye, sizeof(sample.eye));
    memcpy(p + 0x220, euler, sizeof(euler));
    memcpy(p + 0x240, up, sizeof(up));
    write_float(p + 0x250, sample.fov);
    status = gh_game_camera_project_00138740(&next, display, frontier);
    if (status) return status;

    const float increment = multiply(multiply(read_float(p + 0x2b4), ((const uint8_t *)work)[0x569]), 0.5f);
    float next_frame = sum(frame, increment);
    uint32_t next_flags = flags | 8u;
    if (next_frame >= s->motion.frames) {
        next_flags |= 2u;
        next_frame = sum(next_frame, -increment);
    }
    write_float(p + 0x3a8, next_frame);
    write_word(p + 0x3e4, next_flags);
    s->camera = next;
    return GH_CAMERA_OK;
}

gh_camera_status gh_game_motion_camera_move_00138600(gh_game_motion_camera *s,
    const gh_game_init_state *init, const gh_boot_work *work,
    const gh_camera_projection_display *display, uint32_t *frontier) {
    if (!frontier) return GH_CAMERA_ARGUMENT;
    *frontier = 0x138600;
    if (!s || !init || !work || !display || !s->camera.constructed || init->active_camera != &s->identity ||
        s->camera.identity != &s->identity || read_word(s->camera.fields + 0x35c) != 0x41d8f8) {
        return GH_CAMERA_ARGUMENT;
    }
    if (!s->move_stage) {
        fenv_t environment;
        if (feholdexcept(&environment)) return GH_CAMERA_NUMERIC;
        if (fesetround(FE_TONEAREST)) {
            (void)fesetenv(&environment);
            return GH_CAMERA_NUMERIC;
        }
        gh_camera_status status = motion_step(s, work, display, frontier);
        if (fesetenv(&environment)) return GH_CAMERA_NUMERIC;
        if (status) return status;
        s->move_stage = 1;
    }
    if (s->move_stage == 1) {
        gh_camera_status status = gh_game_camera_shake_step_00138558(&s->camera, frontier);
        if (status) return status;
        s->move_stage = 2;
    }
    gh_camera_status status = gh_game_camera_project_00138740(&s->camera, display, frontier);
    if (!status) s->move_stage = 0;
    return status;
}
