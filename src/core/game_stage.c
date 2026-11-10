/* Actual150C10,309D88,30A200 registry/transfer slice. Only packet shapes
 * supported by the independently verified font_texture helper are admitted. */
#include "game_stage.h"
#include "../assets/tm3.h"
#include <stdlib.h>
#include <string.h>
#define VRAM_BYTES (4u*1024u*1024u)
typedef struct stage_material_node {gh_stage_material view;uint8_t *pixels;uint64_t vram_generation;struct stage_material_node *next;} stage_material_node;
struct gh_stage_textures {uint8_t *vram;gh_stage_channel channel[256];gh_stage_texture_cursor cursor;
 stage_material_node *materials;size_t material_count;uint64_t next_material_id,vram_generation;size_t channel_bytes[256];};
static uint16_t u16(const uint8_t *p){return (uint16_t)((unsigned)p[0]|(unsigned)p[1]<<8);}
static uint32_t u32(const uint8_t *p){return (uint32_t)u16(p)|(uint32_t)u16(p+2)<<16;}
static uint64_t u64(const uint8_t *p){return (uint64_t)u32(p)|(uint64_t)u32(p+4)<<32;}
/* Generalized page pitch around project's independently derived CT32 page
 * bit permutation, verified against primary PCSX2 GSTables in font evidence. */
