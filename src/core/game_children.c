/* Native readable recovery of1612C8/15FD98 and their9vtable-derivedmethods.
 * Exactmapping and unchangedbytes:game-children-evidence.json. */
#include "game_children.h"
#include <stdlib.h>
#include <string.h>
struct gh_game_children {uint8_t bytes[0xf18];const void *room_jump;
 gh_game_child_view pairs[9];gh_id_base bases[18];uint32_t count;};
static void w16(uint8_t *p,size_t o,uint16_t v){p[o]=(uint8_t)v;p[o+1]=(uint8_t)(v>>8);}
static void w32(uint8_t *p,size_t o,uint32_t v){for(unsigned i=0;i<4;i++)p[o+i]=(uint8_t)(v>>(i*8));}
static void vec0(uint8_t *p){w32(p,0,0);w32(p,4,0);w32(p,8,0);w32(p,12,0x3f800000);}
static void construct_base(uint8_t *p){
 /*2A9DC0 storesVF0 vectors;2A9DF8 replaces them with actual3BD880 vec(0,0,0,1). */
 memset(p,0,16);p[0x18]=p[0x19]=0;w16(p,0x1a,0);p[0x1c]=p[0x1d]=p[0x1e]=0;
 vec0(p+0x20);vec0(p+0x30);w16(p,0x44,0x80);p[0x46]=6;w32(p,0x40,0x3c23d70a);
}
static const uint16_t base_offsets[]={0x60,0x160,0x1e0,0x290,0x310,0x480,0x5a0,0x620,0x730,0x880,0x960,0x9c0,0xa80,0xae0,0xba0,0xc40,0xd10,0xe50};
static const uint16_t offsets[]={0x60,0x310,0x620,0x730,0x880,0x9c0,0xba0,0xc40,0xae0};
static const uint8_t kinds[]={0,1,2,3,4,5,8,7,6};
static const uint32_t vtables[]={0x41f6c8,0x41ff30,0x41f388,0x41f5e0,0x41fb20,0x41fbe8,0x41fc60,0x41fdf8,0x41f200};
static const uint32_t methods[]={0x164ef8,0x16c238,0x161bb0,0x1627a0,0x166618,0x1673b0,0x167f70,0x168528,0x161688};
static const uint16_t sizes[]={0x2b0,0x310,0x110,0x150,0x140,0x120,0xa0,0x290,0xc0};
gh_child_status gh_game_children_create(const void *room_jump,gh_game_children **out){
 gh_game_children *s;if(!out)return GH_CHILD_ARGUMENT;*out=0;s=calloc(1,sizeof(*s));if(!s)return GH_CHILD_MEMORY;
 for(unsigned i=0;i<18;i++){construct_base(s->bytes+base_offsets[i]);gh_id_base_clear_002a9df8(&s->bases[i]);}
 vec0(s->bytes+0x580);vec0(s->bytes+0x830);vec0(s->bytes+0x840);vec0(s->bytes+0xe30);
 s->room_jump=room_jump; /*2BE968 original pointer; never truncatehostaddress*/
 for(unsigned i=0;i<9;i++)w32(s->bytes,offsets[i]+0x70,vtables[i]);
 *out=s;return GH_CHILD_OK;
}
const void *gh_game_children_room_jump(const gh_game_children *s){return s?s->room_jump:0;}
void gh_game_children_destroy(gh_game_children *s){free(s);}
static void initialize_child(uint8_t *p,unsigned kind){
 /*1614D0 commonbody; its repeated derivedbyteclears have no extra effects. */
 p[0x5d]=0;w32(p,0x60,0xffffffffu);p[0x58]=p[0x59]=0;w32(p,0x50,0);p[0x64]=0;
 p[0x5a]=p[0x5b]=p[0x5c]=0;
 if(kind==0)w32(p,0x298,0);else if(kind==1)w32(p,0x264,0);else if(kind==8)p[0x90]=0;
}
gh_child_status gh_game_children_initialize_0015fd98(gh_game_children *s){
 if(!s)return GH_CHILD_ARGUMENT;
 memset(s->bytes,0,0x48);memset(s->pairs,0,sizeof(s->pairs));s->count=0;
 for(unsigned i=0;i<9;i++){
  gh_game_child_view *v=&s->pairs[s->count++];
  v->bytes=s->bytes+offsets[i];v->size=sizes[i];v->original_offset=offsets[i];v->kind=kinds[i];
  v->vtable_address=vtables[i];v->init_entry=methods[i];
  for(unsigned j=0;j<18;j++)if(base_offsets[j]==offsets[i])v->native_base=&s->bases[j];
 }
 for(unsigned i=0;i<9;i++){initialize_child(s->bytes+s->pairs[i].original_offset,s->pairs[i].kind);s->pairs[i].initialized=1;}
 w16(s->bytes,0x50,0);w32(s->bytes,0x48,0xffffffffu);w32(s->bytes,0xed0,0);s->bytes[0xed4]=0;
 for(unsigned i=0;i<9;i++)w32(s->bytes,0xf08-i*4,0xffffffffu);s->bytes[0xf15]=0;return GH_CHILD_OK;
}
gh_child_status gh_game_child_at(const gh_game_children *s,uint32_t n,gh_game_child_view *out){
 if(!s || !out)return GH_CHILD_ARGUMENT;if(n>=s->count)return GH_CHILD_BOUNDS;*out=s->pairs[n];return GH_CHILD_OK;
}
gh_child_status gh_game_children_manager_view(const gh_game_children *s,const uint8_t **data,size_t *size){
 if(!s || !data || !size)return GH_CHILD_ARGUMENT;*data=s->bytes;*size=sizeof(s->bytes);return GH_CHILD_OK;
}

gh_child_status gh_game_children_id_base(gh_game_children *s,uint32_t offset,gh_id_base **out){
 if(!s || !out)return GH_CHILD_ARGUMENT;*out=0;
 for(unsigned i=0;i<18;i++)if(base_offsets[i]==offset){*out=&s->bases[i];return GH_CHILD_OK;}
 return GH_CHILD_BOUNDS;
}
