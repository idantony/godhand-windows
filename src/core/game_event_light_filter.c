#include "game_event_light_filter.h"

#include <fenv.h>
#include <math.h>
#include <string.h>

enum { OBJECT_LIGHT = 6, FILTER = 7 };

static uint32_t read_word(const uint8_t *p)
{
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}

static unsigned read_half(const uint8_t *p)
{
    return p[0] | (unsigned)p[1] << 8;
}

static void put_half(uint8_t *p, unsigned value)
{
    p[0] = (uint8_t)value;
    p[1] = (uint8_t)(value >> 8);
}

static int signed_half(const uint8_t *p)
{
    const unsigned value = read_half(p);
    return value < 0x8000u ? (int)value : (int)value - 0x10000;
}

static float read_float(const uint8_t *p)
{
    const uint32_t bits = read_word(p);
    float value;
    memcpy(&value, &bits, sizeof(value));
    return value;
}

static float binary_add(float a, float b)
{
    const volatile float result = a + b;
    return result;
}

static float binary_mul(float a, float b)
{
    const volatile float result = a * b;
    return result;
}

static int enter_math(fenv_t *env)
{
    if (feholdexcept(env)) return 0;
    if (!fesetround(FE_TONEAREST)) return 1;
    (void)fesetenv(env);
    return 0;
}

/* REL04F08 sums the primary cut lengths and clamps within the addressed cut. */
static gh_game_event_status cut_time(const gh_event_data_prefix *data, unsigned cut,
    float frame, float *out)
{
    float prior = 0;
    for (unsigned i = 0; i <= cut; ++i) {
        gh_event_camera_cut camera;
        gh_game_event_status status = gh_event_camera_cut_at(data, i, &camera);
        if (status) return status;
        const unsigned length = camera.primary ? read_half(camera.primary + 4) : 0;
        if (i == cut) {
            const float last = (float)length - 1.0f;
            if (length) prior = binary_add(prior, frame > last ? last : frame);
        } else {
            prior = binary_add(prior, (float)length);
        }
    }
    *out = prior;
    return GH_EVENT_OK;
}

gh_game_event_status gh_event_light_interpolation_factor_00004f08(const gh_event_data_prefix *data,
    const uint8_t *previous, const uint8_t *next, unsigned cut, float frame, float *out)
{
    if (!data || !previous || !next || !out || !isfinite(frame)) return GH_EVENT_ARGUMENT;
    const unsigned first = read_half(previous + 6), last = read_half(next + 6);
    if (first > 0x7fffu || last > 0x7fffu || previous[3] > cut ||
        next[3] < cut || next[3] >= data->camera_count) return GH_EVENT_UNSUPPORTED;
    float start, end, now;
    gh_game_event_status status = cut_time(data, previous[3], (float)first, &start);
    if (!status) status = cut_time(data, next[3], (float)last, &end);
    if (!status) status = cut_time(data, cut, frame, &now);
    if (status) return status;
    if (!(end > start) || now < start || now > end) return GH_EVENT_UNSUPPORTED;
    const volatile float remaining = binary_add(end, -now) / binary_add(end, -start);
    *out = binary_add(1.0f, -remaining);
    return GH_EVENT_OK;
}

float gh_event_light_lerp_00006c38(float a, float b, float t)
{
    if (a == b) return a;
    return binary_add(binary_mul(a, binary_add(1.0f, -t)), binary_mul(b, t));
}


static int read_component(gh_event_components *components, unsigned index,
                          uint32_t original_offset, size_t size, gh_event_component_view *out)
{
    return components && components->constructed &&
           !gh_event_component_at(components, index, out) &&
           out->bytes && out->original_offset == original_offset && out->size == size;
}

static gh_game_event_status read_track(const gh_event_light_filter *state,
    gh_event_components *components, const gh_event_data_prefix *data, unsigned index,
    gh_event_component_view *component, gh_event_sequence_view *track)
{
    if (!state || !data || !data->resource || !data->resource->loaded ||
        data->components != components ||
        (state->data && (state->data != data || state->components != components ||
         state->allocation != data->resource->allocation.pointer)))
        return GH_EVENT_ARGUMENT;
    if (!read_component(components, index, index == OBJECT_LIGHT ? 0xd860 : 0xd268,
                        index == OBJECT_LIGHT ? 0x34 : 0x5e8, component))
        return GH_EVENT_ARGUMENT;
    gh_game_event_status status = gh_event_sequence_at(data, index, track);
    if (status) return status;
    if (read_word(component->bytes + 0x14) != track->count) return GH_EVENT_FORMAT;
    return GH_EVENT_OK;
}

