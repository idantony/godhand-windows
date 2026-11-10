#include "game_world_light_step.h"
#include <string.h>
#include <math.h>
#include <fenv.h>
static uint32_t word(const uint8_t*p){uint32_t v;memcpy(&v,p,4);return v;}
static void put(uint8_t*p,uint32_t v){memcpy(p,&v,4);}
static float scalar(const uint8_t*p){float v;memcpy(&v,p,4);return v;}
static float mul(float a,float b){volatile float r=a*b;return r;}
static float add(float a,float b){volatile float r=a+b;return r;}
static gh_light_status fail(gh_game_world_light*s,uint32_t pc,int detail,gh_light_status e){s->frontier=pc;s->detail=detail;return e;}

gh_light_status gh_game_set_background_0030e5c0(gh_boot_work*work,unsigned mode,uint8_t r,uint8_t g,uint8_t b){
 if(!work)return GH_LIGHT_ARGUMENT;if(mode>1)return GH_LIGHT_OK;
 uint8_t*p=(uint8_t*)work;uint64_t a,c;memcpy(&a,p+0x3e0,8);memcpy(&c,p+0x270,8);
 uint64_t bits=mode?0x16u:0x10u;a=(a&UINT64_C(0xffffffffffff8000))|bits;c=(c&UINT64_C(0xffffffffffff8000))|bits;
 if(mode)put(p+0x60c,(uint32_t)r|(uint32_t)g<<8|(uint32_t)b<<16);
 memcpy(p+0x3e0,&a,8);memcpy(p+0x270,&c,8);return GH_LIGHT_OK;
}

gh_light_status gh_game_world_light_move_002d7560(gh_game_world_light*s,gh_boot_work*work,const gh_room_pools*pools,gh_camera_numeric_policy policy){
 if(!s||!s->constructed||!work)return GH_LIGHT_ARGUMENT;s->frontier=0;s->detail=0;
 uint8_t*p=s->scalar;const uint8_t*w=(const uint8_t*)work;
 if((word(w+0x5d8)&0x4000u)||(word(w+0x5e0)&0x80000u))return GH_LIGHT_OK;
 if(!s->ready)return GH_LIGHT_ARGUMENT;
 if(policy!=GH_CAMERA_NATIVE_BINARY32_REFERENCE)return fail(s,0x2d7560,0,GH_LIGHT_UNSUPPORTED);
 if(s->area.bytes)return fail(s,0x1f83e8,0,GH_LIGHT_UNSUPPORTED);
 if(word(p+0x16284)&0x10u)return fail(s,0x2d8c60,0,GH_LIGHT_UNSUPPORTED);
 const gh_room_vector*objects;gh_room_pool_status ps=gh_room_registry_view(pools,0x754c58,&objects);
 if(ps)return fail(s,0x2d7560,ps,GH_LIGHT_ARGUMENT);
 if(objects->count)return fail(s,0x2d74b8,0,GH_LIGHT_UNSUPPORTED);
 for(unsigned i=0;i<256;i++)if(word(p+0x80+i*0x70+4))return fail(s,0x2d7560,(int)i,GH_LIGHT_UNSUPPORTED);
 size_t count=word(p+0x78);if(count>256)return fail(s,0x2d8ff8,0,GH_LIGHT_BOUNDS);
 for(size_t i=0;i<count&&i<48;i++)if(word(p+0x80+i*0x70+0x58)&0x100000u)return fail(s,0x156488,(int)i,GH_LIGHT_UNSUPPORTED);
 /* With FAR absent,2D8EB0 genuinely returnsNULL; LAR is not dereferenced
  * when the SAME canonical model registry3 is empty. */
 float target[5]={scalar(p+0x10),scalar(p+0x14),(float)p[4],(float)p[5],(float)p[6]},next[5];
 fenv_t saved;if(feholdexcept(&saved))return fail(s,0x2d7560,0,GH_LIGHT_UNSUPPORTED);if(fesetround(FE_TONEAREST)){fesetenv(&saved);return fail(s,0x2d7560,0,GH_LIGHT_UNSUPPORTED);}
 int valid=1;for(unsigned i=0;i<5;i++){next[i]=add(mul(scalar(p+0x16150+4*i),.96f),mul(target[i],.04f));if(!isfinite(next[i]))valid=0;}
 int restore=fesetenv(&saved);if(!valid||restore)return fail(s,0x2d7560,0,GH_LIGHT_UNSUPPORTED);
 put(p+0x16124,0);memcpy(p+0x16150,next,sizeof(next));
 put(p+0x16138,255);put(p+0x16134,0);put(p+0x16140,255);put(p+0x1613c,255);
 gh_game_set_background_0030e5c0(work,1,p[0],p[1],p[2]);
 put(p+0x16144,word(p+0x16138));put(p+0x16148,word(p+0x1613c));put(p+0x1614c,word(p+0x16140));
 return GH_LIGHT_OK;
}
