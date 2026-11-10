#include "game_door.h"
#include <stddef.h>
#include <string.h>
_Static_assert(offsetof(gh_game_door,wait)==0,"Door wait");
_Static_assert(offsetof(gh_game_door,position_bits)==4,"Door position");
_Static_assert(offsetof(gh_game_door,target_room)==0x14,"Door room");
_Static_assert(offsetof(gh_game_door,flags)==0x18,"Door flags");
_Static_assert(offsetof(gh_game_door,phase38)==0x38,"Door phase");
_Static_assert(offsetof(gh_game_door,initialized)==0x3c,"Door scalar extent");
static uint16_t h(const uint8_t*p){return (uint16_t)((unsigned)p[0]|(unsigned)p[1]<<8);}
static uint32_t w(const uint8_t*p){return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static uint64_t dword(const uint8_t*p){return (uint64_t)w(p)|((uint64_t)w(p+4)<<32);}
static void put(uint8_t*p,uint32_t n){for(unsigned i=0;i<4;i++)p[i]=(uint8_t)(n>>(8*i));}
static void put64(uint8_t*p,uint64_t n){put(p,(uint32_t)n);put(p+4,(uint32_t)(n>>32));}
static gh_door_status fail(gh_game_door*s,uint32_t f,gh_door_status e){s->frontier=f;return e;}
typedef int (*read_fn)(void*,uint64_t,void*,size_t);
typedef struct {const uint8_t*bytes;size_t size;} memory_source;
static int read_memory(void*p,uint64_t off,void*out,size_t n){memory_source*s=p;if(off>s->size||n>s->size-off)return 1;memcpy(out,s->bytes+(size_t)off,n);return 0;}
static int read_disc(void*p,uint64_t off,void*out,size_t n){return gh_disc_read_boot_elf(p,off,out,n)!=GH_DISC_OK;}
static gh_door_status load(gh_game_door*s,read_fn read,void*ctx,uint64_t total){
 if(!s||!read)return GH_DOOR_ARGUMENT;if(s->initialized)return GH_DOOR_BUSY;
 uint8_t eh[52],ph[32],scalar[0x3c]={0};if(total<52||read(ctx,0,eh,52)||memcmp(eh,"\177ELF\1\1\1",7)||h(eh+18)!=8||h(eh+42)!=32)return GH_DOOR_FORMAT;
 unsigned count=h(eh+44);uint64_t at=w(eh+28);if(count>128||at>total||count*32u>total-at)return GH_DOOR_BOUNDS;
 for(unsigned i=0;i<count;i++){
  if(read(ctx,at+i*32,ph,32))return GH_DOOR_BOUNDS;
  uint64_t off=w(ph+4),base=w(ph+8),fs=w(ph+16),ms=w(ph+20),address=0x583ec0;
  if(w(ph)!=1||address<base||address+sizeof(scalar)>base+ms)continue;
  if(fs>ms||off>total||fs>total-off)return GH_DOOR_BOUNDS;
  uint64_t delta=address-base;if(delta<fs){uint64_t n=fs-delta;if(n>sizeof(scalar))n=sizeof(scalar);if(read(ctx,off+delta,scalar,(size_t)n))return GH_DOOR_BOUNDS;}
  memset(s,0,sizeof(*s));memcpy(s,scalar,sizeof(scalar));s->initialized=1;return GH_DOOR_OK;
 }return GH_DOOR_BOUNDS;
}
gh_door_status gh_game_door_from_elf(gh_game_door*s,const uint8_t*b,size_t n){if(!b)return GH_DOOR_ARGUMENT;memory_source src={b,n};return load(s,read_memory,&src,n);}
gh_door_status gh_game_door_from_disc(gh_game_door*s,gh_disc*d){if(!d)return GH_DOOR_ARGUMENT;return load(s,read_disc,d,gh_disc_boot_elf_size(d));}
gh_door_status gh_game_door_reset_001ffe08(gh_game_door*s){if(!s||!s->initialized)return GH_DOOR_ARGUMENT;s->wait=0;return GH_DOOR_OK;}
/* 31AFA0 ->31B410. Its first parameter is unused; Work+178 is the genuine
 * input destination, not a new pad owner. All loads/stores are 64/8 bit. */
static void stop_input(gh_boot_work*work){uint8_t*p=(uint8_t*)work+0x178;const uint64_t mask=UINT64_C(0x000f00807ff00000);
 put64(p+64,dword(p+64)&mask);put64(p+56,dword(p+56)&mask);
 put64(p,dword(p+24)&mask);put64(p+40,dword(p+40)&mask);put64(p+48,dword(p+48)&mask);
 p[0x6b]=0x80;p[0x7b]=0;p[0x68]=p[0x69]=p[0x6a]=0x80;
 p[0x6f]=p[0x6e]=p[0x6c]=p[0x6d]=0;memset(p+0x70,0,12);
 uint8_t*r=(uint8_t*)work;put(r+0x5d8,w(r+0x5d8)|0x2000000);
}
static gh_door_status kill_messages(gh_game_door*s,gh_game_scene_ui*m){
 if(!m||!m->ready||!m->root)return fail(s,0x2aecd0,GH_DOOR_UNSUPPORTED);
 int indices[16],count=0,i=m->active_head;uint32_t seen=0;
 while(i!=-1){if(i<0||i>=16||(seen&(1u<<(unsigned)i)))return fail(s,0x2aecd0,GH_DOOR_BOUNDS);seen|=1u<<(unsigned)i;indices[count++]=i;i=m->blocks[i].next;}
 /* KillAll requests asynchronous state6 teardown. It does not unlink or free;
  * those operations belong to the later original Menu update consumer. */
 for(int k=0;k<count;k++){uint8_t*p=m->blocks[indices[k]].payload;put(p+8,w(p+8)|0x40);if((uint32_t)p[0]-6u>=2){p[3]=0;p[0]=6;p[1]=p[2]=0;}}
 return GH_DOOR_OK;
}
static gh_door_status fade(gh_game_door*s,const gh_game_door_bindings*b,int reverse){
 uint32_t flags=s->flags;if(!(flags&3))return GH_DOOR_OK;
 if(!b->fade)return fail(s,0x307208,GH_DOOR_UNSUPPORTED);
 if(!b->display||!b->display->initialized)return fail(s,0x3c1180,GH_DOOR_UNSUPPORTED);
 if(!reverse&&!b->scene_step)return fail(s,0x20003c,GH_DOOR_ARGUMENT);
 const uint8_t*w0=(uint8_t*)b->work;gh_screen_fade_context c={b->display->screen_width,b->display->gs_y_origin,w(w0+0x564),w(w0+0x5e0)};
 uint32_t from=flags&1?0x00a0a0a0:0,to=flags&1?0x80a0a0a0:0xff000000;if(reverse){uint32_t n=from;from=to;to=n;}
 if(gh_screen_fade_start_00307208(b->fade,&c,0,reverse?8:10,0,from,to,reverse?15:30))return fail(s,0x307208,GH_DOOR_UNSUPPORTED);
 if(!reverse)b->scene_step->cooldown_1b0=30;return GH_DOOR_OK;
}
static gh_door_status external(gh_game_door*s,const gh_game_door_bindings*b,uint32_t address){
 if(!b->external)return fail(s,address,GH_DOOR_UNSUPPORTED);uint32_t f=0;gh_door_status e=b->external(b->context,address,&f);return e?fail(s,f?f:address,e):GH_DOOR_OK;
}
static gh_door_status begin(gh_game_door*s,const gh_game_door_bindings*b){
 uint8_t*p=(uint8_t*)b->work;gh_door_status e;
 if(!s->begin_stage){if(w(p+0x5e4)&0x2000000){s->frontier=0;return GH_DOOR_OK;}s->begin_stage=1;}
 if(s->begin_stage==1){if(w(p+0x584)&0x100)put(p+0x584,w(p+0x584)&~0x100u);else if((e=kill_messages(s,b->messages)))return e;s->begin_stage=2;}
 if(s->begin_stage==2){if((e=fade(s,b,0)))return e;s->begin_stage=3;}
 if(s->begin_stage==3){s->phase38=s->verify_phase39=0;put(p+0x5e4,w(p+0x5e4)|0x2000000);stop_input(b->work);s->begin_stage=4;}
 if(s->begin_stage==4){put(p+0x5e0,w(p+0x5e0)|0x100000);put(p+0x5d8,w(p+0x5d8)|0x4d00000);
  if(!b->scenario.manager)return fail(s,0x2000a8,GH_DOOR_ARGUMENT);b->scenario.manager->flags|=1u;s->begin_stage=5;}
 if(b->work->room!=0x20){
  if(s->begin_stage==5){if((e=external(s,b,0x12ac60)))return e;s->begin_stage=6;}
  if(s->begin_stage==6){if((e=external(s,b,0x12b9f0)))return e;s->begin_stage=7;}
  if(!b->player||!b->player->ready||!b->player->view.bytes||b->player->view.size<0x550)return fail(s,0x2db6b0,GH_DOOR_UNSUPPORTED);
  put(b->player->view.bytes+0x54c,0x447a0000);
 }
 s->begin_stage=0;s->frontier=0;return GH_DOOR_OK;
}
gh_door_status gh_game_door_request_001ffeb8(gh_game_door*s,const gh_game_door_bindings*b,uint16_t room,const float position[3],float yaw,uint8_t kind,uint32_t flags,uint8_t force){
 if(!s||!s->initialized||!b||!b->work||!position)return GH_DOOR_ARGUMENT;
 uint16_t target=room==b->work->room?UINT16_MAX:room;memcpy(&s->target_room,&target,2);memcpy(s->position_bits,position,12);memcpy(&s->yaw_bits,&yaw,4);s->point=0xff;s->kind=kind;s->flags=flags;s->force3a=force;
 return begin(s,b);
}
static int reset_clock_room(uint16_t room){return room==0x100||room==0x200||room==0x300||room==0x400||room==0x500||room==0x600||room==0x700||room==0x802||room==0x806;}
gh_door_status gh_game_door_step_001ffe10(gh_game_door*s,const gh_game_door_bindings*b){
 if(!s||!s->initialized||!b||!b->work)return GH_DOOR_ARGUMENT;uint8_t*p=(uint8_t*)b->work;gh_door_status e;
 if(!s->step_stage){if(s->wait)s->wait=(uint16_t)(s->wait-1u);s->step_stage=1;}
 if(!(w(p+0x5e4)&0x2000000)&&s->step_stage<5){s->step_stage=0;s->frontier=0;return GH_DOOR_OK;}
 if(!b->fade)return fail(s,0x74748c,GH_DOOR_UNSUPPORTED);
 if(s->step_stage==1){if(b->fade->state&&!(b->fade->state&4)){s->step_stage=0;s->frontier=0;return GH_DOOR_OK;}
  put(p+0x5d8,w(p+0x5d8)|0x10000);
  if(!(w(p+0x598)&0x40)&&!s->force3a&&s->target_room!=-1&&s->kind!=1)return fail(s,0x2006b8,GH_DOOR_UNSUPPORTED);
  put(p+0x5e4,w(p+0x5e4)&~0x20u);stop_input(b->work);s->step_stage=2;
 }
 if(s->target_room==-1){
  /* Same-room reposition requires actual player/scenario transform callbacks;
   * an empty actor list cannot prove those operations unnecessary. */
  if(s->phase38==0)return fail(s,0x2c4440,GH_DOOR_UNSUPPORTED);
  if(s->phase38==1){
   if(s->step_stage!=5){put(p+0x5e4,w(p+0x5e4)&~0x2000000u);if(!b->scenario.manager)return fail(s,0x200158,GH_DOOR_ARGUMENT);b->scenario.manager->flags&=~1u;memset(p+0x5d8,0,8);put(p+0x5e0,w(p+0x5e0)&~0x100000u);s->step_stage=5;}
   if(!b->player||!b->player->ready||!b->player->view.bytes||b->player->view.size<0x550)return fail(s,0x2db6b0,GH_DOOR_UNSUPPORTED);
   put(b->player->view.bytes+0x54c,0x41f00000);
  }
  s->step_stage=0;s->frontier=0;return GH_DOOR_OK;
 }
 uint16_t room;memcpy(&room,&s->target_room,2);
 if(s->step_stage==2){if(!b->loop)return fail(s,0x2002cc,GH_DOOR_ARGUMENT);
  b->loop->game_state[0]=5;b->loop->game_state[3]=b->loop->game_state[1]=b->loop->game_state[2]=0;
  p[0x5b4]=(uint8_t)room;p[0x5b5]=(uint8_t)(room>>8);p[0x5b8]=s->kind;put(p+0x5e4,w(p+0x5e4)|0x8000);
  put(p+0x5c0,s->position_bits[0]);put(p+0x5c8,s->position_bits[2]);put(p+0x5d8,w(p+0x5d8)&0xffafffff);put(p+0x5e0,w(p+0x5e0)&~0x100000u);put(p+0x5c4,s->position_bits[1]);put(p+0x5d0,s->yaw_bits);p[0x5ba]=s->point;s->step_stage=3;
 }
 if(s->step_stage==3){if(reset_clock_room(room)){if(!b->prepare)return fail(s,0x2d9d48,GH_DOOR_ARGUMENT);b->prepare->global_0061a994=0;}s->step_stage=4;}
 if(b->fade->state&&!(s->flags&0x80000000)){if((e=fade(s,b,1)))return e;}
 s->step_stage=0;s->frontier=0;return GH_DOOR_OK;
}
