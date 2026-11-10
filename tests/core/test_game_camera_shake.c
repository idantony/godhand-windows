#include "game_camera_shake.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <fenv.h>
#define C(x)do{if(!(x)){fprintf(stderr,"FAIL%d %s\n",__LINE__,#x);exit(1);}}while(0)
static void store(void*p,float x){memcpy(p,&x,4);}
static float readf(const void*p){float x;memcpy(&x,p,4);return x;}
int main(void){gh_game_init_state init;gh_game_init_state_initialize(&init);gh_game_pl_camera c={0};C(!gh_game_camera_construct_pl_00139bc8(&c,&init.camera,GH_CAMERA_NATIVE_BINARY32_REFERENCE));uint8_t original[0x530];memcpy(original,c.fields,sizeof(original));uint32_t f;C(!gh_game_camera_shake_step_00138558(&c,&f)&&!memcmp(original,c.fields,sizeof(original)));c.fields[0x278]=2;c.fields[0x264]=2;store(c.fields+0x268,4);store(c.fields+0x270,.5f);store(c.fields+0x274,.25f);store(c.fields+0x26c,.5f);store(c.fields+0x254,8);store(c.fields+0x25c,.25f);store(c.fields+0x260,-3);store(c.fields+0x258,-1);gh_game_pl_camera base=c;uint8_t reference[0x530];float offsets[2];const int modes[4]={FE_TONEAREST,FE_DOWNWARD,FE_UPWARD,FE_TOWARDZERO};
 for(unsigned i=0;i<4;i++){c=base;C(!fesetround(modes[i])&&!feclearexcept(FE_ALL_EXCEPT)&&!feraiseexcept(FE_DIVBYZERO));C(!gh_game_camera_shake_step_00138558(&c,&f)&&!gh_game_camera_shake_offsets_00138740(&c,offsets,&f)&&fegetround()==modes[i]&&fetestexcept(FE_ALL_EXCEPT)==FE_DIVBYZERO);if(!i)memcpy(reference,c.fields,sizeof(reference));else C(!memcmp(reference,c.fields,sizeof(reference)));C(c.fields[0x278]==1&&c.fields[0x264]==1&&readf(c.fields+0x268)==2&&readf(c.fields+0x274)==.75f&&readf(c.fields+0x254)==2);C(fabs(readf(c.fields+0x260)-(2*3.1415927410125732421875-4))<.000001);C(fabs(offsets[1]-2*sin(.75))<.001&&fabs(offsets[0]-2*sin(readf(c.fields+0x260)))<.001);}
 C(!fesetround(FE_TONEAREST));for(unsigned j=0;j<sizeof(reference);j++)if(!((j>=0x254&&j<0x258)||(j>=0x260&&j<0x265)||(j>=0x268&&j<0x26c)||(j>=0x274&&j<0x279)))C(c.fields[j]==base.fields[j]);C(!gh_game_camera_shake_step_00138558(&c,&f)&&!c.fields[0x278]&&!c.fields[0x264]&&!gh_game_camera_shake_offsets_00138740(&c,offsets,&f)&&offsets[0]==0&&offsets[1]==0);
 c=base;c.fields[0x278]=0x80;C(!gh_game_camera_shake_step_00138558(&c,&f)&&c.fields[0x278]==0x7f);c=base;float nan=NAN;store(c.fields+0x260,nan);memcpy(original,c.fields,sizeof(original));C(gh_game_camera_shake_step_00138558(&c,&f)==GH_CAMERA_NUMERIC&&f==0x138558&&!memcmp(original,c.fields,sizeof(original)));offsets[0]=123;offsets[1]=456;C(gh_game_camera_shake_offsets_00138740(&c,offsets,&f)==GH_CAMERA_UNSUPPORTED&&offsets[0]==123&&offsets[1]==456);puts("PASS original138558 dualaxis decay/phasewrap/bytecount and138740 postview deltas, independentdouble sine/wrap,4FEnv/unchangedneighbors/finalcounter0/invalidatomic; no active-shake projection claim");return 0;}

