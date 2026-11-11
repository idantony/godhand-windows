#ifndef GH_MC_RENDERER_H
#define GH_MC_RENDERER_H
#include "startup_host.h"
#include "ui_fade_renderer.h"
#include "../core/cockpit_fan.h"
#include "../core/game_children.h"
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
typedef struct {uint32_t bank;int32_t depth;size_t index;} gh_ui_ot_tag;
/* Bounded native UI packet groups: capture entry/base scalar values now,
 * borrow source/font/menu owners until the SAME frame's flush. Text is one
 * group with its internal glyph order intact. Does not claim GPU completion.
 * Begin expires old groups; flush requires a complete permutation produced
 * by the recovered original OT traversal, never a visual preference.
 * A genuine original NULL-texture return emits no packet: returnsOK with
 * tag.index=SIZE_MAX; caller MUST NOT append that tag to its OT groups. */
gh_id_error gh_mc_renderer_queue_view(gh_mc_renderer *,const gh_id_render_view *,const gh_id_draw_job *,gh_ui_ot_tag *);
/* Each captures ONE original OT group. Fan owns primitive scalar arrays and
 * borrows its immutable texture bytes until same-frame flush. Fade copies its
 * complete scalar/context data. Only visible Fade draws emit a packet group. */
gh_id_error gh_mc_renderer_queue_fan(gh_mc_renderer *,const gh_cockpit_fan *,gh_ui_ot_tag *);
/* Direct Radar 144368 -> 15BCF0 mode1 group, TEST3001D, Z0. */
gh_id_error gh_mc_renderer_queue_radar(gh_mc_renderer *,const gh_cockpit_radar_quad *,gh_ui_ot_tag *);
gh_id_error gh_mc_renderer_queue_fade(gh_mc_renderer *,const gh_fade_draw *,const gh_ui_fade_context *,gh_ui_ot_tag *);
/* Actual160D50 cover packet, TEST3001D. Alpha0/1 is a genuine GREATER1
 * discard (including Z), retained as ONE packet group, never NULL sentinel.
 * Positive alpha currently remains explicit15B880 frontier. */
gh_id_error gh_mc_renderer_queue_cover(gh_mc_renderer *,const gh_child_cover_quad *,gh_ui_ot_tag *);
/* Original glyph and shadow groups. Publish two tags or none. Groups whose
 * samples all fail TEST have no color or depth writes. */
gh_id_error gh_mc_renderer_queue_message(gh_mc_renderer *,const gh_scene_message_glyph_pair *,gh_ui_ot_tag tags[2]);
/* Query the sorted groups before reading the world depth surface. Positive
 * glyphs require a tracked depth prefix; discard-only groups need no readback.
 * Order is a complete permutation, as required by the flush functions. */
gh_id_error gh_mc_renderer_queued_message_depth(gh_mc_renderer *,const size_t *order,size_t count,int *required);
/* Plural original packet groups: numeric digits each get their own tag,
 * genuine NULL texture returns count0, font/ordinary ID returns count1.
 * Capacity/memory failures leave queue, tags and count unchanged. */
gh_id_error gh_mc_renderer_queue_view_groups(gh_mc_renderer *,const gh_id_render_view *,
 const gh_id_draw_job *,gh_ui_ot_tag *,size_t capacity,size_t *count);
/* Door map uses the same queue with its typed original chapter message.
 * Text expansion is captured at submission and ordinary jobs share the
 * existing group path. */
gh_id_error gh_mc_renderer_queue_view_groups_with_chapter(gh_mc_renderer *,const gh_id_render_view *,
 const gh_id_draw_job *,uint16_t chapter_message,gh_ui_ot_tag *,size_t capacity,size_t *count);
/* CT32 opaque clear before the ordered groups: RGB is Work+60C low24,
 * alpha0, explicit native FBA0 reference. This does not itself implement the
 * outer-loop previous-parity submission or recover all earlier GS state. */
gh_id_error gh_mc_renderer_flush_queued(gh_mc_renderer *,const size_t *order,size_t count,uint32_t clear_rgb);
/* Same ordered UI consumer over a completed native world color surface.
 * Caller must prove every world OT group precedes these UI groups. Current
 * recovered UI TEST uses ZTST ALWAYS; later UI/world interleaving needs a
 * common depth surface and remains outside this bounded composition API. */
gh_id_error gh_mc_renderer_flush_queued_on_background(gh_mc_renderer *,const size_t *order,size_t count,
 const uint8_t *rgba,size_t bytes);
/* Positive Message updates and a terminal Fade can use this same world OT depth.
 * Intervening UI with untracked depth mutations invalidates the prefix. */
gh_id_error gh_mc_renderer_queue_view_groups_at_z(gh_mc_renderer *,const gh_id_render_view *,const gh_id_draw_job *,
 gh_ui_ot_tag *,size_t capacity,size_t *count,uint16_t z);
gh_id_error gh_mc_renderer_flush_queued_on_background_with_depth(gh_mc_renderer *,const size_t *order,size_t count,
 const uint8_t *rgba,size_t bytes,const uint32_t *z,size_t z_count);
gh_id_error gh_mc_renderer_draw(gh_mc_renderer *,const gh_startup_snapshot *,const gh_id_draw_job *);
const unsigned char *gh_mc_renderer_pixels(const gh_mc_renderer *,size_t *bytes);
void gh_mc_renderer_inspect(const gh_mc_renderer *,gh_mc_render_status *);
#ifdef __cplusplus
}
#endif
#endif
