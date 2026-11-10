#include "game_axis_angle.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fenv.h>
#define C(x) do{if(!(x)){fprintf(stderr,"line%d %s\n",__LINE__,#x);exit(1);}}while(0)
int main(void){const float axes[][3]={{1,0,0},{0,1,0},{0,0,1},{-1,0,0},{0,-1,0},{1,2,3},{-2,1,4},{.25f,-.5f,.125f}};const float angles[]={0,.25f,-.5f,.75f,1.5f};const float pivot[4]={2,-4,8,1};const int modes[]={FE_TONEAREST,FE_DOWNWARD,FE_UPWARD,FE_TOWARDZERO};unsigned checks=0;
 for(unsigned i=0;i<8;i++)for(unsigned j=0;j<5;j++){float exact[16];for(unsigned mode=0;mode<4;mode++){C(!fesetround(modes[mode])&&!feclearexcept(FE_ALL_EXCEPT)&&!feraiseexcept(FE_INVALID));float actual[16];uint32_t f;C(!gh_axis_angle_matrix_0015af30(angles[j],axes[i],pivot,actual,GH_STATIC_BINARY32_REFERENCE,&f)&&!f&&fegetround()==modes[mode]&&fetestexcept(FE_ALL_EXCEPT)==FE_INVALID);if(!mode)memcpy(exact,actual,64);else C(!memcmp(exact,actual,64));C(!fesetround(FE_TONEAREST));
  long double norm=sqrtl((long double)axes[i][0]*axes[i][0]+(long double)axes[i][1]*axes[i][1]+(long double)axes[i][2]*axes[i][2]),a[3];for(unsigned k=0;k<3;k++)a[k]=axes[i][k]/norm;long double c=cosl(angles[j]),s=sinl(angles[j]),expected[16]={0};expected[15]=1;for(unsigned col=0;col<3;col++)for(unsigned row=0;row<3;row++){long double skew=0;if(row!=col){unsigned k=3-row-col;skew=((row==1&&col==0)||(row==2&&col==1)||(row==0&&col==2))?a[k]:-a[k];}expected[col*4+row]=(row==col?c:0)+(1-c)*a[row]*a[col]+s*skew;}for(unsigned row=0;row<3;row++){expected[12+row]=pivot[row];for(unsigned col=0;col<3;col++)expected[12+row]-=expected[col*4+row]*pivot[col];}for(unsigned k=0;k<16;k++)C(fabsl(actual[k]-expected[k])<0.00005L);checks++;
 }}
 float out[16],before[16],axis[3]={NAN,0,0};memset(out,0xa5,64);memcpy(before,out,64);uint32_t f;C(gh_axis_angle_matrix_0015af30(.5f,axis,pivot,out,GH_STATIC_BINARY32_REFERENCE,&f)==GH_ACTOR_MATRIX_UNSUPPORTED&&f==0x15af30&&!memcmp(before,out,64));C(!fesetround(FE_TONEAREST));printf("PASS actual15AF30 align/transposedSDKrotation/pivot, independent longdouble Rodrigues %u cases/4FEnv+exceptions/outputatomic; explicitbinary32reference nothardware\n",checks);return 0;}