/* Fixed frame-zero keys use the primary camera. Original 8378
 * maps FFFF to lane FF, and the four timing repair calls keep these times. */
static gh_game_event_status mapped_keys(const gh_event_data_prefix *data,
                                        const gh_event_sequence_view *track, unsigned index)
{
    const unsigned event = data->components->event_no;
    const int object_light = index == OBJECT_LIGHT;
    const int ev05 = event == 5;
    const int ev06 = event == 6;
    const int ev1a = event == 0x1a;
    const int ev1b = event == 0x1b;
    if ((!ev05 && !ev06 && !ev1a && !ev1b && event != 1 && event != 2 && event != 4) ||
        data->camera_count != (ev05 ? 18 : ev06 ? 14 : ev1a ? 11 : ev1b ? 12 : event == 1 ? 10 : event == 2 ? 28 : 24))
        return GH_EVENT_UNSUPPORTED;
    static const uint8_t ev1b_light_cuts[] = {0, 6, 7, 8, 11};
    static const uint8_t ev1b_filter_cuts[] = {1, 2, 4, 5, 7, 8, 9, 10, 11};
    static const uint8_t ev05_filter_cuts[] = {1,7,8,9,12,15,16};
    static const uint8_t ev06_filter_cuts[] = {0,1,2,3,5,7,8,9,10,11,12,13};
    const unsigned count = ev05 ? (object_light ? 1 : sizeof(ev05_filter_cuts)) :
        ev06 ? (object_light ? 1 : sizeof(ev06_filter_cuts)) :
        ev1a ? (object_light ? 1 : 8) :
        ev1b ? (object_light ? sizeof(ev1b_light_cuts) : sizeof(ev1b_filter_cuts)) :
        event == 4 ? (object_light ? 10 : 29) : (object_light ? event : event == 1 ? 10 : 25);
    const size_t stride = object_light ? 0x1c : 0x2e8;
    if (!track->records || track->count != count || track->stride != stride ||
        track->bytes != count * stride) return GH_EVENT_UNSUPPORTED;
    for (unsigned i = 0; i < count; ++i) {
        const uint8_t *key = track->records + i * stride;
        unsigned cut = i;
        if (ev05) cut = object_light ? 0 : ev05_filter_cuts[i];
        else if (ev06) cut = object_light ? 0 : ev06_filter_cuts[i];
        else if (ev1b) cut = object_light ? ev1b_light_cuts[i] : ev1b_filter_cuts[i];
        else if (ev1a && !object_light) {
            static const uint8_t cuts[] = {0, 1, 2, 3, 4, 5, 8, 9};
            cut = cuts[i];
        } else if (event == 4) cut = key[3];
        else if (object_light && event == 1) cut = 1;
        if (!object_light && event == 2 && i >= 16) cut += 3;
        const unsigned frame = read_half(key + 6);
        const uint32_t supported_modes = event == 4 ? (object_light ? 1u : 3u) : 0;
        uint16_t length;
        gh_event_camera_cut camera;
        if (read_word(key) != (0xffffu | cut << 24) || read_half(key + 4) ||
            (event != 4 && frame) || (read_word(key + 8) & ~supported_modes) ||
            gh_event_camera_cut_at(data, cut, &camera) || !camera.primary ||
            gh_event_camera_cut_length_00004e60(data, cut, &length) || frame >= length)
            return GH_EVENT_UNSUPPORTED;
    }
    return GH_EVENT_OK;
}

static void bind(gh_event_light_filter *state, gh_event_components *components,
                 const gh_event_data_prefix *data)
{
    state->data = data;
    state->allocation = data->resource->allocation.pointer;
    state->components = components;
}

