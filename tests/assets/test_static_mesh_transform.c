#include "../../src/assets/static_mesh_transform.h"
#include "../../src/platform/disc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fenv.h>
#include <math.h>
#define C(x) do{if(!(x)){fprintf(stderr,"line%d %s\n",__LINE__,#x);exit(1);}}while(0)
static uint32_t bits(float f){uint32_t n;memcpy(&n,&f,4);return n;}
static uint64_t word(uint64_t h,uint32_t n){for(unsigned i=0;i<4;i++)h=(h^((n>>(i*8))&255))*UINT64_C(1099511628211);return h;}
static uint64_t hash(const gh_static_gs_vertex*v,const uint32_t*indices,const gh_static_geometry_result*r){uint64_t h=UINT64_C(14695981039346656037);
 for(size_t i=0;i<r->vertices;i++){for(unsigned j=0;j<3;j++)h=word(h,bits(v[i].stq[j]));for(unsigned j=0;j<4;j++)h=word(h,v[i].rgba[j]);for(unsigned j=0;j<3;j++)h=word(h,(uint32_t)v[i].xyz_fixed4[j]);h=word(h,v[i].fog_fixed4);h=word(h,v[i].raw_control);h=word(h,v[i].clip_flags);h=word(h,v[i].adc);h=word(h,v[i].triangle_tested);}
 for(size_t i=0;i<r->triangles*3;i++)h=word(h,indices[i]);return h;}
int main(int argc,char**argv){C(argc==2);gh_disc*d;size_t i;gh_disc_entry file;C(!gh_disc_open(argv[1],&d)&&!gh_disc_find(d,"st1/r100.dat",&i)&&!gh_disc_entry_at(d,i,&file));
 uint8_t*room=malloc((size_t)file.size);C(room&&!gh_disc_read(d,i,0,room,(size_t)file.size));gh_bundle bundle,scp;gh_bundle_entry child,scr;
 C(!gh_bundle_open(room,(size_t)file.size,&bundle)&&!gh_bundle_find(&bundle,"SCP",0,&child)&&!gh_bundle_open(child.bytes,child.size,&scp)&&!gh_bundle_find(&scp,"SCR",0,&scr));
 long n=(long)scr.size;C(n==12640);uint8_t*b=malloc((size_t)n);C(b);memcpy(b,scr.bytes,(size_t)n);free(room);gh_disc_close(d);
 gh_scene_mesh_stream s={.bytes=b,.size=(size_t)n,.header_offset=128,.position_offset=160,.uv_offset=544,.color_offset=736,.vertices=48,.material=37};
 /* Explicit numeric oracle fixtures, NOT invented live game-camera state. */
 gh_static_transform_inputs in={.screen_matrix={100,0,0,0,0,-100,0,0,0,0,1,.05f,2048,2048,1000,2},.clip_matrix={.25f,0,0,0,0,.25f,0,0,0,0,.25f,0,0,0,0,1},.uv_offset={.25f,-.125f},.color_scale={.75f,1,1.25f,.5f},.fog={100,-25}};
 const int modes[4]={FE_TONEAREST,FE_DOWNWARD,FE_UPWARD,FE_TOWARDZERO};gh_static_gs_vertex v[38],saved[38];uint32_t indices[108];gh_static_geometry_result r;gh_static_transform_frontier frontier;
 for(unsigned m=0;m<4;m++)for(size_t batch=0;batch<2;batch++){
  C(!fesetround(modes[m])&&!feclearexcept(FE_ALL_EXCEPT)&&!feraiseexcept(FE_DIVBYZERO));
  C(!gh_static_mesh_transform_batch(&s,0x1001000,batch,&in,GH_STATIC_BINARY32_REFERENCE,v,38,indices,108,&r,&frontier));
  C(fegetround()==modes[m]&&fetestexcept(FE_ALL_EXCEPT)==FE_DIVBYZERO&&!frontier.ee_entry&&!frontier.vu_byte_pc);
  C(r.vertices==(batch?12:38)&&r.triangles==(batch?2:6)&&r.material==37&&r.primitive==0x7c);
  C(hash(v,indices,&r)==(batch?UINT64_C(0x2c638b4a467c4373):UINT64_C(0xe29f8fcf28105766))); /* independentPython orderedbinary32 oracle */
 }
 C(!fesetround(FE_TONEAREST));
 C(!gh_static_mesh_transform_batch(&s,0x1001000,0,&in,GH_STATIC_BINARY32_REFERENCE,v,38,indices,108,&r,&frontier));
 C(v[0].xyz_fixed4[0]==16704&&v[0].xyz_fixed4[1]==15870&&v[0].xyz_fixed4[2]==7880&&v[0].fog_fixed4==787&&v[0].rgba[0]==42&&v[0].rgba[3]==64&&v[0].adc&&!v[0].triangle_tested);
 const uint32_t expected[18]={0,1,2,2,1,3,12,13,14,14,13,15,24,25,26,26,25,27};C(!memcmp(expected,indices,sizeof(expected)));
 memcpy(saved,v,sizeof(v));gh_static_geometry_result before=r;
 C(gh_static_mesh_transform_batch(&s,0x1001000,0,&in,0,v,38,indices,108,&r,&frontier)==GH_STATIC_UNSUPPORTED&&frontier.vu_byte_pc==0x2138&&!memcmp(saved,v,sizeof(v))&&!memcmp(&before,&r,sizeof(r)));
 gh_static_transform_inputs bad=in;bad.screen_matrix[15]=0;bad.screen_matrix[11]=0;
 C(gh_static_mesh_transform_batch(&s,0x1001000,0,&bad,GH_STATIC_BINARY32_REFERENCE,v,38,indices,108,&r,&frontier)==GH_STATIC_UNSUPPORTED&&frontier.vu_byte_pc==0x21b0&&!memcmp(saved,v,sizeof(v)));
 C(gh_static_mesh_transform_batch(&s,0x1001000,0,&in,GH_STATIC_BINARY32_REFERENCE,v,37,indices,108,&r,&frontier)==GH_STATIC_BOUNDS&&!memcmp(saved,v,sizeof(v)));
 bad=in;bad.clip_matrix[12]=100;C(!gh_static_mesh_transform_batch(&s,0x1001000,0,&bad,GH_STATIC_BINARY32_REFERENCE,v,38,indices,108,&r,&frontier)&&!r.triangles&&v[2].clip_flags==1&&v[2].adc); /* actuallayer14 suppresses clippedtriangles; no inventedpolygonclip */
 bad=in;bad.color_scale[0]=NAN;C(gh_static_mesh_transform_batch(&s,0x1001000,0,&bad,GH_STATIC_BINARY32_REFERENCE,v,38,indices,108,&r,&frontier)==GH_STATIC_ARGUMENT);
 free(b);puts("PASS actualR100 layer14 algorithm with independentbinary32 geometry/UV/color/fog/clip/cull oracle,4FEnvmodes, boundedatomicfailure; nothardwarecapture/livegamecamera");return 0;
}
