#define main original_blend_test
#include "test_skinned_mesh_blend.c"
#undef main
#include "skinned_mesh_transform30.h"
#include <float.h>
#include <limits.h>
static void store_word(void*p,uint32_t n){memcpy(p,&n,4);}
static gh_static_mesh_status run(unsigned program,const gh_skinned_mesh_stream*s,const gh_skinned_program2_inputs*in,gh_static_gs_vertex*v,uint32_t*i,gh_static_geometry_result*r,gh_static_transform_frontier*f){if(program==2)return gh_skinned_transform_program2_batch(s,0,in,GH_STATIC_BINARY32_REFERENCE,v,26,i,72,r,f);gh_skinned_program30_inputs p={.base=*in,.half_vector={0,0,1},.squarings=2};return gh_skinned_transform_program30_batch(s,0,&p,GH_STATIC_BINARY32_REFERENCE,v,26,i,72,r,f);}
static gh_skinned_program2_inputs inputs(float palette[42][16]){gh_skinned_program2_inputs p={.palette=(uint8_t*)palette,.palette_bytes=42*64,.part_count=42,.fog=64};for(unsigned k=0;k<4;k++){p.screen[k*5]=1;p.normal_light[k*5]=1;p.light_color[k*5]=1;}p.normal_light[12]=p.normal_light[13]=p.normal_light[14]=p.normal_light[15]=1;p.clip[15]=1;return p;}
int main(int argc,char**argv){C(argc==2);float palette[42][16]={0};for(unsigned i=0;i<42;i++)for(unsigned k=0;k<4;k++)palette[i][k*5]=1;
 /* Known numerical primitive is a UNIT TEST, not original game imagery. */
 uint8_t storage[288]={0};gh_skinned_mesh_stream s={.bytes=storage,.size=sizeof(storage),.vertices=7,.position_offset=0,.normal_offset=128,.uv_offset=160,.color_offset=192,.skin_offset=224};
 const float p[7][3]={{-.5f,-.5f,0},{-.5f,.5f,0},{.5f,-.5f,0},{.5f,.5f,0},{0,0,0},{-.5f,-.5f,0},{-.5f,.5f,0}};
 for(unsigned i=0;i<8;i++){if(i<7)memcpy(storage+i*16,p[i],12);storage[128+i*4+2]=127;memset(storage+192+i*4,128,4);storage[224+i*8+4]=100;}store_word(storage+3*16+12,1);store_word(storage+4*16+12,1);store_word(storage+5*16+12,1);
 const int modes[]={FE_TONEAREST,FE_DOWNWARD,FE_UPWARD,FE_TOWARDZERO};
 for(unsigned program=2;program<=30;program+=28)for(unsigned mode=0;mode<4;mode++){
  C(!fesetround(modes[mode]));gh_skinned_program2_inputs in=inputs(palette);in.clip[0]=1;in.clip[12]=.75f;gh_static_gs_vertex v[26];uint32_t idx[72];gh_static_geometry_result r;gh_static_transform_frontier f;C(!feclearexcept(FE_ALL_EXCEPT)&&!feraiseexcept(FE_DIVBYZERO));int flags=fetestexcept(FE_ALL_EXCEPT);
  C(!run(program,&s,&in,v,idx,&r,&f)&&r.vertices==7&&r.triangles==1&&idx[0]==4&&idx[1]==5&&idx[2]==6&&v[2].clip_flags==1&&v[3].clip_flags==1&&v[4].adc&&v[5].adc&&!v[6].adc);C(fegetround()==modes[mode]&&fetestexcept(FE_ALL_EXCEPT)==flags);
  in.screen[15]=-1;C(!run(program,&s,&in,v,idx,&r,&f)&&r.triangles==1&&idx[2]==6&&v[6].stq[2]==-1);/* Source has no behind-camera success/failure invention. */
  in=inputs(palette);in.clip[12]=0x1p-140f;C(!run(program,&s,&in,v,idx,&r,&f));for(unsigned i=0;i<7;i++)C(!v[i].clip_flags);in.clip[15]=0x1p-140f;C(!run(program,&s,&in,v,idx,&r,&f));for(unsigned i=0;i<7;i++)C(!v[i].clip_flags);in.clip[12]=0x1p-126f;C(!run(program,&s,&in,v,idx,&r,&f)&&!r.triangles);for(unsigned i=0;i<7;i++)C(v[i].clip_flags==1);
  for(unsigned axis=0;axis<3;axis++)for(unsigned sign=0;sign<2;sign++){in=inputs(palette);in.clip[12+axis]=sign?-2:2;C(!run(program,&s,&in,v,idx,&r,&f)&&r.vertices==7&&!r.triangles);for(unsigned i=0;i<7;i++)C(v[i].clip_flags==(1u<<(axis*2+sign)));}
  in=inputs(palette);in.clip[12]=2;in.screen[15]=0;C(!run(program,&s,&in,v,idx,&r,&f)&&!r.triangles&&v[0].stq[2]==FLT_MAX&&v[0].xyz_fixed4[0]==INT32_MIN&&v[2].xyz_fixed4[0]==INT32_MAX);
  /* Same zeroW without sourceADC/clip reaches an unsupported MAC overflow;
   * no fabricated cross sign or whole-packet success. */
  in.clip[12]=0;gh_static_gs_vertex before[26];memset(before,0xa5,sizeof(before));memcpy(v,before,sizeof(v));C(run(program,&s,&in,v,idx,&r,&f)==GH_STATIC_UNSUPPORTED&&f.vu_byte_pc==(program==2?0x490u:0x668u)&&!memcmp(v,before,sizeof(v)));
  store_word(storage+2*16+12,1);memcpy(v,before,sizeof(v));C(run(program,&s,&in,v,idx,&r,&f)==GH_STATIC_UNSUPPORTED&&f.vu_byte_pc==(program==2?0x4a8u:0x680u)&&!memcmp(v,before,sizeof(v)));store_word(storage+2*16+12,0);
  for(unsigned i=2;i<7;i++)store_word(storage+i*16+12,0x8000);C(!run(program,&s,&in,v,idx,&r,&f)&&!r.triangles);for(unsigned i=2;i<7;i++)store_word(storage+i*16+12,i==3||i==4||i==5?1:0);
  in=inputs(palette);in.clip[12]=2;in.screen[0]=1e10f;C(!run(program,&s,&in,v,idx,&r,&f)&&!r.triangles&&v[0].xyz_fixed4[0]==INT32_MIN&&v[2].xyz_fixed4[0]==INT32_MAX);
  in=inputs(palette);in.clip[0]=1;in.clip[12]=.75f;memcpy(storage+6*16,p[5],12);C(!run(program,&s,&in,v,idx,&r,&f)&&r.vertices==7&&r.triangles==1&&idx[0]==4&&idx[1]==5&&idx[2]==6&&v[5].xyz_fixed4[0]==v[6].xyz_fixed4[0]&&v[5].xyz_fixed4[1]==v[6].xyz_fixed4[1]);memcpy(storage+6*16,p[6],12);
  in=inputs(palette);in.clip[12]=2;for(unsigned k=0;k<4;k++)in.screen[3+k*4]=-0.0f;for(unsigned i=0;i<7;i++)for(unsigned k=0;k<3;k++){float positive=.5f;memcpy(storage+i*16+k*4,&positive,4);}C(!run(program,&s,&in,v,idx,&r,&f)&&!r.triangles&&v[0].stq[2]==-FLT_MAX&&v[0].xyz_fixed4[0]==INT32_MIN);for(unsigned i=0;i<7;i++)memcpy(storage+i*16,p[i],12);
  in=inputs(palette);in.screen[15]=FLT_MAX;memcpy(v,before,sizeof(v));C(run(program,&s,&in,v,idx,&r,&f)==GH_STATIC_UNSUPPORTED&&f.vu_byte_pc==(program==2?0x290u:0x280u)&&!memcmp(v,before,sizeof(v)));
  /* FiniteUV/Q is still required even when a source triangle is clipped. */
  in.screen[15]=0;in.uv[0]=FLT_MAX;memcpy(v,before,sizeof(v));C(run(program,&s,&in,v,idx,&r,&f)==GH_STATIC_UNSUPPORTED&&f.vu_byte_pc==(program==2?0x460u:0x608u)&&!memcmp(v,before,sizeof(v)));
  C(fegetround()==modes[mode]);
 }
 C(!fesetround(FE_TONEAREST));
 /* Real ISO source: all six clipplanes, negativeW and zeroW recover known
  * source-discarded geometry without dropping the logical batch/packet. */
 gh_disc*d;C(!gh_disc_open(argv[1],&d));size_t index;gh_disc_entry entry;C(!gh_disc_find(d,"pl/pl00.dat",&index)&&!gh_disc_entry_at(d,index,&entry));uint8_t*raw=malloc((size_t)entry.size);C(raw&&!gh_disc_read(d,index,0,raw,(size_t)entry.size));gh_disc_close(d);gh_bundle b;gh_bundle_entry md;gh_model_view model;C(!gh_bundle_open(raw,(size_t)entry.size,&b)&&!gh_bundle_find(&b,"MD",0,&md)&&!gh_model_open_scr(md.bytes,md.size,&model));gh_skinned_mesh_stream actual;C(!gh_skinned_mesh_stream_at(&model,0,0,&actual));
 unsigned cases=0,strict_uv=0;for(unsigned program=2;program<=30;program+=28)for(unsigned axis=0;axis<3;axis++)for(unsigned sign=0;sign<2;sign++)for(unsigned w=0;w<3;w++){
  gh_skinned_program2_inputs in=inputs(palette);in.clip[12+axis]=sign?-2:2;in.screen[15]=w==0?1:w==1?-1:0;in.uv[0]=in.uv[1]=-.5f;gh_static_gs_vertex v[26];uint32_t idx[72];gh_static_geometry_result r;gh_static_transform_frontier f;gh_static_gs_vertex untouched[26];memset(untouched,0xa5,sizeof(untouched));memcpy(v,untouched,sizeof(v));gh_static_mesh_status status=run(program,&actual,&in,v,idx,&r,&f);
  if(w==2){C(status==GH_STATIC_UNSUPPORTED&&f.vu_byte_pc==(program==2?0x460u:0x608u)&&!memcmp(v,untouched,sizeof(v)));strict_uv++;}
  else {C(!status&&r.vertices==(actual.vertices<26?actual.vertices:26)&&!r.triangles);for(size_t i=0;i<r.vertices;i++)C(v[i].clip_flags==(1u<<(axis*2+sign)));cases++;}
 }
 free(raw);printf("PASS program2/30 partialCLIP history releases triangle456, all6planes, negativeW preserved, zeroW signedMax/FTOI4 saturation/sourceADC discard, strict unsafeMAC/UV atomic,4FEnv, %u actualPL00 discard cases/%u precise unsafezeroUV frontiers; no GPU claim\n",cases,strict_uv);return 0;
}
