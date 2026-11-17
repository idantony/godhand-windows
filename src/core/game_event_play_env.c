#include "game_event_play_env.h"
#include "game_scenario_soft_event.h"
#include "game_player_node_cache.h"
#include <fenv.h>
#include <math.h>
#include <limits.h>
#include <string.h>

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

gh_game_event_status gh_event_env_move_play_000059a0(
    gh_event_components *components, gh_boot_work *work, uint32_t *frontier, int *relative)
{
    if (!frontier || !relative || !work) return GH_EVENT_ARGUMENT;
    *frontier = 0x59a0;
    *relative = 1;
    gh_event_component_view env;
    gh_game_event_status status = component(components, 0, 0xd200, 0x44, &env);
    if (status) return status;
    write_word(env.bytes + 0x28, read_word(env.bytes + 0x28) & ~8u);
    uint32_t mode = read_word(env.bytes + 0x0c);
    if (mode < 1 || mode > 3) {
        uint8_t *raw_work = (uint8_t *)work;
        write_word(raw_work + 0x5e4, read_word(raw_work + 0x5e4) | 0x40000000u);
        *frontier = 0x31afa0;
        *relative = 0;
        if (gh_scenario_stop_input_0031afa0(work)) return GH_EVENT_CALLEE;
        write_word(raw_work + 0x5d8, read_word(raw_work + 0x5d8) & ~0x10u);
    }
    *frontier = 0;
    return GH_EVENT_OK;
}

gh_game_event_status gh_event_env_extra_move_00006ad8(
    gh_event_components *components, uint32_t *frontier)
{
    if (!frontier) return GH_EVENT_ARGUMENT;
    *frontier = 0x6ad8;
    gh_event_component_view env;
    gh_game_event_status status = component(components, 0, 0xd200, 0x44, &env);
    if (status) return status;
    if (read_word(env.bytes + 0x30)) return GH_EVENT_UNSUPPORTED;
    *frontier = 0;
    return GH_EVENT_OK;
}

