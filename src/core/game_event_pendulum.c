#include "game_event_pendulum.h"
#include "game_actor_external_parent.h"
#include <string.h>

static uint32_t word(const uint8_t *p) {
    uint32_t value;
    memcpy(&value, p, sizeof(value));
    return value;
}

/* Main 2DF950 writes the six profile-2A0 records at 61AF10. */
static const gh_cloth_sphere spheres[] = {
    {0,4,4,1,0.075f,0,{0,0.06f,0.02f,1},{0,0.06f,0.02f,1}},
    {0,4,4,1,0.075f,0,{0,0.02f,0.02f,1},{0,0.02f,0.02f,1}},
    {0,3,3,1,0.055f,0,{0,0.01f,0,1},{0,0.01f,0,1}},
    {0,2,2,1,0.09f,0,{0,0.195f,-0.03f,1},{0,0.195f,-0.03f,1}},
    {0,5,5,1,0.09f,0,{-0.03f,-0.015f,0,1},{-0.03f,-0.015f,0,1}},
    {0,11,11,1,0.09f,0,{0.03f,-0.015f,0,1},{0.03f,-0.015f,0,1}},
};

gh_game_event_status gh_game_event_pendulum_002df860(gh_event_pendulum_call *call,
    gh_event_actor_factory *actor, const gh_event_actor_extra *extra, const gh_cloth_step_bindings *b,
    gh_player_shared_vector *pivot, uint32_t *frontier) {
    if (!frontier) return GH_EVENT_ARGUMENT;
    *frontier = 0x2df860;
    if (!call || !actor || !extra || !b || !pivot || !pivot->initialized ||
        actor->kind != 0x60b || !actor->model.ready || actor->model.root.size < GH_EVENT_ACTOR_BYTES ||
        actor->model.root.bytes != actor->object.view.bytes || !actor->model.root.bytes)
        return GH_EVENT_ARGUMENT;
    uint8_t *root = actor->model.root.bytes;
    const gh_spring_profile_2a0 *profile = &extra->pendulum_profile;
    const gh_actor_model *parent = extra->pendulum_parent;
    const gh_actor_external_parent_set *links = actor->model.external_parts;
    if (!root[0x1390] || word(root+0x1394) != 0x2a0 || !profile->loaded ||
        profile->original_scratch != 0x61af10 || !links || links->child != &actor->model ||
        !parent || links->parent != parent || !parent->ready ||
        word(root+0x2f20) != 10 || word(root+0x2f58) != 6)
        return GH_EVENT_UNSUPPORTED;
    gh_cloth_operands operands = {
        .model = &actor->model, .parent = parent, .config = root+0x2f20,
        .records = root+0x13a0, .indices = profile->indices,
        .coefficients = {profile->coefficients[0], profile->coefficients[1], profile->coefficients[2]},
        .spheres = spheres, .sphere_count = sizeof(spheres)/sizeof(*spheres),
        .count = 10, .tables = (1u<<0)|(1u<<5)|(1u<<6)|(1u<<7)|(1u<<8)|(1u<<9)
    };
    gh_cloth_status status = gh_cloth_step_operands_00104010(&call->prefix, b, &operands);
    if (status != GH_CLOTH_UNSUPPORTED || call->prefix.frontier != 0x10430c) {
        if (call->prefix.frontier) *frontier = call->prefix.frontier;
        return GH_EVENT_UNSUPPORTED;
    }
    if ((status = gh_cloth_integrate_forces_0010430c(&call->forces, &call->prefix)))
        *frontier = call->forces.frontier;
    else if ((status = gh_cloth_constrain_angles_0010497c(&call->angles, &call->forces, pivot)))
        *frontier = call->angles.frontier;
    else if ((status = gh_cloth_constrain_distances_00104ec8(&call->distances, &call->angles)))
        *frontier = call->distances.frontier;
    else if ((status = gh_cloth_finish_00105a94(&call->finish, &call->distances)))
        *frontier = call->finish.frontier;
    if (status) {
        if (!*frontier) *frontier = 0x104010;
        return GH_EVENT_UNSUPPORTED;
    }
    *frontier = 0;
    return GH_EVENT_OK;
}
