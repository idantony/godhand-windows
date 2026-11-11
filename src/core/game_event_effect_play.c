#include "game_event_effect_play.h"

#include <string.h>

static uint32_t word(const uint8_t *p)
{
    uint32_t value;
    memcpy(&value, p, sizeof(value));
    return value;
}

static void put(uint8_t *p, uint32_t value)
{
    memcpy(p, &value, sizeof(value));
}

static gh_game_event_status absent(const gh_event_data_prefix *data, unsigned index,
    uint32_t address, gh_event_component_view *view)
{
    if (!data || !data->components || !data->components->constructed ||
        gh_event_component_at((gh_event_components *)data->components, index, view) ||
        !view->bytes || view->original_offset != address || view->size != 0x18)
        return GH_EVENT_ARGUMENT;
    gh_event_sequence_view track;
    gh_game_event_status status = gh_event_sequence_at(data, index, &track);
    if (status) return status;
    if (data->components->event_no != 1 || track.records || track.count ||
        word(view->bytes + 0x10) || word(view->bytes + 0x14))
        return GH_EVENT_UNSUPPORTED;
    return GH_EVENT_OK;
}

gh_game_event_status gh_event_vertex_init_absent_0000a200(const gh_event_data_prefix *data,
    uint32_t *frontier)
{
    if (!frontier) return GH_EVENT_ARGUMENT;
    *frontier = 0xb2c0;
    gh_event_component_view component;
    gh_game_event_status status = absent(data, 3, 0xda08, &component);
    if (status) return status;
    memcpy(component.bytes, "VERTEX", sizeof("VERTEX"));
    *frontier = 0;
    return GH_EVENT_OK;
}

gh_game_event_status gh_event_vertex_move_absent_0000a220(const gh_event_data_prefix *data,
    uint32_t *frontier)
{
    if (!frontier) return GH_EVENT_ARGUMENT;
    *frontier = 0xa220;
    gh_event_component_view component;
    gh_game_event_status status = absent(data, 3, 0xda08, &component);
    if (!status) *frontier = 0;
    return status;
}

static int same_bank(const gh_event_data_prefix *data, const gh_game_effects *effects)
{
    if (!effects || !effects->ready || !effects->event_registered_41c ||
        !data || !data->resource || !data->resource->loaded || !data->effect_register ||
        !data->data || data->bytes < 0x20) return 0;
    const size_t offset = word(data->data + 0x1c);
    return offset && offset < data->bytes && effects->event_data.bytes == data->data + offset &&
           effects->event_data.size <= data->bytes - offset;
}

gh_game_event_status gh_event_effect_init_00005000(gh_event_effect_play *state,
    const gh_event_data_prefix *data, const gh_game_event_config *config, gh_boot_work *work,
    gh_game_effects *effects, uint32_t *frontier)
{
    if (!frontier) return GH_EVENT_ARGUMENT;
    *frontier = 0x5000;
    if (!state || state->stage || !config || !config->initialized || !work || !same_bank(data, effects) ||
        (state->effects && state->effects != effects)) return GH_EVENT_ARGUMENT;
    gh_event_component_view component;
    *frontier = 0xc728;
    gh_game_event_status status = absent(data, 4, 0xd1e8, &component);
    if (status) return status;
    memcpy(component.bytes, "EFFECT", sizeof("EFFECT"));
    uint8_t *system = (uint8_t *)work + 0x5e4;
    put(system, word(system) | 0x10000u | ((config->flags & 0x200u) ? 0x4000000u : 0));
    *state = (gh_event_effect_play){.data = data, .allocation = data->resource->allocation.pointer,
                                   .effects = effects, .work = work};
    *frontier = 0;
    return GH_EVENT_OK;
}

static int live(const uint8_t *masks, unsigned stride, unsigned slot)
{
    const unsigned offset = (slot >> 5) * 4;
    const uint32_t bit = UINT32_C(0x80000000) >> (slot & 31);
    return (word(masks + offset) & bit) && !(word(masks + offset + stride) & bit) &&
           !(word(masks + offset + 2 * stride) & bit);
}

static gh_game_event_status effect_status(gh_effect_status status)
{
    switch (status) {
        case GH_EFFECT_OK: return GH_EVENT_OK;
        case GH_EFFECT_ARGUMENT: return GH_EVENT_ARGUMENT;
        case GH_EFFECT_BOUNDS: return GH_EVENT_BOUNDS;
        default: return GH_EVENT_UNSUPPORTED;
    }
}

