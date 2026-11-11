#include "game_enemy_request.h"
#include <string.h>
static uint32_t word(const uint8_t*p){uint32_t v;memcpy(&v,p,4);return v;}
static uint16_t half(const uint8_t*p){uint16_t v;memcpy(&v,p,2);return v;}
static uint32_t converted(uint16_t raw,uint32_t factor){int value=raw<32768?(int)raw:(int)raw-65536;if(!value)return 0;unsigned magnitude=(unsigned)(value<0?-value:value);uint64_t n=(uint64_t)magnitude*((factor&0x7fffff)|0x800000);unsigned high=0;for(uint64_t k=n;k>>=1;)high++;int exponent=(int)high+(int)(factor>>23)-150;uint64_t mantissa=high>23?n>>(high-23):n<<(23-high);return (value<0?0x80000000u:0)|((uint32_t)(exponent+127)<<23)|((uint32_t)mantissa&0x7fffff);}
gh_scene_placement_status gh_enemy_request_from_room_00294ca0(const gh_room_state*room,unsigned index,gh_em_numeric_policy policy,gh_enemy_request*out,int*spawn,int16_t*saved,uint32_t*f){
 if(!room||!room->active||!out||!spawn||!saved||!f)return GH_PLACE_ARGUMENT;*f=0;*spawn=0;if(room->size<9||room->active[8]>64||index>=room->active[8]||room->size<12+(size_t)(index+1)*24)return GH_PLACE_BOUNDS;
 const uint8_t*p=room->active+12+(size_t)index*24;if(p[0x14]==1||(word(p+8)&0x80000000u))return GH_PLACE_OK;
 if(policy!=GH_EM_NUMERIC_PCSX2_CHOPZERO_REFERENCE){*f=0x294e34;return GH_PLACE_UNSUPPORTED;}
 gh_enemy_request next={0};next.object_kind=(uint32_t)p[0x12]+0x200;
 for(unsigned i=0;i<3;i++)next.position_xyz[i]=converted(half(p+i*2),0x3d4ccccd);next.yaw_bits=converted(half(p+6),0x3c8efa35);next.flags=word(p+8);next.dependency_kind=(uint32_t)p[0x11]+0x200;next.dependency_flags=word(p+0xc);next.appearance=p[0x10];next.entry_number=p[0x13];uint16_t raw=half(p+0x16);*saved=(int16_t)(raw<32768?(int)raw:(int)raw-65536);*out=next;*spawn=1;return GH_PLACE_OK;
}
gh_actor_motion_status gh_enemy_apply_saved_vitality_00294f34(gh_game_title_player*object,int16_t saved){if(!object||!object->view.bytes||object->view.size<0x54c)return GH_ACTOR_MOTION_ARGUMENT;if(saved>0){uint16_t raw=half(object->view.bytes+0x548);int max=raw<32768?(int)raw:(int)raw-65536;int16_t current=saved<max?saved:(int16_t)max;memcpy(object->view.bytes+0x54a,&current,2);}return GH_ACTOR_MOTION_OK;}