gh_game_event_status gh_event_object_light_init_00008648(gh_event_light_filter *state,
    gh_event_components *components, const gh_event_data_prefix *data,
    gh_game_world_light *owner, uint32_t *frontier)
{
    if (!frontier) return GH_EVENT_ARGUMENT;
    *frontier = 0x8648;
    if (!state || !owner || !owner->constructed || !owner->ready ||
        (state->world_light && state->world_light != owner)) return GH_EVENT_ARGUMENT;
    gh_event_component_view component;
    gh_event_sequence_view track;
    *frontier = 0xbe58;
    gh_game_event_status status = read_track(state, components, data, OBJECT_LIGHT, &component, &track);
    if (status) return status;
    if ((status = mapped_keys(data, &track, OBJECT_LIGHT))) return status;

    memcpy(component.bytes, "OBJLIGHT", sizeof("OBJLIGHT"));
    memset(component.bytes + 0x18, 0, 0x1c);
    memcpy(component.bytes + 0x24, owner->scalar + 0x40, 0x10);
    bind(state, components, data);
    state->world_light = owner;
    *frontier = 0;
    return GH_EVENT_OK;
}

gh_game_event_status gh_event_filter_init_00007040(gh_event_light_filter *state,
    gh_event_components *components, const gh_event_data_prefix *data,
    gh_scene_filters *owner, uint32_t *frontier)
{
    if (!frontier) return GH_EVENT_ARGUMENT;
    *frontier = 0x7040;
    if (!state || !owner || !owner->ready ||
        (state->filters && state->filters != owner)) return GH_EVENT_ARGUMENT;
    gh_event_component_view component;
    gh_event_sequence_view track;
    *frontier = 0xc250;
    gh_game_event_status status = read_track(state, components, data, FILTER, &component, &track);
    if (status) return status;
    if ((status = mapped_keys(data, &track, FILTER))) return status;

    memcpy(component.bytes, "FILTER", sizeof("FILTER"));
    memset(component.bytes + 0x18, 0, 0x2e8);
    memcpy(component.bytes + 0x28, owner->work.active140c8, 0x2d8);
    bind(state, components, data);
    state->filters = owner;
    *frontier = 0;
    return GH_EVENT_OK;
}

typedef struct {
    const uint8_t *previous, *next;
    unsigned cut;
    float frame;
} gh_event_light_key_pair;

static gh_game_event_status active_keys(gh_event_light_filter *state, unsigned index,
    gh_event_component_view *component, gh_event_light_key_pair *keys, uint32_t *frontier)
{
    gh_event_sequence_view track;
    *frontier = index == OBJECT_LIGHT ? 0xbcc8 : 0xc0c0;
    gh_game_event_status status = read_track(state, state->components, state->data,
                                             index, component, &track);
    if (status) return status;
    if ((status = mapped_keys(state->data, &track, index))) return status;
    gh_event_component_view camera, env;
    if (!read_component(state->components, 1, 0xd1c8, 0x18, &camera) ||
        !read_component(state->components, 0, 0xd200, 0x44, &env))
        return GH_EVENT_ARGUMENT;
    const unsigned cut = read_half(camera.bytes + 6);
    const float frame = read_float(camera.bytes + 0x0c);
    if (camera.bytes[4] || cut >= state->data->camera_count ||
        cut != read_word(env.bytes + 0x10) || !isfinite(frame) || frame < 0)
        return GH_EVENT_UNSUPPORTED;
    *keys = (gh_event_light_key_pair){.previous = component->bytes + 0x18, .cut = cut, .frame = frame};
    for (unsigned i = 0; i < track.count; ++i) {
        const uint8_t *candidate = track.records + i * track.stride;
        const float key_frame = (float)read_half(candidate + 6);
        if (candidate[3] < cut || (candidate[3] == cut && key_frame <= frame)) keys->previous = candidate;
        if (candidate[3] > cut || (candidate[3] == cut && key_frame >= frame)) {
            keys->next = candidate;
            break;
        }
    }
    return GH_EVENT_OK;
}

static int interpolates(const gh_event_light_key_pair *keys)
{
    return (keys->previous[8] & 1u) && keys->next && keys->previous != keys->next;
}

gh_game_event_status gh_event_object_light_move_000086b0(gh_event_light_filter *state,
                                                          uint32_t *frontier)
{
    if (!frontier) return GH_EVENT_ARGUMENT;
    *frontier = 0x86b0;
    if (!state || !state->world_light || !state->world_light->ready) return GH_EVENT_ARGUMENT;
    gh_event_component_view component;
    gh_event_light_key_pair keys;
    gh_game_event_status status = active_keys(state, OBJECT_LIGHT, &component, &keys, frontier);
    if (status) return status;
    uint8_t *destination = state->world_light->scalar + 0x40;
    if (!interpolates(&keys)) {
        memcpy(destination, keys.previous + 0x0c, 0x0c);
    } else {
        *frontier = 0x87a8;
        fenv_t env;
        if (!enter_math(&env)) return GH_EVENT_CALLEE;
        float t, result[3];
        status = gh_event_light_interpolation_factor_00004f08(state->data,
            keys.previous, keys.next, keys.cut, keys.frame, &t);
        if (!status) {
            for (unsigned i = 0; i < 3; ++i) {
                const unsigned at = 0x0c + i * 4;
                result[i] = gh_event_light_lerp_00006c38(read_float(keys.previous + at),
                    read_float(keys.next + at), t);
                if (!isfinite(result[i])) status = GH_EVENT_UNSUPPORTED;
            }
        }
        if (fesetenv(&env)) return GH_EVENT_CALLEE;
        if (status) return status;
        memcpy(destination, result, sizeof(result));
    }
    *frontier = 0;
    return GH_EVENT_OK;
}

