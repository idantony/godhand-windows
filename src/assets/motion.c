#include "motion.h"
#include <string.h>
#include <math.h>
#include <fenv.h>
static uint32_t u32(const uint8_t*p){uint32_t n;memcpy(&n,p,4);return n;}
static unsigned u16(const uint8_t*p){return p[0]|(unsigned)p[1]<<8;}
static float f32(const uint8_t*p){float x;memcpy(&x,p,4);return x;}
static float quant(unsigned n){uint32_t x=((n<<14)&0x7fff00u)|(((n<<14)&0x1f800000u)+0x28000000u)|((n&0x8000u)<<16);float f;memcpy(&f,&x,4);return f;}
static int span(size_t n,size_t o,size_t z){return o<=n&&z<=n-o;}
static float add(float a,float b){volatile float x=a+b;return x;}
static float sub(float a,float b){volatile float x=a-b;return x;}
static float mul(float a,float b){volatile float x=a*b;return x;}
static float divv(float a,float b){volatile float x=a/b;return x;}
gh_motion_status gh_motion_open_mtb3(const void*data,size_t n,size_t o,gh_motion_view*out){
 if(!data||!out)return GH_MOTION_ARGUMENT;const uint8_t*p=data;if(!span(n,o,8))return GH_MOTION_BOUNDS;
 if(memcmp(p+o,"mtb3",4))return GH_MOTION_FORMAT;uint16_t frames=(uint16_t)u16(p+o+4);unsigned count=p[o+6];
 if(!frames||!count||!span(n,o+8,(size_t)count*12))return GH_MOTION_BOUNDS;
 *out=(gh_motion_view){p,n,o,frames,(uint8_t)count,p[o+7]};return GH_MOTION_OK;
}
gh_motion_status gh_motion_track_at(const gh_motion_view*v,unsigned index,gh_motion_track*out){
 if(!v||!v->bytes||!out)return GH_MOTION_ARGUMENT;if(index>=v->count||!span(v->size,v->offset+8+(size_t)index*12,12))return GH_MOTION_BOUNDS;
 const uint8_t*p=v->bytes+v->offset+8+(size_t)index*12;uint32_t packed=u32(p);
 *out=(gh_motion_track){packed,u32(p+4),u32(p+8),(uint16_t)(packed>>16),(uint8_t)packed,(uint8_t)((packed>>8)&15),(uint8_t)((packed>>12)&15)};return GH_MOTION_OK;
}
static gh_motion_status sample(const gh_motion_view*v,const gh_motion_track*t,float time,float*out,gh_motion_cache*cache,uint32_t*frontier){
 if(t->method==0){memcpy(out,&t->data,4);cache->first_value=*out;return isfinite(*out)?GH_MOTION_OK:GH_MOTION_NUMERIC;}
 if(t->method==2||t->method==6){*out=0;return GH_MOTION_OK;}
 if(t->method>6){*frontier=0x3bd910;return GH_MOTION_UNSUPPORTED;}
 if(!t->keys)return GH_MOTION_FORMAT;size_t base=v->offset+t->data;
 if(base<v->offset)return GH_MOTION_BOUNDS;
 unsigned header=t->method==1?12:t->method==3?24:t->method==4?8:0;
 unsigned stride=t->method==1?4:t->method==3?8:t->method==4?2:16;
 unsigned keys=t->method==4?v->frames:t->keys;
 if(!span(v->size,base,header+(size_t)keys*stride))return GH_MOTION_BOUNDS;
 const uint8_t*p=v->bytes+base;float coefficients[6]={0};
 for(unsigned i=0;i<6;i++){
  if(t->method==1)coefficients[i]=quant(u16(p+i*2));
  else if(t->method==3||t->method==4){if(i<header/4)coefficients[i]=f32(p+i*4);}
  if(!isfinite(coefficients[i]))return GH_MOTION_NUMERIC;
 }
 const uint8_t*k=p+header;unsigned previous=0,next=0,previous_frame=0,next_frame=0,acc=0;
 if(t->method==4){previous=(unsigned)time;next=previous+1<keys?previous+1:previous;previous_frame=previous;next_frame=next;
  /* Original2DA8D8 is angular short-arc interpolation, separate dependency. */
  if(next!=previous&&(t->channel==3||t->channel==4||t->channel==5)){*frontier=0x2da8d8;return GH_MOTION_UNSUPPORTED;}
 }else{
  int found=0;
  for(unsigned i=0;i<keys;i++){
   unsigned frame=t->method==1?(acc+=k[(size_t)i*stride]):u16(k+(size_t)i*stride);
   if((i&&frame<=previous_frame)||frame>=v->frames)return GH_MOTION_FORMAT;
   if(time<=(float)frame){next=i;next_frame=frame;if(i==0&&keys>1){next=1;next_frame=t->method==1?frame+k[stride]:u16(k+stride);if(next_frame<=frame||next_frame>=v->frames)return GH_MOTION_FORMAT;}found=1;break;}
   previous=i;previous_frame=frame;
  }
  if(!found){next=previous;next_frame=previous_frame;}
  if(time<0||(!next&&time<(float)next_frame))return GH_MOTION_UNSUPPORTED;
 }
 float values[2],incoming[2]={0},outgoing[2]={0};unsigned indices[]={previous,next};
 for(unsigned j=0;j<2;j++){const uint8_t*q=k+(size_t)indices[j]*stride;
  if(t->method==1){values[j]=add(mul((float)q[1],coefficients[1]),coefficients[0]);incoming[j]=add(mul((float)q[2],coefficients[3]),coefficients[2]);outgoing[j]=add(mul((float)q[3],coefficients[5]),coefficients[4]);}
  else if(t->method==3){values[j]=add(mul((float)u16(q+2),coefficients[1]),coefficients[0]);incoming[j]=add(mul((float)u16(q+4),coefficients[3]),coefficients[2]);outgoing[j]=add(mul((float)u16(q+6),coefficients[5]),coefficients[4]);}
  else if(t->method==4)values[j]=add(mul((float)u16(q),coefficients[1]),coefficients[0]);
  else{values[j]=f32(q+4);incoming[j]=f32(q+8);outgoing[j]=f32(q+12);}
  if(!isfinite(values[j])||!isfinite(incoming[j])||!isfinite(outgoing[j]))return GH_MOTION_NUMERIC;
 }
 *cache=(gh_motion_cache){base+header+(size_t)previous*stride,base+header+(size_t)next*stride,(uint16_t)previous_frame,(uint16_t)next_frame,values[0],values[1],outgoing[0],incoming[1],t->method};
 if(previous==next||time==(float)previous_frame){*out=values[0];return GH_MOTION_OK;}if(time==(float)next_frame){*out=values[1];return GH_MOTION_OK;}
 float u=divv(sub(time,(float)previous_frame),(float)(next_frame-previous_frame));
 if(t->method==4){*out=add(mul(values[1],u),mul(values[0],sub(1,u)));return GH_MOTION_OK;}
 float u2=mul(u,u),u3=mul(u2,u),twice=add(u3,u3),thrice=mul(u2,3);
 float a=mul(values[0],add(sub(twice,thrice),1)),b=mul(values[1],add(-twice,thrice));
 float c=mul(outgoing[0],add(sub(sub(u3,u2),u2),u)),d=mul(incoming[1],sub(u3,u2));
 *out=add(add(add(a,b),c),d);return isfinite(*out)?GH_MOTION_OK:GH_MOTION_NUMERIC;
}
gh_motion_status gh_motion_sample_reference(const gh_motion_view*v,unsigned index,float time,float*out,gh_motion_cache*cache,uint32_t*frontier){
 if(!v||!out||!cache||!frontier||!isfinite(time)||time<0||time>(float)(v->frames-1))return GH_MOTION_ARGUMENT;
 *frontier=0;gh_motion_track t;gh_motion_status e=gh_motion_track_at(v,index,&t);if(e)return e;
 fenv_t env;if(fegetenv(&env)||fesetround(FE_TONEAREST))return GH_MOTION_NUMERIC;float value=0;gh_motion_cache next={0};e=sample(v,&t,time,&value,&next,frontier);if(fesetenv(&env))return GH_MOTION_NUMERIC;if(!e){*out=value;*cache=next;}return e;
}
