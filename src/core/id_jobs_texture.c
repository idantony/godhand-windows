/* T32 image/CLUT upload views and PSMT8/verifiedtitlePSMT4 conversion. Coordinate
 * equations independently derived/verified against official PCSX2 GSTables;
 * no GPL tables/source copied. See id-jobs-evidence.json/title-texture-evidence.json. */
#include "id_jobs.h"
#include "../assets/font_texture.h"
#include "../assets/tim2.h"
#include "../assets/gs_indexed_layout.h"
#include <stdlib.h>
#include <string.h>
static uint16_t u16(const unsigned char *p){return (uint16_t)(p[0]|p[1]<<8);}
static uint32_t u32(const unsigned char *p){return (uint32_t)u16(p)|(uint32_t)u16(p+2)<<16;}
static uint64_t u64(const unsigned char *p){return (uint64_t)u32(p)|(uint64_t)u32(p+4)<<32;}
gh_id_error gh_id_texture_open(const gh_id_state *m,uint32_t slot,uint32_t n,gh_id_texture *out){
 const gh_bundle_entry *table;uint32_t count,offset,i;size_t end;
 if(!m||!out)return GH_ID_ARGUMENT;if(slot==33||!n)return GH_ID_NOT_FOUND;if(slot>=32)return GH_ID_BOUNDS;
 table=&m->pair[slot].texture_data;if(!table->bytes)return GH_ID_NOT_FOUND;if(table->size<4)return GH_ID_BOUNDS;
 count=u32(table->bytes);if(count>(table->size-4)/4)return GH_ID_BOUNDS;if(n>count)return GH_ID_NOT_FOUND;
 offset=u32(table->bytes+(size_t)n*4);if(!offset)return GH_ID_NOT_FOUND;
 if(offset<4+(size_t)count*4||offset>=table->size)return GH_ID_BOUNDS;
 end=table->size;for(i=1;i<=count;i++){uint32_t next=u32(table->bytes+(size_t)i*4);if(next&&(next<4+(size_t)count*4||next>table->size))return GH_ID_BOUNDS;if(next>offset&&next<end)end=next;}
 gh_id_view source={table->bytes+offset,end-offset,offset};
 return gh_id_texture_open_view(&source,out);
}
static int tim2_hud8(const gh_tim2_picture *picture)
{
 return picture->tex0==GH_ID_TIM2_128X64_PSMT8_TEX0&&picture->width==128&&
        picture->height==64&&picture->image_type==5&&picture->clut_type==3&&picture->colors==256;
}
gh_id_error gh_id_texture_open_view(const gh_id_view *source,gh_id_texture *out){
 if(!source||!source->bytes||!out)return GH_ID_ARGUMENT;
 gh_id_texture t={.source=*source};
 if(t.source.size>=4&&!memcmp(t.source.bytes,"TIM2",4)){
  gh_tim2_picture picture;gh_tim2_error pe=gh_tim2_first(t.source.bytes,t.source.size,&picture);
  if(pe)return pe==GH_TIM2_BOUNDS?GH_ID_BOUNDS:GH_ID_UNSUPPORTED;
  /* ActualMC confirmationcursor: languageTBL1 item47. Original151630
   * classifiesTIM2as0;15BCF0 calls15E0A0 linearPSMT4+8x2CLUTupload. */
  int loading=picture.width==32&&picture.height==16&&picture.tex0==UINT64_C(0x115408000);
  int cursor=picture.width==32&&picture.height==32&&((picture.tex0>>30)&15)==5;
  int indexed4=(loading||cursor)&&picture.image_type==4&&picture.clut_type==3&&picture.colors==16&&
                ((picture.tex0>>20)&63)==20&&((picture.tex0>>26)&15)==5&&!((picture.tex0>>35)&3);
  if(!indexed4&&!tim2_hud8(&picture))return GH_ID_UNSUPPORTED;
  t.width=picture.width;t.height=picture.height;t.tex0=picture.tex0;*out=t;return GH_ID_OK;
 }
 if(t.source.size<48||u32(t.source.bytes)&0x80000000U)return GH_ID_UNSUPPORTED;
 t.tex0=u64(t.source.bytes+32);
 {unsigned psm=(unsigned)((t.tex0>>20)&63),power=(unsigned)((t.tex0>>30)&15);
  unsigned tw=(unsigned)((t.tex0>>26)&15);if(((t.tex0>>51)&31)!=0)return GH_ID_UNSUPPORTED;
  if(psm==19){if(t.tex0==UINT64_C(0x20000001e1310000)){t.width=256;t.height=128;*out=t;return GH_ID_OK;}if(t.tex0==GH_ID_RADAR_TEX0){t.width=128;t.height=256;*out=t;return GH_ID_OK;}if(tw!=9||(power!=8&&power!=9))return GH_ID_UNSUPPORTED;}
  else if(psm==20){if(tw!=9||power!=9||((t.tex0>>56)&31))return GH_ID_UNSUPPORTED;}
  else return GH_ID_UNSUPPORTED;
  t.width=512;t.height=(uint16_t)(1U<<power);
 }
 *out=t;return GH_ID_OK;
}
static unsigned addr32(unsigned x,unsigned y){
 unsigned bx=(x%64)/8,by=(y%32)/8;
 unsigned block=(bx&1)^((bx&2)<<1)^((bx&4)<<2)^((by&1)<<1)^((by&2)<<2);
 unsigned col=(x&1)^((x&2)<<1)^((x&4)<<1)^((y&1)<<1)^((y&2)<<3)^((y&4)<<3);
 return (y/32)*2048+block*64+col;
}
static unsigned addr4(unsigned x,unsigned y){
 /* Actual512widePSMT4: 128x128page,32x16block,4x8column. Sameindependently
  * derivedGSbitpermutation asfontatlas; extendYpages, notlinear-nibbleguess. */
 unsigned bx=(x%128)/32,by=(y%128)/16,cx=x%32,cy=y%16;
 unsigned block=((bx&1)<<1)^((bx&2)<<2)^(by&1)^((by&2)<<1)^((by&4)<<2);
 unsigned column=((cx&1)<<3)^((cx&2)<<4)^((cx&4)<<4)^((cx&8)>>2)^((cx&16)>>2)^((cy&1)<<4)^((cy&2)?65:0)^((cy&4)?192:0)^((cy&8)<<5);
 return ((x/128)+(y/128)*4)*16384+block*512+column;
}
gh_id_error gh_id_texture_rgba(const gh_id_texture *t,void *rgba,size_t capacity){
 gh_font_slot slot;gh_font_transfer image,clut;unsigned *inverse;unsigned x,y,i,words,psm;unsigned char *out=rgba;
 if(!t||!rgba||!t->source.bytes)return GH_ID_ARGUMENT;if(capacity<(size_t)t->width*t->height*4)return GH_ID_CAPACITY;
 if(t->source.size>=4&&!memcmp(t->source.bytes,"TIM2",4)){
  gh_tim2_picture p;gh_tim2_error e=gh_tim2_first(t->source.bytes,t->source.size,&p);
  if(e)return e==GH_TIM2_BOUNDS?GH_ID_BOUNDS:GH_ID_UNSUPPORTED;
  if(p.width!=t->width||p.height!=t->height||t->tex0!=p.tex0)return GH_ID_UNSUPPORTED;
  if(tim2_hud8(&p))e=gh_tim2_rgba8(&p,rgba,capacity);
  else{
   if(t->width!=32||(t->height!=32&&(t->height!=16||t->tex0!=UINT64_C(0x115408000))))return GH_ID_UNSUPPORTED;
   e=gh_tim2_rgba4(&p,rgba,capacity);
  }
  return e==GH_TIM2_OK?GH_ID_OK:e==GH_TIM2_BOUNDS?GH_ID_BOUNDS:GH_ID_UNSUPPORTED;
 }
 int level=t->width==256&&t->height==128&&t->tex0==UINT64_C(0x20000001e1310000);
 int radar=t->width==128&&t->height==256&&t->tex0==GH_ID_RADAR_TEX0;
 if(t->source.size<48||(!level&&!radar&&(t->width!=512||(t->height!=256&&t->height!=512))))return GH_ID_UNSUPPORTED;
 psm=(unsigned)((t->tex0>>20)&63);if(psm!=19&&(psm!=20||t->height!=512||((t->tex0>>51)&63)||((t->tex0>>56)&31)))return GH_ID_UNSUPPORTED;
 memset(&slot,0,sizeof(slot));slot.bound=1;slot.mst=t->source.bytes;slot.mst_size=t->source.size;
 for(i=0;i<2;i++){slot.table_offset[i]=u32(slot.mst+8+i*4);slot.table_count[i]=u16(slot.mst+16+i*2);}
 if(gh_font_transfer_at(&slot,0,0,&image)||gh_font_transfer_at(&slot,1,0,&clut))return GH_ID_UNSUPPORTED;
 if(image.width!=64||image.height!=((unsigned)t->width*t->height)/(psm==20?512u:256u)||image.buffer_width!=1||image.destination_base||clut.width!=16||clut.height!=16||clut.destination_base)return GH_ID_UNSUPPORTED;
 for(i=0;i<256;i++)if(clut.pixels[i*4+3]>128)return GH_ID_UNSUPPORTED;
 words=(unsigned)image.pixel_bytes/4;inverse=malloc((size_t)words*sizeof(*inverse));if(!inverse)return GH_ID_MEMORY;
 for(i=0;i<words;i++)inverse[i]=UINT32_MAX;
 for(y=0;y<image.height;y++)for(x=0;x<64;x++){unsigned a=addr32(x,y);if(a>=words||inverse[a]!=UINT32_MAX){free(inverse);return GH_ID_FORMAT;}inverse[a]=y*64+x;}
 for(y=0;y<t->height;y++)for(x=0;x<t->width;x++){
  unsigned a=psm==20?addr4(x,y):(unsigned)gh_gs_psmt8_offset(x,y,t->width/128),index,palette;const unsigned char *color;
  if(a/(psm==20?8:4)>=words){free(inverse);return GH_ID_BOUNDS;}
  if(psm==20){index=(image.pixels[inverse[a/8]*4+(a%8)/2]>>((a&1)*4))&15;palette=(index&7)+(index>>3)*16;}
  else{index=image.pixels[inverse[a/4]*4+a%4];
  /* CSM1 PSMT8 palette index swaps bits3/4, matching official gsKit T8 CLUT
   * rotation and unlike its T4 path. Other bits remain unchanged. */
  palette=gh_gs_csm1_palette256(index);}
  color=clut.pixels+palette*4;i=(y*t->width+x)*4;
  out[i]=color[0];out[i+1]=color[1];out[i+2]=color[2];out[i+3]=color[3]>=128?255:(unsigned char)(color[3]*2);
 }
 free(inverse);return GH_ID_OK;
}
