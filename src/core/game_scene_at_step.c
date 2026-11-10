#include "game_scene_at_step.h"
#include "game_scene_at_geometry.h"
#include <string.h>
#include <math.h>
#include <fenv.h>
static uint32_t word(const uint8_t*p){uint32_t n;memcpy(&n,p,4);return n;}
static void put(uint8_t*p,uint32_t n){memcpy(p,&n,4);}
static float number(const uint8_t*p){float x;memcpy(&x,p,4);return x;}
static float bits(uint32_t n){float x;memcpy(&x,&n,4);return x;}
static float add(float a,float b){volatile float v=a+b;return v;}
static float mul(float a,float b){volatile float v=a*b;return v;}
static gh_scene_at_status stop(uint32_t*f,uint32_t pc,gh_scene_at_status status){if(f)*f=pc;return status;}
static const uint32_t handlers[14]={0x2c2070,0x2c2078,0x2c2138,0x2c21a0,0x2c2330,0x2c23e0,0x2c23f0,0x2c23f8,0x2c2418,0x2c2468,0x2c2470,0x2c2478,0x2c2480,0x2c2488};
static const uint8_t concurrent[14]={1,0,1,1,0,1,1,1,1,1,0,1,1,1};
static gh_scene_at_status unit_contains(gh_at_native_unit u,const gh_title_player_view*player,const float*endpoint,const float*origin,int*inside,uint32_t*f){
 const uint8_t*p=u.bytes;if(word(p+0x48))return stop(f,0x2c1d68,GH_AT_UNSUPPORTED);gh_scene_at_status e=gh_scene_at_region_contains_001f83e8(p+4,p[0x38]?endpoint:origin,inside);if(e)return stop(f,0x1f83e8,e);
 if(*inside&&(p[0x38]&2)){float yaw=number(player->bytes+0x104);if(!isfinite(yaw))return stop(f,0x2c1ae0,GH_AT_UNSUPPORTED);fenv_t env;if(feholdexcept(&env))return GH_AT_BOUNDS;if(fesetround(FE_TONEAREST)){(void)fesetenv(&env);return GH_AT_BOUNDS;}float angle=mul((float)((int)(int8_t)p[0x45]*2),bits(0x3c8efa35)),delta=fabsf(add(angle,-yaw));if(delta>bits(0x40490fdb))delta=add(bits(0x40c90fdb),-delta);float threshold=mul((float)((int)(int8_t)p[0x46]*2),bits(0x3c8efa35));int accept=!(threshold<delta);if(fesetenv(&env))return GH_AT_BOUNDS;*inside=accept;}
 return GH_AT_OK;
}
static gh_scene_at_status dispatch(gh_boot_work*w,const gh_scene_at_step_callbacks*c,unsigned kind,gh_at_native_unit u,uint8_t mask,int*out,uint32_t*f){
 *out=0;if(kind==0)return GH_AT_OK; /* Actual2C2070 jrra; return0. */
 if(kind==1&&(w->flags&8))return GH_AT_OK; /* Original2078 Door gate. */
 if(kind==3){uint8_t*p=u.bytes;unsigned action=p[0x5c];if(action==0){unsigned index=p[0x5e]|(unsigned)p[0x5f]<<8;if(index<64){uint8_t*field=(uint8_t*)w+0x5ec+(index>>5)*4;uint32_t bit=UINT32_C(0x80000000)>>(index&31),v=word(field);put(field,p[0x5d]==1?v|bit:v&~bit);}return GH_AT_OK;}if(action!=2)return GH_AT_OK;}
 if(!c||!c->dispatch)return stop(f,handlers[kind],GH_AT_UNSUPPORTED);return c->dispatch(c->context,handlers[kind],u,mask,out,f);
}
static gh_scene_at_status run(gh_scene_at_manager*s,gh_game_scene_tables*actors,const gh_title_player_view*player,gh_boot_work*work,const gh_scene_at_step_callbacks*c,int*out,uint32_t*f){
 gh_scene_ray_request ray={.mask=7,.flags=0x10,.segmented=1};float hit_position[4]={0,0,0,1};float local[3]={0,.4f,1},matrix[16];fenv_t env;int hit;
 if(f)*f=0;if(!s||!s->ready||!actors||!player||!player->bytes||player->size<GH_TITLE_PLAYER_BYTES||!out)return GH_AT_ARGUMENT;*out=0;if(player->alias_f0!=player->bytes+0xb0)return stop(f,0x2db6b0,GH_AT_ARGUMENT);
 for(unsigned i=0;i<16;i++)matrix[i]=number(player->bytes+0x80+i*4);gh_scene_at_status e=gh_scene_at_apply_world_001508d8(matrix,local,ray.to);if(e)return stop(f,0x1508d8,e);memcpy(ray.from,player->alias_f0,12);ray.from[3]=1;for(unsigned i=0;i<3;i++)if(!isfinite(ray.from[i]))return stop(f,0x2c1618,GH_AT_UNSUPPORTED);
 if(feholdexcept(&env))return GH_AT_BOUNDS;if(fesetround(FE_TONEAREST)){(void)fesetenv(&env);return GH_AT_BOUNDS;}ray.from[1]=add(ray.from[1],.4f);if(fesetenv(&env))return GH_AT_BOUNDS;ray.hit_position=hit_position;gh_scene_ray_status status=gh_scene_ray_query_00131e10(actors,&ray,&hit,f);if(status)return status==GH_RAY_UNSUPPORTED?GH_AT_UNSUPPORTED:GH_AT_BOUNDS;if(hit)memcpy(ray.to,hit_position,12);
 gh_at_native_unit u;e=gh_scene_at_first_unit_002c30c0(s,&u);if(e)return stop(f,0x2c30c8,e);if(u.bytes&&!work)return stop(f,0x2c30c8,GH_AT_UNSUPPORTED);uint8_t mask=1;if(work){if(word((const uint8_t*)work+0x1a0)&1)mask=3;if(word((const uint8_t*)work+0x178)&1)mask|=4;}
 size_t seen=0,limit=gh_scene_at_native_unit_count(s);int result=0;
 while(u.bytes){if(++seen>limit)return stop(f,0x2c30c8,GH_AT_BOUNDS);uint8_t*p=u.bytes;uint8_t current=mask&0x8f;put(p+0x50,word(p+0x50)&0xcfffffffu);
  if(!(p[0x34]&1)){put(p+0x50,word(p+0x50)&0xbfffffffu);mask=current;goto next;}
  int inside=0;e=unit_contains(u,player,ray.to,ray.from,&inside,f);if(e)return e;uint32_t flags=word(p+0x50);int consume=0;
  if(inside){flags|=0x80000000u;if(mask&2)flags|=0x08000000u;put(p+0x50,flags);if(!(flags&0x40000000u)){current|=0x10;put(p+0x50,flags|0x20000000u);}if(word(p+0x50)&0x04000000u)current|=0x40;consume=1;}
  else{put(p+0x50,flags&0x7fffffffu);if(flags&0x40000000u){put(p+0x50,(flags&0x7fffffffu)|0x10000000u);current|=0x20;consume=1;}}
  mask=current;if(consume&&(p[0x35]!=2||word(p+0x3c))){if((word((const uint8_t*)work+0x5e4)&0x40000000u)&&(p[0x35]<7||p[0x35]>8))goto next;unsigned kind=word(p+0x3c)?2:p[0x35];if(!(p[0x3b]&8)){if((p[0x3b]&mask)&&kind<14&&(!result||concurrent[kind])){put(p+0x50,word(p+0x50)|0x02000000u);int returned;e=dispatch(work,c,kind,u,mask,&returned,f);if(e)return e;if(returned==1)result=1;if((p[0x3b]&0x80)&&(p[0x35]!=2||word(p+0x3c))){p[0x34]&=0xfe;if(p[0x35]==9)return stop(f,0x2c0e60,GH_AT_UNSUPPORTED);}}}
   else if(!(word((const uint8_t*)work+0x5d8)&0x02000000u)){gh_scene_at_task_request request={u,p[0x39],p[0x3a],kind==6?word(p+0x5c):kind<14?handlers[kind]:0,kind==6?word(p+0x64):0,kind==6?0:2};if(kind>=14)return stop(f,0x1f7858,GH_AT_UNSUPPORTED);if(!c||!c->spawn)return stop(f,0x1f7858,GH_AT_UNSUPPORTED);e=c->spawn(c->context,&request,f);if(e)return e;}
  }
next:e=gh_scene_at_next_unit_002c30c8(s,u,&u);if(e)return stop(f,0x2c30c8,e);
 }
 e=gh_scene_at_first_unit_002c30c0(s,&u);if(e)return stop(f,0x2c30c8,e);seen=0;while(u.bytes){if(++seen>limit)return stop(f,0x2c30c8,GH_AT_BOUNDS);uint32_t flags=word(u.bytes+0x50);int now=(flags&0x80000000u)!=0,old=(flags&0x40000000u)!=0;flags=(old!=now&&now)?flags|0x20000000u:flags&0xdfffffffu;flags=(old!=now&&old)?flags|0x10000000u:flags&0xefffffffu;put(u.bytes+0x50,flags);e=gh_scene_at_next_unit_002c30c8(s,u,&u);if(e)return stop(f,0x2c30c8,e);}*out=result;return GH_AT_OK;
}
gh_scene_at_status gh_scene_at_step_native_002c1618(gh_scene_at_manager*s,gh_game_scene_tables*a,const gh_title_player_view*p,gh_boot_work*w,const gh_scene_at_step_callbacks*c,int*out,uint32_t*f){return run(s,a,p,w,c,out,f);}
gh_scene_at_status gh_scene_at_step_002c1618(gh_scene_at_manager*s,gh_game_scene_tables*a,const gh_title_player_view*p,int*out,uint32_t*f){return run(s,a,p,NULL,NULL,out,f);}