gh_game_event_status gh_event_se_init_absent_00009a88(
    gh_event_components *components, const gh_event_data_prefix *data, uint32_t *frontier)
{
    if (!frontier) return GH_EVENT_ARGUMENT;
    *frontier = 0x9a88;
    if (!data || data->components != components) return GH_EVENT_ARGUMENT;
    gh_event_component_view sound;
    gh_game_event_status status = component(components, 8, 0xd9f0, 0x18, &sound);
    if (status) return status;
    gh_event_sequence_view sequence;
    status = gh_event_sequence_at(data, 8, &sequence);
    if (status) return status;
    if (sequence.count || read_word(sound.bytes + 0x14)) {
        *frontier = 0xb388;
        return GH_EVENT_UNSUPPORTED;
    }
    memcpy(sound.bytes, "SE", 3);
    *frontier = 0;
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

static unsigned fade_half(const uint8_t *p)
{
    return p[0] | (unsigned)p[1] << 8;
}

static gh_game_event_status fade_track(const gh_event_data_prefix *data,
    gh_event_component_view *view, gh_event_sequence_view *track)
{
    gh_game_event_status status = component((gh_event_components *)data->components, 13, 0xd248, 0x1c, view);
    if (status) return status;
    status = gh_event_sequence_at(data, 13, track);
    if (status) return status;
    if (data->components->event_no != 2 || data->camera_count != 28 ||
        !track->records || track->count != 14 || track->stride != 0x10 ||
        track->bytes != 0xe0 || read_word(view->bytes + 0x10) ||
        read_word(view->bytes + 0x14) != track->count)
        return GH_EVENT_UNSUPPORTED;
    return GH_EVENT_OK;
}

gh_game_event_status gh_event_fade_init_00006c70(gh_event_components *components,
    const gh_event_data_prefix *data, uint32_t *frontier)
{
    if (frontier) *frontier = 0x297f80;
    if (!components || !data || data->components != components) return GH_EVENT_ARGUMENT;
    if (components->event_no == 1 || components->event_no == 4 || components->event_no == 5 ||
        components->event_no == 6 || components->event_no == 7 || components->event_no == 8 || components->event_no == 0x1a || components->event_no == 0x1b)
        return gh_event_fade_init_absent_00006c70(components, data->data, data->bytes, frontier);

    gh_event_component_view view;
    gh_event_sequence_view track;
    gh_game_event_status status = fade_track(data, &view, &track);
    if (status) return status;
    for (unsigned i = 0; i < track.count; ++i) {
        const uint8_t *row = track.records + i * track.stride;
        uint16_t length;
        gh_event_camera_cut cut;
        if (fade_half(row) != 0xffff || row[2] || row[4] || row[5] ||
            read_word(row + 8) != 1 || gh_event_camera_cut_at(data, row[3], &cut) || !cut.primary ||
            gh_event_camera_cut_length_00004e60(data, row[3], &length) || fade_half(row + 6) >= length)
            return GH_EVENT_UNSUPPORTED;
    }
    memcpy(view.bytes, "FADE", sizeof("FADE"));
    view.bytes[0x18] = 0;
    if (frontier) *frontier = 0;
    return GH_EVENT_OK;
}

static gh_game_event_status fade_color(const gh_event_data_prefix *data,
    const uint8_t *previous, const uint8_t *next, unsigned cut, float frame, uint32_t *color)
{
    if (!next || previous == next) {
        *color = read_word(previous + 0x0c);
        return GH_EVENT_OK;
    }

    unsigned start[28] = {0};
    for (unsigned i = 0; i < next[3]; ++i) {
        gh_event_camera_cut source;
        gh_game_event_status status = gh_event_camera_cut_at(data, i, &source);
        if (status) return status;
        start[i + 1] = start[i] + (source.primary ? fade_half(source.primary + 4) : 0);
    }
    const unsigned low = start[previous[3]] + fade_half(previous + 6);
    const unsigned high = start[next[3]] + fade_half(next + 6);
    if (low >= high) return GH_EVENT_FORMAT;

    fenv_t saved;
    if (feholdexcept(&saved)) return GH_EVENT_CALLEE;
    if (fesetround(FE_TONEAREST)) {
        (void)fesetenv(&saved);
        return GH_EVENT_CALLEE;
    }
    const float time = (float)start[cut] + frame;
    const float fraction = 1.0f - ((float)high - time) / (float)(high - low);
    uint32_t rgba = 0;
    if (fraction >= 0 && fraction <= 1) {
        for (unsigned i = 0; i < 4; ++i) {
            const unsigned a = previous[0x0c + i], b = next[0x0c + i];
            const float n = a == b ? (float)a : (float)a * (1.0f - fraction) + (float)b * fraction;
            rgba |= (uint32_t)(uint8_t)n << (8 * i);
        }
    }
    if (fesetenv(&saved)) return GH_EVENT_CALLEE;
    if (!(fraction >= 0 && fraction <= 1)) return GH_EVENT_FORMAT;
    *color = rgba;
    return GH_EVENT_OK;
}

gh_game_event_status gh_event_fade_move_00006ca0(gh_event_components *components,
    const gh_event_data_prefix *data, gh_screen_fade *fade, const gh_display_state *display,
    const gh_boot_work *work, uint32_t *frontier, int *relative)
{
    if (!frontier || !relative) return GH_EVENT_ARGUMENT;
    *frontier = 0x6ca0;
    *relative = 1;
    if (!components || !data || data->components != components) return GH_EVENT_ARGUMENT;
    if (components->event_no == 1 || components->event_no == 4 || components->event_no == 5 ||
        components->event_no == 6 || components->event_no == 7 || components->event_no == 8 || components->event_no == 0x1a || components->event_no == 0x1b) {
        *relative = 0;
        return gh_event_fade_move_absent_00006ca0(components, fade, frontier);
    }
    gh_event_component_view view, env, camera;
    gh_event_sequence_view track;
    gh_game_event_status status = fade_track(data, &view, &track);
    if (status) return status;
    if (memcmp(view.bytes, "FADE", sizeof("FADE"))) return GH_EVENT_ARGUMENT;
    status = component(components, 0, 0xd200, 0x44, &env);
    if (status) return status;
    status = component(components, 1, 0xd1c8, 0x18, &camera);
    if (status) return status;

    const unsigned cut = fade_half(camera.bytes + 6);
    float frame;
    memcpy(&frame, camera.bytes + 0x0c, sizeof(frame));
    if (camera.bytes[4] || cut >= data->camera_count || !isfinite(frame) || frame < 0)
        return GH_EVENT_UNSUPPORTED;

    const uint8_t *previous = NULL, *next = NULL;
    unsigned previous_order = 0, next_order = UINT_MAX;
    for (unsigned i = 0; i < track.count; ++i) {
        const uint8_t *row = track.records + i * track.stride;
        const unsigned key_cut = row[3], key_frame = fade_half(row + 6);
        const unsigned order = key_cut << 16 | key_frame;
        if ((key_cut < cut || (key_cut == cut && (float)key_frame <= frame)) &&
            (!previous || order >= previous_order)) {
            previous = row;
            previous_order = order;
        }
        if ((key_cut > cut || (key_cut == cut && (float)key_frame >= frame)) &&
            order <= next_order) {
            next = row;
            next_order = order;
        }
    }

    uint32_t color = 0;
    if (previous) {
        if (read_word(env.bytes + 0x28) & 0x20u) {
            *frontier = 0;
            return GH_EVENT_OK;
        }
        *frontier = 0x6d60;
        status = fade_color(data, previous, next, cut, frame, &color);
        if (status) return status;
    }
    if (!(color >> 24)) {
        if (view.bytes[0x18]) {
            if (!fade) return GH_EVENT_ARGUMENT;
            gh_screen_fade_kill_003076e0(fade);
            view.bytes[0x18] = 0;
        }
    } else {
        *frontier = 0x307208;
        *relative = 0;
        if (!fade || !display || !display->initialized || !work) return GH_EVENT_ARGUMENT;
        const uint8_t *raw = (const uint8_t *)work;
        const gh_screen_fade_context context = {display->screen_width, display->gs_y_origin,
            read_word(raw + 0x564), read_word(raw + 0x5e0)};
        if (gh_screen_fade_start_00307208(fade, &context, 0, 0x1a, 0, color, color, 0))
            return GH_EVENT_CALLEE;
        gh_screen_fade_set_ot_00307760(fade, 6, 9);
        view.bytes[0x18] = 1;
    }
    *frontier = 0;
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
    if (callback == 0x298060 && components->event_no == 1 &&
        config->record_address == 0x3c055c) {
        write_word(env.bytes + 0x34, 0x298088);
    } else if (callback == 0x2980e8 && components->event_no == 2 &&
               config->record_address == 0x3c0568) {
        write_word(env.bytes + 0x34, 0x298110);
    } else {
        return GH_EVENT_UNSUPPORTED;
    }
    return GH_EVENT_OK;
}

gh_game_event_status gh_event_env_extra_move_object_00006b08(
    gh_event_components *components, gh_actor_model *model,
    gh_core_save_view save, uint32_t *frontier)
{
    if (!frontier) return GH_EVENT_ARGUMENT;
    *frontier = 0x298088;
    gh_event_component_view env;
    gh_game_event_status status = component(components, 0, 0xd200, 0x44, &env);
    if (status) return status;
    uint32_t callback = read_word(env.bytes + 0x34);
    if (callback) {
        const int ev02 = callback == 0x298110 && components->event_no == 2;
        if (!ev02 && (callback != 0x298088 || components->event_no != 1))
            return GH_EVENT_UNSUPPORTED;
        if (model) {
            if (!model->root.bytes || model->root.size < 0x300) return GH_EVENT_BOUNDS;
            uint8_t *bytes = model->root.bytes;
            uint32_t kind = (uint32_t)bytes[0x2fe] | (uint32_t)bytes[0x2ff] << 8;
            if (ev02) {
                if ((int32_t)read_word(env.bytes + 0x10) >= 16 && (kind == 0x605 || kind == 0x607)) {
                    *frontier = 0x14a338;
                    if (!model->ready || model->native_identity != model) return GH_EVENT_ARGUMENT;
                    static const uint8_t hand_closed[8] = "RH_00", hand_open[8] = "RH_01";
                    int found;
                    if (gh_actor_model_set_mesh_visible_0014a338(model, hand_closed, 8, 0, &found) ||
                        (kind == 0x605 && gh_actor_model_set_mesh_visible_0014a338(model, hand_open, 8, 1, &found)))
                        return GH_EVENT_CALLEE;
                }
            } else {
                if (save.bytes && save.size <= 0xae) return GH_EVENT_BOUNDS;
                if (kind == 0x667 && save.bytes && save.bytes[0xae])
                    write_word(bytes + 0x250, read_word(bytes + 0x250) | 2);
            }
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
            int ignored = 0;
            gh_game_event_status status = visit_component(play, b, 0x296ef0, &ignored, frontier);
            if (status) return status;
            event->fields.unknown07 = 0;
            event->fields.state = 4;
            event->fields.phase = 0;
            event->fields.unknown06 = 0;
            play->quit_pending = 0;
            *original_done = 1;
        }
    }
    *frontier = 0;
    return GH_EVENT_OK;
}
