#include "game_event_side_play.h"
#include "game_message_step.h"

#include <stdlib.h>
#include <string.h>

static uint16_t half(const uint8_t *p)
{
    return (uint16_t)(p[0] | (unsigned)p[1] << 8);
}

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

static gh_game_event_status get_component(const gh_event_data_prefix *data, unsigned index,
    uint32_t address, size_t size, gh_event_component_view *out)
{
    if (!data || !data->resource || !data->resource->loaded || !data->components ||
        !data->components->bound || !data->components->constructed ||
        index >= GH_EVENT_COMPONENT_COUNT) return GH_EVENT_ARGUMENT;
    *out = data->components->components[index];
    if (!out->bytes || out->original_offset != address || out->size != size)
        return GH_EVENT_ARGUMENT;
    if (data->components->event_no == 1 ||
        (data->components->event_no == 2 &&
         (index == 0 || index == 5 || index == 9 || index == 10))) return GH_EVENT_OK;
    return GH_EVENT_UNSUPPORTED;
}

static gh_game_event_status get_absent(const gh_event_data_prefix *data, unsigned index,
    uint32_t address, size_t size, gh_event_component_view *out)
{
    gh_game_event_status status = get_component(data, index, address, size, out);
    if (status) return status;
    gh_event_sequence_view track;
    status = gh_event_sequence_at(data, index, &track);
    if (status) return status;
    if (track.records || track.count || word(out->bytes + 0x10) || word(out->bytes + 0x14))
        return GH_EVENT_UNSUPPORTED;
    return GH_EVENT_OK;
}

static gh_game_event_status scroll_light_track(const gh_event_data_prefix *data,
    gh_event_component_view *component, gh_event_sequence_view *track)
{
    gh_game_event_status status = get_component(data, 5, 0xd910, 0xdc, component);
    if (status) return status;
    status = gh_event_sequence_at(data, 5, track);
    if (status) return status;
    if (word(component->bytes + 0x10) || word(component->bytes + 0x14) != track->count)
        return GH_EVENT_UNSUPPORTED;
    if (data->components->event_no == 1)
        return !track->records && !track->count ? GH_EVENT_OK : GH_EVENT_UNSUPPORTED;
    if (!track->records || track->count != 2 || track->stride != 0x60 || track->bytes != 0xc0)
        return GH_EVENT_UNSUPPORTED;

    /* EV02 supplies two camera keys, with no interpolation or origin
     * substitution. B770 normalizes their anonymous actor index to -1. */
    for (unsigned i = 0; i < track->count; ++i) {
        const uint8_t *row = track->records + i * track->stride;
        if (word(row) != ((26u + i) << 24 | 0xffffu) || word(row + 4) || word(row + 8) ||
            word(row + 0x28) != 0x3f800000u)
            return GH_EVENT_UNSUPPORTED;
        for (unsigned offset = 0x40; offset < 0x60; offset += 8) {
            if (!(word(row + offset) & 0x7fffffffu) &&
                !(word(row + offset + 4) & 0x7fffffffu)) return GH_EVENT_UNSUPPORTED;
        }
        gh_event_camera_cut camera;
        status = gh_event_camera_cut_at(data, row[3], &camera);
        if (status) return status;
        if (!camera.primary) return GH_EVENT_UNSUPPORTED;
    }
    return GH_EVENT_OK;
}

/* REL8B20 origin layout, grouped by the native WorldLight fields it saves. */
static void capture_scroll_light(uint8_t *key, const uint8_t *world)
{
    memset(key, 0, 0x60);
    memcpy(key + 0x0c, world + 0x30, 0x10);
    memcpy(key + 0x1c, world + 0x50, 0x10);
    memcpy(key + 0x2c, world, 8);
    memcpy(key + 0x34, world + 0x60, 4);
    memcpy(key + 0x38, world + 8, 8);
    memcpy(key + 0x40, world + 0x10, 8);
    memcpy(key + 0x48, world + 0x64, 8);
    memcpy(key + 0x50, world + 0x18, 0x10);
}