static uint64_t addr32(uint32_t x,uint32_t y,uint8_t pitch){
 uint32_t bx=(x%64)/8,by=(y%32)/8;
 uint32_t block=(bx&1)^((bx&2)<<1)^((bx&4)<<2)^((by&1)<<1)^((by&2)<<2);
 uint32_t col=(x&1)^((x&2)<<1)^((x&4)<<1)^((y&1)<<1)^((y&2)<<3)^((y&4)<<3);
 return (((uint64_t)x/64+(uint64_t)(y/32)*pitch)*2048+block*64+col)*4;
}
gh_stage_status gh_stage_textures_create(gh_stage_textures **out){
 gh_stage_textures *s;if(!out)return GH_STAGE_ARGUMENT;*out=0;
 s=calloc(1,sizeof(*s));if(!s)return GH_STAGE_MEMORY;
 s->vram=calloc(1,VRAM_BYTES);if(!s->vram){free(s);return GH_STAGE_MEMORY;}
 s->cursor.image_base=0x2600; /*1508F8 constructor, not guessed GPU heap*/
 *out=s;return GH_STAGE_OK;
}
void gh_stage_tm3_cache_clear(gh_stage_textures*s){if(s){stage_material_node*n=s->materials;while(n){stage_material_node*next=n->next;free(n->pixels);free(n);n=next;}s->materials=NULL;s->material_count=0;}}
gh_stage_status gh_stage_texture_forget_source_range(gh_stage_textures*s,const void*base,size_t bytes){
 if(!s||(!base&&bytes))return GH_STAGE_ARGUMENT;uintptr_t start=(uintptr_t)base;
 if(bytes>UINTPTR_MAX-start)return GH_STAGE_BOUNDS;
 for(unsigned i=0;i<256;i++){uintptr_t p=(uintptr_t)s->channel[i].source;
  if(s->channel[i].source&&p>=start&&p-start<bytes)memset(&s->channel[i],0,sizeof(s->channel[i]));}
 stage_material_node**link=&s->materials;
 while(*link){stage_material_node*n=*link;uintptr_t p=(uintptr_t)n->view.source;
  if(p>=start&&p-start<bytes){*link=n->next;free(n->pixels);free(n);s->material_count--;}
  else link=&n->next;
 }
 return GH_STAGE_OK;
}
void gh_stage_textures_destroy(gh_stage_textures *s){if(s){gh_stage_tm3_cache_clear(s);free(s->vram);free(s);}}
gh_stage_status gh_stage_tm3_resolve(gh_stage_textures*s,const void*raw,size_t n,unsigned member,unsigned mode,const gh_stage_material**out){
 if(!s||!raw||!out)return GH_STAGE_ARGUMENT;*out=NULL;
 if(mode>3)return GH_STAGE_UNSUPPORTED;
 gh_tm3_bundle bundle;gh_tm3_picture picture;gh_tm3_error e=gh_tm3_open(raw,n,&bundle);
 if(!e)e=gh_tm3_member(&bundle,member,&picture);
 if(e)return e==GH_TM3_BOUNDS?GH_STAGE_BOUNDS:GH_STAGE_UNSUPPORTED;
 unsigned pool=mode==1?1:mode==2?2:0;
 /* Original151398 key is source identity in its selected cache pool. */
 for(stage_material_node*p=s->materials;p;p=p->next)if(p->view.source==picture.source&&p->view.cache_pool==pool){*out=&p->view;return GH_STAGE_OK;}
 if(s->material_count>=1024||s->next_material_id==UINT64_MAX)return GH_STAGE_MEMORY;
 stage_material_node*p=calloc(1,sizeof(*p));if(!p)return GH_STAGE_MEMORY;
 size_t bytes=(size_t)picture.width*picture.height*4;p->pixels=malloc(bytes);if(!p->pixels){free(p);return GH_STAGE_MEMORY;}
 e=gh_tm3_rgba_gs(&picture,p->pixels,bytes);if(e){free(p->pixels);free(p);return e==GH_TM3_BOUNDS?GH_STAGE_BOUNDS:GH_STAGE_UNSUPPORTED;}
 p->view=(gh_stage_material){picture.source,p->pixels,picture.source_bytes,bytes,++s->next_material_id,picture.tex0,picture.width,picture.height,(uint8_t)mode,(uint8_t)pool,20,1,(uint8_t)((picture.tex0>>35)&3)};
 p->next=s->materials;s->materials=p;s->material_count++;*out=&p->view;return GH_STAGE_OK;
}
static gh_stage_status transfer(uint8_t *vram,const gh_font_transfer *t,uint32_t base){
 if(t->psm!=0 || !t->buffer_width || t->width>(uint32_t)t->buffer_width*64)return GH_STAGE_UNSUPPORTED;
 for(uint32_t y=0;y<t->height;y++)for(uint32_t x=0;x<t->width;x++){
  uint64_t a=(uint64_t)base*256+addr32(x,y,t->buffer_width);
  if(a>VRAM_BYTES-4)return GH_STAGE_BOUNDS;
  memcpy(vram+(size_t)a,t->pixels+((size_t)y*t->width+x)*4,4);
 }
 return GH_STAGE_OK;
}
gh_stage_status gh_stage_texture_load(gh_stage_textures *s,const void *data,size_t bytes,uint32_t *out){
 const uint8_t *b=data;gh_font_slot slot={0};uint8_t *candidate;gh_stage_channel channel={0};
 uint32_t count,relative,allocated,palette_count;uint64_t tex0;gh_stage_status e;
 if(!s || !b || !out)return GH_STAGE_ARGUMENT;*out=UINT32_MAX;
 for(unsigned i=0;i<256;i++)if(s->channel[i].valid && s->channel[i].source==b){*out=i;return GH_STAGE_OK;}
 if(s->vram_generation==UINT64_MAX)return GH_STAGE_MEMORY;
 if(bytes<48)return GH_STAGE_BOUNDS;
 count=u32(b);if(count!=1)return GH_STAGE_UNSUPPORTED;
 relative=u32(b+24);if(relative || s->cursor.next_channel>=256)return GH_STAGE_UNSUPPORTED;
 allocated=((uint32_t)u16(b+4)*u16(b+6))/64;
 palette_count=u16(b+18);tex0=u64(b+32);
 if(palette_count>256 || s->cursor.clut_index>256-palette_count)return GH_STAGE_BOUNDS;
 if(!allocated || s->cursor.image_base>0x3fff || s->cursor.clut_index>0xff ||
    allocated>0x4000-s->cursor.image_base)return GH_STAGE_BOUNDS;
 channel.valid=1;channel.psm=(uint8_t)((tex0>>20)&63);
 channel.width=(uint16_t)(1u<<((tex0>>26)&15));channel.height=(uint16_t)(1u<<((tex0>>30)&15));
 channel.base=(uint16_t)(((tex0&0x3fff)+s->cursor.image_base)&0x3fff);
 channel.clut_base=(uint16_t)((0x1c00+4*(s->cursor.clut_index+(uint32_t)((tex0>>37)&0x3fff)))&0x3fff);
 channel.tex0=(tex0&UINT64_C(0x1ff8001fffffc000))|channel.base|
  ((uint64_t)channel.clut_base<<37)|UINT64_C(0x8000000400000000);
 channel.tex1=u64(b+40);channel.source=b;
 slot.bound=1;slot.mst=b;slot.mst_size=bytes;
 for(unsigned table=0;table<2;table++){slot.table_offset[table]=u32(b+8+table*4);slot.table_count[table]=u16(b+16+table*2);}
 if(!slot.table_count[0])return GH_STAGE_UNSUPPORTED;
 candidate=malloc(VRAM_BYTES);if(!candidate)return GH_STAGE_MEMORY;memcpy(candidate,s->vram,VRAM_BYTES);
 for(unsigned table=0;table<2;table++)for(unsigned i=0;i<slot.table_count[table];i++){
  gh_font_transfer t;gh_font_error fe=gh_font_transfer_at(&slot,table,i,&t);uint32_t base;
  if(fe){free(candidate);return fe==GH_FONT_BOUNDS?GH_STAGE_BOUNDS:GH_STAGE_UNSUPPORTED;}
  base=table?(0x1c00+4*(s->cursor.clut_index+t.destination_base)):
   (s->cursor.image_base+t.destination_base);
  if(base>0x3fff){free(candidate);return GH_STAGE_BOUNDS;}
  e=transfer(candidate,&t,base);if(e){free(candidate);return e;}
 }
 free(s->vram);s->vram=candidate;*out=s->cursor.next_channel;
 s->channel[*out]=channel;s->channel_bytes[*out]=bytes;s->vram_generation++;s->cursor.count++;s->cursor.next_channel++;
 s->cursor.image_base+=allocated;s->cursor.clut_index+=palette_count;return GH_STAGE_OK;
}
gh_stage_status gh_stage_core_textures(gh_stage_textures *s,const void *data,size_t bytes){
 const uint8_t *b=data;uint32_t count;
 if(!s || !b)return GH_STAGE_ARGUMENT;if(bytes<12)return GH_STAGE_BOUNDS;
 count=u32(b);if(count<2 || count>(bytes-4)/4)return GH_STAGE_BOUNDS;
 for(unsigned i=1;i<=2;i++){
  uint32_t off=u32(b+i*4);size_t end=bytes;uint32_t channel;gh_stage_status e;
  if(off<4+(size_t)count*4 || off>=bytes)return GH_STAGE_BOUNDS;
  for(unsigned j=1;j<=count;j++){uint32_t next=u32(b+j*4);if(next>off && next<end)end=next;}
  e=gh_stage_texture_load(s,b+off,end-off,&channel);if(e)return e;
 }
 return GH_STAGE_OK;
}
gh_stage_status gh_stage_texture_channel(const gh_stage_textures *s,uint32_t n,gh_stage_channel *out){
 if(!s || !out)return GH_STAGE_ARGUMENT;if(n>=256)return GH_STAGE_BOUNDS;
 if(!s->channel[n].valid)return GH_STAGE_UNAVAILABLE;*out=s->channel[n];return GH_STAGE_OK;
}
gh_stage_status gh_stage_texture_cursor_get(const gh_stage_textures *s,gh_stage_texture_cursor *out){
 if(!s || !out)return GH_STAGE_ARGUMENT;*out=s->cursor;return GH_STAGE_OK;
}
gh_stage_status gh_stage_vram_ct32(const gh_stage_textures *s,uint16_t base,uint8_t pitch,
 uint32_t x,uint32_t y,uint32_t *out){
 uint64_t a;if(!s || !out || !pitch)return GH_STAGE_ARGUMENT;
 a=(uint64_t)base*256+addr32(x,y,pitch);if(a>VRAM_BYTES-4)return GH_STAGE_BOUNDS;
 *out=u32(s->vram+(size_t)a);return GH_STAGE_OK;
}

