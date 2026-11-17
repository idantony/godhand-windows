#include "game_event_pendulum.h"
#include "game_actor_external_parent.h"
#include "game_player_springs_frame.h"
#include <fenv.h>
#include <math.h>
#include <string.h>

static uint32_t word(const uint8_t *p) {
    uint32_t value;
    memcpy(&value, p, sizeof(value));
    return value;
}

static void put(uint8_t *p, uint32_t value)
{
    memcpy(p, &value, sizeof(value));
}

static gh_game_event_status simulate(gh_event_pendulum_call *call, const gh_cloth_operands *operands,
    const gh_cloth_step_bindings *bindings, gh_player_shared_vector *pivot, uint32_t *frontier)
{
    return gh_cloth_frame_00104010(call,operands,bindings,pivot,frontier)?GH_EVENT_UNSUPPORTED:GH_EVENT_OK;
}

/* Original 2DF950 profiles 2A0 and 252 share the same six records. */
static const gh_cloth_sphere paired_spheres[] = {
    {0,4,4,1,0.075f,0,{0,0.06f,0.02f,1},{0,0.06f,0.02f,1}},
    {0,4,4,1,0.075f,0,{0,0.02f,0.02f,1},{0,0.02f,0.02f,1}},
    {0,3,3,1,0.055f,0,{0,0.01f,0,1},{0,0.01f,0,1}},
    {0,2,2,1,0.09f,0,{0,0.195f,-0.03f,1},{0,0.195f,-0.03f,1}},
    {0,5,5,1,0.09f,0,{-0.03f,-0.015f,0,1},{-0.03f,-0.015f,0,1}},
    {0,11,11,1,0.09f,0,{0.03f,-0.015f,0,1},{0.03f,-0.015f,0,1}},
};

static const gh_cloth_sphere el26_spheres[] = {
    {0,4,4,1,.05f,0,{0,.12f,.08f,1},{0,.12f,.08f,1}},
    {0,4,4,1,.06f,0,{0,.08f,.08f,1},{0,.08f,.08f,1}},
    {0,4,4,1,.055f,0,{0,.03f,.08f,1},{0,.03f,.08f,1}},
    {0,4,4,1,.08f,0,{0,.06f,.04f,1},{0,.06f,.04f,1}}
};

gh_game_event_status gh_game_event_pendulum_002df860(gh_event_pendulum_call *call,
    gh_event_actor_factory *actor, const gh_event_actor_extra *extra, const gh_cloth_step_bindings *b,
    gh_player_shared_vector *pivot, uint32_t *frontier) {
    if (!frontier) return GH_EVENT_ARGUMENT;
    *frontier = 0x2df860;
    if (!call || !actor || !extra || !b || !pivot || !pivot->initialized ||
        !actor->model.ready || actor->model.root.size < GH_EVENT_ACTOR_BYTES ||
        actor->model.root.bytes != actor->object.view.bytes || !actor->model.root.bytes)
        return GH_EVENT_ARGUMENT;
    uint8_t *root = actor->model.root.bytes;
    const gh_actor_external_parent_set *links = actor->model.external_parts;
    const gh_event_spring_profile *profile;
    const gh_actor_model *parent;
    const gh_cloth_sphere *spheres = paired_spheres;
    size_t sphere_count = sizeof(paired_spheres)/sizeof(*paired_spheres);
    uint32_t kind, scratch;
    switch (actor->kind) {
    case 0x60b:
        profile = &extra->pendulum_profile;
        parent = extra->pendulum_parent;
        kind = 0x2a0;
        scratch = 0x61af10;
        break;
    case 0x610:
        profile = &extra->ev04_pendulums[0];
        parent = &actor->model;
        kind = 0x250;
        scratch = 0x61a9a0;
        spheres = gh_cloth_250_spheres;
        sphere_count = sizeof(gh_cloth_250_spheres)/sizeof(*gh_cloth_250_spheres);
        break;
    case 0x616:
        profile = &extra->ev04_pendulums[1];
        parent = links ? links->parent : NULL;
        kind = 0x252;
        scratch = 0x61b150;
        break;
    case 0x626:
        profile = &extra->ev04_pendulums[2];
        parent = links ? links->parent : NULL;
        kind = 0x256;
        scratch = 0x61acd0;
        spheres = el26_spheres;
        sphere_count = sizeof(el26_spheres)/sizeof(*el26_spheres);
        break;
    default:
        return GH_EVENT_UNSUPPORTED;
    }
    if (!root[0x1390] || word(root+0x1394) != kind || !profile->loaded || profile->original_scratch != scratch ||
        !parent || !parent->ready ||
        (parent != &actor->model && (!links || links->child != &actor->model || links->parent != parent)) ||
        word(root+0x2f20) != profile->count || word(root+0x2f58) != sphere_count)
        return GH_EVENT_UNSUPPORTED;
    gh_cloth_operands operands = {
        .model = &actor->model, .parent = parent, .config = root+0x2f20,
        .records = root+0x13a0, .indices = profile->indices,
        .coefficients = {profile->coefficients[0], profile->coefficients[1], profile->coefficients[2]},
        .spheres = spheres, .sphere_count = sphere_count,
        .count = profile->count, .tables = (1u<<0)|(1u<<5)|(1u<<6)|(1u<<7)|(1u<<8)|(1u<<9)
    };
    return simulate(call, &operands, b, pivot, frontier);
}