gh_game_event_status gh_event_scr_light_init_00008b20(gh_event_scr_light *state,
    const gh_event_data_prefix *data, gh_game_world_light *world, gh_game_init_state *init,
    gh_core_save_view save, const gh_light_player_parts *player, uint32_t *frontier, int *relative)
{
    if (!frontier || !relative) return GH_EVENT_ARGUMENT;
    *frontier = 0x8b20;
    *relative = 1;
    if (!state || !world || !world->constructed || !world->ready || !init ||
        !save.bytes || save.size < 0x18 || (state->stage &&
        (state->data != data || state->world != world || state->init != init ||
         state->save.bytes != save.bytes || state->save.size != save.size)))
        return GH_EVENT_ARGUMENT;
    gh_event_component_view component;
    *frontier = 0xb770;
    gh_event_sequence_view track;
    gh_game_event_status status = scroll_light_track(data, &component, &track);
    if (status) return status;
    if (!state->stage) {
        memcpy(component.bytes, "SCRLIGHT", sizeof("SCRLIGHT"));
        put(component.bytes + 0x18, word(world->scalar + 0x16288));
        world->scalar[0x16170] = 0x1e;
        put(world->scalar + 0x16284, 0);
        *state = (gh_event_scr_light){.data = data, .world = world, .init = init, .save = save, .stage = 1};
    }
    if (state->stage == 1) {
        *relative = 0;
        *frontier = 0x2d8408;
        gh_light_status child = gh_game_world_light_select_002d8408(world, 0, player);
        if (child) {
            if (world->frontier) *frontier = world->frontier;
            return child == GH_LIGHT_UNSUPPORTED ? GH_EVENT_UNSUPPORTED : GH_EVENT_CALLEE;
        }
        init->side_counter[4] = 0;
        put(init->side_counter + 8, 0);
        put(init->side_counter + 0x0c, 0);
        capture_scroll_light(component.bytes + 0x1c, world->scalar);
        put(save.bytes + 0x14, word(save.bytes + 0x14) & 0x7fffffffu);
        state->stage = 2;
    }
    *frontier = 0;
    return GH_EVENT_OK;
}

gh_game_event_status gh_event_scr_light_move_00008ef8(gh_event_scr_light *state,
    uint32_t *frontier)
{
    if (!frontier) return GH_EVENT_ARGUMENT;
    *frontier = 0x8ef8;
    if (!state || state->stage != 2 || !state->world || !state->world->ready)
        return GH_EVENT_ARGUMENT;
    gh_event_component_view component;
    gh_event_sequence_view track;
    gh_game_event_status status = scroll_light_track(state->data, &component, &track);
    if (status) return status;
    const uint8_t *key = component.bytes + 0x1c;
    if (track.count) {
        gh_event_component_view camera;
        if (gh_event_component_at((gh_event_components *)state->data->components, 1, &camera) ||
            !camera.bytes || camera.original_offset != 0xd1c8 || camera.size != 0x18)
            return GH_EVENT_ARGUMENT;
        const unsigned cut = half(camera.bytes + 6);
        if (camera.bytes[4] || cut >= state->data->camera_count) return GH_EVENT_UNSUPPORTED;
        for (unsigned i = 0; i < track.count; ++i) {
            const uint8_t *row = track.records + i * track.stride;
            if (row[3] <= cut) key = row;
        }
    }
    uint8_t *world = state->world->scalar;
    memcpy(world + 0x30, key + 0x0c, 0x0c);
    put(world + 0x3c, 0x3f800000);
    memcpy(world + 0x50, key + 0x1c, 0x0c);
    put(world + 0x5c, 0x3f800000);
    memcpy(world, key + 0x2c, 8);
    memcpy(world + 0x60, key + 0x34, 4);
    memcpy(world + 8, key + 0x38, 8);
    memcpy(world + 0x10, key + 0x40, 8);
    memcpy(world + 0x64, key + 0x48, 8);
    memcpy(world + 0x18, key + 0x50, 0x10);
    *frontier = 0;
    return GH_EVENT_OK;
}

