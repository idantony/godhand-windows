/* Actual 2A4CB0 font binding and MST relocation behavior. Native views retain
 * immutable offsets instead of setting guest sign-bit and writing pointers. */
#include "font_state.h"
#include <string.h>
static uint16_t u16(const unsigned char *p){return (uint16_t)(p[0]|p[1]<<8);}
static uint32_t u32(const unsigned char *p){return (uint32_t)u16(p)|(uint32_t)u16(p+2)<<16;}
gh_font_error gh_font_texture_at(const gh_font_slot *s,unsigned t,unsigned i,gh_font_texture_record *out){
 size_t r,o;uint32_t delta;
 if(!s||!out||!s->bound)return GH_FONT_ARGUMENT;
 if(t>1||i>=s->table_count[t])return GH_FONT_BOUNDS;
 r=s->table_offset[t]+(size_t)i*16;
 if(r>s->mst_size||s->mst_size-r<16)return GH_FONT_BOUNDS;
 delta=u32(s->mst+r+4);if(delta>s->mst_size-s->table_offset[t])return GH_FONT_BOUNDS;
 o=s->table_offset[t]+delta;out->record=s->mst+r;out->record_offset=r;out->data=s->mst+o;out->data_offset=o;out->available=s->mst_size-o;
 return GH_FONT_OK;
}
static gh_font_error bind(gh_font_slot *dst,const gh_bundle_entry *mst,const gh_bundle_entry *msa){
 gh_font_slot s;unsigned t,i;gh_font_texture_record record;
 if(msa->size<8||mst->size<20)return GH_FONT_BOUNDS;
 if(!u16(msa->bytes)||!u16(msa->bytes+2)||!u16(msa->bytes+4)||!msa->bytes[6]||!msa->bytes[7]||!u32(mst->bytes)||!u16(mst->bytes+4)||!u16(mst->bytes+6)||!u32(mst->bytes+8)||!u32(mst->bytes+12))return GH_FONT_OK;
 if(u32(mst->bytes)&0x80000000U)return GH_FONT_UNSUPPORTED;
 memset(&s,0,sizeof(s));s.mst=mst->bytes;s.mst_size=mst->size;s.msa=msa->bytes;s.msa_size=msa->size;
 s.width=u16(msa->bytes+2);s.height=u16(msa->bytes+4);s.cell_width=msa->bytes[6];s.cell_height=msa->bytes[7];s.bound=1;
 for(t=0;t<2;t++){
  s.table_offset[t]=u32(s.mst+8+t*4);s.table_count[t]=u16(s.mst+16+t*2);
  if(s.table_offset[t]>s.mst_size||(size_t)s.table_count[t]>(s.mst_size-s.table_offset[t])/16)return GH_FONT_BOUNDS;
  for(i=0;i<s.table_count[t];i++){gh_font_error e=gh_font_texture_at(&s,t,i,&record);if(e)return e;}
 }
 *dst=s;return GH_FONT_OK;
}
gh_font_error gh_font_bind_pair_002a4cb0(gh_font_state*s,unsigned slot,const gh_bundle_entry*mst,const gh_bundle_entry*msa){
 if(!s||slot>=5)return GH_FONT_ARGUMENT;if(!mst||!msa||!mst->bytes||!msa->bytes)return GH_FONT_OK;return bind(&s->slots[slot],mst,msa);
}
gh_font_error gh_font_bind_language(gh_font_state *state,const gh_bundle *language){
 static const unsigned slots[]={0,1,4};unsigned i;gh_font_state next;
 if(!state||!language)return GH_FONT_ARGUMENT;next=*state;
 for(i=0;i<3;i++){
  gh_bundle_entry mst,msa;gh_bundle_error a=gh_bundle_find(language,"MST",i,&mst),b=gh_bundle_find(language,"MSA",i,&msa);
  if(a==GH_BUNDLE_NOT_FOUND||b==GH_BUNDLE_NOT_FOUND)continue;
  if(a||b)return GH_FONT_BOUNDS;
  {gh_font_error e=bind(&next.slots[slots[i]],&mst,&msa);if(e)return e;}
 }
 *state=next;return GH_FONT_OK;
}
gh_font_error gh_font_initialize(gh_font_state *state,const gh_bundle *language,int32_t selector){
 gh_font_state next;unsigned occurrence;
 if(!state||!language)return GH_FONT_ARGUMENT;
 next=*state;
 if(selector==0){gh_font_error e=gh_font_bind_language(&next,language);if(e)return e;}
 else if(selector>0&&selector<7){
  for(occurrence=0;occurrence<2;occurrence++){
   gh_bundle_entry mst,msa;gh_bundle_error a=gh_bundle_find(language,"MST",occurrence,&mst),b=gh_bundle_find(language,"MSA",occurrence,&msa);
   if(a==GH_BUNDLE_NOT_FOUND||b==GH_BUNDLE_NOT_FOUND)continue;
   if(a||b)return GH_FONT_BOUNDS;
   {gh_font_error e=bind(&next.slots[occurrence?4:0],&mst,&msa);if(e)return e;}
  }
 }
 next.draw_word_2a08c=0;*state=next;return GH_FONT_OK;
}
gh_font_error gh_font_glyph_at(const gh_font_state *state,uint16_t code,gh_font_glyph *out){
 unsigned slot=code>>12,index=code&4095;const gh_font_slot *s;uint32_t word;gh_font_glyph g;
 if(!state||!out)return GH_FONT_ARGUMENT;
 if(!code)return GH_FONT_UNSUPPORTED;
 if(slot>=5||!state->slots[slot].bound)return GH_FONT_UNSUPPORTED;
 s=&state->slots[slot];
 if(index>=u16(s->msa)||s->msa_size<8||(size_t)index>=(s->msa_size-8)/4)return GH_FONT_BOUNDS;
 word=u32(s->msa+8+(size_t)index*4);g.packed=word;g.low_byte=(uint8_t)word;
 g.left=(uint8_t)((word>>8)&63);g.right=(uint8_t)((word>>20)&63);g.bearing=(uint8_t)((word>>14)&63);
 g.width=(int16_t)((int)g.right-(int)g.left+1);g.baseline_delta=(int16_t)((int)s->cell_height-(int)g.bearing);
 *out=g;return GH_FONT_OK;
}
