#ifndef GH_FONT_STATE_H
#define GH_FONT_STATE_H
#include "../assets/core_bundle.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef enum {GH_FONT_OK,GH_FONT_ARGUMENT,GH_FONT_BOUNDS,GH_FONT_UNSUPPORTED} gh_font_error;
typedef struct {const unsigned char *mst,*msa;size_t mst_size,msa_size;uint16_t width,height,cell_width,cell_height;uint16_t table_count[2];size_t table_offset[2];int bound;} gh_font_slot;
typedef struct {gh_font_slot slots[5];uint32_t draw_word_2a08c;} gh_font_state;
typedef struct {const unsigned char *record,*data;size_t record_offset,data_offset,available;} gh_font_texture_record;
/* Binds original slot0,1,4 from MST/MSA occurrences0,1,2. Original missing
 * or zero-header pairs leave the slot unchanged. Caller zero-initializes.
 * Does not claim font rendering/upload completion or invent glyph bitfields. */
gh_font_error gh_font_bind_language(gh_font_state *,const gh_bundle *);
/* Complete original 2A4970 selector/control flow; no upload occurs there. */
gh_font_error gh_font_initialize(gh_font_state *,const gh_bundle *,int32_t selector);
/* Actual2A4CB0 sharedpairbinder. NULLMST/MSA preservesexisting slot exactly;
 * immutableboundedoffsetviews replace originalin-placeguestrelocation. */
gh_font_error gh_font_bind_pair_002a4cb0(gh_font_state *,unsigned,const gh_bundle_entry *,const gh_bundle_entry *);
typedef struct {uint32_t packed;uint8_t low_byte,left,right,bearing;int16_t width,baseline_delta;} gh_font_glyph;
/* Original encoded glyph high nibble selects slot, low12 selects MSA word.
 * Glyph0 has language-specific special width; rejected by this query. */
gh_font_error gh_font_glyph_at(const gh_font_state *,uint16_t code,gh_font_glyph *);
gh_font_error gh_font_texture_at(const gh_font_slot *,unsigned table,unsigned index,gh_font_texture_record *);
#ifdef __cplusplus
}
#endif
#endif
