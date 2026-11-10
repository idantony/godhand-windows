#include "game_scene_aux.h"
#include <stdio.h>
#include <string.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"FAIL %d %s\n",__LINE__,#x);return 1;}}while(0)
static void put(gh_boot_work*w,size_t o,uint32_t n){memcpy((uint8_t*)w+o,&n,4);}
static uint32_t clock_fixture(void*p){uint32_t*n=p;*n+=7;return *n;}
int main(void){
 gh_boot_work w={0};gh_game_scene_aux s={0};uint16_t wait=0xffff;
 CHECK(!gh_scene_door_step_001ffe10(&s,&wait,&w)&&wait==0xfffe);wait=1;CHECK(!gh_scene_door_step_001ffe10(&s,&wait,&w)&&!wait);
 put(&w,0x5e4,0x02000000);wait=5;
 CHECK(gh_scene_door_step_001ffe10(&s,&wait,&w)==GH_AUX_UNSUPPORTED&&s.frontier==0x2000f8&&wait==4);
 CHECK(gh_scene_door_step_001ffe10(&s,&wait,&w)==GH_AUX_UNSUPPORTED&&wait==4);put(&w,0x5e4,0);CHECK(!gh_scene_door_step_001ffe10(&s,&wait,&w)&&wait==4);
 CHECK(!gh_scene_cursor_step_002bf2d0(&s,&w,NULL));uint64_t x=UINT64_MAX,y=UINT64_C(0x123456789abcdef0);memcpy((uint8_t*)&w+0x3e0,&x,8);memcpy((uint8_t*)&w+0x270,&y,8);
 put(&w,0x5e8,0x40000000);uint8_t kind=4;CHECK(gh_scene_cursor_step_002bf2d0(&s,&w,&kind)==GH_AUX_UNSUPPORTED&&s.frontier==0x2bf540);
 uint64_t a,b;memcpy(&a,(uint8_t*)&w+0x3e0,8);memcpy(&b,(uint8_t*)&w+0x270,8);CHECK(a==((x&UINT64_C(0xffffffffffff8000))|0x16)&&b==((y&UINT64_C(0xffffffffffff8000))|0x16));
 kind=0;CHECK(!gh_scene_cursor_step_002bf2d0(&s,&w,&kind));
 uint32_t counter=0;s.effect_count_3c3bb8=17;
 CHECK(gh_scene_effects_step_0030f228(&s,&w,clock_fixture,&counter)==GH_AUX_UNSUPPORTED&&counter==7&&!s.effect_count_3c3bb8&&s.frontier==0x300628);
 CHECK(gh_scene_effects_step_0030f228(&s,&w,clock_fixture,&counter)==GH_AUX_UNSUPPORTED&&counter==7);
 put(&w,0x5d8,0xf00c6000);CHECK(!gh_scene_effects_step_0030f228(&s,&w,clock_fixture,&counter)&&counter==14&&s.effect_profile_752d00==(uint32_t)-7);
 CHECK(!gh_scene_effects_step_0030f228(&s,&w,clock_fixture,&counter)&&counter==28);
 puts("PASS original door countdown/retry, inactive cursor and active display prefix, R020 effect suppression and native diagnostic timing");return 0;
}
