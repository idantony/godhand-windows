#include "game_scene_message_draw.h"

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

static int32_t signed_word(const uint8_t *p)
{
    int32_t value;
    memcpy(&value, p, sizeof(value));
    return value;
}

gh_scene_ui_status gh_scene_message_render_002af028(gh_game_scene_ui *ui,
    const gh_boot_work *work, const gh_font_state *font, const gh_display_state *display,
    gh_id_view palette, gh_scene_message_emit emit, void *context, uint32_t *frontier)
{
    if (!frontier) return GH_SCENE_UI_ARGUMENT;
    gh_scene_ui_status status = gh_scene_message_prepare_draw_002af028(ui, work, frontier);
    if (status != GH_SCENE_UI_UNSUPPORTED || *frontier != 0x2b1c80) return status;

    const unsigned slot = (unsigned)ui->active_head;
    const gh_scene_message_block *block = ui->blocks + slot;
    const gh_scene_message_sources *source = ui->sources + slot;
    const uint8_t *p = block->payload;
    if (!work->language || block->next != -1 || p[0xa4 - 0xc] || !source->bound ||
        source->ruby.bytes || source->ruby.size || !source->primary.bytes ||
        source->primary.size < 6 || source->cursor_7c || source->cursor_8c > 4)
        return GH_SCENE_UI_UNSUPPORTED;
    const uint8_t *text = source->primary.bytes;
    const uint16_t code = half(text);
    if (code < 14 || code >= 0x1000 || half(text + 2) != 0xa301 || half(text + 4) != 0x8001)
        return GH_SCENE_UI_UNSUPPORTED;
    if (signed_word(p + 0x84 - 0xc) > signed_word(p + 0x68 - 0xc)) {
        *frontier = 0;
        return GH_SCENE_UI_OK;
    }

    *frontier = 0x2b0480;
    if (!font || !display || !display->initialized || !palette.bytes || palette.size < 84)
        return GH_SCENE_UI_ARGUMENT;
    float original_height;
    memcpy(&original_height, &display->screen_height_bits, sizeof(original_height));
    if (original_height != (float)display->screen_lines) return GH_SCENE_UI_UNSUPPORTED;
    gh_font_glyph glyph;
    *frontier = 0x2a4b20;
    if (gh_font_glyph_at(font, code, &glyph) || glyph.width <= 0 || glyph.baseline_delta <= 0)
        return GH_SCENE_UI_UNSUPPORTED;
    const gh_font_slot *atlas = font->slots;
    /* Original English 2AF688/2AF698, with no scaling command before
     * the first glyph. 2B0700 adds and subtracts its trailing spacing. */
    const volatile float advance = ((float)glyph.width * 0.9f) * 0.9f;
    const volatile float spacing = 2.0f * 0.9f;
    const volatile float line_width = (spacing + advance) - spacing;
    const volatile float baseline = 12.0f * 1.1f;
    const float line_height = (float)atlas->cell_height * 1.1f + baseline;
    const float x = (float)half(p + 0xae - 0xc) - line_width * 0.5f;
    const float y = ((float)half(p + 0xb0 - 0xc) - line_height * 0.5f) + baseline;
    gh_font_draw_state draw = {.scale_x = 0.9f * 0.9f, .scale_y = 1.1f, .alignment_flags = 9,
        .screen_width = display->screen_width, .screen_height = display->screen_lines,
        .gs_y_origin = display->gs_y_origin, .rgba = word(palette.bytes + 80)};
    gh_scene_message_glyph_pair pair = {.font = atlas, .bank = 6, .depth = 2, .font_mode = 0x21,
        .normal = {.code = code, .x = x, .y = y},
        .shadow = {.code = code, .x = x + 2.0f, .y = y + 2.0f}};
    *frontier = 0x2a53a8;
    if (gh_font_draw_quad(font, &draw, code, pair.normal.x, pair.normal.y, 65535.0f, &pair.normal.quad))
        return GH_SCENE_UI_UNSUPPORTED;
    draw.rgba = 0xff000000u;
    if (gh_font_draw_quad(font, &draw, code, pair.shadow.x, pair.shadow.y, 65535.0f, &pair.shadow.quad))
        return GH_SCENE_UI_UNSUPPORTED;
    *frontier = 0x2a5930;
    if (!emit) return GH_SCENE_UI_UNSUPPORTED;
    uint32_t child = 0;
    status = emit(context, &pair, &child);
    *frontier = status ? (child ? child : 0x2a5930) : 0;
    return status;
}

static unsigned channel(const uint8_t *atlas, unsigned x, unsigned y, unsigned component)
{
    const unsigned value = atlas[4u * ((y & 255u) * 512u + (x & 511u)) + component];
    return component == 3 ? (value == 255 ? 128 : value / 2) : value;
}