static uint64_t address8(unsigned x,unsigned y,unsigned pitch){
 unsigned bx=(x%128)/16,by=(y%64)/16,cx=x%16,cy=y%16;
 unsigned block=(bx&1)^((bx&2)<<1)^((bx&4)<<2)^((by&1)<<1)^((by&2)<<2);
 unsigned col=((cx&1)<<2)^((cx&2)<<3)^((cx&4)<<3)^((cx&8)>>2)^((cy&1)<<3)^((cy&2)?33:0)^((cy&4)?96:0)^((cy&8)<<4);
 return ((uint64_t)x/128+(uint64_t)(y/64)*pitch)*8192+block*256+col;
}
static uint64_t address4(unsigned x,unsigned y,unsigned pitch){
 unsigned bx=(x%128)/32,by=(y%128)/16,cx=x%32,cy=y%16;
 unsigned block=((bx&1)<<1)^((bx&2)<<2)^(by&1)^((by&2)<<1)^((by&4)<<2);
 unsigned col=((cx&1)<<3)^((cx&2)<<4)^((cx&4)<<4)^((cx&8)>>2)^((cx&16)>>2)^((cy&1)<<4)^((cy&2)?65:0)^((cy&4)?192:0)^((cy&8)<<5);
 return ((uint64_t)x/128+(uint64_t)(y/128)*pitch)*16384+block*512+col;
}
static gh_stage_status channel_rgba(const gh_stage_textures *s,gh_stage_channel c,
 void *buffer,size_t capacity,int raw_alpha){
 uint8_t *out=buffer;unsigned pitch;
 if(!s||!buffer)return GH_STAGE_ARGUMENT;
 if((c.psm!=19 && c.psm!=20) || ((c.tex0>>51)&15) || ((c.tex0>>55)&1) || ((c.tex0>>56)&31))return GH_STAGE_UNSUPPORTED;
 if(capacity<(size_t)c.width*c.height*4)return GH_STAGE_BOUNDS;
 pitch=(unsigned)((c.tex0>>14)&63)/2;
 for(unsigned y=0;y<c.height;y++)for(unsigned x=0;x<c.width;x++){
  uint64_t unit=c.psm==19?address8(x,y,pitch):address4(x,y,pitch);
  uint64_t a=(uint64_t)c.base*256+(c.psm==19?unit:unit/2),color;
  unsigned index,pal;uint32_t pixel;
  if(a>=VRAM_BYTES)return GH_STAGE_BOUNDS;
  index=c.psm==19?s->vram[(size_t)a]:(s->vram[(size_t)a]>>((unit&1)*4))&15;
  pal=(index&~24u)|((index&8)<<1)|((index&16)>>1);
  color=(uint64_t)c.clut_base*256+addr32(pal&15,pal>>4,1);
  if(color>VRAM_BYTES-4)return GH_STAGE_BOUNDS;
  pixel=u32(s->vram+(size_t)color);
  {size_t dst=((size_t)y*c.width+x)*4;unsigned alpha=pixel>>24;
   out[dst]=(uint8_t)pixel;out[dst+1]=(uint8_t)(pixel>>8);out[dst+2]=(uint8_t)(pixel>>16);
   out[dst+3]=raw_alpha?(uint8_t)alpha:(uint8_t)(alpha>=128?255:alpha*2);}
 }
 return GH_STAGE_OK;
}