/* Both original class static initializers 27A7F0 / 27B2E8 install these
 * identical 11 shape records into their respective 585240 / 585450 owners. */
static const gh_cloth_sphere accessory_spheres[] = {
    {0,17,17,1.0f,.15f,0,{0,-.2f,0,1},{0,-.2f,0,1}},
    {0,18,18,1.0f,.11f,0,{0,0,0,1},{0,0,0,1}},
    {0,18,19,.66f,.11f,0,{0,0,0,1},{0,0,0,1}},
    {0,18,19,.33f,.11f,0,{0,0,0,1},{0,0,0,1}},
    {0,19,19,1.0f,.11f,0,{0,0,0,1},{0,0,0,1}},
    {0,19,20,.33f,.11f,0,{0,0,0,1},{0,0,0,1}},
    {0,22,22,1.0f,.11f,0,{0,0,0,1},{0,0,0,1}},
    {0,22,23,.66f,.11f,0,{0,0,0,1},{0,0,0,1}},
    {0,22,23,.33f,.11f,0,{0,0,0,1},{0,0,0,1}},
    {0,23,23,1.0f,.11f,0,{0,0,0,1},{0,0,0,1}},
    {0,23,24,.33f,.11f,0,{0,0,0,1},{0,0,0,1}}
};

/* 27A414/27AEF4 translate the child's existing world matrices to the
 * current parent anchor before 107C68 resets the constrained records. */
static gh_game_event_status reset_accessory(gh_actor_model *model,
    const gh_actor_part *child, const gh_actor_part *parent, const gh_event_spring_profile *profile)
{
    uint8_t *root = model->root.bytes;
    const unsigned count = model->part_count;
    if (!count || count > 44 || root[0x2b4] != count ||
        word(root + 0x2134) || word(root + 0x2184) ||
        !child->alias_f0 || !parent->alias_f0) return GH_EVENT_UNSUPPORTED;
    const gh_actor_part *parts[44];
    for (unsigned i = 0; i < count; ++i) {
        if (gh_actor_model_part_at(model, i, &parts[i]) || !parts[i]->bytes)
            return GH_EVENT_BOUNDS;
    }

    fenv_t env;
    if (feholdexcept(&env)) return GH_EVENT_CALLEE;
    if (fesetround(FE_TONEAREST)) {
        (void)fesetenv(&env);
        return GH_EVENT_CALLEE;
    }
    float anchor[3], first[3], delta[3], original[44][3], moved[44][3];
    memcpy(anchor, parent->alias_f0, sizeof(anchor));
    memcpy(first, child->alias_f0, sizeof(first));
    for (unsigned j = 0; j < 3; ++j) {
        const volatile float difference = anchor[j] - first[j];
        delta[j] = difference;
    }
    int valid = 1;
    for (unsigned i = 0; i < count; ++i) {
        memcpy(original[i], parts[i]->bytes + 0xb0, sizeof(original[i]));
        for (unsigned j = 0; j < 3; ++j) {
            const volatile float value = original[i][j] + delta[j];
            moved[i][j] = value;
            if (!isfinite(value)) valid = 0;
        }
    }
    if (fesetenv(&env)) return GH_EVENT_CALLEE;
    if (!valid) return GH_EVENT_UNSUPPORTED;

    for (unsigned i = 0; i < count; ++i)
        memcpy(parts[i]->bytes + 0xb0, moved[i], sizeof(moved[i]));
    gh_player_springs_status status = gh_spring_records_reset_00107c68(model, root + 0x5b0,
        profile->indices[0], profile->count, GH_STATIC_BINARY32_REFERENCE);
    if (!status) return GH_EVENT_OK;
    for (unsigned i = 0; i < count; ++i)
        memcpy(parts[i]->bytes + 0xb0, original[i], sizeof(original[i]));
    return status == GH_SPRINGS_BOUNDS ? GH_EVENT_BOUNDS : GH_EVENT_UNSUPPORTED;
}