static unsigned sample(const uint8_t *atlas, uint32_t u, uint32_t v, unsigned component)
{
    /* TEX1=0x60 selects linear. Shift the fixed16 coordinates by half a
     * texel before extracting the repeat index and native 4-bit weights. */
    u -= 32768u;
    v -= 32768u;
    const unsigned x = u >> 16, y = v >> 16;
    const unsigned fx = (u >> 12) & 15u, fy = (v >> 12) & 15u;
    const unsigned a = (channel(atlas, x, y, component) * (16u - fx) +
                        channel(atlas, x + 1, y, component) * fx) >> 4;
    const unsigned b = (channel(atlas, x, y + 1, component) * (16u - fx) +
                        channel(atlas, x + 1, y + 1, component) * fx) >> 4;
    return (a * (16u - fy) + b * fy) >> 4;
}

static gh_scene_ui_status raster_glyph(const gh_id_text_glyph *glyph,
    const uint8_t *atlas, size_t bytes, uint8_t *rgba, uint32_t *depth, int *discarded)
{
    if (!glyph || !atlas || !discarded) return GH_SCENE_UI_ARGUMENT;
    if (bytes < 512u * 256u * 4u) return GH_SCENE_UI_BOUNDS;
    const gh_font_quad *q = &glyph->quad;
    if (q->slot || q->palette_parity != (glyph->code & 1) ||
        q->x0 >= q->x1 || q->y0 >= q->y1 || q->u0 > q->u1 || q->v0 > q->v1 ||
        q->u1 > 512u * 16u + 8u || q->v1 > 256u * 16u + 8u || q->z != 65535)
        return GH_SCENE_UI_UNSUPPORTED;

    const int left = ((int)q->x0 - 28672 + 15) / 16;
    const int right = ((int)q->x1 - 28672 + 15) / 16;
    const int top = ((int)q->y0 - 29184 + 15) / 16;
    const int bottom = ((int)q->y1 - 29184 + 15) / 16;
    const uint32_t du = (uint32_t)(q->u1 - q->u0) << 12;
    const uint32_t dv = (uint32_t)(q->v1 - q->v0) << 12;
    const unsigned tint_alpha = q->rgba >> 24;
    *discarded = 1;
    for (int y = top < 0 ? 0 : top; y < bottom && y < 448; ++y) {
        const unsigned offset_y = (unsigned)(29184 + y * 16 - q->y0);
        const uint32_t v = ((uint32_t)q->v0 << 12) +
            (uint32_t)((uint64_t)dv * offset_y / (q->y1 - q->y0));
        for (int x = left < 0 ? 0 : left; x < right && x < 512; ++x) {
            const unsigned offset_x = (unsigned)(28672 + x * 16 - q->x0);
            const uint32_t u = ((uint32_t)q->u0 << 12) +
                (uint32_t)((uint64_t)du * offset_x / (q->x1 - q->x0));
            unsigned alpha = (sample(atlas, u, v, 3) * tint_alpha) >> 7;
            if (alpha <= 1) continue; /* TEST3001D: GREATER 1, AFAIL KEEP. */
            *discarded = 0;
            if (!rgba) return GH_SCENE_UI_OK;
            if (alpha > 255) alpha = 255;
            const size_t pixel = (size_t)y * 512u + (unsigned)x;
            uint8_t *destination = rgba + pixel * 4u;
            for (unsigned k = 0; k < 3; ++k) {
                const unsigned tint = (q->rgba >> (8u * k)) & 255u;
                unsigned source = sample(atlas, u, v, k) * tint >> 7;
                if (source > 255) source = 255;
                const int numerator = (int)(destination[k] * 128u) +
                    ((int)source - destination[k]) * (int)alpha;
                destination[k] = numerator <= 0 ? 0 : numerator >= 255 * 128 ? 255 : (uint8_t)(numerator / 128);
            }
            destination[3] = (uint8_t)alpha;
            if (depth) depth[pixel] = q->z;
        }
    }
    return GH_SCENE_UI_OK;
}

gh_scene_ui_status gh_scene_message_glyph_discarded(const gh_id_text_glyph *glyph,
    const uint8_t *atlas, size_t bytes, int *discarded)
{
    return raster_glyph(glyph, atlas, bytes, NULL, NULL, discarded);
}

gh_scene_ui_status gh_scene_message_glyph_blit(const gh_id_text_glyph *glyph,
    const uint8_t *atlas, size_t atlas_bytes, uint8_t *rgba, size_t rgba_bytes,
    uint32_t *depth, size_t depth_count)
{
    if (!rgba || (!depth && depth_count)) return GH_SCENE_UI_ARGUMENT;
    if (rgba_bytes < 512u * 448u * 4u || (depth && depth_count < 512u * 448u))
        return GH_SCENE_UI_BOUNDS;
    int discarded;
    return raster_glyph(glyph, atlas, atlas_bytes, rgba, depth, &discarded);
}
