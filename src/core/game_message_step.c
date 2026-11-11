/* Actual patched 2AEFA8 and 2AF028 prefixes. MIT LICENSE.upstream. */
#include "game_message_step.h"

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

static uint64_t doubleword(const uint8_t *p)
{
    return word(p) | (uint64_t)word(p + 4) << 32;
}

static int32_t signed_word(const uint8_t *p)
{
    int32_t value;
    memcpy(&value, p, sizeof(value));
    return value;
}

static void put(uint8_t *p, uint32_t value)
{
    memcpy(p, &value, sizeof(value));
}

static void put_half(uint8_t *p, uint16_t value)
{
    p[0] = (uint8_t)value;
    p[1] = (uint8_t)(value >> 8);
}

static void put_float(uint8_t *p, float value)
{
    memcpy(p, &value, sizeof(value));
}

static gh_scene_ui_status active_slots(const gh_game_scene_ui *ui, unsigned slots[16],
    unsigned *count)
{
    if (!ui->ready || !ui->root) return GH_SCENE_UI_ARGUMENT;
    uint32_t seen = 0;
    *count = 0;
    for (int32_t i = ui->active_head; i != -1; i = ui->blocks[i].next) {
        if (i < 0 || i >= 16 || (seen & (1u << i))) return GH_SCENE_UI_BOUNDS;
        seen |= 1u << i;
        slots[(*count)++] = (unsigned)i;
    }
    return GH_SCENE_UI_OK;
}

static gh_scene_ui_status clear_event_cut(gh_scene_message_sources *source,
    gh_event_timing *timing, uint32_t *frontier)
{
    *frontier = 0x297bf8;
    if (!timing || !timing->components || !timing->allocation.pointer)
        return GH_SCENE_UI_UNSUPPORTED;
    const gh_event_components *components = timing->components;
    const gh_event_component_view env = components->components[0];
    if (!components->constructed || components->event_no != timing->event_number ||
        !env.bytes || env.original_offset != 0xd200 || env.size != 0x44 ||
        word(env.bytes + 8) != timing->count) return GH_SCENE_UI_ARGUMENT;
    const uint32_t cut = word(env.bytes + 0x10);
    if (cut >= timing->count) return GH_SCENE_UI_BOUNDS;
    gh_event_cut_timing *rows = timing->allocation.pointer;
    rows[cut].kind = 0;
    source->confirm_pending = 0;
    return GH_SCENE_UI_OK;
}

static int native_confirm(const gh_boot_work *work)
{
    const uint8_t *raw = (const uint8_t *)work;
    /* 31A808 publishes the original language-selected confirm mask. */
    const uint64_t mask = work->language ? UINT64_C(0x10) : UINT64_C(0x100000000);
    return (doubleword(raw + 0x178) & mask) || (word(raw + 0x1a0) & 0x10000000u);
}

static int code_at(const gh_scene_message_sources *source, size_t offset, uint16_t *code)
{
    if (!source->primary.bytes || offset > source->primary.size ||
        source->primary.size - offset < 2) return 0;
    *code = half(source->primary.bytes + offset);
    return 1;
}

