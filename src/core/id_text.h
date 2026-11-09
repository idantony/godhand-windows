#ifndef GH_ID_TEXT_H
#define GH_ID_TEXT_H
#include "id_jobs.h"
#include "packed_message.h"
#include "font_draw.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct {int32_t language,screen_width,screen_height,gs_y_origin;gh_id_view palette;} gh_id_text_context;
typedef struct {uint16_t code;gh_font_quad quad;float x,y;} gh_id_text_glyph;
typedef struct {size_t glyph_count,line_count;float width,height;uint16_t opcode;uint32_t frontier_address;size_t source_offset;} gh_id_text_result;
/* Borrow actual50-word original style table3C23B0 from user's ELF32LE LOAD
 * segment. Does not publish/copy original palette constants into source. */
gh_id_error gh_id_text_palette_from_elf(const void *,size_t,gh_id_view *);
/* OriginalTransText2AB0B8 -> setMessage -> draw/measure. Supported plain
 * non-JP/no-ruby glyphs, A001 newline,A301 non-drawing command,8001 end.
 * Other icons/control/dynamic-number opcodes report exact frontier; never skip
 * them to claim the message drew. Output is original order glyph primitives,
 * not fullMC Trans/render success. Source buffers/font must outlive result. */
gh_id_error gh_id_text_prepare(const gh_id_base *,const gh_id_entry *,const gh_menu_state *,
 const gh_packed_message_state *,const gh_font_state *,const gh_id_text_context *,
 gh_id_text_glyph *,size_t capacity,gh_id_text_result *);
#ifdef __cplusplus
}
#endif
#endif
