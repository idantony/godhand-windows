#include "game_event_play_env.h"

static uint32_t read_word(const uint8_t *bytes) {
    return (uint32_t)bytes[0] | (uint32_t)bytes[1] << 8 |
           (uint32_t)bytes[2] << 16 | (uint32_t)bytes[3] << 24;
}

static void write_word(uint8_t *bytes, uint32_t value) {
    for (unsigned byte = 0; byte < 4; ++byte) bytes[byte] = (uint8_t)(value >> (byte * 8));
}

static gh_game_event_status component(gh_event_components *components, unsigned index,
                                      uint32_t offset, size_t bytes, gh_event_component_view *view) {
    if (!components || !components->constructed) return GH_EVENT_ARGUMENT;
    gh_game_event_status status = gh_event_component_at(components, index, view);
    if (status != GH_EVENT_OK) return status;
    if (!view->bytes || view->original_offset != offset || view->size != bytes) return GH_EVENT_BOUNDS;
    return GH_EVENT_OK;
}

gh_game_event_status gh_event_env_init_play_00005930(
    gh_event_components *components, const gh_game_event_config *config,
    gh_boot_work *work, gh_screen_fade *fade, const gh_display_state *display,
    uint32_t *frontier) {
    if (frontier) *frontier = 0x296cf4;
    if (!config || !config->initialized || !work) return GH_EVENT_ARGUMENT;

    gh_event_component_view env;
    gh_game_event_status status = component(components, 0, 0xd200, 0x44, &env);
    if (status != GH_EVENT_OK) return status;
    const uint32_t mode = read_word(env.bytes + 0x0c);
    const int tool_mode = mode >= 1 && mode <= 3;
    const int start_fade = !tool_mode && ((config->flags & 8) || !(config->flags & 2));
    uint8_t *raw_work = (uint8_t *)work;
    gh_screen_fade next_fade;
    if (start_fade) {
        if (frontier) *frontier = 0x307208;
        if (!fade || !display || !display->initialized) return GH_EVENT_ARGUMENT;
        const gh_screen_fade_context context = {
            display->screen_width, display->gs_y_origin,
            read_word(raw_work + 0x564), read_word(raw_work + 0x5e0)
        };
        next_fade = *fade;
        if (gh_screen_fade_start_00307208(&next_fade, &context, 0, 8, 0,
                                         0xff000000u, 0, 15) != GH_FADE_OK) {
            return GH_EVENT_CALLEE;
        }
    }

    if (!tool_mode) write_word(raw_work + 0x5d8, read_word(raw_work + 0x5d8) & ~0x40000000u);
    write_word(env.bytes + 0x18, 0);
    write_word(env.bytes + 0x1c, 0);
    write_word(env.bytes + 0x10, UINT32_MAX);
    write_word(env.bytes + 0x14, 0xbf800000u);
    write_word(env.bytes + 0x2c, 255);
    if (start_fade) *fade = next_fade;
    if (frontier) *frontier = 0;
    return GH_EVENT_OK;
}

gh_game_event_status gh_event_fade_init_absent_00006c70(
    gh_event_components *components, const void *resource, size_t resource_bytes,
    uint32_t *frontier) {
    if (frontier) *frontier = 0x297f80;
    if (!resource) return GH_EVENT_ARGUMENT;
    if (resource_bytes < 0x50) return GH_EVENT_BOUNDS;
    if (read_word((const uint8_t *)resource + 0x4c)) return GH_EVENT_UNSUPPORTED;

    gh_event_component_view view;
    gh_game_event_status status = component(components, 13, 0xd248, 0x1c, &view);
    if (status != GH_EVENT_OK) return status;
    write_word(view.bytes + 0x10, 0);
    write_word(view.bytes + 0x14, 0);
    view.bytes[0x18] = 0;
    if (frontier) *frontier = 0;
    return GH_EVENT_OK;
}

gh_game_event_status gh_event_fade_move_absent_00006ca0(
    gh_event_components *components, gh_screen_fade *fade, uint32_t *frontier) {
    if (frontier) *frontier = 0x296eb4;
    gh_event_component_view view;
    gh_game_event_status status = component(components, 13, 0xd248, 0x1c, &view);
    if (status != GH_EVENT_OK) return status;
    if (read_word(view.bytes + 0x10) || read_word(view.bytes + 0x14)) return GH_EVENT_UNSUPPORTED;
    if (view.bytes[0x18]) {
        if (!fade) return GH_EVENT_ARGUMENT;
        gh_screen_fade_kill_003076e0(fade);
        view.bytes[0x18] = 0;
    }
    if (frontier) *frontier = 0;
    return GH_EVENT_OK;
}

static gh_game_event_status extra_play_init(gh_event_components *components,
    const gh_game_event_config *config, uint32_t *frontier)
{
    *frontier = 0x296cfc;
    gh_event_component_view env;
    gh_game_event_status status = component(components, 0, 0xd200, 0x44, &env);
    if (status) return status;
    if (read_word(env.bytes + 0x28) & 0x40000000) return GH_EVENT_OK;
    if (!config || !config->initialized || !config->record) return GH_EVENT_ARGUMENT;
    uint32_t callback = read_word(config->record + 8);
    if (!callback) return GH_EVENT_OK;
    if (callback != 0x298060 || components->event_no != 1 ||
        config->record_address != 0x3c055c)
        return GH_EVENT_UNSUPPORTED;

    /* 298060 supplies the only EV01 ExtraPlayMoveObj identity to REL+6A80. */
    write_word(env.bytes + 0x34, 0x298088);
    return GH_EVENT_OK;
}