static gh_game_event_status cut_sweep(gh_event_effect_play *state,
    const gh_effect_release_bindings *storage, uint32_t *frontier)
{
    for (; state->next_esp < 1024; ++state->next_esp) {
        const unsigned i = state->next_esp;
        const uint8_t *p = storage->esp + i * 0x400;
        if (!live(storage->esp + 0x100000, 0x80, i) || !(word(p + 0x104) & 4)) continue;
        *frontier = 0x2fbe50;
        gh_effect_esp_view view;
        if (gh_effect_create_esp_at(state->effects, i, &view) || view.payload != p)
            return GH_EVENT_UNSUPPORTED;
        uint32_t child = 0;
        gh_effect_status status = gh_effect_create_esp_pause_002fbe50(state->effects, i, view.serial, &child);
        if (status) {
            if (child) *frontier = child;
            return effect_status(status);
        }
    }
    /* 300C00 selects the same live slots by flag4. 302918's entire body
     * sets the emitter retirement request, consumed later by frame update. */
    for (unsigned i = 0; i < 256; ++i) {
        uint8_t *p = storage->emitters + i * 0x260;
        if (live(storage->emitters + 0x26000, 0x20, i) && (word(p + 0x118) & 4))
            put(p + 0x114, word(p + 0x114) | 0x40000000u);
    }
    return GH_EVENT_OK;
}

gh_game_event_status gh_event_effect_move_00005050(gh_event_effect_play *state,
    const gh_effect_create_bindings *bindings, uint32_t *frontier, int *relative)
{
    if (!frontier || !relative) return GH_EVENT_ARGUMENT;
    *frontier = 0x5050;
    *relative = 1;
    if (!state || !bindings || !state->data || !state->data->resource ||
        state->allocation != state->data->resource->allocation.pointer ||
        !same_bank(state->data, state->effects) || state->work != bindings->work ||
        state->effects != bindings->storage.effects) return GH_EVENT_ARGUMENT;
    gh_event_component_view component, env;
    gh_game_event_status status = absent(state->data, 4, 0xd1e8, &component);
    if (status) return status;
    if (gh_event_component_at((gh_event_components *)state->data->components, 0, &env) ||
        !env.bytes || env.size != 0x44 || env.original_offset != 0xd200) return GH_EVENT_ARGUMENT;
    float frame;
    memcpy(&frame, env.bytes + 0x14, sizeof(frame));
    const int cut_start = (word(env.bytes + 0x28) & 1) && frame == 0;
    if (!cut_start) {
        if (state->stage) return GH_EVENT_ARGUMENT;
        *frontier = 0;
        return GH_EVENT_OK;
    }
    const uint32_t cut = word(env.bytes + 0x10);
    if (cut >= state->data->camera_count) return GH_EVENT_BOUNDS;
    const gh_effect_release_bindings *storage = &bindings->storage;
    if (!storage->esp || storage->esp_bytes < 0x100180 ||
        !storage->emitters || storage->emitter_bytes < 0x26060 ||
        (state->stage && (state->cut != cut || state->esp != storage->esp ||
                          state->emitters != storage->emitters))) return GH_EVENT_ARGUMENT;
    if (!state->stage) {
        state->cut = cut;
        state->esp = storage->esp;
        state->emitters = storage->emitters;
        state->next_esp = 0;
        state->stage = 1;
    }
    *relative = 0;
    if (state->stage == 1) {
        status = cut_sweep(state, storage, frontier);
        if (status) return status;
        state->stage = 2;
    }
    if (state->stage == 2) {
        *frontier = 0x306030;
        int present;
        gh_effect_status child = gh_effect_event_cut_present_002fa128(state->effects, cut, &present);
        if (child) return effect_status(child);
        if (present) {
            *frontier = 0x306528;
            child = gh_effect_create_event_cut_begin_002fa158(&state->create, cut);
            if (child) return effect_status(child);
            state->stage = 3;
        }
    }
    if (state->stage == 3) {
        gh_effect_status child = gh_game_effect_create_00306528(&state->create, bindings);
        if (child || !state->create.complete) {
            *frontier = state->create.frontier ? state->create.frontier : 0x306528;
            return child ? effect_status(child) : GH_EVENT_CALLEE;
        }
    }
    state->stage = 0;
    *frontier = 0;
    return GH_EVENT_OK;
}
