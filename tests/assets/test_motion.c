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
static unsigned half(const uint8_t *p){return p[0]|(unsigned)p[1]<<8;}
static float number(const uint8_t *p){float value;memcpy(&value,p,sizeof(value));return value;}
static void test_ev04_camera_arc(gh_disc *disc)
{
 size_t index;
 gh_disc_entry entry;
 C(!gh_disc_find(disc,"event/ev04.dat",&index)&&!gh_disc_entry_at(disc,index,&entry));
 uint8_t *data=malloc((size_t)entry.size);
 C(data&&!gh_disc_read(disc,index,0,data,(size_t)entry.size));
 const size_t camera=rd(data+4)+4*12+4;
 C(camera+4<=entry.size&&rd(data+camera)==0x40f0u);
 gh_motion_view view;
 C(!gh_motion_open_mtb3(data,(size_t)entry.size,rd(data+camera),&view)&&view.frames==42);
 gh_motion_track track;
 C(!gh_motion_track_at(&view,6,&track)&&track.packed==0x002a4300u&&track.keys==42);
 const size_t values=view.offset+track.data;
 C(values+8+(size_t)track.keys*2<=entry.size);
 const float scale=number(data+values+4),bias=number(data+values);
 float sample;
 gh_motion_cache cache;
 uint32_t frontier;
 for(unsigned frame=0;frame+1<view.frames;frame++){
  const volatile float low=scale*(float)half(data+values+8+frame*2);
  const volatile float high=scale*(float)half(data+values+10+frame*2);
  C(!gh_motion_sample_reference(&view,6,(float)frame+.5f,&sample,&cache,&frontier)&&!frontier);
  const volatile float a=low+bias,b=high+bias,delta=b-a;
  const volatile float movement=delta*.5f;
  C(sample==a+movement&&cache.first_frame==frame&&cache.next_frame==frame+1);
 }
 C(!gh_motion_sample_reference(&view,6,41,&sample,&cache,&frontier)&&sample==0);
 free(data);

 /* The original angular branch is taken at integer frames with another
  * sample. Exact final sample lacks this wrap, as in 159230..159254. */
 uint8_t fixture[40]={0};
 memcpy(fixture,"mtb3",4);fixture[4]=2;fixture[6]=1;
 wr(fixture+8,0x00024300);wr(fixture+16,24);
 sf(fixture+24,3.125f);sf(fixture+28,-6.25f);fixture[34]=1;
 C(!gh_motion_open_mtb3(fixture,sizeof(fixture),0,&view));
 C(!gh_motion_sample_reference(&view,0,0,&sample,&cache,&frontier)&&sample==3.125f);
 C(!gh_motion_sample_reference(&view,0,.5f,&sample,&cache,&frontier)&&
   fabsf(fabsf(sample)-0x1.921fb6p+1f)<1e-6f);
 C(!gh_motion_sample_reference(&view,0,1,&sample,&cache,&frontier)&&sample==-3.125f);
 sf(fixture+24,6.25f);
 C(!gh_motion_sample_reference(&view,0,0,&sample,&cache,&frontier)&&sample<0&&sample>-.04f);
 fixture[34]=0;
 C(!gh_motion_sample_reference(&view,0,1,&sample,&cache,&frontier)&&sample==6.25f);
 puts("PASS actual EV04 cut4 42-key camera roll and original short arc, endpoint wrap and final sample");
}


int main(int argc,char**argv){C(argc==2);gh_disc*d;C(!gh_disc_open(argv[1],&d));size_t index;gh_disc_entry e;C(!gh_disc_find(d,"pl/pl00.dat",&index)&&!gh_disc_entry_at(d,index,&e));size_t n=(size_t)e.size;uint8_t*raw=malloc(n);C(raw&&!gh_disc_read(d,index,0,raw,n));test_ev04_camera_arc(d);gh_disc_close(d);
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
