#include "game_scene_at_damage.h"
#include "game_scene_at_geometry.h"
#include <string.h>
static uint32_t w(const void*p){uint32_t n;memcpy(&n,p,4);return n;}
static gh_scene_at_status stop(uint32_t*f,uint32_t pc,gh_scene_at_status e){if(f)*f=pc;return e;}
gh_scene_at_status gh_scene_at_damage_region_002c2490(gh_scene_at_manager*s,const float*point,uint32_t*out,int*hit,uint32_t*f){if(f)*f=0;if(!s||!s->ready||!point||!hit)return GH_AT_ARGUMENT;*hit=0;gh_at_native_unit unit;gh_scene_at_status e=gh_scene_at_first_unit_002c30c0(s,&unit);if(e)return stop(f,0x2c30c0,e);size_t guard=0,count=gh_scene_at_native_unit_count(s);while(unit.bytes){if(++guard>count||unit.size<0x68)return stop(f,0x2c30c8,GH_AT_BOUNDS);const uint8_t*p=unit.bytes;if((w(p+0x34)&0xff01)==0xb01){if(w(p+0x48))return stop(f,0x2c1d68,GH_AT_UNSUPPORTED);int inside;e=gh_scene_at_region_contains_001f83e8(p+4,point,&inside);if(e)return stop(f,0x1f83e8,e);if(inside){if(out)memcpy(out,p+0x5c,12);*hit=1;return GH_AT_OK;}}e=gh_scene_at_next_unit_002c30c8(s,unit,&unit);if(e)return stop(f,0x2c30c8,e);}return GH_AT_OK;}
