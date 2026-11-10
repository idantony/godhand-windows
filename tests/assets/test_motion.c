#include "../assets/motion.h"
#include "../platform/disc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <fenv.h>
#define C(x) do{if(!(x)){fprintf(stderr,"FAIL %d %s\n",__LINE__,#x);exit(1);}}while(0)
static uint32_t rd(const void*p){uint32_t x;memcpy(&x,p,4);return x;}
static void wr(void*p,uint32_t x){memcpy(p,&x,4);}
static void sf(void*p,float x){memcpy(p,&x,4);}
int main(int argc,char**argv){C(argc==2);gh_disc*d;C(!gh_disc_open(argv[1],&d));size_t index;gh_disc_entry e;C(!gh_disc_find(d,"pl/pl00.dat",&index)&&!gh_disc_entry_at(d,index,&e));size_t n=(size_t)e.size;uint8_t*raw=malloc(n);C(raw&&!gh_disc_read(d,index,0,raw,n));gh_disc_close(d);
 gh_motion_view m;C(!gh_motion_open_mtb3(raw,n,rd(raw+0x28),&m)&&m.frames==61&&m.count==81);gh_motion_track t;C(!gh_motion_track_at(&m,0,&t)&&t.packed==0x1410ff&&t.method==1&&t.keys==20);float value;gh_motion_cache cache;uint32_t frontier;
 const int modes[]={FE_TONEAREST,FE_DOWNWARD,FE_UPWARD,FE_TOWARDZERO};uint32_t golden[4][80];
 for(unsigned mode=0;mode<4;mode++){C(!fesetround(modes[mode]));for(unsigned frame=0;frame<4;frame++){float tm=(float)(frame==3?60:frame==2?30:frame);for(unsigned track=0;track<80;track++){
  C(!gh_motion_sample_reference(&m,track,tm,&value,&cache,&frontier)&&isfinite(value)&&fegetround()==modes[mode]);uint32_t bits=rd(&value);if(!mode)golden[frame][track]=bits;else C(bits==golden[frame][track]);
 }} }C(!fesetround(FE_TONEAREST));
 C(golden[0][0]==0xb8680000&&golden[2][0]==0x3bdf4800&&golden[3][0]==0xb8680000);C(golden[1][0]>=0x3b8ca04b&&golden[1][0]<=0x3b8ca04d); /* independent70-digit Decimal oracle nearest0x3B8CA04C, <=1ULP binary32 */
 C(gh_motion_sample_reference(&m,80,0,&value,&cache,&frontier)==GH_MOTION_UNSUPPORTED&&frontier==0x3bd910);
 uint8_t fixture[64]={0};memcpy(fixture,"mtb3",4);fixture[4]=5;fixture[6]=1;wr(fixture+8,0x25000);wr(fixture+16,32); /* method5, two keys */
 sf(fixture+36,2);sf(fixture+44,4);fixture[48]=4;sf(fixture+52,6);sf(fixture+56,0);sf(fixture+60,0);gh_motion_view f;C(!gh_motion_open_mtb3(fixture,sizeof(fixture),0,&f));
 C(!gh_motion_sample_reference(&f,0,0,&value,&cache,&frontier)&&value==2);C(!gh_motion_sample_reference(&f,0,4,&value,&cache,&frontier)&&value==6);C(!gh_motion_sample_reference(&f,0,2,&value,&cache,&frontier)&&value==4.5f&&cache.first_frame==0&&cache.next_frame==4&&cache.out_tangent==4);
 gh_motion_view truncated=f;truncated.size=63;value=123;C(gh_motion_sample_reference(&truncated,0,2,&value,&cache,&frontier)==GH_MOTION_BOUNDS&&value==123);fixture[48]=0;C(gh_motion_sample_reference(&f,0,2,&value,&cache,&frontier)==GH_MOTION_FORMAT);
 printf("PASS actualPL00 80realtracks x4times x4FEnv, finitevalues/caches, originalsentinel rejection, knownHermite2/6 tangents4/0 midpoint4.5, atomicbounds; sample firstroot %08x/%08x/%08x/%08x\n",golden[0][0],golden[1][0],golden[2][0],golden[3][0]);free(raw);return 0;}