static gh_scene_ui_status advance_english(uint8_t *p, gh_scene_message_sources *source,
    const gh_game_scene_ui *ui, const gh_boot_work *work, uint32_t *frontier)
{
    *frontier = 0x2b3240;
    if (!work->language || p[0xa4 - 0xc] || word(p + 0x1c - 0xc) ||
        word(p + 0x6c - 0xc) != 0x3f800000u || p[1] > 1)
        return GH_SCENE_UI_UNSUPPORTED;
    uint16_t current;
    *frontier = 0x2b3338;
    if (!code_at(source, source->cursor_8c, &current)) return GH_SCENE_UI_BOUNDS;
    if (!(current & 0x8000u)) {
        uint16_t next;
        if (!code_at(source, source->cursor_8c + 2, &next)) return GH_SCENE_UI_BOUNDS;
        if (next != 0xa301) return GH_SCENE_UI_UNSUPPORTED;
    } else if (current != 0xa301 && current != 0x8001) return GH_SCENE_UI_UNSUPPORTED;
    if (current == 0x8001 && (word(p + 0xb4 - 0xc) || word(p + 0x40 - 0xc))) {
        *frontier = 0x2b4428;
        return GH_SCENE_UI_UNSUPPORTED;
    }

    p[1] = 1; /* 2B45D0 and 2B45D8 are empty. */
    uint32_t flags = word(p + 0x14 - 0xc);
    if (word((const uint8_t *)work + 0x1a0) & 0x10000000u) flags |= 1u;
    put(p + 0x14 - 0xc, flags);
    int32_t increment = ui->root->timer;
    if (!word(p + 0x34 - 0xc) && ((flags & 1u) || native_confirm(work))) increment *= 10;
    uint8_t *budget = p + 0x68 - 0xc, *time = p + 0x90 - 0xc;
    put(budget, word(budget) + (uint32_t)increment);
    if (signed_word(budget) < signed_word(time)) return GH_SCENE_UI_OK;
    if (!(current & 0x8000u)) {
        /* No actor voice is present, so 2B33D8 returns before the glyph
         * sound. 2B4768 advances one glyph and its original 30 timer units. */
        source->cursor_8c += 2;
        put(time, word(time) + 30);
        if (signed_word(budget) < signed_word(time)) return GH_SCENE_UI_OK;
    }
    /* A301 -> 2B3AD0 and 8001 -> 2B3B70 both return 2 without consuming
     * the command. The latter enters the original six-tick close state. */
    p[0] = current == 0x8001 ? 6 : 3;
    p[1] = p[2] = p[3] = 0;
    put(budget, word(time));
    return GH_SCENE_UI_OK;
}

static gh_scene_ui_status close_window(uint8_t *p, const gh_scene_message_sources *source,
    uint32_t *frontier)
{
    *frontier = 0x2b4188;
    if (p[1] > 1) return GH_SCENE_UI_UNSUPPORTED;
    uint8_t *flags = p + 0x14 - 0xc;
    uint32_t tick = p[1] ? word(p + 0x10 - 0xc) : 0;
    if (tick >= 6) return GH_SCENE_UI_BOUNDS;
    if (tick == 5 && !(word(flags) & 0x40u)) {
        uint16_t code;
        *frontier = 0x2b42f0;
        if (!code_at(source, source->cursor_8c, &code)) return GH_SCENE_UI_BOUNDS;
        if ((code & 0xff00u) != 0x8000) return GH_SCENE_UI_UNSUPPORTED;
    }
    if (!p[1]) {
        put(flags, word(flags) & ~0x10u);
        p[1] = 1;
    }
    ++tick;
    put(p + 0x10 - 0xc, tick);
    const unsigned left = 6 - tick;
    const volatile float fraction = (float)left / 6.0f;
    const volatile float square = (float)(left * left) / 36.0f;
    const float progress = (fraction + fraction) - square;
    put_float(p + 0x98 - 0xc, progress);
    put_float(p + 0x9c - 0xc, progress);
    put_float(p + 0xa0 - 0xc, square);
    if (tick == 6) {
        put(flags, word(flags) & ~8u);
        p[0] = 7;
        p[1] = p[2] = p[3] = 0;
    }
    return GH_SCENE_UI_OK;
}

static gh_scene_ui_status recycle_window(gh_game_scene_ui *ui, unsigned slot, uint32_t *frontier)
{
    *frontier = 0x2aef10;
    gh_scene_message_block *block = ui->blocks + slot;
    uint8_t *p = block->payload;
    const int32_t previous = block->previous, next = block->next, free_slot = ui->free_head;
    if (previous < -1 || previous >= 16 || next < -1 || next >= 16 ||
        free_slot < -1 || free_slot >= 16 || free_slot == (int32_t)slot ||
        (previous == -1 ? ui->active_head != (int32_t)slot : ui->blocks[previous].next != (int32_t)slot) ||
        (next != -1 && ui->blocks[next].previous != (int32_t)slot)) return GH_SCENE_UI_BOUNDS;
    const uint32_t flags = word(p + 0x14 - 0xc);
    if (flags & 0x1000u) {
        *frontier = 0x2b2348;
        if (word(p + 0x1c - 0xc) || word(p + 0xb4 - 0xc) || word(p + 0x40 - 0xc))
            return GH_SCENE_UI_UNSUPPORTED;
        put(p + 0x20 - 0xc, UINT32_MAX);
        ui->root->state = 0;
        put(p + 0x14 - 0xc, flags & ~0x1000u);
    }
    if (previous != -1) ui->blocks[previous].next = next;
    else ui->active_head = next;
    if (next != -1) ui->blocks[next].previous = previous;
    if (free_slot != -1) ui->blocks[free_slot].previous = (int32_t)slot;
    block->flags = block->mode = 0;
    block->previous = -1;
    block->next = free_slot;
    ui->free_head = (int32_t)slot;
    ui->sources[slot] = (gh_scene_message_sources){0};
    return GH_SCENE_UI_OK;
}