gh_game_event_status gh_event_scr_light_quit_00009728(gh_event_scr_light *state,
    const gh_game_event_config *config, const gh_light_player_parts *player,
    uint32_t *frontier, int *relative)
{
    if (!frontier || !relative) return GH_EVENT_ARGUMENT;
    *frontier = 0x9728;
    *relative = 1;
    if (!state || state->stage != 2 || !config || !config->initialized ||
        !state->save.bytes || state->save.size < 0x18 || !state->world ||
        !state->world->ready || !state->init) return GH_EVENT_ARGUMENT;
    gh_game_world_light *world = state->world;
    if (!state->quit_stage) {
        uint8_t *save_flags = state->save.bytes + 0x14;
        put(save_flags, word(save_flags) & 0x7fffffffu);
        if (config->flags & 0x100u) {
            state->quit_stage = 2;
        } else {
            put(world->scalar + 0x78, 0);
            memset(world->scalar + 0x80, 0, 0x7000);
            memset(world->native_light_owners, 0, sizeof(world->native_light_owners));
            for (unsigned i = 0; i < 256; ++i)
                world->scalar[0x80 + i * 0x70] = 0xff;
            state->quit_stage = 1;
        }
    }
    if (state->quit_stage == 1) {
        *frontier = 0x2d8408;
        *relative = 0;
        gh_light_status status = gh_game_world_light_select_002d8408(world, 0, player);
        if (status) {
            if (world->frontier) *frontier = world->frontier;
            return status == GH_LIGHT_UNSUPPORTED ? GH_EVENT_UNSUPPORTED : GH_EVENT_CALLEE;
        }
        state->init->side_counter[4] = 0;
        put(state->init->side_counter + 8, 0);
        put(state->init->side_counter + 0x0c, 0);
        state->quit_stage = 2;
    }
    *frontier = 0;
    return GH_EVENT_OK;
}

gh_game_event_status gh_event_bgm_init_000046d8(const gh_event_data_prefix *data,
    gh_bgm_requests *bgm, uint32_t *frontier, int *relative)
{
    if (!frontier || !relative) return GH_EVENT_ARGUMENT;
    *frontier = 0xc7f0;
    *relative = 1;
    gh_event_component_view component;
    gh_game_event_status status = get_absent(data, 9, 0xd1b0, 0x18, &component);
    if (status) return status;
    memcpy(component.bytes, "BGM", sizeof("BGM"));
    *relative = 0;
    *frontier = 0x2d1800;
    if (!bgm) return GH_EVENT_ARGUMENT;
    gh_bgm_status child = gh_bgm_event_stop_002d1800(bgm, 1);
    if (child) {
        const uint32_t detail = gh_bgm_requests_frontier(bgm);
        if (detail) *frontier = detail;
        return child == GH_BGM_UNSUPPORTED ? GH_EVENT_UNSUPPORTED : GH_EVENT_CALLEE;
    }
    *frontier = 0;
    return GH_EVENT_OK;
}

gh_game_event_status gh_event_bgm_move_00004708(const gh_event_data_prefix *data,
    uint32_t *frontier)
{
    if (!frontier) return GH_EVENT_ARGUMENT;
    *frontier = 0x4708;
    gh_event_component_view component;
    gh_game_event_status status = get_absent(data, 9, 0xd1b0, 0x18, &component);
    if (!status) *frontier = 0;
    return status;
}

