#include "game_scene_aux.h"
#include <stdio.h>
#include <string.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"FAIL %d %s\n",__LINE__,#x);return 1;}}while(0)
static void put(gh_boot_work*w,size_t o,uint32_t n){memcpy((uint8_t*)w+o,&n,4);}
static uint32_t get(const gh_boot_work*w,size_t offset){uint32_t n;memcpy(&n,(const uint8_t*)w+offset,4);return n;}
static uint32_t clock_fixture(void*p){uint32_t*n=p;*n+=7;return *n;}
static gh_scene_aux_status start_load(void *context){gh_boot_work *work=context;put(work,0x5e8,get(work,0x5e8)|0x80000000u);return GH_AUX_OK;}
int main(void){
 gh_boot_work w={0};gh_game_scene_aux s={0};uint16_t wait=0xffff;
 CHECK(!gh_scene_door_step_001ffe10(&s,&wait,&w)&&wait==0xfffe);wait=1;CHECK(!gh_scene_door_step_001ffe10(&s,&wait,&w)&&!wait);
 put(&w,0x5e4,0x02000000);wait=5;
 CHECK(gh_scene_door_step_001ffe10(&s,&wait,&w)==GH_AUX_UNSUPPORTED&&s.frontier==0x2000f8&&wait==4);
 CHECK(gh_scene_door_step_001ffe10(&s,&wait,&w)==GH_AUX_UNSUPPORTED&&wait==4);put(&w,0x5e4,0);CHECK(!gh_scene_door_step_001ffe10(&s,&wait,&w)&&wait==4);
 CHECK(!gh_scene_cursor_step_002bf2d0(&s,&w,NULL,NULL));uint64_t x=UINT64_MAX,y=UINT64_C(0x123456789abcdef0);memcpy((uint8_t*)&w+0x3e0,&x,8);memcpy((uint8_t*)&w+0x270,&y,8);
 put(&w,0x5e8,0x00100000);memset(s.save_load,0xab,sizeof(s.save_load));
 CHECK(gh_scene_cursor_open_load_002bf268(&s,&w)==GH_AUX_OK);
 uint8_t expected[0x80];memset(expected,0xab,sizeof(expected));
 const uint8_t prefix[]={2,0,0,0,0,0,0,0,0xab,0,0,0xab,0,0};
 memcpy(expected,prefix,sizeof(prefix));CHECK(!memcmp(s.save_load,expected,sizeof(expected)));
 CHECK(get(&w,0x5e8)==0x40100000);
 put(&w,0x5e0,0x01234567);put(&w,0x5d8,0x02000400);put(&w,0x5dc,0x12345678);
 CHECK(gh_scene_cursor_step_002bf2d0(&s,&w,NULL,NULL)==GH_AUX_UNSUPPORTED&&s.frontier==0x2ae7f0);
 uint32_t saved[3];memcpy(saved,s.save_load+0x70,sizeof(saved));
 CHECK(saved[0]==0x01234567&&saved[1]==0x02000400&&saved[2]==0x12345678);
 CHECK(get(&w,0x5e0)==0xfff34567&&get(&w,0x5d8)==0xd8862400);
 CHECK(!gh_scene_cursor_step_002bf2d0(&s,&w,start_load,&w)&&s.save_load[1]==1);
 CHECK(!memcmp(saved,s.save_load+0x70,sizeof(saved))&&get(&w,0x5e8)==0xc0100000);
 CHECK(!gh_scene_cursor_step_002bf2d0(&s,&w,NULL,NULL)&&s.save_load[0]==2);
 put(&w,0x5e8,0x40100000);w.flags|=0x40;
 CHECK(!gh_scene_cursor_step_002bf2d0(&s,&w,NULL,NULL)&&s.save_load[0]==4&&s.save_load[0xc]==1);
 CHECK(!gh_scene_cursor_step_002bf2d0(&s,&w,NULL,NULL)&&s.save_load[0]==5&&s.save_load[0xc]==1);
 CHECK(gh_scene_cursor_step_002bf2d0(&s,&w,NULL,NULL)==GH_AUX_UNSUPPORTED&&s.frontier==0x2bf568);
 uint64_t a,b;memcpy(&a,(uint8_t*)&w+0x3e0,8);memcpy(&b,(uint8_t*)&w+0x270,8);CHECK(a==((x&UINT64_C(0xffffffffffff8000))|0x16)&&b==((y&UINT64_C(0xffffffffffff8000))|0x16));
 s.save_load[0]=0;CHECK(!gh_scene_cursor_step_002bf2d0(&s,&w,NULL,NULL));
 put(&w,0x5d8,0);
 uint32_t counter=0;s.effect_count_3c3bb8=17;
 CHECK(gh_scene_effects_step_0030f228(&s,&w,clock_fixture,&counter)==GH_AUX_UNSUPPORTED&&counter==7&&!s.effect_count_3c3bb8&&s.frontier==0x300628);
 CHECK(gh_scene_effects_step_0030f228(&s,&w,clock_fixture,&counter)==GH_AUX_UNSUPPORTED&&counter==7);
 put(&w,0x5d8,0xf00c6000);CHECK(!gh_scene_effects_step_0030f228(&s,&w,clock_fixture,&counter)&&counter==14&&s.effect_profile_752d00==(uint32_t)-7);
 CHECK(!gh_scene_effects_step_0030f228(&s,&w,clock_fixture,&counter)&&counter==28);
 puts("PASS original door countdown/retry, inactive cursor and active display prefix, R020 effect suppression and native diagnostic timing");return 0;
}