static gh_scene_ui_status wait_english(uint8_t *p, gh_scene_message_sources *source,
    gh_game_scene_ui *ui, const gh_boot_work *work, gh_event_timing *timing, uint32_t *frontier)
{
    *frontier = 0x2b3d70;
    uint16_t command;
    if (!code_at(source, source->cursor_8c, &command)) return GH_SCENE_UI_BOUNDS;
    if (command != 0xa301 || p[0xa4 - 0xc] || p[1] > 1) return GH_SCENE_UI_UNSUPPORTED;
    uint32_t flags = word(p + 0x14 - 0xc);
    if (!p[1]) {
        if (half(p + 0x2e - 0xc)) return GH_SCENE_UI_OK;
        p[0xb2 - 0xc] = 0;
        flags |= 0x80u;
        put(p + 0x14 - 0xc, flags);
        p[1] = 1;
    }
    ++p[0xb2 - 0xc];
    ++ui->wait_frames_3c2550;
    const uint32_t system = word((const uint8_t *)work + 0x5e4);
    if (system & 0x10u) return GH_SCENE_UI_OK;
    if (word(p + 0x34 - 0xc) == 1) {
        const uint32_t waited = ui->wait_frames_3c2550;
        if (waited < 90 || waited > INT32_MAX) return GH_SCENE_UI_OK;
    } else if (!native_confirm(work)) return GH_SCENE_UI_OK;

    ui->wait_frames_3c2550 = 0;
    put(p + 0x14 - 0xc, flags & ~0x81u);
    source->cursor_8c += 2; /* A301 is a one-halfword, zero-cost command. */
    p[0] = 2;
    p[1] = p[2] = p[3] = 0;
    if (system & 0x20000000u) {
        source->confirm_pending = 1;
        return clear_event_cut(source, timing, frontier);
    }
    return GH_SCENE_UI_OK;
}

static gh_scene_ui_status opening(gh_scene_message_block *block, gh_scene_message_sources *source,
    gh_game_scene_ui *ui, const gh_boot_work *work, gh_event_timing *timing,
    const unsigned *slots, unsigned count, uint32_t *frontier)
{
    uint8_t *p = block->payload;
    const unsigned state = p[0], phase = p[1];
    *frontier = 0x2b2438;
    if (!source->bound) return GH_SCENE_UI_UNSUPPORTED;
    if (source->confirm_pending || state == 2 || state == 3 || state == 6) {
        gh_scene_ui_status status = source->confirm_pending ? clear_event_cut(source, timing, frontier) :
            state == 2 ? advance_english(p, source, ui, work, frontier) :
            state == 3 ? wait_english(p, source, ui, work, timing, frontier) :
            close_window(p, source, frontier);
        if (status) return status;
        goto finish;
    }
    if (state == 7) goto finish;
    if (state > 1) return GH_SCENE_UI_UNSUPPORTED;
    if (p[0xa4 - 0xc] || word(p + 0x18 - 0xc) || word(p + 0x1c - 0xc)) {
        *frontier = state == 0 ? 0x2b2e80 : 0x2b3078;
        return GH_SCENE_UI_UNSUPPORTED;
    }
    if (state == 0) {
        *frontier = 0x2b2588;
        if (phase > 1 || !source->primary.bytes || source->cursor_8c > source->primary.size ||
            source->primary.size - source->cursor_8c < 2) return GH_SCENE_UI_BOUNDS;
        unsigned same_group = 0;
        for (unsigned i = 0; i < count; ++i)
            same_group += ui->blocks[slots[i]].payload[0x2c - 0xc] == p[0x2c - 0xc];
        p[1] = 1; /* 2B2580 is the original empty phase-0 initializer. */
        if (same_group >= 2) goto finish;
        if (half(source->primary.bytes + source->cursor_8c) & 0x8000u) {
            *frontier = 0x2b2638;
            return GH_SCENE_UI_UNSUPPORTED;
        }
        put_half(p + 0x88 - 0xc, UINT16_MAX);
        ++p[0x30 - 0xc];
        put(p + 0x84 - 0xc, word(p + 0x90 - 0xc));
        source->cursor_7c = source->cursor_8c;
        p[0] = 1;
        p[1] = p[2] = p[3] = 0;
    }
    if (!p[1]) {
        /* 2B3078 mode 0 sets phase 2, then runs one original 2B3190
         * animation tick before the next call can take phase 2 directly. */
        put(p + 0x10 - 0xc, 1);
        put(p + 0x14 - 0xc, word(p + 0x14 - 0xc) | 8u);
        const volatile float fraction = 1.0f / 6.0f, square = 1.0f / 36.0f;
        const float progress = (fraction + fraction) - square;
        put_float(p + 0x98 - 0xc, progress);
        put_float(p + 0x9c - 0xc, progress);
        put_float(p + 0xa0 - 0xc, square);
        p[1] = 2;
    } else if (p[1] == 2) {
        put(p + 0x14 - 0xc, word(p + 0x14 - 0xc) | 0x10u);
        p[0] = 2;
        p[1] = p[2] = p[3] = 0;
    } else {
        *frontier = 0x2b3190;
        return GH_SCENE_UI_UNSUPPORTED;
    }

finish:
    if (half(p + 0x2e - 0xc))
        put_half(p + 0x2e - 0xc, (uint16_t)(half(p + 0x2e - 0xc) - 1u));
    if (word(p + 0xb4 - 0xc) == 1) put(p + 0xb8 - 0xc, 1);
    return GH_SCENE_UI_OK;
}

