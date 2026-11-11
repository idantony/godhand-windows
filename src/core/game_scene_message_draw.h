#ifndef GH_GAME_SCENE_MESSAGE_DRAW_H
#define GH_GAME_SCENE_MESSAGE_DRAW_H

#include "display_state.h"
#include "game_message_step.h"
#include "id_text.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    const gh_font_slot *font;
    gh_id_text_glyph normal, shadow;
    uint32_t bank, depth, font_mode;
} gh_scene_message_glyph_pair;

/* The provider inserts both original font groups atomically. 2AF728 submits
 * normal, then shadow, to the same bank/depth. The shared OT orders them. */
typedef gh_scene_ui_status (*gh_scene_message_emit)(void *,
    const gh_scene_message_glyph_pair *, uint32_t *frontier);

/* 2AF028 -> 2B1C80 for the actual English EV01 single-glyph stream.
 * Palette is the bounded 3C23B0 view from gh_id_text_palette_from_elf. */
gh_scene_ui_status gh_scene_message_render_002af028(gh_game_scene_ui *, const gh_boot_work *,
    const gh_font_state *, const gh_display_state *, gh_id_view palette,
    gh_scene_message_emit, void *context, uint32_t *frontier);

/* Alpha-test coverage for the original 512x448 font draw. The decoded atlas
 * must be the glyph's selected palette parity. A rejected sample writes
 * neither color nor depth. */
gh_scene_ui_status gh_scene_message_glyph_discarded(const gh_id_text_glyph *,
    const uint8_t *atlas_rgba, size_t atlas_bytes, int *discarded);

/* Original font sprite on a 512x448 CT32 surface using the native separable
 * 4-bit linear sampler. Optional depth is the writable main Z16S plane.
 * The original depth test always passes, so existing depth is not sampled. */
gh_scene_ui_status gh_scene_message_glyph_blit(const gh_id_text_glyph *,
    const uint8_t *atlas_rgba, size_t atlas_bytes, uint8_t *rgba, size_t rgba_bytes,
    uint32_t *depth, size_t depth_count);

#ifdef __cplusplus
}
#endif
#endif
