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

/* Flag sweeps may exclude bank D7. Final resource release selects bank 6
 * directly. All three active masks are read anew for each slot. */
static int selected(const uint8_t *p, unsigned offset, uint32_t flags, int bank, int exclude) {
    if (flags && !(word(p + offset) & flags)) return 0;
    if (bank < 0) return 1;
    unsigned tag_offset = offset == 0x104 ? 0x23a : 0x18c;
    unsigned tag = p[tag_offset] | (unsigned)p[tag_offset + 1] << 8;
    return (tag == (unsigned)bank) != exclude;
}

static gh_game_event_status sweep(const gh_effect_release_bindings *storage, unsigned *next_esp,
    uint32_t flags, int bank, int exclude, uint32_t *frontier)
{
    for (; *next_esp < 1024; ++*next_esp) {
        const unsigned i = *next_esp;
        const uint8_t *p = storage->esp + i * 0x400;
        if (!live(storage->esp + 0x100000, 0x80, i) || !selected(p, 0x104, flags, bank, exclude)) continue;
        *frontier = 0x2fbe50;
        gh_effect_esp_view view;
        if (gh_effect_create_esp_at(storage->effects, i, &view) || view.payload != p)
            return GH_EVENT_UNSUPPORTED;
        uint32_t child = 0;
        gh_effect_status status = gh_effect_create_esp_pause_002fbe50(storage->effects, i, view.serial, &child);
        if (status) {
            if (child) *frontier = child;
            return effect_status(status);
        }
    }
    /* 302918 only requests emitter retirement. 300628 owns its actual
     * deferred child unlink, mask progression, and destructor. */
    for (unsigned i = 0; i < 256; ++i) {
        uint8_t *p = storage->emitters + i * 0x260;
        if (live(storage->emitters + 0x26000, 0x20, i) && selected(p, 0x118, flags, bank, exclude))
            put(p + 0x114, word(p + 0x114) | 0x40000000u);
    }
    *next_esp = 0;
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
        status = sweep(storage, &state->next_esp, 4, -1, 0, frontier);
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


gh_game_event_status gh_event_effect_quit_00005478(gh_event_effect_quit *state,
    const gh_event_effect_play *play, const gh_effects_frame_bindings *b, uint32_t *frontier)
{
    if (!frontier) return GH_EVENT_ARGUMENT;
    *frontier = 0x2fa310;
    if (!state || !play || !b || !play->data || !play->data->resource ||
        play->allocation != play->data->resource->allocation.pointer || play->stage ||
        play->work != b->work || !play->work || play->effects != b->storage.effects || !play->effects ||
        !b->storage.esp || b->storage.esp_bytes < 0x100180 ||
        !b->storage.emitters || b->storage.emitter_bytes < 0x26060) return GH_EVENT_ARGUMENT;
    if (state->play && (state->play != play || state->esp != b->storage.esp ||
        state->emitters != b->storage.emitters)) return GH_EVENT_ARGUMENT;
    if (state->completed) {
        *frontier = 0;
        return GH_EVENT_OK;
    }
    if (!state->play) {
        state->play = play;
        state->esp = b->storage.esp;
        state->emitters = b->storage.emitters;
    }
    gh_game_effects *effects = play->effects;
    gh_game_event_status status;
    if (state->stage == 0) {
        status = sweep(&b->storage, &state->next_esp, 2, 0xd7, 1, frontier);
        if (status) return status;
        state->stage = 1;
    }
    if (state->stage == 1) {
        status = sweep(&b->storage, &state->next_esp, 0x40, -1, 0, frontier);
        if (status) return status;
        effects->field414 = 0;
        state->stage = 2;
    }
    if (state->stage == 2) {
        gh_effect_status child = gh_effects_frame_refresh_regions_00305c20(&state->region, b);
        if (child) {
            *frontier = state->region.frontier ? state->region.frontier : 0x305c20;
            return effect_status(child);
        }
        state->stage = 3;
    }
    if (state->stage == 3) {
        *frontier = 0x2fa0e8;
        if (effects->event_registered_41c != 1) state->stage = 5;
        else {
            if (!same_bank(play->data, effects)) return GH_EVENT_ARGUMENT;
            unsigned index;
            for (index = 0; index < 64 && effects->resources[index].kind != 6; ++index) {}
            if (index == 64 || !effects->resources[index].references) {
                *frontier = 0x305778;
                return GH_EVENT_UNSUPPORTED;
            }
            const gh_effect_view emd = effects->emd_tables[6];
            if (!emd.bytes || emd.size < 16 || word(emd.bytes) != 0x444d45u ||
                word(emd.bytes + 4) > 64 || word(emd.bytes + 4) > (emd.size - 16) / 4) {
                *frontier = 0x306bb0;
                return GH_EVENT_BOUNDS;
            }
            if (effects->resources[index].references == 1) {
                for (uint32_t i = 0; i < word(emd.bytes + 4); ++i) {
                    if (word(emd.bytes + 16 + 4*i)) {
                        *frontier = 0x306a48;
                        return GH_EVENT_UNSUPPORTED;
                    }
                }
            }
            effects->event_registered_41c = 0;
            if (--effects->resources[index].references) state->stage = 5;
            else {
                /* EV01 EMD has no populated pointer slots. 306BB0 clears its
                 * bank entry without clearing the shared emitter table. */
                effects->emd_tables[6] = (gh_effect_view){0};
                state->resource_slot = index;
                state->stage = 4;
            }
        }
    }
    if (state->stage == 4) {
        status = sweep(&b->storage, &state->next_esp, 0, 6, 0, frontier);
        if (status) return status;
        effects->resources[state->resource_slot] = (gh_effect_resource){.kind = 5};
        effects->event_data = (gh_effect_view){0};
        state->stage = 5;
    }
    uint8_t *system = (uint8_t *)play->work + 0x5e4;
    put(system, word(system) & ~0x10000u);
    state->completed = 1;
    *frontier = 0;
    return GH_EVENT_OK;
}
