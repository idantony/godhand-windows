#include "../../src/assets/static_mesh_clip10.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fenv.h>
#define C(x) do{if(!(x)){fprintf(stderr,"line%d %s\n",__LINE__,#x);exit(1);}}while(0)
static gh_static_clip10_vertex v(float x,float y,float z){gh_static_clip10_vertex r={{x,y,z,1},{x,y},{100,120,140,160}};return r;}
int main(void){
 gh_static_clip10_vertex tri[3]={v(-.5f,-.5f,0),v(2,-.5f,0),v(-.5f,.5f,0)};
 gh_static_clip10_inputs in={.projection={100,0,0,0,0,100,0,0,0,0,1,0,2048,2048,1000,1},.fog={100,-25}};
 gh_static_gs_vertex out[9]={0},saved[9];uint32_t indices[21];gh_static_geometry_result r;gh_static_transform_frontier f;
 const int modes[]={FE_TONEAREST,FE_DOWNWARD,FE_UPWARD,FE_TOWARDZERO};
 for(unsigned mode=0;mode<4;mode++){
  C(!fesetround(modes[mode])&&!feclearexcept(FE_ALL_EXCEPT)&&!feraiseexcept(FE_DIVBYZERO));
  C(!gh_static_clip10_triangle(tri,&in,0x7d,GH_STATIC_BINARY32_REFERENCE,out,9,indices,21,&r,&f));
  C(fegetround()==modes[mode]&&fetestexcept(FE_ALL_EXCEPT)==FE_DIVBYZERO&&!f.ee_entry);
  C(r.vertices==4&&r.triangles==2&&r.primitive==0x7d);
  const int32_t xy[8]={31968,31968,34368,31968,34368,32608,31968,33568};
  for(unsigned i=0;i<4;i++){C(out[i].xyz_fixed4[0]==xy[i*2]&&out[i].xyz_fixed4[1]==xy[i*2+1]&&out[i].xyz_fixed4[2]==16000);C(out[i].fog_fixed4==1200&&out[i].stq[2]==1&&!out[i].adc);C(out[i].rgba[0]==100&&out[i].rgba[3]==160);}
  const uint32_t ix[6]={0,1,2,0,2,3};C(!memcmp(ix,indices,sizeof(ix)));
  C(out[0].stq[0]==-.5f&&out[1].stq[0]==1&&out[2].stq[0]==1&&out[3].stq[0]==-.5f);
 }
 C(!fesetround(FE_TONEAREST));
 gh_static_clip10_vertex gradient[3]={tri[0],tri[1],tri[2]};
 const uint32_t colors[3][4]={{0,40,80,100},{100,140,180,200},{200,240,40,0}};
 for(unsigned i=0;i<3;i++)memcpy(gradient[i].rgba,colors[i],sizeof(colors[i]));
 C(!gh_static_clip10_triangle(gradient,&in,0x7d,GH_STATIC_BINARY32_REFERENCE,out,9,indices,21,&r,&f));
 const uint32_t expected_colors[4][4]={{0,40,80,100},{60,100,140,160},{140,180,124,120},{200,240,40,0}};
 for(unsigned i=0;i<4;i++)C(!memcmp(out[i].rgba,expected_colors[i],sizeof(expected_colors[i])));
 memcpy(saved,out,sizeof(out));gh_static_geometry_result old=r;
 C(gh_static_clip10_triangle(tri,&in,0x7d,GH_STATIC_BINARY32_REFERENCE,out,3,indices,21,&r,&f)==GH_STATIC_BOUNDS&&!memcmp(saved,out,sizeof(out))&&!memcmp(&old,&r,sizeof(r)));
 gh_static_clip10_vertex reverse[3]={tri[2],tri[1],tri[0]};
 C(!gh_static_clip10_triangle(reverse,&in,0x7d,GH_STATIC_BINARY32_REFERENCE,out,9,indices,21,&r,&f)&&!r.vertices&&!r.triangles);
 for(unsigned axis=0;axis<3;axis++)for(unsigned side=0;side<2;side++){
  unsigned other=(axis+1)%3;float direction=side?1:-1;
  gh_static_clip10_vertex part[3]={v(0,0,0),v(0,0,0),v(0,0,0)};
  const float horizontal[3]={-.5f,2,-.5f},vertical[3]={-.5f,-.5f,.5f};
  gh_static_clip10_inputs mapped=in;memset(mapped.projection,0,sizeof(mapped.projection));
  mapped.projection[axis*4]=100*direction;mapped.projection[other*4+1]=100;
  mapped.projection[12]=mapped.projection[13]=2048;mapped.projection[14]=1000;mapped.projection[15]=1;
  for(unsigned i=0;i<3;i++){part[i].clip[axis]=horizontal[i]*direction;part[i].clip[other]=vertical[i];}
  C(!gh_static_clip10_triangle(part,&mapped,0x7d,GH_STATIC_BINARY32_REFERENCE,out,9,indices,21,&r,&f)&&r.vertices==4&&r.triangles==2);
  C(out[1].xyz_fixed4[0]==34368&&out[2].xyz_fixed4[1]==32608);
 }
 for(unsigned axis=0;axis<3;axis++)for(unsigned side=0;side<2;side++){
  gh_static_clip10_vertex outside[3]={v(-.5f,-.5f,0),v(.5f,-.5f,0),v(-.5f,.5f,0)};
  for(unsigned i=0;i<3;i++)outside[i].clip[axis]=side?2:-2;
  C(!gh_static_clip10_triangle(outside,&in,0x5d,GH_STATIC_BINARY32_REFERENCE,out,9,indices,21,&r,&f)&&!r.vertices&&!r.triangles&&r.primitive==0x5d);
 }
 in.projection[15]=0;C(gh_static_clip10_triangle(tri,&in,0x7d,GH_STATIC_BINARY32_REFERENCE,out,9,indices,21,&r,&f)==GH_STATIC_UNSUPPORTED&&f.vu_byte_pc==0xd48);
 puts("PASS originalprogram10 sixplane clipping/fan explicitbinary32 fixture, intersections/ordering/cull/GSfixed outputs/4FEnv/atomicbounds; nothardwarecapture");return 0;
}
