#include "game_scene_at_jump.h"
#include <string.h>
#include <fenv.h>
#include <math.h>
static uint32_t w(const uint8_t*p){uint32_t n;memcpy(&n,p,4);return n;}
static unsigned h(const uint8_t*p){return p[0]|(unsigned)p[1]<<8;}
static int signed_half(const uint8_t*p){uint16_t u=(uint16_t)h(p);int16_t s;memcpy(&s,&u,2);return s;}
static float number(uint32_t u){float x;memcpy(&x,&u,4);return x;}
static gh_scene_at_status fail(uint32_t*f,uint32_t pc,gh_scene_at_status e){*f=pc;return e;}
gh_scene_at_status gh_scene_at_find_jump_002c2b58(gh_scene_at_manager*s,const gh_bundle*room,uint16_t current_room,const float*point,float*out,float*yaw,int*original,uint32_t*f){
 if(!s||!room||!point||!out||!yaw||!original||!f||!s->ready)return GH_AT_ARGUMENT;*f=0;*original=0;for(unsigned i=0;i<3;i++)if(!isfinite(point[i]))return fail(f,0x1f83e8,GH_AT_UNSUPPORTED);
 gh_at_native_unit u;gh_scene_at_status e=gh_scene_at_first_unit_002c30c0(s,&u);if(e)return fail(f,0x2c30c8,e);size_t visited=0,limit=gh_scene_at_native_unit_count(s);
 while(u.bytes){if(++visited>limit||u.size<0x9c)return fail(f,0x2c30c8,GH_AT_BOUNDS);
  if(u.bytes[0x35]==1&&u.bytes[0x7a]==1){if(w(u.bytes+0x48))return fail(f,0x2c1d68,GH_AT_UNSUPPORTED);uint8_t shape[0x30];memcpy(shape,u.bytes+4,sizeof(shape));int inside;
   e=gh_scene_at_region_contains_001f83e8(shape,point,&inside);if(e)return fail(f,0x1f83e8,e);
   if(inside){gh_bundle_entry jump;gh_bundle_error be=gh_bundle_find(room,"JMP",0,&jump);if(be==GH_BUNDLE_NOT_FOUND)return GH_AT_OK;if(be)return fail(f,0x31ccf0,GH_AT_BOUNDS);
    if(jump.size<4)return fail(f,0x2c2b58,GH_AT_BOUNDS);unsigned count=w(jump.bytes),index=u.bytes[0x79];if(count>(jump.size-4)/20)return fail(f,0x2c2b58,GH_AT_BOUNDS);if(index>=count)return GH_AT_OK;
    const uint8_t*r=jump.bytes+4+(size_t)index*20;if(signed_half(r+8)!=(int32_t)current_room)return GH_AT_OK;
    fenv_t env;if(feholdexcept(&env))return fail(f,0x2c2b58,GH_AT_UNSUPPORTED);if(fesetround(FE_TONEAREST)){(void)fesetenv(&env);return fail(f,0x2c2b58,GH_AT_UNSUPPORTED);}
    float position[3];for(unsigned k=0;k<3;k++){volatile float value=(float)signed_half(r+2*k)*number(0x3c23d70a);position[k]=value;}volatile float angle=(float)signed_half(r+6)*number(0x3c8efa35);float result_yaw=angle;
    if(fesetenv(&env))return fail(f,0x2c2b58,GH_AT_UNSUPPORTED);memcpy(out,position,12);*yaw=result_yaw;*original=1;return GH_AT_OK;
   }
  }
  e=gh_scene_at_next_unit_002c30c8(s,u,&u);if(e)return fail(f,0x2c30c8,e);
 }return GH_AT_OK;
}
