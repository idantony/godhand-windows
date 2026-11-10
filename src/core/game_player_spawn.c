#include "game_player_spawn.h"
#include "id_layout_rotation.h"
#include <string.h>
#include <math.h>
#include <fenv.h>
static uint32_t word(const uint8_t*p){uint32_t v;memcpy(&v,p,4);return v;}
static float value(const uint8_t*p){float v;memcpy(&v,p,4);return v;}
static void put(uint8_t*p,uint32_t v){memcpy(p,&v,4);}
static float bits(uint32_t n){float x;memcpy(&x,&n,4);return x;}
static float add(float a,float b){volatile float v=a+b;return v;}
static float mul(float a,float b){volatile float v=a*b;return v;}
static gh_player_spawn_status fail(gh_game_player_spawn*s,uint32_t pc,int detail,gh_player_spawn_status e){s->frontier=pc;s->detail=detail;return e;}
static int coeff(const uint8_t*b,size_t n){if(!b||n<52||memcmp(b,"\177ELF\1\1\1",7)||b[18]!=8||b[19])return 0;size_t table=word(b+28);unsigned step=b[42]|(unsigned)b[43]<<8,count=b[44]|(unsigned)b[45]<<8;if(step<32||count>128||table>n||(size_t)count*step>n-table)return 0;unsigned found=0;const uint32_t expected[4]={0x362e9c14,0xb94fb21f,0x3c08873e,0xbe2aaaa4};for(unsigned i=0;i<count;i++){const uint8_t*p=b+table+(size_t)i*step;if(word(p)!=1)continue;uint32_t off=word(p+4),base=word(p+8),files=word(p+16),mem=word(p+20);if(files>mem||off>n||files>n-off||(uint64_t)base+mem>UINT64_C(0x100000000))return 0;if(base<=0x3c3c90&&(uint64_t)0x3c3ca0<=(uint64_t)base+files){if(found++)return 0;if(memcmp(b+off+(0x3c3c90-base),expected,16))return 0;}}return found==1;}
static void identity(float*m){memset(m,0,64);for(unsigned i=0;i<4;i++)m[i*5]=1;}
static void matrix(float*out,const float*a,const float*b){float m[16];for(unsigned c=0;c<4;c++)for(unsigned r=0;r<4;r++){float v=mul(a[r],b[c*4]);v=add(v,mul(a[4+r],b[c*4+1]));v=add(v,mul(a[8+r],b[c*4+2]));m[c*4+r]=add(v,mul(a[12+r],b[c*4+3]));}memcpy(out,m,64);}
static gh_player_spawn_status world_matrix(gh_game_player_spawn*s){uint8_t*p=s->b.player->view.bytes;
 if(p[0x140])return fail(s,0x147728,p[0x140],GH_PLAYER_SPAWN_UNSUPPORTED);if(!coeff(s->b.elf,s->b.elf_bytes))return fail(s,0x30a5d0,0,GH_PLAYER_SPAWN_UNSUPPORTED);
 fenv_t env;if(feholdexcept(&env))return GH_PLAYER_SPAWN_NUMERIC;if(fesetround(FE_TONEAREST)){(void)fesetenv(&env);return GH_PLAYER_SPAWN_NUMERIC;}float sn,cs;gh_id_error e=gh_id_sincos_reference(s->yaw,&sn,&cs);float m[16],translation[16],rx[16],rz[16],sy0,cy0;
 /* Actual147728 order0 always invokes allthree SDKrotation leaves. */
 if(!e)e=gh_id_sincos_reference(0,&sy0,&cy0);
 if(!e){float x[16]={1,0,0,0,0,cy0,sy0,0,0,-sy0,cy0,0,0,0,0,1},y[16]={cs,0,-sn,0,0,1,0,0,sn,0,cs,0,0,0,0,1},z[16]={cy0,sy0,0,0,-sy0,cy0,0,0,0,0,1,0,0,0,0,1};memcpy(rx,x,64);memcpy(rz,z,64);identity(m);matrix(m,rx,m);matrix(m,y,m);matrix(m,rz,m);identity(translation);memcpy(translation+12,s->position,12);matrix(m,translation,m);}
 if(fesetenv(&env))return GH_PLAYER_SPAWN_NUMERIC;if(e)return fail(s,0x147728,e,GH_PLAYER_SPAWN_UNSUPPORTED);for(unsigned i=0;i<16;i++)if(!isfinite(m[i]))return fail(s,0x147b60,0,GH_PLAYER_SPAWN_NUMERIC);memcpy(p+0x80,m,64);return GH_PLAYER_SPAWN_OK;
}
gh_player_spawn_status gh_game_player_spawn_0031c5c0(gh_game_player_spawn*s,const gh_player_spawn_bindings*b){
 if(!s||!b||!b->player||!b->work||!b->actors)return GH_PLAYER_SPAWN_ARGUMENT;gh_title_player_view*v=&b->player->view;if(!v->bytes||v->size<GH_TITLE_PLAYER_BYTES||v->alias_f0!=v->bytes+0xb0||v->alias_d0!=v->bytes+0x30)return GH_PLAYER_SPAWN_ARGUMENT;
 if(s->begun&&(s->b.player!=b->player||s->player_bytes!=v->bytes||s->room!=(uint16_t)b->work->room||s->b.work!=b->work||s->b.model!=b->model||s->b.actors!=b->actors||s->b.follow!=b->follow||s->b.camera!=b->camera||s->b.init!=b->init||s->b.elf!=b->elf||s->b.elf_bytes!=b->elf_bytes||s->b.ray!=b->ray||s->b.ray_context!=b->ray_context))return GH_PLAYER_SPAWN_BUSY;
 if(s->complete)return GH_PLAYER_SPAWN_OK;s->frontier=0;s->detail=0;
 if(!s->begun){s->b=*b;s->player_bytes=v->bytes;s->room=(uint16_t)b->work->room;s->begun=1;}
 if(!s->stage){fenv_t env;if(feholdexcept(&env))return GH_PLAYER_SPAWN_NUMERIC;if(fesetround(FE_TONEAREST)){(void)fesetenv(&env);return GH_PLAYER_SPAWN_NUMERIC;}const uint8_t*w=(const uint8_t*)b->work;for(unsigned i=0;i<3;i++)s->position[i]=value(w+0x5c0+i*4);s->position[1]=add(s->position[1],bits(0x3e4ccccd));s->position[3]=1;s->yaw=value(w+0x5d0);
  /* Raw31C634/680 uses AND, not OR. Its impossible finite-range clamp is
   * retained; no invented normalizedyaw or zero fallback. */
  if(s->yaw<-bits(0x40490fdb)&&bits(0x40490fdb)<s->yaw)s->yaw=0;
  int good=isfinite(s->position[0])&&isfinite(s->position[1])&&isfinite(s->position[2])&&isfinite(s->yaw);memcpy(s->ray_to,s->position,16);s->ray_to[1]=add(s->ray_to[1],-5);if(fesetenv(&env))return GH_PLAYER_SPAWN_NUMERIC;if(!good)return fail(s,0x31c620,0,GH_PLAYER_SPAWN_NUMERIC);put((uint8_t*)b->work+0x598,word((const uint8_t*)b->work+0x598)&~0x40u);put((uint8_t*)b->work+0x5e4,word((const uint8_t*)b->work+0x5e4)&~0x8000u);s->stage=1;
 }
 if(s->stage==1){if((uint16_t)b->work->room!=0x20){gh_scene_ray_request r={0};memcpy(r.from,s->position,16);memcpy(r.to,s->ray_to,16);r.mask=2;r.segmented=1;r.hit_position=s->ray_to;int hit=0;uint32_t f=0;gh_scene_ray_status e=b->ray?b->ray(b->ray_context,b->actors,&r,&hit,&f):gh_scene_ray_query_00131e10(b->actors,&r,&hit,&f);if(e)return fail(s,f?f:0x131e10,e,e==GH_RAY_UNSUPPORTED?GH_PLAYER_SPAWN_UNSUPPORTED:GH_PLAYER_SPAWN_CALLEE);if(hit)s->position[1]=s->ray_to[1];}s->stage=2;}
 if(s->stage==2){uint8_t*p=v->bytes;memcpy(v->alias_f0,s->position,12);put(p+0x100,0);memcpy(p+0x104,&s->yaw,4);put(p+0x108,0);memcpy(p+0x490,s->position,12);memcpy(p+0xb0,s->position,12);memcpy(p+0x30,s->position,12);float initial[16];identity(initial);memcpy(initial+12,s->position,12);memcpy(p+0x80,initial,64);gh_player_spawn_status e=world_matrix(s);if(e)return e;s->stage=3;}
 if(s->stage==3){if(!b->model||b->model->root.bytes!=v->bytes)return fail(s,0x149eb8,0,GH_PLAYER_SPAWN_UNSUPPORTED);gh_actor_model_status e=gh_actor_model_update_hierarchy_00149eb8(b->model,GH_STATIC_BINARY32_REFERENCE);if(e)return fail(s,b->model->frontier?b->model->frontier:0x149eb8,e,e==GH_ACTOR_MODEL_UNSUPPORTED?GH_PLAYER_SPAWN_UNSUPPORTED:GH_PLAYER_SPAWN_CALLEE);s->stage=4;}
 if(s->stage==4){if(!b->camera||!b->init||!b->follow)return fail(s,0x139db8,0,GH_PLAYER_SPAWN_UNSUPPORTED);uint32_t f=0;gh_camera_status e=gh_game_camera_bind_default_00139db8(b->follow,b->camera,b->init,v,b->work,b->elf,b->elf_bytes,&f);if(e)return fail(s,f?f:0x139db8,e,e==GH_CAMERA_UNSUPPORTED?GH_PLAYER_SPAWN_UNSUPPORTED:GH_PLAYER_SPAWN_CALLEE);s->stage=5;s->complete=1;}
 return GH_PLAYER_SPAWN_OK;
}