gh_game_event_status gh_event_env_extra_move_object_00006b08(
    gh_event_components *components, gh_title_player_view *object,
    gh_core_save_view save, uint32_t *frontier)
{
    if (!frontier) return GH_EVENT_ARGUMENT;
    *frontier = 0x298088;
    gh_event_component_view env;
    gh_game_event_status status = component(components, 0, 0xd200, 0x44, &env);
    if (status) return status;
    uint32_t callback = read_word(env.bytes + 0x34);
    if (callback) {
        if (callback != 0x298088 || components->event_no != 1) return GH_EVENT_UNSUPPORTED;
        if (object) {
            if (!object->bytes || object->size < 0x300) return GH_EVENT_BOUNDS;
            if (save.bytes && save.size <= 0xae) return GH_EVENT_BOUNDS;
            uint8_t *bytes = object->bytes;
            uint32_t kind = (uint32_t)bytes[0x2fe] | (uint32_t)bytes[0x2ff] << 8;
            if (kind == 0x667 && save.bytes && save.bytes[0xae])
                write_word(bytes + 0x250, read_word(bytes + 0x250) | 2);
        }
    }
    *frontier = 0;
    return GH_EVENT_OK;
}

static gh_game_event_status visit_component(gh_event_play_state *play, const gh_event_play_bindings *b,
    uint32_t site, int *end, uint32_t *frontier)
{
    *frontier = site;
    if (!b->visit) return GH_EVENT_UNSUPPORTED;
    uint32_t child = 0;
    int relative = 0;
    gh_game_event_status status = b->visit(b->context, site, end, &child, &relative);
    if (status && child) {
        *frontier = child;
        play->frontier_relative = (uint8_t)(relative != 0);
    }
    return status;
}

gh_game_event_status gh_event_play_00296c28(gh_event_play_state *play,
    const gh_event_play_bindings *b, int *original_done, uint32_t *frontier)
{
    if (!original_done || !frontier) return GH_EVENT_ARGUMENT;
    *original_done = 0;
    *frontier = 0x296c28;
    if (!play || !b || !b->event || !b->event->initialized || !b->work) return GH_EVENT_ARGUMENT;
    gh_game_event *event = b->event;
    gh_event_components *components = b->components;
    play->frontier_relative = 0;
    if (play->event &&
        (play->event != event || play->components != components ||
         play->event_number != event->fields.data_number))
        return GH_EVENT_ARGUMENT;

    if (!event->fields.phase) {
        if ((event->fields.flags & 6) == 2) {
            if (!components || !components->constructed ||
                components->event_no != event->fields.data_number)
                return GH_EVENT_ARGUMENT;
            play->event = event;
            play->components = components;
            play->event_number = event->fields.data_number;
            if (!play->next_initialization) {
                uint8_t *raw_work = (uint8_t *)b->work;
                write_word(raw_work + 0x5e4, read_word(raw_work + 0x5e4) | 1);
                gh_game_event_status status = gh_event_env_init_play_00005930(
                    components, b->config, b->work, b->fade, b->display, frontier);
                if (status) return status;
                play->next_initialization = 1;
            }
            if (play->next_initialization == 1) {
                gh_game_event_status status = extra_play_init(components, b->config, frontier);
                if (status) return status;
                play->next_initialization = 2;
            }
            static const uint32_t init_sites[] = {
                0x296d08, 0x296d14, 0x296d20, 0x296d2c, 0x296d38, 0x296d44, 0x296d50,
                0x296d5c, 0x296d68, 0x296d74, 0x296d80, 0x296d8c, 0x296d98, 0x296da4, 0x296db0
            };
            for (; play->next_initialization - 2u < sizeof(init_sites) / sizeof(*init_sites);
                 ++play->next_initialization) {
                int ignored = 0;
                gh_game_event_status status = visit_component(play, b, init_sites[play->next_initialization - 2u],
                    &ignored, frontier);
                if (status) return status;
            }
            event->fields.flags |= 4;
        }
        event->fields.phase++;
    }
    if (event->fields.phase == 1 && (event->fields.flags & 4)) {
        static const uint32_t move_sites[] = {
            0x296dfc, 0x296e04, 0x296e0c, 0x296e18, 0x296e24, 0x296e30, 0x296e3c,
            0x296e48, 0x296e54, 0x296e60, 0x296e6c, 0x296e78, 0x296e84, 0x296e90,
            0x296e9c, 0x296ea8, 0x296eb4, 0x296ec0, 0x296ec8
        };
        if (!play->quit_pending) {
            int end = 0;
            for (; play->next_move < sizeof(move_sites) / sizeof(*move_sites); ++play->next_move) {
                end = 0;
                gh_game_event_status status = visit_component(play, b, move_sites[play->next_move], &end, frontier);
                if (status) return status;
            }
            play->next_move = 0;
            play->quit_pending = (uint8_t)(end != 0);
        }
        if (play->quit_pending) {
            *frontier = 0x296ef0;
            return GH_EVENT_UNSUPPORTED;
        }
    }
    *frontier = 0;
    return GH_EVENT_OK;
}