gh_game_event_status gh_game_event_accessory_move_0027a320(gh_event_pendulum_call *call,
    gh_event_actor_factory *actor, const gh_event_spring_profile *profile, const gh_boot_work *work,
    const gh_cloth_step_bindings *bindings, gh_player_shared_vector *pivot, uint32_t *frontier)
{
    if (!frontier || !actor || (actor->kind != 0x253 && actor->kind != 0x628)) return GH_EVENT_ARGUMENT;
    const int em53 = actor->kind == 0x253;
    *frontier = em53 ? 0x27a320 : 0x27ae10;
    if (!call || !bindings || !pivot || !pivot->initialized || !work || !profile ||
        !profile->loaded || profile->original_scratch != (em53 ? 0x585240u : 0x585450u) ||
        !actor->model.ready || actor->model.root.size != 0x21c0 ||
        actor->model.root.bytes != actor->object.view.bytes) return GH_EVENT_ARGUMENT;
    if (word((const uint8_t *)work + 0x5d8) & 0x40000000u) {
        *frontier = 0;
        return GH_EVENT_OK;
    }
    uint8_t *root = actor->model.root.bytes;
    const gh_actor_model *parent = actor->model.external_parent;
    const unsigned part_index = word(root + 0x21b4);
    const gh_actor_part *parent_part, *child_part;
    const uint8_t *external;
    if (!parent || !parent->root.alias_f0 || actor->model.external_owner != actor ||
        gh_actor_model_part_at(parent, part_index, &parent_part) ||
        gh_actor_model_external_parent_at(&actor->model, 0, &external) || external != parent_part->bytes ||
        gh_actor_model_part_at(&actor->model, 0, &child_part) ||
        word(root + 0x2130) != profile->count || word(root + 0x2168) != 11) return GH_EVENT_ARGUMENT;
    if (!call->prefix.stage) {
        if (root[0x2f4] == 4) {
            *frontier = 0x14ea50;
            return GH_EVENT_UNSUPPORTED;
        }
        if (word(root + 0x2f0) & 2) {
            *frontier = 0x107c68;
            const gh_game_event_status status = reset_accessory(&actor->model, child_part, parent_part, profile);
            if (status) return status;
        }
        memcpy(actor->model.root.alias_f0, parent->root.alias_f0, 12);
        memcpy(child_part->alias_f0, parent_part->alias_f0, 12);
        memcpy(root + 0x100, parent_part->bytes + 0x100, 12);
        put(root + 0x24c, word(parent->root.bytes + 0x24c));
        put(root + 0x250, (word(root + 0x250) & ~0x10010u) | (word(parent->root.bytes + 0x250) & 0x10010u));
        *frontier = 0x149eb8;
        gh_actor_model_status status = gh_actor_model_update_hierarchy_00149eb8(&actor->model,
            GH_STATIC_BINARY32_REFERENCE);
        if (status) return status == GH_ACTOR_MODEL_UNSUPPORTED ? GH_EVENT_UNSUPPORTED : GH_EVENT_CALLEE;
    }
    gh_cloth_operands operands = {
        .model = &actor->model, .parent = parent, .config = root + 0x2130,
        .records = root + 0x5b0, .count = profile->count, .indices = profile->indices,
        .coefficients = {profile->coefficients[0], profile->coefficients[1], profile->coefficients[2]},
        .spheres = accessory_spheres, .sphere_count = sizeof(accessory_spheres)/sizeof(*accessory_spheres),
        .tables = (1u<<0)|(1u<<1)|(1u<<5)|(1u<<6)|(1u<<7)|(1u<<8)|(1u<<9)
    };
    return simulate(call, &operands, bindings, pivot, frontier);
}