/* REL71A0 applies the chosen key, with the origin's 0x64-byte field override. */
static void apply_filter_key(gh_scene_filters *filters, const uint8_t *component,
                             const uint8_t *key)
{
    uint8_t *active = filters->work.active140c8;
    memcpy(active, key + 0x10, 0x2d8);
    memcpy(active + 0x208, component + 0x230, 0x64);
}

/* REL07610 materializes a preset onto the key payload. Keep asset bytes
 * borrowed and build the exact original payload in a local buffer. */
static const uint8_t *resolve_filter_preset(const gh_scene_filters *filters,
    const gh_event_component_view *component, const uint8_t *key, uint8_t resolved[0x2e8])
{
    if (!key || !(key[8] & 2u)) return key;
    memcpy(resolved, key, 0x2e8);
    const unsigned selector = resolved[9];
    if (selector == 255) {
        memcpy(resolved + 0x10, component->bytes + 0x28, 0x2d8);
    } else {
        const unsigned index = selector < 32 ? selector + 1 : 0;
        const uint8_t *preset = filters->work.presets143a0 + 0x10 +
            GH_FILTER_PRESET_BYTES * index;
        static const unsigned destination[] = {0x24,0x88,0xec,0x150,0x1b4,0x284};
        static const unsigned source[] = {0x1fc,0x08,0x6c,0xd0,0x134,0x198};
        for (unsigned i = 0; i < sizeof(source) / sizeof(*source); ++i)
            memcpy(resolved + destination[i], preset + source[i], 0x64);
        memcpy(resolved + 0x10, preset, 8);
        memcpy(resolved + 0x18, preset + 0x260, 8);
        resolved[0x20] = preset[0x26a];
        memcpy(resolved + 0x27c, preset + 0x268, 2);
        memcpy(resolved + 0x27e, preset + 0x26c, 6);
    }
    return resolved;
}

/* In-between REL7B48 keys retain the first descriptor when either count
 * differs. Matching descriptors interpolate RGB, alpha and knot positions. */
static gh_game_event_status interpolate_knots(uint8_t *out, const uint8_t *a,
    const uint8_t *b, float t)
{
    if (!t) return GH_EVENT_OK;
    if (t == 1) {
        memcpy(out, b, 0x64);
        return GH_EVENT_OK;
    }
    if (read_word(a) != read_word(b)) return GH_EVENT_OK;
    const unsigned colors = read_half(a), alphas = read_half(a + 2);
    if (colors > 8 || alphas > 8) return GH_EVENT_UNSUPPORTED;
    const float left = binary_add(1.0f, -t);
    for (unsigned i = 0; i < colors; ++i) {
        const unsigned color = 4 + i * 4;
        for (unsigned channel = 0; channel < 3; ++channel)
            out[color + channel] = (uint8_t)binary_add(binary_mul(a[color + channel], left),
                binary_mul(b[color + channel], t));
        out[color + 3] = 0;
        const unsigned knot = 0x24 + i * 2;
        const float position = gh_event_light_lerp_00006c38((float)read_half(a + knot),
            (float)read_half(b + knot), t);
        put_half(out + knot, (unsigned)position);
    }
    put_half(out + 0x22 + colors * 2, 255);
    for (unsigned i = 0; i < alphas; ++i) {
        const unsigned alpha = 0x34 + i * 4;
        memset(out + alpha, 0, 3);
        out[alpha + 3] = (uint8_t)binary_add(binary_mul(a[alpha + 3], left),
            binary_mul(b[alpha + 3], t));
        const unsigned knot = 0x54 + i * 2;
        const float position = gh_event_light_lerp_00006c38((float)read_half(a + knot),
            binary_add(left, (float)read_half(b + knot)), t);
        put_half(out + knot, (unsigned)position);
    }
    put_half(out + 0x52 + alphas * 2, 255);
    return GH_EVENT_OK;
}

