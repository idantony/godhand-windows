/* Actual ELF374B70/375988 mapping and374CB8/375AB0 lookup semantics.
 * Host-owned registry replaces platform RPC commits; no IOP reply invented. */
#include "audio_startup.h"
#include <stdlib.h>
#include <string.h>
#define BANKS 48 /* original registry initialization clears0x600 / stride0x20 */
typedef struct directory {uint32_t count;uint32_t *pairs;} directory;
typedef struct bank {uint8_t *bytes;size_t size;gh_sound_bank view;
 uint64_t generation;uint32_t count,table,event_count,event_table;
 uint32_t flags;directory afs;
} bank;
struct gh_audio_registry {bank banks[BANKS];uint64_t next;
 uint8_t *bgm_table;size_t bgm_size;directory bgm_afs;uint32_t bgm_flags;};
static uint32_t le(const uint8_t *p){return (uint32_t)p[0]|(uint32_t)p[1]<<8|
 (uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static int valid_id(int16_t id){return id>=0 && id<BANKS;}
static int fits(size_t size,uint32_t off,size_t n){return off<=size && n<=size-off;}
static const bank *get(const gh_audio_registry *r,int16_t id){
 return r && valid_id(id) && r->banks[id].bytes?&r->banks[id]:0;}
gh_audio_status gh_audio_registry_create(gh_audio_registry **out){
 if(!out)return GH_AUDIO_ARGUMENT;*out=calloc(1,sizeof(**out));
 return *out?GH_AUDIO_OK:GH_AUDIO_MEMORY;
}
void gh_audio_registry_destroy(gh_audio_registry *r){
 if(r){for(unsigned i=0;i<BANKS;i++){free(r->banks[i].bytes);free(r->banks[i].afs.pairs);}
 free(r->bgm_table);free(r->bgm_afs.pairs);free(r);}
}
gh_audio_status gh_audio_commit_se(gh_audio_registry *r,int16_t id,const void *ses,
 size_t size,gh_audio_token *token){
 bank b={0};gh_sound_span prog,sys,samples;gh_sound_status e;
 if(token){token->generation=0;token->bank=-1;token->owner=0;}
 if(!r || !ses || !token || !valid_id(id))return GH_AUDIO_ARGUMENT;
 if(r->next==UINT64_MAX)return GH_AUDIO_UNSUPPORTED;
 /* Snapshot first: every validation and view uses owned bytes. */
 if(size>64u*1024u*1024u)return GH_AUDIO_BOUNDS;
 b.bytes=malloc(size?size:1);if(!b.bytes)return GH_AUDIO_MEMORY;
 memcpy(b.bytes,ses,size);b.size=size;
 e=gh_sound_bank_open(b.bytes,size,&b.view);
 if(e!=GH_SOUND_OK){free(b.bytes);return e==GH_SOUND_BOUNDS?GH_AUDIO_BOUNDS:GH_AUDIO_FORMAT;}
 prog=b.view.sections[0];sys=b.view.sections[2];samples=b.view.sections[3];
 {gh_sound_span afd=b.view.sections[1];
  if(afd.size<16 || afd.data[12] || le(afd.data)>65536 ||
     !fits(afd.size,le(afd.data+4),(size_t)le(afd.data)*32))goto bad;
 }
 if(prog.size<32)goto bad;
 b.count=le(prog.data);b.table=le(prog.data+4);
 b.event_count=le(prog.data+20);b.event_table=le(prog.data+24);
 /* Lookup routines compare SIGNED16 counts. Reject ambiguous wider values. */
 if(b.count>32767 || b.event_count>32767)goto bad;
 if(!fits(prog.size,b.table,(size_t)b.count*48) ||
    !fits(prog.size,b.event_table,(size_t)b.event_count*48))goto bad;
 /* Optional relative pointers used by both commit commands. */
 for(unsigned i=16;i<=28;i+=4)if(i!=20){
  uint32_t off=le(prog.data+i);if(off && off>=prog.size)goto bad;
 }
 if(sys.size%16)goto bad;
 for(size_t i=0;i<sys.size;i+=16)if(le(sys.data+i)>=samples.size)goto bad;
 /* All committed pointers now resolve to own image; transfer copies and both
  * table registrations become visible together. Playback remains separate. */
 b.generation=++r->next;b.flags=0x77; /* directory completion8 still absent */
 free(r->banks[id].bytes);free(r->banks[id].afs.pairs);r->banks[id]=b;
 token->bank=id;token->generation=b.generation;token->owner=r;return GH_AUDIO_OK;
bad:free(b.bytes);return GH_AUDIO_BOUNDS;
}
int gh_audio_token_complete(const gh_audio_registry *r,gh_audio_token t){
 const bank *b=get(r,t.bank);return b && t.owner==r && t.generation && b->generation==t.generation;
}
gh_audio_status gh_audio_release_bank(gh_audio_registry *r,int16_t id){
 if(!r || !valid_id(id))return GH_AUDIO_ARGUMENT;
 free(r->banks[id].bytes);free(r->banks[id].afs.pairs);memset(&r->banks[id],0,sizeof(bank));return GH_AUDIO_OK;
}
gh_audio_status gh_audio_get_program(const gh_audio_registry *r,int16_t id,
 int32_t index,int event,uint8_t out[48]){
 const bank *b=get(r,id);const uint8_t *p;uint32_t count,table;
 if(!r || !out || !valid_id(id) || (event!=0 && event!=1))return GH_AUDIO_ARGUMENT;
 if(!b)return GH_AUDIO_NOT_FOUND;
 count=event?b->event_count:b->count;table=event?b->event_table:b->table;
 if(index<0 || (uint32_t)index>=count)return GH_AUDIO_BOUNDS;
 p=b->view.sections[0].data+table+(size_t)index*48;
 if(p[15]&0x80)return GH_AUDIO_DISABLED;
 memcpy(out,p,48);out[0]=(uint8_t)id;out[1]=(uint8_t)((uint16_t)id>>8);
 out[2]=(uint8_t)index;out[3]=(uint8_t)((uint32_t)index>>8);return GH_AUDIO_OK;
}
gh_audio_status gh_audio_bank_views(const gh_audio_registry *r,int16_t id,
 gh_sound_span *programs,gh_sound_span *sys,gh_sound_span *samples,gh_sound_span *afd){
 const bank *b=get(r,id);
 if(!r || !valid_id(id) || !programs || !sys || !samples || !afd)return GH_AUDIO_ARGUMENT;
 if(!b)return GH_AUDIO_NOT_FOUND;
 *programs=b->view.sections[0];*afd=b->view.sections[1];
 *sys=b->view.sections[2];*samples=b->view.sections[3];return GH_AUDIO_OK;
}
gh_audio_status gh_audio_sample_at(const gh_audio_registry *r,int16_t id,
 uint32_t slot,gh_sound_span *descriptor,gh_sound_span *tail){
 const bank *b=get(r,id);gh_sound_span sys,data;uint32_t off;
 if(!r || !descriptor || !tail || !valid_id(id))return GH_AUDIO_ARGUMENT;
 if(!b)return GH_AUDIO_NOT_FOUND;
 sys=b->view.sections[2];data=b->view.sections[3];
 if(slot>=sys.size/16)return GH_AUDIO_BOUNDS;
 descriptor->data=sys.data+(size_t)slot*16;descriptor->size=16;
 off=le(descriptor->data);tail->data=data.data+off;tail->size=data.size-off;
 return GH_AUDIO_OK;
}

static gh_audio_status whole(gh_disc *d,const char *path,uint8_t **out,size_t *size){
 gh_disc_entry e;size_t i;uint8_t *bytes;
 *out=0;
 if(gh_disc_find(d,path,&i)!=GH_DISC_OK || gh_disc_entry_at(d,i,&e)!=GH_DISC_OK)
  return GH_AUDIO_NOT_FOUND;
 if(e.size>64u*1024u*1024u)return GH_AUDIO_BOUNDS;
 bytes=malloc(e.size?(size_t)e.size:1);if(!bytes)return GH_AUDIO_MEMORY;
 if(gh_disc_read(d,i,0,bytes,(size_t)e.size)!=GH_DISC_OK){free(bytes);return GH_AUDIO_FORMAT;}
 *out=bytes;*size=(size_t)e.size;return GH_AUDIO_OK;
}
static gh_audio_status read_directory(gh_disc *d,const char *path,directory *out){
 uint8_t head[8],*raw;uint32_t count,*pairs;size_t i;gh_disc_entry e;
 if(gh_disc_find(d,path,&i)!=GH_DISC_OK || gh_disc_entry_at(d,i,&e)!=GH_DISC_OK)
  return GH_AUDIO_NOT_FOUND;
 if(e.size<8 || gh_disc_read(d,i,0,head,8)!=GH_DISC_OK)return GH_AUDIO_BOUNDS;
 if(memcmp(head,"AFS\0",4))return GH_AUDIO_FORMAT;
 count=le(head+4);
 if(count>65536 || (uint64_t)count*8+8>e.size)return GH_AUDIO_BOUNDS;
 raw=malloc(count?(size_t)count*8:1);pairs=calloc(count?count:1,2*sizeof(uint32_t));
 if(!raw || !pairs){free(raw);free(pairs);return GH_AUDIO_MEMORY;}
 if(gh_disc_read(d,i,8,raw,(size_t)count*8)!=GH_DISC_OK){free(raw);free(pairs);return GH_AUDIO_FORMAT;}
 for(uint32_t j=0;j<count;j++){
  uint32_t off=le(raw+(size_t)j*8),size=le(raw+(size_t)j*8+4);
  /* Keep actual byte offsets plus sizes; compact EE sector sizes are derivable
   * without silently losing tail padding or inventing payload residency. */
  if(off<(uint64_t)count*8+8 || off>e.size || size>e.size-off){free(raw);free(pairs);return GH_AUDIO_BOUNDS;}
  pairs[(size_t)j*2]=off;pairs[(size_t)j*2+1]=size;
 }
 free(raw);out->count=count;out->pairs=pairs;return GH_AUDIO_OK;
}
static int afd_valid(gh_sound_span afd){
 uint32_t count,off;if(afd.size<16)return 0;
 count=le(afd.data);off=le(afd.data+4);
 return !afd.data[12] && count<=65536 && fits(afd.size,off,(size_t)count*32);
}
static int afd_links_valid(gh_sound_span afd,const directory *dir){
 uint32_t count=le(afd.data),off=le(afd.data+4);
 for(uint32_t i=0;i<count;i++){
  const uint8_t *row=afd.data+off+(size_t)i*32;
  uint32_t member=(uint32_t)row[2]|(uint32_t)row[3]<<8;
  if(member>=dir->count)return 0;
 }
 return 1;
}
gh_audio_status gh_audio_startup_load(gh_audio_registry *r,gh_disc *disc){
 gh_audio_status e;uint8_t *bytes;size_t size;gh_audio_token token;
 if(!r || !disc)return GH_AUDIO_ARGUMENT;
 r->bgm_flags=0;
 /* Original release/load/wait for core, then release/load/wait for pl00. */
 for(unsigned i=0;i<2;i++){
  int16_t id=i?3:0;bank *b;directory dir={0};
  e=whole(disc,i?"se/pl00.ses":"se/core.ses",&bytes,&size);if(e)return e;
  e=gh_audio_commit_se(r,id,bytes,size,&token);free(bytes);if(e)return e;
  b=&r->banks[id];
  if(!b->view.external_afs || !afd_valid(b->view.sections[1]))return GH_AUDIO_FORMAT;
  e=read_directory(disc,b->view.external_afs,&dir);if(e)return e;
  if(!afd_links_valid(b->view.sections[1],&dir)){free(dir.pairs);return GH_AUDIO_BOUNDS;}
  b->afs=dir;b->flags|=8;
  /* Native ownership replaces guest heap shrink; pointers remain valid rather
   * than setting original flag80 after an unperformed realloc. */
 }
 /* Original BGM0 Reset, LoadNumbered(0,0), then directory-ready poll. */
 e=whole(disc,"bgm/bgm00.afd",&bytes,&size);if(e)return e;
 {gh_sound_span table={bytes,size};directory dir={0};
  if(!afd_valid(table)){free(bytes);return GH_AUDIO_FORMAT;}
  e=read_directory(disc,"bgm/bgm00.afs",&dir);
  if(e){free(bytes);return e;}
  if(!afd_links_valid(table,&dir)){free(bytes);free(dir.pairs);return GH_AUDIO_BOUNDS;}
  free(r->bgm_table);free(r->bgm_afs.pairs);
  r->bgm_table=bytes;r->bgm_size=size;r->bgm_afs=dir;r->bgm_flags=0xf;
 }
 return GH_AUDIO_OK;
}
int gh_audio_startup_ready(const gh_audio_registry *r,uint32_t *core,uint32_t *pl00,uint32_t *bgm){
 if(!r)return 0;
 if(core)*core=r->banks[0].flags;if(pl00)*pl00=r->banks[3].flags;if(bgm)*bgm=r->bgm_flags;
 return (r->banks[0].flags&0x7f)==0x7f && (r->banks[3].flags&0x7f)==0x7f && r->bgm_flags==0xf;
}
gh_audio_status gh_audio_afs_member(const gh_audio_registry *r,int16_t id,
 uint32_t index,uint64_t *off,uint64_t *size){
 const directory *dir;
 if(!r || !off || !size || (id!=-1 && !valid_id(id)))return GH_AUDIO_ARGUMENT;
 dir=id==-1?&r->bgm_afs:&r->banks[id].afs;
 if(!dir->pairs)return GH_AUDIO_NOT_FOUND;
 if(index>=dir->count)return GH_AUDIO_BOUNDS;
 *off=dir->pairs[(size_t)index*2];*size=dir->pairs[(size_t)index*2+1];return GH_AUDIO_OK;
}
gh_audio_status gh_audio_bgm0_table(const gh_audio_registry *r,gh_sound_span *out){
 if(!r || !out)return GH_AUDIO_ARGUMENT;
 if(!r->bgm_table || r->bgm_flags!=0xf)return GH_AUDIO_NOT_FOUND;
 out->data=r->bgm_table;out->size=r->bgm_size;return GH_AUDIO_OK;
}

gh_audio_status gh_audio_external_sample(const gh_audio_registry *r,int16_t id,
 uint32_t index,gh_sound_span *row,uint64_t *offset,uint64_t *size){
 gh_sound_span afd;const bank *b;uint32_t count,off,member;
 if(!r || !row || !offset || !size || (id!=-1 && !valid_id(id)))return GH_AUDIO_ARGUMENT;
 if(id==-1){if(r->bgm_flags!=0xf)return GH_AUDIO_NOT_FOUND;afd.data=r->bgm_table;afd.size=r->bgm_size;}
 else {b=get(r,id);if(!b || !b->afs.pairs)return GH_AUDIO_NOT_FOUND;afd=b->view.sections[1];}
 if(!afd_valid(afd))return GH_AUDIO_FORMAT;
 count=le(afd.data);off=le(afd.data+4);if(index>=count)return GH_AUDIO_BOUNDS;
 row->data=afd.data+off+(size_t)index*32;row->size=32;
 member=(uint32_t)row->data[2]|(uint32_t)row->data[3]<<8;
 return gh_audio_afs_member(r,id,member,offset,size);
}