static gh_game_event_status scroll_views(const gh_room_pools *pools, const gh_scene_meshes *meshes,
    uint8_t ***views, uint32_t *count)
{
    const gh_room_vector *list;
    if (!meshes || meshes->pools != pools || gh_room_registry_view(pools, 0x754c38, &list))
        return GH_EVENT_ARGUMENT;
    uint8_t **rows = list->count ? malloc((size_t)list->count * sizeof(*rows)) : NULL;
    if (list->count && !rows) return GH_EVENT_CALLEE;
    const uint8_t *handles = list->allocation.pointer;
    for (unsigned i = 0; i < list->count; ++i) {
        const uint32_t handle = word(handles + i * 4);
        void *owner;
        if (gh_room_object_lookup(pools, handle, GH_ROOM_OBJECT_SCROLL_MESH, meshes, &owner)) {
            free(rows);
            return GH_EVENT_ARGUMENT;
        }
        const gh_scene_mesh_object *mesh = owner;
        if (mesh->handle != handle || !mesh->allocation.pointer || !mesh->factory_ready) {
            free(rows);
            return GH_EVENT_BOUNDS;
        }
        rows[i] = mesh->allocation.pointer;
    }
    *views = rows;
    *count = list->count;
    return GH_EVENT_OK;
}

static uint32_t layer_bit(const uint8_t *object)
{
    return UINT32_C(0x80000000) >> (object[0x2fd] & 31);
}

static void apply_layer(uint8_t *const *rows, unsigned count, const uint8_t *mask, uint32_t flag)
{
    for (unsigned i = 0; i < count; ++i) {
        uint8_t *object = rows[i];
        const uint32_t selected = word(mask + 4 * (object[0x2fd] >> 5)) & layer_bit(object);
        const uint32_t flags = word(object + 0x250);
        put(object + 0x250, selected ? flags | flag : flags & ~flag);
    }
}

gh_game_event_status gh_event_scr_layer_init_00008948(const gh_event_data_prefix *data,
    const gh_room_pools *pools, const gh_scene_meshes *meshes, uint32_t *frontier, int *relative)
{
    if (!frontier || !relative) return GH_EVENT_ARGUMENT;
    *frontier = 0xba70;
    *relative = 1;
    gh_event_component_view component;
    gh_game_event_status status = get_absent(data, 10, 0xd898, 0x78, &component);
    if (status) return status;
    *relative = 0;
    *frontier = 0x2db5a0;
    uint8_t **rows;
    uint32_t count;
    status = scroll_views(pools, meshes, &rows, &count);
    if (status) return status;
    uint8_t *cache = component.bytes + 0x18, *origin = component.bytes + 0x38;
    uint8_t *saved = component.bytes + 0x58;
    memcpy(component.bytes, "SCRLAYER", sizeof("SCRLAYER"));
    memset(cache, 0, 0x60);
    for (unsigned i = 0; i < count; ++i) {
        const uint8_t *object = rows[i];
        if (word(object + 0x250) & 2) {
            uint8_t *part = saved + 4 * (object[0x2fd] >> 5);
            put(part, word(part) | layer_bit(object));
        }
    }
    apply_layer(rows, count, origin, 2);
    apply_layer(rows, count, origin, 0x8000);
    free(rows);
    *frontier = 0;
    return GH_EVENT_OK;
}

gh_game_event_status gh_event_scr_layer_move_00008a40(const gh_event_data_prefix *data,
    const gh_room_pools *pools, const gh_scene_meshes *meshes, uint32_t *frontier, int *relative)
{
    if (!frontier || !relative) return GH_EVENT_ARGUMENT;
    *frontier = 0x8a40;
    *relative = 1;
    gh_event_component_view component;
    gh_game_event_status status = get_absent(data, 10, 0xd898, 0x78, &component);
    if (status) return status;
    uint8_t *cache = component.bytes + 0x18, *origin = component.bytes + 0x38;
    put(origin, word(origin) & 0x7fffffffu);
    if (memcmp(origin, cache, 0x20)) {
        *relative = 0;
        *frontier = 0x2db518;
        uint8_t **rows;
        uint32_t count;
        status = scroll_views(pools, meshes, &rows, &count);
        if (status) return status;
        apply_layer(rows, count, origin, 0x8000);
        free(rows);
        memcpy(cache, origin, 0x20);
    }
    *frontier = 0;
    return GH_EVENT_OK;
}

