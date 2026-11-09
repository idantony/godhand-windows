/* Actual cIDBase_setPackedMessData2A9F30 and message env leaf setters2AF628..
 * 2AF724. Semantic source also upstream001d0fd8.c/0012e670.c, MIT notice
 * src/core/LICENSE.upstream. Native view adaptation never truncates pointers. */
#include "packed_message.h"
#include <string.h>
#include <limits.h>
static uint16_t u16(const unsigned char *p){return (uint16_t)(p[0]|p[1]<<8);}
static uint32_t u32(const unsigned char *p){return (uint32_t)u16(p)|(uint32_t)u16(p+2)<<16;}
static void set32(unsigned char *p,unsigned o,uint32_t v){unsigned i;for(i=0;i<4;i++)p[o+i]=(unsigned char)(v>>(i*8));}
static void setfloat(unsigned char *p,unsigned o,float v){uint32_t bits;memcpy(&bits,&v,4);set32(p,o,bits);}
static gh_id_error font_pair(gh_font_slot *dst,const gh_bundle_entry *mst,const gh_bundle_entry *msa){
 gh_font_slot slot;gh_font_texture_record record;unsigned t,i;
 if(mst->size<20||msa->size<8)return GH_ID_BOUNDS;
 if(!u16(msa->bytes)||!u16(msa->bytes+2)||!u16(msa->bytes+4)||!msa->bytes[6]||!msa->bytes[7]||!u32(mst->bytes)||!u16(mst->bytes+4)||!u16(mst->bytes+6)||!u32(mst->bytes+8)||!u32(mst->bytes+12))return GH_ID_OK;
 if(u32(mst->bytes)&0x80000000U)return GH_ID_UNSUPPORTED;
 memset(&slot,0,sizeof(slot));slot.mst=mst->bytes;slot.mst_size=mst->size;slot.msa=msa->bytes;slot.msa_size=msa->size;
 slot.width=u16(msa->bytes+2);slot.height=u16(msa->bytes+4);slot.cell_width=msa->bytes[6];slot.cell_height=msa->bytes[7];slot.bound=1;
 for(t=0;t<2;t++){
  slot.table_offset[t]=u32(slot.mst+8+t*4);slot.table_count[t]=u16(slot.mst+16+t*2);
  if(slot.table_offset[t]>slot.mst_size||slot.table_count[t]>(slot.mst_size-slot.table_offset[t])/16)return GH_ID_BOUNDS;
  for(i=0;i<slot.table_count[t];i++)if(gh_font_texture_at(&slot,t,i,&record)!=GH_FONT_OK)return GH_ID_BOUNDS;
 }
 *dst=slot;return GH_ID_OK;
}
gh_id_error gh_packed_message_bind(gh_id_base *self,const gh_id_state *id,gh_font_state *font,
 gh_packed_message_state *state,const gh_bundle *language,int32_t selector,
 uint32_t resource_slot,int32_t arg,int *original_return){
 gh_id_view packed;gh_id_error error;gh_bundle pack;gh_bundle_entry mst,msa,cursor;
 if(!self||!id||!font||!state||!original_return)return GH_ID_ARGUMENT;
 *original_return=0;if(arg<0||arg==INT32_MAX)return GH_ID_BOUNDS;
 error=gh_id_lookup_002acd78(id,resource_slot,(uint32_t)arg+1,&packed);
 if(error==GH_ID_NOT_FOUND)return GH_ID_OK;if(error)return error;
 if(gh_bundle_open(packed.bytes,packed.size,&pack)!=GH_BUNDLE_OK)return GH_ID_FORMAT;
 self->packed=packed;
 if(selector>=0&&selector<7){
  gh_bundle_error a,b;
  if(selector&&!language)return GH_ID_ARGUMENT;
  a=gh_bundle_find(selector?language:&pack,"MST",0,&mst);b=gh_bundle_find(&pack,"MSA",0,&msa);
  if(a!=GH_BUNDLE_NOT_FOUND&&a!=GH_BUNDLE_OK)return GH_ID_BOUNDS;
  if(b!=GH_BUNDLE_NOT_FOUND&&b!=GH_BUNDLE_OK)return GH_ID_BOUNDS;
  if(!a&&!b){error=font_pair(&font->slots[3],&mst,&msa);if(error)return error;}
 }
 error=GH_ID_OK;
 {gh_bundle_error e=gh_bundle_find(&pack,"MSD",0,&cursor);
  if(e==GH_BUNDLE_NOT_FOUND)memset(&state->cursor,0,sizeof(state->cursor));
  else if(e)return GH_ID_BOUNDS;
  else{state->cursor.bytes=cursor.bytes;state->cursor.size=cursor.size;state->cursor.offset=packed.offset+cursor.offset;}}
 /* Original envInit writes these fields through actual setters. Preserve
  * untouched padding54..57 except the halfword54; do not invent full memset. */
 {unsigned o;for(o=0;o<=0x1c;o+=4)set32(state->environment,o,0);}
 setfloat(state->environment,0x20,0);setfloat(state->environment,0x24,0);set32(state->environment,0x28,2);
 setfloat(state->environment,0x2c,2);setfloat(state->environment,0x30,-10);setfloat(state->environment,0x34,1);setfloat(state->environment,0x38,0);
 setfloat(state->environment,0x3c,selector?0.9f:1);setfloat(state->environment,0x40,1);setfloat(state->environment,0x44,selector?0.9f:1);setfloat(state->environment,0x48,1);
 set32(state->environment,0,1);set32(state->environment,0x4c,2);set32(state->environment,0x50,0);
 state->environment[0x54]=255;state->environment[0x55]=255;set32(state->environment,0x58,6);set32(state->environment,0x5c,UINT32_MAX);setfloat(state->environment,0x60,65535);
 state->initialized=1;*original_return=1;return error;
}