gh_stage_status gh_stage_texture_rgba(const gh_stage_textures *s,uint32_t n,
 void *buffer,size_t capacity){
 if(!buffer)return GH_STAGE_ARGUMENT;
 gh_stage_channel c;gh_stage_status e=gh_stage_texture_channel(s,n,&c);if(e)return e;
 return channel_rgba(s,c,buffer,capacity,0);
}

gh_stage_status gh_stage_tex0_material(gh_stage_textures *s,uint64_t tex0,const gh_stage_material **out){
 if(!s||!out)return GH_STAGE_ARGUMENT;*out=NULL;
 /* CLD changes palette-load commands, not the already loaded native VRAM.
  * TBW is compared by actual sampled addresses below, never ignored blindly. */
 const uint64_t ignored=(UINT64_C(7)<<61)|(UINT64_C(63)<<14);
 unsigned found=256;
 for(unsigned i=0;i<256;i++){
  const gh_stage_channel*c=s->channel+i;if(!c->valid||(c->tex0&~ignored)!=(tex0&~ignored))continue;
  if(c->psm!=19&&c->psm!=20)continue;
  unsigned a=(unsigned)((c->tex0>>14)&63)/2,b=(unsigned)((tex0>>14)&63)/2;int same_addresses=1;
  for(unsigned y=0;y<c->height&&same_addresses;y++)for(unsigned x=0;x<c->width;x++){
   uint64_t first=c->psm==19?address8(x,y,a):address4(x,y,a);
   uint64_t second=c->psm==19?address8(x,y,b):address4(x,y,b);
   if(first!=second){same_addresses=0;break;}
  }
  if(!same_addresses)continue;if(found!=256)return GH_STAGE_UNSUPPORTED;found=i;
 }
 if(found==256)return GH_STAGE_UNAVAILABLE;
 gh_stage_channel channel=s->channel[found];
 for(stage_material_node*p=s->materials;p;p=p->next)if(p->view.cache_pool==3&&p->view.source==channel.source&&p->view.source_tex0==tex0&&p->vram_generation==s->vram_generation){*out=&p->view;return GH_STAGE_OK;}
 if(s->material_count>=1024||s->next_material_id==UINT64_MAX)return GH_STAGE_MEMORY;
 stage_material_node*p=calloc(1,sizeof(*p));if(!p)return GH_STAGE_MEMORY;
 size_t bytes=(size_t)channel.width*channel.height*4;p->pixels=malloc(bytes);if(!p->pixels){free(p);return GH_STAGE_MEMORY;}
 channel.tex0=tex0;gh_stage_status e=channel_rgba(s,channel,p->pixels,bytes,1);
 if(e){free(p->pixels);free(p);return e;}
 p->view=(gh_stage_material){channel.source,p->pixels,s->channel_bytes[found],bytes,++s->next_material_id,tex0,channel.width,channel.height,0,3,channel.psm,(uint8_t)((tex0>>34)&1),(uint8_t)((tex0>>35)&3)};
 p->vram_generation=s->vram_generation;p->next=s->materials;s->materials=p;s->material_count++;*out=&p->view;return GH_STAGE_OK;
}