gh_game_event_status gh_event_scr_layer_quit_00008af0(const gh_event_data_prefix *data,
    const gh_room_pools *pools, const gh_scene_meshes *meshes, uint32_t *frontier, int *relative)
{
    if (!frontier || !relative) return GH_EVENT_ARGUMENT;
    *frontier = 0x8af0;
    *relative = 1;
    gh_event_component_view component;
    gh_game_event_status status = get_component(data, 10, 0xd898, 0x78, &component);
    if (status) return status;
    *frontier = 0x2db518;
    *relative = 0;
    uint8_t **rows;
    uint32_t count;
    status = scroll_views(pools, meshes, &rows, &count);
    if (status) return status;
    apply_layer(rows, count, component.bytes + 0x38, 0x8000);
    apply_layer(rows, count, component.bytes + 0x58, 2);
    free(rows);
    *frontier = 0;
    return GH_EVENT_OK;
}

gh_game_event_status gh_event_message_init_00008100(gh_event_message_play *state,
    const gh_event_data_prefix *data, const gh_game_event_config *config, gh_game_scene_ui *ui,
    uint32_t filter_flags, uint16_t filter_message, gh_event_message_create create,
    void *context, uint32_t *frontier, int *relative)
{
    if (!frontier || !relative) return GH_EVENT_ARGUMENT;
    *frontier = 0x8100;
    *relative = 1;
    if (!state || !config || !config->initialized || !config->record ||
        (state->stage && (state->data != data || state->ui != ui))) return GH_EVENT_ARGUMENT;
    gh_event_component_view env;
    gh_game_event_status status = get_component(data, 0, 0xd200, 0x44, &env);
    if (status) return status;
    if (!state->stage) {
        state->data = data;
        state->ui = ui;
        const uint32_t mode = word(env.bytes + 0x0c);
        state->message_id = half(config->record);
        if ((filter_flags & 0x200u) && filter_message != UINT16_MAX) state->message_id = filter_message;
        if (mode == 1 || mode == 2) state->message_id = UINT16_MAX;
        state->stage = state->message_id == UINT16_MAX ? 2 : 1;
    }
    if (state->stage == 1) {
        *relative = 0;
        *frontier = 0x2aeb68;
        if (!ui || !ui->ready || !create) return GH_EVENT_UNSUPPORTED;
        uint32_t child = 0;
        uint16_t work_id;
        status = create(context, ui, state->message_id, &work_id, &child);
        if (status) {
            if (child) *frontier = child;
            return status;
        }
        state->work_id = work_id;
        state->stage = 2;
    }
    *frontier = 0;
    return GH_EVENT_OK;
}

void gh_event_message_move_000081a8(void)
{
}

gh_game_event_status gh_event_message_quit_000081b0(const gh_event_message_play *state,
    uint32_t *frontier, int *relative)
{
    if (!frontier || !relative) return GH_EVENT_ARGUMENT;
    *frontier = 0x81b0;
    *relative = 1;
    if (!state || state->stage != 2) return GH_EVENT_ARGUMENT;
    if (state->message_id == UINT16_MAX) {
        *frontier = 0;
        return GH_EVENT_OK;
    }
    int found;
    *relative = 0;
    switch (gh_scene_message_delete_002aebf8(state->ui, state->work_id, &found, frontier)) {
        case GH_SCENE_UI_OK: return GH_EVENT_OK; /* The original caller ignores found. */
        case GH_SCENE_UI_ARGUMENT: return GH_EVENT_ARGUMENT;
        case GH_SCENE_UI_BOUNDS: return GH_EVENT_BOUNDS;
        default: return GH_EVENT_UNSUPPORTED;
    }
}
