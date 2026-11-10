#include "game_om3eb.h"
#include "game_om_base.h"
#include <string.h>
static uint32_t word(const void*p){uint32_t n;memcpy(&n,p,4);return n;}
gh_scene_placement_status gh_om3eb_construct_001bf478(void*p,size_t n,gh_title_player_view*out){
 if(!p||!out)return GH_PLACE_ARGUMENT;if(n<0x610)return GH_PLACE_BOUNDS;
 gh_scene_placement_status e=gh_om_base_construct_001b6e90(p,n,out);if(e)return e;
 uint32_t vt=0x4296f0;memcpy((uint8_t*)p+0x214,&vt,4);((uint8_t*)p)[0x600]=0xff;out->original_vtable214=vt;return GH_PLACE_OK;
}
gh_scene_placement_status gh_om3eb_initialize_001bf4e0(gh_om_init_state*s,gh_om_object*o,const gh_om_initializer_bindings*b,int*out){
 if(!o||!out||!o->runtime_created||o->runtime_kind!=0x3eb||o->source.bytes)return GH_PLACE_ARGUMENT;
 int original=0;gh_scene_placement_status e=gh_om_common_initialize_001b6fb8(s,o,b,&original);if(e)return e;*out=0;if(!original)return GH_PLACE_UNSUPPORTED;
 uint32_t flags=word(o->view.bytes+0x5b8)&~0x40u;memcpy(o->view.bytes+0x5b8,&flags,4);*out=1;return GH_PLACE_OK;
}
gh_scene_placement_status gh_om3eb_set_position_001b7e50(gh_om_object*o,const float pos[4]){
 if(!o||!pos||!o->runtime_created||o->runtime_kind!=0x3eb||!o->view.bytes||o->view.size<0x5bc||!o->view.alias_f0)return GH_PLACE_ARGUMENT;
 uint8_t*p=o->view.bytes;uintptr_t a=(uintptr_t)o->view.alias_f0,base=(uintptr_t)p;if(a<base||a-base>o->view.size||12>o->view.size-(a-base))return GH_PLACE_BOUNDS;
 memmove(p+0x490,pos,12);memmove(o->view.alias_f0,p+0x490,12);uint32_t flags=word(p+0x5b8)|0x10u;memcpy(p+0x5b8,&flags,4);return GH_PLACE_OK;
}
