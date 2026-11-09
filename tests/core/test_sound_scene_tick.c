#include "sound_scene_tick.h"
#include <fenv.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"line%d %s\n",__LINE__,#x);exit(1);}}while(0)
int main(int argc,char**argv){
 gh_disc*d;gh_audio_registry*r;gh_sound_control c={0};gh_game_children*children;
 CHECK(argc==2&&!gh_disc_open(argv[1],&d)&&!gh_audio_registry_create(&r)&&!gh_audio_startup_load(r,d)&&!gh_sound_control_from_startup(&c,d,r));
 CHECK(!gh_game_children_create(NULL,&children)&&!gh_game_children_initialize_0015fd98(children));
 const int rounds[]={FE_TONEAREST,FE_DOWNWARD,FE_UPWARD,FE_TOWARDZERO};
 const uint32_t golden[]={0x3dcccccd,0x3e4ccccd,0x3e99999a,0x3ecccccd,0x3f000000,0x3f19999a,0x3f333334,0x3f4cccce,0x3f666668,0x3f800000};
 for(unsigned mode=0;mode<4;mode++){
  gh_sound_scene s={0};gh_boot_work w={0};CHECK(!gh_sound_scene_from_startup(&s,&c,r));
  CHECK(fesetround(rounds[mode])==0);feclearexcept(FE_ALL_EXCEPT);feraiseexcept(FE_DIVBYZERO);int flags=fetestexcept(FE_ALL_EXCEPT);
  for(unsigned i=0;i<10;i++){
   CHECK(!gh_sound_scene_tick_002caf28(&s,&w,NULL)&&fegetround()==rounds[mode]&&fetestexcept(FE_ALL_EXCEPT)==flags);
   for(unsigned j=1;j<=3;j++)CHECK(s.ratios60[j]==golden[i]&&s.ratios74[j]==0x3f800000&&s.ratios88[j]==0x3dcccccd);
   CHECK(!s.ratios60[0]&&!s.ratios60[4]&&!s.ratios74[0]&&!s.ratios74[4]);
  }
  uint32_t mask=0x8000000;memcpy((uint8_t*)&w+0x5e4,&mask,4);
  CHECK(gh_sound_scene_tick_002caf28(&s,&w,NULL)==GH_SOUND_SCENE_UNSUPPORTED&&s.frontier==0x2caf6c);
  CHECK(!s.ratios74[1]&&s.ratios74[2]==0x3ee66666); /*actualpre-readstorespreserved*/
  CHECK(!gh_sound_scene_tick_002caf28(&s,&w,children));
  CHECK(s.ratios74[3]==0x3f800000&&s.ratios60[1]==0x3f666666);
  for(unsigned i=0;i<12;i++)CHECK(!gh_sound_scene_tick_002caf28(&s,&w,children));
  CHECK(!s.ratios60[1]&&s.ratios60[2]==0x3ee66666&&s.ratios60[3]==0x3f800000);
 }
 CHECK(!fesetround(FE_TONEAREST));feclearexcept(FE_ALL_EXCEPT);gh_game_children_destroy(children);gh_audio_registry_destroy(r);gh_disc_close(d);
 puts("PASS actualstartupBSS->2CAF28 ramp/Workgate/canonicalchildren/FEnv/partialfrontier; no audiooutputclaim");return 0;
}
