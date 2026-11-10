#include "skinned_mesh_blend.h"
#include "core_bundle.h"
#include "../../src/platform/disc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <fenv.h>
#define C(x) do{if(!(x)){fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x);exit(1);}}while(0)
static uint64_t hash(uint64_t h,const void*p,size_t n){const uint8_t*b=p;for(size_t i=0;i<n;i++)h=(h^b[i])*UINT64_C(1099511628211);return h;}
static void oracle(const gh_skinned_mesh_vertex*v,const float p[42][16],long double position[3],long double normal[3]){
 const long double unit=.00999999977648258209228515625L,nscale=.0078740157186985015869140625L;
 for(unsigned row=0;row<3;row++){
  long double result_p=0,result_n=0;
  for(unsigned bone=0;bone<3;bone++){
   unsigned slot=v->bone_indices[bone+1]/4;long double transform_p=p[slot][12+row],transform_n=0;
   for(unsigned axis=0;axis<3;axis++){
    long double signed_n=v->normal[axis];if(signed_n>=128)signed_n-=256;
    transform_p+=(long double)p[slot][axis*4+row]*v->position[axis];
    transform_n+=(long double)p[slot][axis*4+row]*signed_n*nscale;
   }
   result_p+=transform_p*v->weights[bone]*unit;result_n+=transform_n*v->weights[bone]*unit;
  }
  position[row]=result_p;normal[row]=result_n;
 }
}
int main(int argc,char**argv){
 C(argc==2);gh_disc*d=NULL;C(!gh_disc_open(argv[1],&d));
 float palette[42][16]={0};for(unsigned i=0;i<42;i++){
  palette[i][0]=1+(float)i/64;palette[i][5]=.5f+(float)i/128;palette[i][10]=1.5f;
  palette[i][1]=-.125f;palette[i][4]=.25f;palette[i][6]=.0625f;palette[i][8]=-.03125f;
  palette[i][12]=(float)i/16;palette[i][13]=-(float)i/32;palette[i][14]=.125f;palette[i][15]=1;
 }
 const char*paths[]={"om/om00.dat","pl/pl00.dat","pl/pl01.dat"};const int modes[]={FE_TONEAREST,FE_DOWNWARD,FE_UPWARD,FE_TOWARDZERO};unsigned total=0,negative=0;
 for(unsigned asset=0;asset<3;asset++){
  size_t index;gh_disc_entry e;C(!gh_disc_find(d,paths[asset],&index)&&!gh_disc_entry_at(d,index,&e));uint8_t*raw=malloc((size_t)e.size);C(raw&&!gh_disc_read(d,index,0,raw,(size_t)e.size));uint64_t before=hash(0,raw,(size_t)e.size);
  gh_bundle b;gh_bundle_entry md;gh_model_view m;C(!gh_bundle_open(raw,(size_t)e.size,&b)&&!gh_bundle_find(&b,"MD",0,&md)&&!gh_model_open_scr(md.bytes,md.size,&m));uint64_t baseline=0;
  for(unsigned mode=0;mode<4;mode++){
   C(!fesetround(modes[mode])&&!feclearexcept(FE_ALL_EXCEPT)&&!feraiseexcept(FE_DIVBYZERO));uint64_t result_hash=0;
   for(size_t mesh=0;mesh<m.mesh_count;mesh++){
    size_t streams;C(!gh_skinned_mesh_stream_count(&m,mesh,&streams));for(size_t stream=0;stream<streams;stream++){
     gh_skinned_mesh_stream s;C(!gh_skinned_mesh_stream_at(&m,mesh,stream,&s));for(size_t vertex=0;vertex<s.vertices;vertex++){
      gh_skinned_mesh_vertex v;gh_skinned_blend_result result;uint32_t frontier=99;C(!gh_skinned_mesh_vertex_at(&s,vertex,&v));
      int previous_flags=fetestexcept(FE_ALL_EXCEPT);
      C(!gh_skinned_mesh_blend_program2(&v,palette,42,GH_STATIC_BINARY32_REFERENCE,&result,&frontier)&&!frontier);
      C(fegetround()==modes[mode]&&fetestexcept(FE_ALL_EXCEPT)==previous_flags&&result.raw_control==v.raw_control);
      if(!mode){long double p[3],n[3];oracle(&v,palette,p,n);for(unsigned j=0;j<3;j++){C(fabsl(result.position[j]-p[j])<.00002L*(1+fabsl(p[j]))&&fabsl(result.normal[j]-n[j])<.00002L*(1+fabsl(n[j])));if(v.normal[j]>=128)negative++;}total++;}
      result_hash=hash(result_hash,&result,sizeof(result));
     }
    }
   }
   if(!mode)baseline=result_hash;else C(result_hash==baseline);
  }
  C(hash(0,raw,(size_t)e.size)==before);free(raw);
 }
 C(total>8000&&negative>100&&!fesetround(FE_TONEAREST));float identity[1][16]={{1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1}};
 gh_skinned_mesh_vertex v={.position={1,2,3},.normal={127,0,0},.weights={200,0,0},.raw_control=0x80008001};gh_skinned_blend_result result;uint32_t frontier;
 C(!gh_skinned_mesh_blend_program2(&v,identity,1,GH_STATIC_BINARY32_REFERENCE,&result,&frontier)&&result.position[0]==2&&result.position[1]==4&&result.position[2]==6&&result.normal[0]==2&&result.raw_control==v.raw_control);
 gh_skinned_blend_result unchanged;memset(&unchanged,0xa5,sizeof(unchanged));result=unchanged;v.bone_indices[3]=2;C(gh_skinned_mesh_blend_program2(&v,identity,1,GH_STATIC_BINARY32_REFERENCE,&result,&frontier)==GH_STATIC_BOUNDS&&frontier==0x148&&!memcmp(&result,&unchanged,sizeof(result)));
 v.bone_indices[3]=0;v.position[0]=NAN;C(gh_skinned_mesh_blend_program2(&v,identity,1,GH_STATIC_BINARY32_REFERENCE,&result,&frontier)==GH_STATIC_UNSUPPORTED&&!memcmp(&result,&unchanged,sizeof(result)));
 gh_disc_close(d);printf("PASS %u actual OM00/PL00/PL01 vertices, independent longdouble palette-first/vertex-first oracle, signed8 normals (%u negative), unnormalized percent weights/normals, fourFEnv/exceptions, atomicfailure; geometryleaf only, no shading/renderframe\n",total,negative);return 0;
}
