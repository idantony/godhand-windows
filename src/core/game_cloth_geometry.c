/* Actual106218 flags!=1 branch. Same461600 shared scratch owner. */
#include "game_cloth_templates.h"
#include <string.h>
#include <math.h>
#include <float.h>
#include <fenv.h>
static float add(float a,float b){volatile float r=a+b;return r;}
static float mul(float a,float b){volatile float r=a*b;return r;}
static float readf(const uint8_t*p){float f;memcpy(&f,p,4);return f;}
static int finite_normal(float f){return isfinite(f)&&(f==0||fabsf(f)>=FLT_MIN);}
static void transform(const uint8_t*m,const float*v,float*out){for(unsigned k=0;k<3;k++){float n=mul(readf(m+k*4),v[0]);n=add(n,mul(readf(m+16+k*4),v[1]));n=add(n,mul(readf(m+32+k*4),v[2]));out[k]=add(n,mul(readf(m+48+k*4),v[3]));}}
gh_cloth_status gh_cloth_prepare_records_00106218(gh_cloth_templates*s,const gh_actor_model*m,const gh_cloth_sphere*source,size_t count,gh_static_transform_policy policy,const gh_cloth_geometry**out,uint32_t*f){
 if(!s||!s->loaded||!out||!f)return GH_CLOTH_ARGUMENT;*out=NULL;*f=0;
 if(!m||!source||!count)return GH_CLOTH_OK;
 if(!s->initialized||!m->bound||m->native_identity!=m||!m->root.bytes||m->root.size<0x118)return GH_CLOTH_ARGUMENT;
 if(count>12)return GH_CLOTH_BOUNDS;
 if(policy!=GH_STATIC_BINARY32_REFERENCE){*f=0x106218;return GH_CLOTH_UNSUPPORTED;}
 const gh_actor_part*a[12],*b[12];
 for(size_t i=0;i<count;i++){if(source[i].flags==1){*f=0x106468;return GH_CLOTH_UNSUPPORTED;}
  if(gh_actor_model_part_at(m,source[i].bone_a,a+i)||gh_actor_model_part_at(m,source[i].bone_b,b+i)){*f=0x106354;return GH_CLOTH_BOUNDS;}
  for(unsigned k=0;k<16;k++)if(!finite_normal(readf(a[i]->bytes+0x80+k*4))||!finite_normal(readf(b[i]->bytes+0x80+k*4))){*f=0x30a2b0;return GH_CLOTH_NUMERIC;}
  if(!finite_normal(source[i].blend)||!finite_normal(source[i].radius)){*f=0x1063f0;return GH_CLOTH_NUMERIC;}
  for(unsigned k=0;k<4;k++)if(!finite_normal(source[i].offset_a[k])||!finite_normal(source[i].offset_b[k])){*f=0x30a2b0;return GH_CLOTH_NUMERIC;}
 }
 float scale=readf(m->root.bytes+0x114);if(!finite_normal(scale)){*f=0x106448;return GH_CLOTH_NUMERIC;}
 fenv_t saved;if(feholdexcept(&saved))return GH_CLOTH_NUMERIC;if(fesetround(FE_TONEAREST)){(void)fesetenv(&saved);return GH_CLOTH_NUMERIC;}
 gh_cloth_geometry next=s->geometry;next.count=(uint32_t)count;gh_cloth_status status=GH_CLOTH_OK;
 for(size_t i=0;i<count;i++){float pa[3],pb[3],center[3];transform(a[i]->bytes+0x80,source[i].offset_a,pa);transform(b[i]->bytes+0x80,source[i].offset_b,pb);
  float inverse=add(1,-source[i].blend);for(unsigned k=0;k<3;k++)center[k]=add(mul(pb[k],source[i].blend),mul(pa[k],inverse));float radius=mul(source[i].radius,scale);
  for(unsigned k=0;k<3;k++)if(!finite_normal(pa[k])||!finite_normal(pb[k])||!finite_normal(center[k]))status=GH_CLOTH_NUMERIC;
  if(!finite_normal(radius))status=GH_CLOTH_NUMERIC;if(status){*f=0x1063f0;break;}
  uint32_t flag=0;memcpy(next.records[i],&flag,4);memcpy(next.records[i]+0x90,center,12);memcpy(next.records[i]+0xb0,&radius,4);
 }
 if(fesetenv(&saved))return GH_CLOTH_NUMERIC;if(status)return status;s->geometry=next;*out=&s->geometry;return GH_CLOTH_OK;
}

gh_cloth_status gh_cloth_prepare_00106218(gh_cloth_templates*s,const gh_actor_model*m,
 const gh_cloth_sphere*source,size_t count,gh_static_transform_policy policy,const gh_cloth_geometry**out,uint32_t*f){
 if(!s||!s->loaded||!out||!f)return GH_CLOTH_ARGUMENT;
 if(m&&source&&count&&s->initialized&&m->bound&&m->native_identity==m&&m->root.bytes&&
    m->root.size>=0x118&&count<=12){
  int found=0;
  for(unsigned i=0;i<42;i++)if(source==s->records+i&&count<=42-i)found=1;
  if(!found){*out=NULL;*f=0;return GH_CLOTH_ARGUMENT;}
 }
 return gh_cloth_prepare_records_00106218(s,m,source,count,policy,out,f);
}
