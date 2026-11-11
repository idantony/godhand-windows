#include "game_event_side_play.h"

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
    return data->components->event_no == 1 ? GH_EVENT_OK : GH_EVENT_UNSUPPORTED;
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
    gh_game_event_status status = get_absent(data, 5, 0xd910, 0xdc, &component);
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
    gh_game_event_status status = get_absent(state->data, 5, 0xd910, 0xdc, &component);
    if (status) return status;
    const uint8_t *key = component.bytes + 0x1c;
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