gh_scene_ui_status gh_scene_message_update_with_timing_002aefa8(gh_game_scene_ui *ui,
    const gh_boot_work *work, gh_event_timing *timing, uint32_t *frontier)
{
    if (frontier) *frontier = 0;
    if (!ui || !work) return GH_SCENE_UI_ARGUMENT;
    if (word((const uint8_t *)work + 0x5d8) & 0x80000u) return GH_SCENE_UI_OK;
    unsigned slots[16], count;
    gh_scene_ui_status status = active_slots(ui, slots, &count);
    if (status) return status;
    uint32_t child = 0;
    for (unsigned i = 0; i < count;) {
        unsigned slot = slots[i];
        status = opening(ui->blocks + slot, ui->sources + slot, ui, work, timing, slots, count, &child);
        if (!status && ui->blocks[slot].payload[0] == 7) {
            status = recycle_window(ui, slot, &child);
            if (!status) {
                for (unsigned j = i; j + 1 < count; ++j) slots[j] = slots[j + 1];
                --count;
                continue;
            }
        }
        if (status) {
            if (frontier) *frontier = child;
            return status;
        }
        ++i;
    }
    return GH_SCENE_UI_OK;
}

gh_scene_ui_status gh_scene_message_update_002aefa8(gh_game_scene_ui *ui,
    const gh_boot_work *work, uint32_t *frontier)
{
    return gh_scene_message_update_with_timing_002aefa8(ui, work, NULL, frontier);
}

gh_scene_ui_status gh_scene_message_prepare_draw_002af028(gh_game_scene_ui *ui,
    const gh_boot_work *work, uint32_t *frontier)
{
    if (frontier) *frontier = 0;
    if (!ui || !work) return GH_SCENE_UI_ARGUMENT;
    unsigned slots[16], count;
    gh_scene_ui_status status = active_slots(ui, slots, &count);
    if (status) return status;
    const uint8_t *raw = (const uint8_t *)work;
    if ((word(raw + 0x590) & 2) || (word(raw + 0x5e4) & 0x08000000u)) return GH_SCENE_UI_OK;
    for (unsigned i = 0; i < count; ++i) {
        const unsigned slot = slots[i];
        uint8_t *p = ui->blocks[slot].payload;
        if (!ui->sources[slot].bound || p[0xa4 - 0xc]) {
            if (frontier) *frontier = 0x2b16f0;
            return GH_SCENE_UI_UNSUPPORTED;
        }
        put_half(p + 0xa6 - 0xc, 0x100);
        put_half(p + 0xa8 - 0xc, 0x182);
        put_half(p + 0xaa - 0xc, 0x1e0);
        put_half(p + 0xac - 0xc, 0x88);
        put_half(p + 0xae - 0xc, 0x100);
        put_half(p + 0xb0 - 0xc, 0x182);
        if (word(p + 0x14 - 0xc) & 0x10u) {
            if (frontier) *frontier = 0x2b1c80;
            return GH_SCENE_UI_UNSUPPORTED;
        }
    }
    return GH_SCENE_UI_OK;
}