static int signed_priority(const uint8_t *record){unsigned n=u16(record+20);return n<32768?(int)n:(int)n-65536;}
void gh_stage_skills_clear(gh_stage_skills *s){if(s){free(s->image);memset(s,0,sizeof(*s));}}
gh_stage_status gh_stage_skills_initialize(gh_stage_skills *out,const void *data,
 size_t bytes,const gh_boot_work *work,gh_core_save_view save){
 const uint8_t *b=data;uint32_t count,block,off;size_t end;gh_stage_skills s={0};
 uint32_t cheat;
 if(!out || !b || !work || !save.bytes || save.size<GH_CORE_SAVE_BYTES)return GH_STAGE_ARGUMENT;
 if(bytes<28)return GH_STAGE_BOUNDS;count=u32(b);if(count<6 || count>(bytes-4)/4)return GH_STAGE_BOUNDS;
 cheat=u32((const uint8_t *)work+0x594);
 block=(work->language==0 && !(cheat&0x800u))?5:6;off=u32(b+block*4);
 if(off<4+(size_t)count*4 || off>=bytes)return GH_STAGE_BOUNDS;
 end=bytes;for(uint32_t i=1;i<=count;i++){uint32_t next=u32(b+i*4);if(next>off && next<end)end=next;}
 if(end-off<12)return GH_STAGE_BOUNDS;
 if(u32(b+off)!=0x00444c53 || u32(b+off+4)!=2)return GH_STAGE_UNSUPPORTED;
 s.count=u32(b+off+8);if(s.count>114 || (size_t)s.count*36>end-off-12)return GH_STAGE_BOUNDS;
 s.size=end-off;s.image=malloc(s.size);if(!s.image)return GH_STAGE_MEMORY;memcpy(s.image,b+off,s.size);
 for(uint32_t i=0;i<s.count;i++){
  uint8_t *row=s.image+12+(size_t)i*36;uint32_t flags=u32(row+12)|7;
  for(unsigned n=0;n<4;n++)row[12+n]=(uint8_t)(flags>>(8*n));
  /* ActualgetSkill1FAF58 returns0 undercheat800000 regardlessofsavedbyte. */
  if(!(cheat&0x800000u) && save.bytes[0xb0+i]==255)save.bytes[0xb0+i]=0;
  if((cheat&0x800000u) || save.bytes[0xb0+i]<128)s.order[s.available++]=(uint8_t)i;
 }
 /*2C9950 only exchanges strictlower signed16 priorities, preserving ties.
  * Stable insertion yields its same ordered index list without mutatingrows. */
 for(uint32_t i=1;i<s.available;i++){
  uint8_t item=s.order[i];uint32_t j=i;int priority=signed_priority(s.image+12+(size_t)item*36);
  while(j && signed_priority(s.image+12+(size_t)s.order[j-1]*36)>priority){s.order[j]=s.order[j-1];--j;}s.order[j]=item;
 }
 free(out->image);*out=s;return GH_STAGE_OK;
}
gh_stage_status gh_stage_skill_at(const gh_stage_skills *s,uint32_t index,
 uint32_t *original,const uint8_t **record){
 if(!s || !original || !record)return GH_STAGE_ARGUMENT;
 if(!s->image)return GH_STAGE_UNAVAILABLE;if(index>=s->available)return GH_STAGE_BOUNDS;
 *original=s->order[index];*record=s->image+12+(size_t)*original*36;return GH_STAGE_OK;
}

