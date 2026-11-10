#include "game_actor_light_normalize.h"
#include <math.h>
#include <float.h>
#include <fenv.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define C(x) do{if(!(x)){fprintf(stderr,"line%d %s\n",__LINE__,#x);exit(1);}}while(0)
int main(void){
 const int modes[4]={FE_TONEAREST,FE_DOWNWARD,FE_UPWARD,FE_TOWARDZERO};uint32_t pc;
 const float directions[][3]={{-.5f,-.5f,-.5f},{.5f,.5f,.5f},{.5f,-.5f,.5f},{3,4,12},{-9,2,1}};
 for(unsigned mode=0;mode<4;mode++){
  C(!fesetround(modes[mode]));C(!feraiseexcept(FE_INVALID));
  for(unsigned i=0;i<sizeof(directions)/sizeof(directions[0]);i++){
   float out[4];C(!gh_actor_light_normalize_0030a370(directions[i],out,GH_STATIC_BINARY32_REFERENCE,&pc)&&!pc&&out[3]==0&&fegetround()==modes[mode]&&(fetestexcept(FE_INVALID)&FE_INVALID));
   long double length=sqrtl((long double)directions[i][0]*directions[i][0]+(long double)directions[i][1]*directions[i][1]+(long double)directions[i][2]*directions[i][2]);
   for(unsigned k=0;k<3;k++)C(fabsl(out[k]-directions[i][k]/length)<2e-7L);
  }
  for(unsigned bits=0;bits<8;bits++){
   float z[4];for(unsigned k=0;k<3;k++)z[k]=(bits&(1u<<k))?-0.0f:0.0f;z[3]=123;
   C(!gh_actor_light_normalize_0030a370(z,z,GH_STATIC_BINARY32_REFERENCE,&pc)&&!pc&&z[3]==0);
   for(unsigned k=0;k<3;k++)C(z[k]==0&&!!signbit(z[k])==!!(bits&(1u<<k)));
  }
  float bad[][3]={{INFINITY,0,0},{NAN,0,0},{FLT_MAX,0,0},{0x1p-149f,0,0},{0x1p-80f,0,0}};
  for(unsigned i=0;i<sizeof(bad)/sizeof(bad[0]);i++){float out[4]={3,4,5,6},copy[4];memcpy(copy,out,sizeof(copy));C(gh_actor_light_normalize_0030a370(bad[i],out,GH_STATIC_BINARY32_REFERENCE,&pc)==GH_ACTOR_MODEL_UNSUPPORTED&&pc==0x30a370&&!memcmp(copy,out,sizeof(copy))&&fegetround()==modes[mode]);}
  C(!feclearexcept(FE_ALL_EXCEPT));
 }
 C(!fesetround(FE_TONEAREST));puts("PASS source30A370 normal directions, independent longdouble oracle, signed zero DIV saturation, W0, in-place, FEnv and atomic strict domains");return 0;
}
