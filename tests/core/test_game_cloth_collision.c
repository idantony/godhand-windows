#include "game_cloth_collision.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fenv.h>
#define C(x) do{if(!(x)){fprintf(stderr,"line%d %s\n",__LINE__,#x);exit(1);}}while(0)
static void sphere(gh_cloth_geometry*g,unsigned i,float x,float y,float z,float radius){float center[3]={x,y,z};memcpy(g->records[i]+0x90,center,12);memcpy(g->records[i]+0xb0,&radius,4);}
int main(void){gh_cloth_geometry g={0};g.count=1;sphere(&g,0,.5f,.25f,0,.5f);const int modes[4]={FE_TONEAREST,FE_DOWNWARD,FE_UPWARD,FE_TOWARDZERO};uint32_t f;for(unsigned i=0;i<4;i++){float a[3]={1,0,0},b[3]={0,0,0};C(!fesetround(modes[i])&&!feclearexcept(FE_ALL_EXCEPT)&&!feraiseexcept(FE_INVALID));C(!gh_cloth_segment_body_00107360(&g,a,b,GH_STATIC_BINARY32_REFERENCE,&f)&&!f&&a[0]==1&&b[0]==0&&a[1]==-.25f&&b[1]==-.25f&&a[2]==0&&b[2]==0&&fegetround()==modes[i]&&fetestexcept(FE_ALL_EXCEPT)==FE_INVALID);}
 /* Non-unit regression distinguishes the observed original formula from a
  * textbook segment/sphere projection: length2 misses this sphere. */
 sphere(&g,0,1,.25f,0,.5f);float a[3]={2,0,0},b[3]={0,0,0},beforeA[3],beforeB[3];memcpy(beforeA,a,12);memcpy(beforeB,b,12);C(!gh_cloth_segment_body_00107360(&g,a,b,GH_STATIC_BINARY32_REFERENCE,&f)&&!memcmp(a,beforeA,12)&&!memcmp(b,beforeB,12));
 g.count=2;sphere(&g,0,.5f,.25f,0,.5f);sphere(&g,1,.5f,-.5f,0,.5f);a[0]=1;a[1]=a[2]=b[0]=b[1]=b[2]=0;C(!gh_cloth_segment_body_00107360(&g,a,b,GH_STATIC_BINARY32_REFERENCE,&f)&&a[1]==0&&b[1]==0);uint8_t swap[0xc0];memcpy(swap,g.records[0],sizeof(swap));memcpy(g.records[0],g.records[1],sizeof(swap));memcpy(g.records[1],swap,sizeof(swap));a[0]=1;a[1]=a[2]=b[0]=b[1]=b[2]=0;C(!gh_cloth_segment_body_00107360(&g,a,b,GH_STATIC_BINARY32_REFERENCE,&f)&&a[1]==-.25f&&b[1]==-.25f);g.count=1;
 sphere(&g,0,0,0,0,.5f);a[0]=a[1]=a[2]=b[0]=b[1]=b[2]=0;C(!gh_cloth_segment_body_00107360(&g,a,b,GH_STATIC_BINARY32_REFERENCE,&f)&&a[1]==.5f&&b[1]==.5f);g.count=2;sphere(&g,0,.5f,.25f,0,.5f);uint32_t capsule=1;memcpy(g.records[1],&capsule,4);a[0]=1;a[1]=a[2]=b[0]=b[1]=b[2]=0;memcpy(beforeA,a,12);memcpy(beforeB,b,12);C(gh_cloth_segment_body_00107360(&g,a,b,GH_STATIC_BINARY32_REFERENCE,&f)==GH_CLOTH_UNSUPPORTED&&f==0x107808&&!memcmp(a,beforeA,12)&&!memcmp(b,beforeB,12));C(!gh_cloth_segment_body_00107360(NULL,a,b,(gh_static_transform_policy)0,&f)&&!memcmp(a,beforeA,12));C(!fesetround(FE_TONEAREST));puts("PASS actual107360 spherebranch exactunit/nonunit-originalprojection/zerosegmentYfallback/4FEnv/exceptions/capsuleatomicafterpriorpush/NULLgenuinebranch; no textbookformula substitution");return 0;}