static gh_game_event_status interpolate_filter(uint8_t out[0x2e8], const uint8_t *a,
    const uint8_t *b, float t)
{
    memcpy(out, a, 0x2e8);
    for (unsigned at = 0x18; at <= 0x1c; at += 4) {
        const float value = gh_event_light_lerp_00006c38(read_float(a + at), read_float(b + at), t);
        if (!isfinite(value)) return GH_EVENT_UNSUPPORTED;
        memcpy(out + at, &value, sizeof(value));
    }
    const unsigned bytes[] = {0x20,0x27c,0x27d};
    for (unsigned i = 0; i < sizeof(bytes) / sizeof(*bytes); ++i) {
        const unsigned at = bytes[i];
        out[at] = (uint8_t)gh_event_light_lerp_00006c38((float)a[at], (float)b[at], t);
    }
    for (unsigned at = 0x27e; at <= 0x280; at += 2) {
        const float value = gh_event_light_lerp_00006c38((float)signed_half(a + at),
            (float)signed_half(b + at), t);
        put_half(out + at, (unsigned)(int)value);
    }
    put_half(out + 0x282, (unsigned)gh_event_light_lerp_00006c38(
        (float)read_half(a + 0x282), (float)read_half(b + 0x282), t));
    const unsigned knots[] = {0x24,0x88,0xec,0x150,0x1b4,0x284};
    for (unsigned i = 0; i < sizeof(knots) / sizeof(*knots); ++i) {
        const unsigned at = knots[i];
        gh_game_event_status status = interpolate_knots(out + at, a + at, b + at, t);
        if (status) return status;
    }
    return GH_EVENT_OK;
}

gh_game_event_status gh_event_filter_move_000070d0(gh_event_light_filter *state,
                                                     uint32_t *frontier)
{
    if (!frontier) return GH_EVENT_ARGUMENT;
    *frontier = 0x70d0;
    if (!state || !state->filters || !state->filters->ready) return GH_EVENT_ARGUMENT;
    gh_event_component_view component;
    gh_event_light_key_pair keys;
    gh_game_event_status status = active_keys(state, FILTER, &component, &keys, frontier);
    if (status) return status;
    const int interpolate = interpolates(&keys);
    uint8_t previous[0x2e8], next[0x2e8];
    keys.previous = resolve_filter_preset(state->filters, &component, keys.previous, previous);
    keys.next = resolve_filter_preset(state->filters, &component, keys.next, next);
    if (!interpolate) {
        apply_filter_key(state->filters, component.bytes, keys.previous);
    } else {
        *frontier = 0x72a0;
        fenv_t env;
        if (!enter_math(&env)) return GH_EVENT_CALLEE;
        float t;
        uint8_t result[0x2e8];
        status = gh_event_light_interpolation_factor_00004f08(state->data,
            keys.previous, keys.next, keys.cut, keys.frame, &t);
        if (!status) status = interpolate_filter(result, keys.previous, keys.next, t);
        if (fesetenv(&env)) return GH_EVENT_CALLEE;
        if (status) return status;
        memcpy(component.bytes + 0x300, result, sizeof(result));
        apply_filter_key(state->filters, component.bytes, component.bytes + 0x300);
    }
    *frontier = 0;
    return GH_EVENT_OK;
}


gh_game_event_status gh_event_filter_quit_000075a0(gh_event_light_filter *state,
    const gh_game_event_config *config, uint32_t *frontier)
{
    if (!frontier) return GH_EVENT_ARGUMENT;
    *frontier = 0x75a0;
    if (!config || !config->initialized) return GH_EVENT_ARGUMENT;
    if (!(config->flags & 0x100u)) {
        gh_event_component_view component;
        *frontier = 0x71a0;
        if (!state || !state->filters || !state->filters->ready ||
            !state->data || !state->data->resource || !state->data->resource->loaded ||
            state->allocation != state->data->resource->allocation.pointer ||
            !read_component(state->components, FILTER, 0xd268, 0x5e8, &component))
            return GH_EVENT_ARGUMENT;
        apply_filter_key(state->filters, component.bytes, component.bytes + 0x18);
    }
    *frontier = 0;
    return GH_EVENT_OK;
}
