/* Native readable recovery of1612C8/15FD98 and their9vtable-derivedmethods.
 * Exactmapping and unchangedbytes:game-children-evidence.json. */
#include "game_children.h"
#include <stdlib.h>
#include <string.h>
_Static_assert(offsetof(gh_boot_work,system_flags)==0x58c,"747A2C Work58C");
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
 return gh_game_children_room_reset_0015fee0(s);
}
gh_child_status gh_game_children_room_reset_0015fee0(gh_game_children*s){
 if(!s)return GH_CHILD_ARGUMENT;for(unsigned i=0;i<9;i++)w32(s->bytes,0xf08-i*4,0xffffffffu);s->bytes[0xf15]=0;return GH_CHILD_OK;
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

static uint32_t r32(const uint8_t*p){return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static uint16_t r16(const uint8_t*p){return (uint16_t)(p[0]|(unsigned)p[1]<<8);}
static gh_child_status child_stop(uint32_t*f,uint32_t pc,gh_child_status e){if(f)*f=pc;return e;}
gh_child_status gh_game_children_active_00161288(const gh_game_children*s,const gh_game_child_view**out){
 if(!s||!out)return GH_CHILD_ARGUMENT;*out=NULL;if(s->count!=9)return GH_CHILD_BOUNDS;
 uint32_t kind=r32(s->bytes+0x48);if(kind==UINT32_MAX)return GH_CHILD_OK;
 for(unsigned i=0;i<s->count;i++)if(s->pairs[i].kind==kind){*out=&s->pairs[i];break;}return GH_CHILD_OK;
}
gh_child_status gh_game_children_alpha_step_00160c48(gh_game_children*s,uint32_t*f){
 if(f)*f=0;if(!s)return GH_CHILD_ARGUMENT;uint8_t*p=s->bytes;unsigned phase=p[0xed4];
 if(phase==0||phase==1)return GH_CHILD_OK;
 uint32_t count=r32(p+0xed8);
 if(phase==2||phase==3){
  if(count>4)return child_stop(f,0x160c48,GH_CHILD_UNSUPPORTED);
  uint32_t alpha=phase==2?count*32:128-count*32;
  w32(p,0xed0,(r32(p+0xed0)&0x00ffffffu)|(alpha<<24));
  if(count==4)p[0xed4]=(uint8_t)(phase==2?0:1);
 }
 w32(p,0xed8,count+1);return GH_CHILD_OK;
}
gh_child_status gh_game_children_move_001601d8(gh_game_children*s,gh_boot_work*work,uint32_t*f){
 const gh_game_child_view*active=NULL;if(f)*f=0;if(!s||!work)return GH_CHILD_ARGUMENT;
 uint8_t*raw=(uint8_t*)work,*p=s->bytes;
 /* ActualLW747A2C = Work+58C, distinctfromflags584/systemflags5D8. */
 if(work->system_flags&0x80000000u)return GH_CHILD_OK;
 gh_child_status e=gh_game_children_active_00161288(s,&active);if(e)return e;
 if(active&&active->bytes[0x58])return GH_CHILD_OK;
 uint16_t state=r16(p+0x50);
 switch(state){
 case 0:break;
 case 2:if(active)return child_stop(f,0x1601d8,GH_CHILD_UNSUPPORTED);w16(p,0x50,3);break;
 case 3:w16(p,0x50,4);break;
 case 5:
  if(!(r32(raw+0x5dc)&0x100000u)&&active)return child_stop(f,0x1601d8,GH_CHILD_UNSUPPORTED);
  break;
 case 7:
  if(r32(p+0x4c)==0xb)work->flags|=0x8000;
  if(active)return child_stop(f,0x1601d8,GH_CHILD_UNSUPPORTED);
  w32(p,0x48,r32(p+0x4c));break;
 case 9:
  if(active)return child_stop(f,0x1601d8,GH_CHILD_UNSUPPORTED);
  w32(raw,0x5e0,r32(p+0xee4));w32(p,0x48,UINT32_MAX);w16(p,0x50,0x15);break;
 default:
  if(state<0x16)return child_stop(f,0x1601d8,GH_CHILD_UNSUPPORTED);
  break; /* actualout-of-range state also visits160C48 */
 }
 e=gh_game_children_alpha_step_00160c48(s,f);if(e)return e;
 if(work->flags&0x20000u)return child_stop(f,0x2c7850,GH_CHILD_UNSUPPORTED);
 return GH_CHILD_OK;
}

gh_child_status gh_game_children_cover_00160d50(const gh_game_children*s,gh_render_draw_env*env,gh_child_cover_submit submit,void*context,uint32_t*f){
 if(f)*f=0;if(!s||!env)return GH_CHILD_ARGUMENT;
 if(!env->ready)return child_stop(f,0x2c54e8,GH_CHILD_UNSUPPORTED);
 if(!submit)return child_stop(f,0x15b880,GH_CHILD_UNSUPPORTED);
 const gh_child_cover_quad quad={{{-1,-1,0,1},{513,-1,0,1},{-1,449,0,1},{513,449,0,1}},r32(s->bytes+0xed0),1,6,5,1,UINT64_C(0x8000000044),UINT64_C(0x3001d)};
 int emitted=-1;uint32_t pc=0;gh_child_status e=submit(context,&quad,&emitted,&pc);
 if(e)return child_stop(f,pc?pc:0x15b880,e);
 if(emitted!=0&&emitted!=1)return GH_CHILD_ARGUMENT;
 if(gh_render_draw_init_002c54e8(env)!=GH_RENDER_STEP_OK)return child_stop(f,0x2c54e8,GH_CHILD_UNSUPPORTED);
 env->scalar.bank=7;env->scalar.depth=1;return GH_CHILD_OK;
}
gh_child_status gh_game_children_draw_00160b90(const gh_game_children*s,const gh_boot_work*w,gh_render_draw_env*env,gh_child_cover_submit submit,void*context,uint32_t*f){
 if(f)*f=0;if(!s||!w)return GH_CHILD_ARGUMENT;
 if((w->system_flags&0x80000000u)||(r32((const uint8_t*)w+0x5dc)&0x100000u))return GH_CHILD_OK;
 gh_child_status e=gh_game_children_cover_00160d50(s,env,submit,context,f);if(e)return e;
 const gh_game_child_view*active=NULL;e=gh_game_children_active_00161288(s,&active);if(e)return e;
 if(!active||active->bytes[0x59])return GH_CHILD_OK;
 uint16_t state=r16(s->bytes+0x50);
 if(state==4||state==5||state==6||state==8||state==10||state==14||state==17){
  if(r32(active->bytes+0x70)!=active->vtable_address)return child_stop(f,0x160c20,GH_CHILD_UNSUPPORTED);
  static const uint32_t targets[]={0x165e88,0x16dc18,0x162278,0x1640e8,0x166b60,0x1678b8,0x1682a8,0x16acd0,0x161970};
  for(unsigned i=0;i<s->count;i++)if(active==&s->pairs[i])return child_stop(f,targets[i],GH_CHILD_UNSUPPORTED);
  return child_stop(f,0x160c20,GH_CHILD_UNSUPPORTED);
 }
 return GH_CHILD_OK;
}

gh_child_status gh_game_children_quit_001612a8(gh_game_children*s,gh_boot_work*w,uint32_t*f){
 if(f)*f=0;if(!s||!w)return GH_CHILD_ARGUMENT;
 if(r32((const uint8_t*)w+0x5e4)&0x8000000)return child_stop(f,0x15ff10,GH_CHILD_UNSUPPORTED);
 return GH_CHILD_OK;
}
