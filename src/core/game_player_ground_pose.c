#include "game_player_ground_pose.h"
#include <string.h>
#include <math.h>
#include <fenv.h>
static uint32_t word(const uint8_t*p){uint32_t n;memcpy(&n,p,4);return n;}
static unsigned half(const uint8_t*p){return p[0]|(unsigned)p[1]<<8;}
static float number(const uint8_t*p){float n;memcpy(&n,p,4);return n;}
static void store(uint8_t*p,uint32_t n){memcpy(p,&n,4);}
static float add(float a,float b){volatile float x=a+b;return x;}
static float mul(float a,float b){volatile float x=a*b;return x;}
static gh_player_ground_status fail(gh_player_ground_pose_call*c,uint32_t pc,gh_player_ground_status e){c->frontier=pc;return e;}
gh_player_ground_status gh_player_ground_globals_from_elf(gh_player_ground_globals*g,const void*raw,size_t bytes){
 if(!g||!raw)return GH_GROUND_ARGUMENT;if(g->loaded)return GH_GROUND_BUSY;const uint8_t*b=raw;
 if(bytes<52||memcmp(b,"\177ELF\1\1\1",7)||half(b+18)!=8)return GH_GROUND_ARGUMENT;
 uint32_t table=word(b+28);unsigned stride=half(b+42),count=half(b+44);if(stride<32||count>128||table>bytes||count>(bytes-table)/stride)return GH_GROUND_BOUNDS;
 unsigned found=0;float value=0;
 for(unsigned i=0;i<count;i++){
  const uint8_t*p=b+table+(size_t)i*stride;if(word(p)!=1)continue;
  uint32_t offset=word(p+4),base=word(p+8),files=word(p+16),memory=word(p+20);
  if(files>memory||offset>bytes||files>bytes-offset||(uint64_t)base+memory>UINT64_C(0x100000000))return GH_GROUND_BOUNDS;
  if(base<=0x3bd470&&(uint64_t)0x3bd474<=(uint64_t)base+files){if(found++)return GH_GROUND_ARGUMENT;value=number(b+offset+(0x3bd470-base));}
 }
 if(found!=1)return GH_GROUND_BOUNDS;if(!isfinite(value))return GH_GROUND_NUMERIC;g->tilt=value;g->loaded=1;return GH_GROUND_OK;
}
static gh_player_ground_status body(gh_player_ground_pose_call*c,const gh_title_player_view*v){
 uint8_t*p=v->bytes;
 if(c->stage==0){uint32_t flags=word(p+0x258)&~UINT32_C(0x80000000),state=word(p+0x15f4);if((half(p+0x3ac)&0x404)||(state&0x80))flags|=UINT32_C(0x80000000);flags&=~UINT32_C(0x40000000);if(state&0x40000)flags|=UINT32_C(0x40000000);store(p+0x258,flags);c->stage=1;}
 if(c->stage==1){
  if(!p[0x2b4]){c->complete=1;return GH_GROUND_OK;}
  gh_actor_model*m=c->model;if(!m)return fail(c,0x109674,GH_GROUND_UNSUPPORTED);
  if(!m->parts_ready||m->root.bytes!=p||m->part_count!=p[0x2b4]||!m->parts[0].bytes||!m->parts[0].alias_d0)return fail(c,0x109674,GH_GROUND_ARGUMENT);
  c->stage=2;
 }
 if(c->stage==2){
  float n[3];for(unsigned k=0;k<3;k++){n[k]=number(p+0x480+k*4);if(!isfinite(n[k]))return fail(c,0x101e28,GH_GROUND_NUMERIC);}
  float length=sqrtf(add(add(mul(n[0],n[0]),mul(n[1],n[1])),mul(n[2],n[2])));if(!isfinite(length))return fail(c,0x101e28,GH_GROUND_NUMERIC);
  if(length>0){volatile float inverse=1/length;for(unsigned k=0;k<3;k++)n[k]=mul(n[k],inverse);}else memset(n,0,sizeof(n));
  memcpy(p+0x480,n,12);c->target=mul(mul(add(1,-n[1]),1.2f),.5f);if(c->target>.06f)c->target=.06f;if(half(p+0x3ac)&0xc)c->target=0;c->stage=3;
 }
 if(c->stage==3){
  gh_player_ground_globals*g=c->globals;if(!g||!g->loaded)return fail(c,0x109778,GH_GROUND_UNSUPPORTED);
  float y=number(c->model->parts[0].alias_d0+4);if(!isfinite(g->tilt)||!isfinite(y))return fail(c,0x1097ac,GH_GROUND_NUMERIC);
  float next=add(mul(g->tilt,.85f),mul(c->target,.15f));if(next<0)next=0;float shifted=add(y,-next);if(!isfinite(next)||!isfinite(shifted))return fail(c,0x1097b4,GH_GROUND_NUMERIC);
  g->tilt=next;memcpy(c->model->parts[0].alias_d0+4,&shifted,4);c->stage=4;c->complete=1;
 }return GH_GROUND_OK;
}
gh_player_ground_status gh_player_ground_pose_00108ab0(gh_player_ground_pose_call*c,const gh_title_player_view*v,gh_actor_model*m,gh_player_ground_globals*g){
 if(!c||!v||!v->bytes||v->size<GH_TITLE_PLAYER_BYTES)return GH_GROUND_ARGUMENT;
 if(c->active&&(c->actor!=v->bytes||c->model!=m||c->globals!=g))return GH_GROUND_BUSY;
 if(!c->active){c->actor=v->bytes;c->model=m;c->globals=g;c->active=1;}if(c->complete)return GH_GROUND_OK;
 fenv_t env;if(feholdexcept(&env))return GH_GROUND_NUMERIC;if(fesetround(FE_TONEAREST)){fesetenv(&env);return GH_GROUND_NUMERIC;}
 gh_player_ground_status e=body(c,v);if(fesetenv(&env))return GH_GROUND_NUMERIC;if(!e)c->frontier=0;return e;
}
