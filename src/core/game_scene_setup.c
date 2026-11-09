#include "game_scene_setup.h"
#include <string.h>
static uint32_t u32(const uint8_t *p){uint32_t v;memcpy(&v,p,4);return v;}
static void put16(uint8_t *p,uint16_t v){memcpy(p,&v,2);}
static void put32(uint8_t *p,uint32_t v){memcpy(p,&v,4);}
static int valid(gh_scene_scenario_view s,const gh_boot_work *w){return s.bytes&&s.size>=0x114&&s.manager&&w;}
static gh_scene_status hook(gh_scene_hook h,uint32_t callsite,uint32_t *frontier){
 if(h.kind==GH_SCENE_HOOK_ABSENT)return GH_SCENE_OK;
 if(h.kind!=GH_SCENE_HOOK_NATIVE||!h.call){if(frontier)*frontier=callsite;return GH_SCENE_UNSUPPORTED;}
 return h.call(h.context,frontier);
}
gh_scene_status gh_scene_before_objects_002c33d8(gh_scene_scenario_view s,
 gh_boot_work *w,uint8_t *em,gh_scene_hook h,uint32_t *frontier){
 gh_scene_status r;if(frontier)*frontier=0;
 if(!valid(s,w)||!em)return GH_SCENE_ARGUMENT;
 *em=0;r=hook(h,0x002c3400,frontier);if(r!=GH_SCENE_OK)return r;
 /* Reload work flags after the callback, matching original ordering. */
 if(u32((const uint8_t*)w+0x584)&0x240u)s.bytes[0x14]=0;
 return GH_SCENE_OK;
}
gh_scene_status gh_scene_initialize_room_002c3440(gh_scene_scenario_view s,
 gh_boot_work *w,gh_scene_hook h,uint32_t *frontier){
 if(frontier)*frontier=0;
 if(!valid(s,w))return GH_SCENE_ARGUMENT;
 put16(s.bytes+0x112,0xffff);put32(s.bytes+0x104,0);
 for(unsigned off=0xec;off<=0x100;off+=4)put32(s.bytes+off,0);
 s.bytes[0x108]=0;put16(s.bytes+0xe8,0);
 put32((uint8_t*)w+0x610,0xffff);
 s.manager->flags&=~3u;
 if(!(u32((const uint8_t*)w+0x590)&1u))return hook(h,0x002c34d8,frontier);
 return GH_SCENE_OK;
}