static void set32(uint8_t *p,size_t off,uint32_t v){for(unsigned i=0;i<4;i++)p[off+i]=(uint8_t)(v>>(8*i));}
gh_stage_status gh_stage_slots_reset(gh_stage_slots *s,void *heap){
 if(!s || !s->configured)return GH_STAGE_UNAVAILABLE;
 if((uint64_t)s->count*0x5c>s->bytes || (s->count && !s->records))return GH_STAGE_BOUNDS;
 for(uint32_t n=s->count;n;n--){
  uint8_t *p=s->records+(size_t)(n-1)*0x5c;
  set32(p,12,0xffffffffu);set32(p,0x58,0xffff);
  set32(p,0,0);set32(p,4,0);set32(p,16,0);set32(p,8,0);set32(p,28,0);set32(p,32,0xffff);
  set32(p,0x4c,0xffff);set32(p,0x50,0xffff);set32(p,0x54,0xffff);set32(p,24,0);
  for(unsigned i=0;i<10;i++)set32(p,0x48-i*4,0xffffffffu);set32(p,20,0);
 }
 s->heap_handle=heap;return GH_STAGE_OK;
}
static gh_stage_status core_part(const uint8_t *b,size_t size,uint32_t index,const uint8_t **out,size_t *bytes){
 uint32_t count,off;size_t end=size;if(!b || size<4)return GH_STAGE_BOUNDS;
 count=u32(b);if(!index || index>count || count>(size-4)/4)return GH_STAGE_BOUNDS;
 off=u32(b+index*4);if(off<4+(size_t)count*4 || off>=size)return GH_STAGE_BOUNDS;
 for(uint32_t i=1;i<=count;i++){uint32_t next=u32(b+i*4);if(next>off && next<end)end=next;}
 *out=b+off;*bytes=end-off;return GH_STAGE_OK;
}
#define STAGE_TRY(entry,expr) do{gh_stage_status e=(expr);if(e){s->failed_entry=(entry);s->detail=(int)e;return e;}}while(0)
gh_stage_status gh_game_stage_002a5bb8(gh_stage_case1 *s){
 gh_boot_work *work;gh_bundle_entry idd;gh_menu_error me;gh_font_error fe;
 const uint8_t *vibration;size_t vibration_size;
 if(!s || !s->assets || !s->assets->work || !s->game_state || !s->textures ||
  !s->language || !s->font || !s->menu || !s->id || !s->id_global)return GH_STAGE_ARGUMENT;
 work=s->assets->work;s->failed_entry=0;s->detail=0;
 while(!(work->flags&0x2000u)){
  if(!s->assets->threads || gh_task_threads_sleep(s->assets->threads,1)!=GH_THREAD_OK){s->failed_entry=0x2d5aa0;return GH_STAGE_THREAD;}
 }
 STAGE_TRY(0x150c10,gh_stage_core_textures(s->textures,s->assets->core.pointer,s->assets->core_size));
 me=gh_id_bind_global(s->id_global,s->id);if(me){s->failed_entry=0x3c2384;s->detail=(int)me;return GH_STAGE_UNAVAILABLE;}
 me=gh_id_find_language_data(s->language,&idd);if(!me)me=gh_id_system_init_002acc70(s->id,&idd);
 if(me){s->failed_entry=0x2acc70;s->detail=(int)me;return GH_STAGE_UNAVAILABLE;}
 memset(s->work_area,0,sizeof(s->work_area)); /*2BF878 clears14A0+9C30+20*/
 STAGE_TRY(0x2bdb78,core_part(s->core_bin,s->core_bin_bytes,1,&vibration,&vibration_size));
 s->vibration_data=vibration;s->vibration_bytes=vibration_size;s->vibration_phase=0;s->vibration_flags|=1;
 fe=gh_font_initialize(s->font,s->language,work->language);if(fe){s->failed_entry=0x2a4970;s->detail=(int)fe;return GH_STAGE_UNAVAILABLE;}
 me=gh_menu_load_tables_002ae990(s->menu,s->language);if(me){s->failed_entry=0x2ae990;s->detail=(int)me;return GH_STAGE_UNAVAILABLE;}
 if(!s->init_children_15fd98){s->failed_entry=0x15fd98;return GH_STAGE_UNAVAILABLE;}
 s->detail=s->init_children_15fd98(s->context,s);if(s->detail){s->failed_entry=0x15fd98;return GH_STAGE_UNAVAILABLE;}
 if(!s->save.bytes || s->save.size<GH_CORE_SAVE_BYTES){s->failed_entry=0x1f9ff0;return GH_STAGE_ARGUMENT;}
 s->saved_block=s->save.bytes+0x180; /*2962B0 +38 hostpointer*/
 set32(s->object_state,0x14,0);memset(s->object_state+4,0,4);set32(s->object_state,8,0);set32(s->object_state,0x18,0);
 set32(s->object_state,0,0);s->object_state[0x34]=0;
 if(!s->init_cockpit_140c88){s->failed_entry=0x140c88;return GH_STAGE_UNAVAILABLE;}
 s->detail=s->init_cockpit_140c88(s->context,s);if(s->detail){s->failed_entry=0x140c88;return GH_STAGE_UNAVAILABLE;}
 /*1FFE00 genuinely contains onlyreturn; no nativeoperation fabricated. */
 {static const uint8_t offsets[]={0,1,2,3,4,5,6,7,9,10,12,13};
  for(unsigned i=0;i<sizeof(offsets);i++)s->aux_state[offsets[i]]=0;}
 STAGE_TRY(0x1fe6c8,gh_stage_slots_reset(&s->global_slots,0));
 STAGE_TRY(0x1fe6c8,gh_stage_slots_reset(&s->room_slots,s->room_heap));
 STAGE_TRY(0x2c8ef8,gh_stage_skills_initialize(&s->skills,s->core_bin,s->core_bin_bytes,work,s->save));
 *s->game_state=2;s->failed_entry=0;return GH_STAGE_OK;
}
