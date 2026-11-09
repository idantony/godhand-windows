#ifndef GH_MC_RENDERER_H
#define GH_MC_RENDERER_H
#include "startup_host.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct gh_mc_renderer gh_mc_renderer;
typedef struct {
    const gh_font_state *font;
    const gh_menu_state *menu;
    const gh_id_state *manager;
    const gh_id_base *base;
    const gh_packed_message_state *messages;
} gh_id_render_view;
typedef struct {
    gh_id_error id_error;
    uint32_t consumer,frontier;
    uint16_t opcode;
    size_t glyphs,jobs;
    unsigned gpu_error;
} gh_mc_render_status;
/* Own a copy of the actual ELF style palette. Original font/ID views are
 * borrowed only during draw. Cached texture resources reset explicitly when
 * their owning menu/ID resource epoch ends. No replacement text or widgets. */
gh_id_error gh_mc_renderer_create(const void *elf,size_t elf_bytes,int32_t language,gh_mc_renderer **);
void gh_mc_renderer_destroy(gh_mc_renderer *);
void gh_mc_renderer_reset_resources(gh_mc_renderer *);
/* Optional explicit native reference for the separately recovered rotated
 * HUD subset. Default is strict and keeps unknown quad branches unsupported. */
gh_id_error gh_mc_renderer_enable_reference_polygons(gh_mc_renderer *,int enabled);
/* Current verified boot context: 512x448 raw GS RGBA, black/alpha128 clear.
 * Caller begins before the next actual MC frame and presents only after the
 * host reports completed original Sleep. This is not a game-scene renderer. */
void gh_mc_renderer_begin(gh_mc_renderer *);
/* Shared original ID consumers for Title and other recovered UI owners.
 * Resource/font/message views must be the actual caller's canonical views.
 * The MC convenience wrapper below selects the live MC owner from a snapshot. */
gh_id_error gh_mc_renderer_draw_view(gh_mc_renderer *,const gh_id_render_view *,const gh_id_draw_job *);
gh_id_error gh_mc_renderer_draw(gh_mc_renderer *,const gh_startup_snapshot *,const gh_id_draw_job *);
const unsigned char *gh_mc_renderer_pixels(const gh_mc_renderer *,size_t *bytes);
void gh_mc_renderer_inspect(const gh_mc_renderer *,gh_mc_render_status *);
#ifdef __cplusplus
}
#endif
#endif
