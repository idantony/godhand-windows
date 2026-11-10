#include "game_actor_light_ambient.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fenv.h>
#include <math.h>
#define C(x) do{if(!(x)){fprintf(stderr,"line%d %s\n",__LINE__,#x);exit(1);}}while(0)
int main(void){
 gh_game_world_light*s=calloc(1,sizeof(*s));C(s&&!gh_game_world_light_construct_002d6d80(s));
 const float ambient[4]={2,3,4,5},tint[4]={.5f,.25f,2,-1},product[4]={1,.75f,8,-5};memcpy(s->scalar+0x30,ambient,16);
 const int modes[4]={FE_TONEAREST,FE_DOWNWARD,FE_UPWARD,FE_TOWARDZERO};uint32_t pc;
 for(unsigned mode=0;mode<4;mode++){
  uint32_t normal[16],color[16],oldn[16],oldc[16];for(unsigned k=0;k<16;k++){normal[k]=0x7f800001u+k;color[k]=0xff800001u+k;}memcpy(oldn,normal,64);memcpy(oldc,color,64);
  C(!fesetround(modes[mode])&&!feclearexcept(FE_ALL_EXCEPT)&&!feraiseexcept(FE_DIVBYZERO));
  C(!gh_actor_ambient_fast_002d8008(s,normal,color,(const void*)(uintptr_t)1,tint,GH_STATIC_BINARY32_REFERENCE,&pc)&&!pc&&fegetround()==modes[mode]&&fetestexcept(FE_ALL_EXCEPT)==FE_DIVBYZERO);
  C(!memcmp(normal,oldn,60)&&normal[15]==0x3f800000&&!memcmp(color,oldc,48)&&!memcmp(color+12,product,16));
  /* Changed prior-frame words must survive a second invocation too. */
  normal[12]=0xdeadbeef;color[3]=0xaabbccdd;C(!gh_actor_ambient_fast_002d8008(s,normal,color,NULL,tint,GH_STATIC_BINARY32_REFERENCE,&pc)&&normal[12]==0xdeadbeef&&color[3]==0xaabbccdd);
  memcpy(oldn,normal,64);memcpy(oldc,color,64);float bad[4]={1,2,3,NAN};
  C(gh_actor_ambient_fast_002d8008(s,normal,color,NULL,bad,GH_STATIC_BINARY32_REFERENCE,&pc)==GH_ACTOR_MODEL_UNSUPPORTED&&pc==0x30a4d0&&!memcmp(normal,oldn,64)&&!memcmp(color,oldc,64)&&fegetround()==modes[mode]&&fetestexcept(FE_ALL_EXCEPT)==FE_DIVBYZERO);
  float subnormal[4]={0x1p-149f,1,1,1};C(gh_actor_ambient_fast_002d8008(s,normal,color,NULL,subnormal,GH_STATIC_BINARY32_REFERENCE,&pc)==GH_ACTOR_MODEL_UNSUPPORTED&&!memcmp(normal,oldn,64)&&!memcmp(color,oldc,64));
 }
 C(!fesetround(FE_TONEAREST));free(s);puts("PASS actual2D8008 five-word writes, unread position/poison matrices, prior-frame retention, fourFEnv and late atomic failure");return 0;
}
