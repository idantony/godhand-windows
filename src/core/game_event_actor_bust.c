#include "game_event_actor_bust.h"

#include "game_actor_inverse.h"
#include "game_camera.h"

#include <fenv.h>
#include <math.h>
#include <string.h>

static uint32_t word(const uint8_t *bytes)
{
    uint32_t value;
    memcpy(&value, bytes, sizeof(value));
    return value;
}

static float number(const uint8_t *bytes)
{
    float value;
    memcpy(&value, bytes, sizeof(value));
    return value;
}

static float add(float a, float b)
{
    volatile float result = a + b;
    return result;
}

static float multiply(float a, float b)
{
    volatile float result = a * b;
    return result;
}

/* 30A2B0's column order. The first input takes its W from object+2FBC.
 * The second input is the original VF0-based local offset (0,0,.03,1). */
static void transform_xyz(const float matrix[16], const float input[4], float output[3])
{
    for (unsigned axis = 0; axis < 3; ++axis) {
        float result = multiply(matrix[axis], input[0]);
        for (unsigned col = 1; col < 4; ++col)
            result = add(result, multiply(matrix[col * 4 + axis], input[col]));
        output[axis] = result;
    }
}

static float clamp_angle(float value, float limit)
{
    if (value > limit) return limit;
    if (value < -limit) return -limit;
    return value;
}

static gh_game_event_status calculate(const uint8_t *actor, const gh_actor_part *origin,
    const gh_actor_part *bust, float frame, float position[3], float next_offset[3],
    float *next_angle, float *next_velocity, uint32_t *frontier)
{
    float matrix[16], inverse[16], offset[4], previous[3], local[3];
    memcpy(matrix, bust->bytes + 0x80, sizeof(matrix));
    memcpy(position, origin->bytes + 0xb0, sizeof(float) * 3);
    memcpy(previous, actor + 0x2fe0, sizeof(previous));
    memcpy(offset, actor + 0x2fb0, sizeof(offset));
    offset[0] = add(offset[0], add(position[0], -previous[0]));
    offset[2] = add(offset[2], add(position[2], -previous[2]));
    if (gh_actor_matrix_inverse_00147568(matrix, inverse, GH_STATIC_BINARY32_REFERENCE, frontier))
        return GH_EVENT_UNSUPPORTED;
    transform_xyz(inverse, offset, local);
    const float radial = sqrtf(add(multiply(local[0], local[0]), multiply(local[2], local[2])));
    float atan;
    if (gh_game_angle_001f8180(local[1], radial, &atan, GH_CAMERA_NATIVE_BINARY32_REFERENCE, frontier))
        return GH_EVENT_UNSUPPORTED;

    const float limit = 0x1.0c1524p-1f; /* 3F060A92 */
    float velocity = number(actor + 0x2fc0);
    float angle = add(add(number(bust->bytes + 0x100), multiply(clamp_angle(-atan, limit), 0.5f)), velocity);
    if (!frame || (word(actor + 0x2f0) & 1u)) angle = velocity = 0;
    if (angle > limit || angle < -limit) {
        angle = clamp_angle(angle, limit);
        velocity = 0;
    }
    velocity = add(multiply(velocity, add(1.0f, -0.2f)), multiply(-angle, 0.5f));
    const float tip[4] = {0, 0, 0.03f, 1};
    transform_xyz(matrix, tip, next_offset);
    *frontier = 0x2b6a40;
    for (unsigned axis = 0; axis < 3; ++axis) {
        if (!isfinite(position[axis]) || !isfinite(next_offset[axis])) return GH_EVENT_UNSUPPORTED;
    }
    if (!isfinite(angle) || !isfinite(velocity)) return GH_EVENT_UNSUPPORTED;
    *next_angle = angle;
    *next_velocity = velocity;
    return GH_EVENT_OK;
}

gh_game_event_status gh_event_actor_bust_002b6a40(gh_event_actor_factory *actor,
    gh_event_components *components, uint32_t *frontier)
{
    if (!frontier) return GH_EVENT_ARGUMENT;
    *frontier = 0x2b6a40;
    if (!actor || !actor->model.ready || actor->model.native_identity != &actor->model ||
        !actor->model.root.bytes || actor->model.root.bytes != actor->object.view.bytes ||
        actor->model.root.size < GH_EVENT_ACTOR_BYTES) return GH_EVENT_ARGUMENT;
    uint8_t *bytes = actor->model.root.bytes;
    if (bytes[0x2b4] != actor->model.part_count) return GH_EVENT_ARGUMENT;
    if (bytes[0x2b4] < 30) {
        *frontier = 0;
        return GH_EVENT_OK;
    }
    const gh_actor_part *origin, *bust;
    if (gh_actor_model_part_at(&actor->model, 0, &origin) ||
        gh_actor_model_part_at(&actor->model, 29, &bust)) return GH_EVENT_BOUNDS;

    gh_event_component_view env;
    *frontier = 0x297b88;
    if (!components || !components->constructed || gh_event_component_at(components, 0, &env) ||
        !env.bytes || env.original_offset != 0xd200 || env.size < 0x18) return GH_EVENT_ARGUMENT;
    const float frame = number(env.bytes + 0x14);
    if (!isfinite(frame)) return GH_EVENT_UNSUPPORTED;

    fenv_t previous_env;
    *frontier = 0x2b6a40;
    if (feholdexcept(&previous_env)) return GH_EVENT_CALLEE;
    if (fesetround(FE_TONEAREST)) {
        (void)fesetenv(&previous_env);
        return GH_EVENT_CALLEE;
    }
    float position[3], offset[3], angle, velocity;
    gh_game_event_status status = calculate(bytes, origin, bust, frame,
        position, offset, &angle, &velocity, frontier);
    if (fesetenv(&previous_env)) return GH_EVENT_CALLEE;
    if (status) return status;
    memcpy(bytes + 0x2fd0, position, sizeof(position));
    memcpy(bytes + 0x2fe0, position, sizeof(position));
    memcpy(bytes + 0x2fb0, offset, sizeof(offset));
    memcpy(bust->bytes + 0x100, &angle, sizeof(angle));
    memcpy(bytes + 0x2fc0, &velocity, sizeof(velocity));
    *frontier = 0;
    return GH_EVENT_OK;
}
