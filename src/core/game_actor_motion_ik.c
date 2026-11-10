#include "game_actor_motion_ik.h"
#include <string.h>
#include <math.h>
#include <fenv.h>
static uint32_t rd(const uint8_t*p){uint32_t n;memcpy(&n,p,4);return n;}
static unsigned half(const uint8_t*p){return p[0]|(unsigned)p[1]<<8;}
static void wr(uint8_t*p,uint32_t n){memcpy(p,&n,4);}
static float rf(const uint8_t*p){float x;memcpy(&x,p,4);return x;}
static void wf(uint8_t*p,float x){memcpy(p,&x,4);}
static float add(float a,float b){volatile float x=a+b;return x;}
static float sub(float a,float b){volatile float x=a-b;return x;}
static float mul(float a,float b){volatile float x=a*b;return x;}
static float divide(float a,float b){volatile float x=a/b;return x;}
static int bounds(size_t n,size_t o,size_t z){return o<=n&&z<=n-o;}
static int mapped(const uint8_t*b,size_t n,uint32_t address,size_t bytes,const uint8_t**out){
 if(n<52||memcmp(b,"\177ELF\1\1\1",7)||half(b+18)!=8)return 0;size_t table=rd(b+28);unsigned stride=half(b+42),count=half(b+44);if(stride<32||count>128||!bounds(n,table,(size_t)stride*count))return 0;unsigned matches=0;
 for(unsigned i=0;i<count;i++){const uint8_t*p=b+table+(size_t)i*stride;uint32_t base=rd(p+8),off=rd(p+4),files=rd(p+16),memory=rd(p+20);if(rd(p)!=1)continue;
  if(files>memory||!bounds(n,off,files))return 0;if(base<=address&&(uint64_t)address+bytes<=(uint64_t)base+files){if(matches++)return 0;*out=b+off+(address-base);}}
 return matches==1;
}
static float length3(const float*v){float n=add(add(mul(v[0],v[0]),mul(v[1],v[1])),mul(v[2],v[2]));volatile float x=sqrtf(n);return x;}
static void cross(float*out,const float*a,const float*b){out[0]=sub(mul(a[1],b[2]),mul(a[2],b[1]));out[1]=sub(mul(a[2],b[0]),mul(a[0],b[2]));out[2]=sub(mul(a[0],b[1]),mul(a[1],b[0]));out[3]=1;}
static void normalize(float*v){float n=length3(v);if(n>0){float q=divide(1,n);for(unsigned i=0;i<3;i++)v[i]=mul(v[i],q);}else v[0]=v[1]=v[2]=0;v[3]=1;}
static void matrix_quat(float*q,const float*m,const unsigned*cycle){
 float trace=add(add(m[0],m[5]),m[10]);
 if(trace>0){float root=sqrtf(add(trace,1)),s=divide(.5f,root);q[3]=mul(root,.5f);q[0]=mul(sub(m[6],m[9]),s);q[1]=mul(sub(m[8],m[2]),s);q[2]=mul(sub(m[1],m[4]),s);}
 else{unsigned i=m[0]<m[5];if(m[i*5]<m[10])i=2;unsigned j=cycle[i],k=cycle[j];float root=sqrtf(add(sub(sub(m[i*5],m[j*5]),m[k*5]),1)),s=divide(.5f,root);q[i]=mul(root,.5f);q[3]=mul(sub(m[j*4+k],m[k*4+j]),s);q[j]=mul(add(m[i*4+j],m[j*4+i]),s);q[k]=mul(add(m[i*4+k],m[k*4+i]),s);}
}
static void basis_quat(float*q,const float*direction,const float*axis,const unsigned*cycle){
 float side[4],up[4],m[16]={0};cross(side,axis,direction);normalize(side);cross(up,direction,side);normalize(up);for(unsigned i=0;i<3;i++){m[i]=side[i];m[4+i]=up[i];m[8+i]=direction[i];}m[15]=1;matrix_quat(q,m,cycle);
}
static void rotate(float*out,const float*q,const float*v){
 float x=v[0],y=v[1],z=v[2],qx=q[0],qy=q[1],qz=q[2],qw=q[3];
 float a=add(add(sub(mul(qy,z),mul(qz,y)),mul(qx,0)),mul(qw,x));float b=add(add(sub(mul(qz,x),mul(qx,z)),mul(qy,0)),mul(qw,y));float c=add(add(sub(mul(qx,y),mul(qy,x)),mul(qz,0)),mul(qw,z));float d=sub(sub(sub(mul(qw,0),mul(qx,x)),mul(qy,y)),mul(qz,z));
 out[1]=add(add(sub(mul(c,-qx),mul(a,-qz)),mul(b,qw)),mul(d,-qy));out[2]=add(add(sub(mul(a,-qy),mul(b,-qx)),mul(c,qw)),mul(d,-qz));out[0]=add(add(sub(mul(b,-qz),mul(c,-qy)),mul(a,qw)),mul(d,-qx));out[3]=1;
}
gh_actor_motion_status gh_motion_ik_globals_from_elf(gh_motion_ik_globals*g,const void*elf,size_t bytes){
 if(!g||g->initialized||!elf)return GH_ACTOR_MOTION_ARGUMENT;
 const uint8_t*a,*c,*cyclebytes;if(!mapped(elf,bytes,0x3bd8a0,16,&a)||!mapped(elf,bytes,0x3bd8c0,16,&c)||!mapped(elf,bytes,0x41ee48,12,&cyclebytes))return GH_ACTOR_MOTION_BOUNDS;
 gh_motion_ik_globals next={0};for(unsigned i=0;i<4;i++){next.default_axis[i]=rf(c+i*4);next.xaxis[i]=rf(a+i*4);if(!isfinite(next.default_axis[i])||!isfinite(next.xaxis[i]))return GH_ACTOR_MOTION_BOUNDS;}for(unsigned i=0;i<3;i++){next.cycle[i]=rd(cyclebytes+i*4);if(next.cycle[i]>=3)return GH_ACTOR_MOTION_BOUNDS;}next.initialized=1;*g=next;return GH_ACTOR_MOTION_OK;
}
gh_actor_motion_status gh_actor_motion_ik_00152448(gh_actor_model*m,const gh_motion_view*motion,const gh_motion_ik_globals*globals,gh_static_transform_policy policy,uint32_t*frontier,unsigned*part){
 if(!m||!m->parts_ready||m->part_count>255||!m->root.bytes||m->root.size<0x44c||!motion||!globals||!globals->initialized||!frontier||!part)return GH_ACTOR_MOTION_ARGUMENT;*frontier=0;*part=0;
 if(policy!=GH_STATIC_BINARY32_REFERENCE){*frontier=0x152448;return GH_ACTOR_MOTION_UNSUPPORTED;}
 const float*default_axis=globals->default_axis,*xaxis=globals->xaxis;const unsigned*cycle=globals->cycle;
 for(unsigned i=0;i<4;i++)if(!isfinite(default_axis[i])||!isfinite(xaxis[i]))return GH_ACTOR_MOTION_BOUNDS;
 for(unsigned i=0;i<3;i++)if(cycle[i]>=3)return GH_ACTOR_MOTION_BOUNDS;
 uint8_t controls[255]={0};for(unsigned i=0;i<m->part_count;i++){
  if(!m->parts[i].bytes)return GH_ACTOR_MOTION_ARGUMENT;
  for(unsigned k=0;k<motion->count;k++){gh_motion_track t;if(gh_motion_track_at(motion,k,&t))return GH_ACTOR_MOTION_BOUNDS;if(t.part==0x7f||t.part==i){controls[i]=(uint8_t)t.control;break;}}
 }
 for(unsigned i=0;i<3;i++)if(!isfinite(rf(m->root.bytes+0x110+i*4)))return GH_ACTOR_MOTION_BOUNDS;
 for(unsigned i=0;i<m->part_count;i++)for(unsigned j=0;j<3;j++)if(!isfinite(rf(m->parts[i].bytes+0xc0+j*4)))return GH_ACTOR_MOTION_BOUNDS;
 fenv_t env;if(fegetenv(&env)||fesetround(FE_TONEAREST))return GH_ACTOR_MOTION_UNSUPPORTED;
 uint8_t*p=m->root.bytes;wr(p+0x448,0x3f800000);wr(p+0x250,rd(p+0x250)&~0x20u);for(unsigned i=0;i<m->part_count;i++){wr(m->parts[i].bytes+0x158,0);wr(m->parts[i].bytes+0x154,rd(m->parts[i].bytes+0x154)&0xffb01ff7u);}
 gh_actor_motion_status result=GH_ACTOR_MOTION_OK;
 for(unsigned i=0;i<m->part_count;i++){
  *part=i;if(!controls[i]||i+2>=m->part_count)continue;uint8_t*first=m->parts[i].bytes,*middle=m->parts[i+1].bytes,*last=m->parts[i+2].bytes;
  wr(p+0x250,rd(p+0x250)|0x20);wr(first+0x154,rd(first+0x154)|0x6000);wr(last+0x154,rd(last+0x154)|0x8000);
  float delta[4],axis[4],direction[4],q[4];memcpy(axis,default_axis,16);
  for(unsigned j=0;j<3;j++)delta[j]=mul(sub(rf(middle+0xc0+j*4),rf(first+0xc0+j*4)),rf(p+0x110+j*4));delta[3]=1;wf(first+0x158,length3(delta));
  if(controls[i]<=6){axis[0]=axis[1]=axis[2]=0;unsigned component=controls[i]<=2?2:controls[i]<=4?0:1;axis[component]=(controls[i]==1||controls[i]==4||controls[i]==5)?1.0f:-1.0f;}
  uint32_t flags=rd(first+0x154);if(delta[1]==0&&delta[2]==0)flags|=0x30000;else if(delta[0]==0&&delta[2]==0)flags|=0x50000;else if(delta[0]==0&&delta[1]==0)flags|=0x90000;else flags&=0xfff0ffff;wr(first+0x154,flags);
  for(unsigned j=0;j<3;j++)delta[j]=mul(sub(rf(last+0xc0+j*4),rf(middle+0xc0+j*4)),rf(p+0x110+j*4));wf(middle+0x158,length3(delta));
  for(unsigned j=0;j<3;j++)direction[j]=sub(rf(first+0xc0+j*4),rf(last+0xc0+j*4));direction[3]=1;normalize(direction);basis_quat(q,direction,axis,cycle);
  float norm=add(add(add(mul(q[0],q[0]),mul(q[1],q[1])),mul(q[2],q[2])),mul(q[3],q[3]));float inverse=divide(1,norm);
  for(unsigned j=0;j<3;j++)q[j]=mul(-q[j],inverse);q[3]=mul(q[3],inverse);
  float local_axis[4];rotate(local_axis,q,xaxis);int good=1;for(unsigned j=0;j<4;j++)if(!isfinite(q[j])||!isfinite(local_axis[j]))good=0;
  if(!good){*frontier=0x15a190;result=GH_ACTOR_MOTION_UNSUPPORTED;break;}memcpy(first+0x160,q,16);memcpy(first+0x1a0,local_axis,12);
 }
 if(fesetenv(&env))return GH_ACTOR_MOTION_UNSUPPORTED;return result;
}

gh_actor_motion_status gh_actor_matrix_quaternion_0015a190(const float*m,float*out,const gh_motion_ik_globals*g,gh_static_transform_policy policy,uint32_t*f){if(!m||!out||!g||!g->initialized||!f)return GH_ACTOR_MOTION_ARGUMENT;*f=0;if(policy!=GH_STATIC_BINARY32_REFERENCE){*f=0x15a190;return GH_ACTOR_MOTION_UNSUPPORTED;}for(unsigned i=0;i<3;i++)if(g->cycle[i]>=3)return GH_ACTOR_MOTION_BOUNDS;for(unsigned i=0;i<16;i++)if(!isfinite(m[i]))return GH_ACTOR_MOTION_BOUNDS;fenv_t env;if(feholdexcept(&env))return GH_ACTOR_MOTION_UNSUPPORTED;if(fesetround(FE_TONEAREST)){(void)fesetenv(&env);return GH_ACTOR_MOTION_UNSUPPORTED;}float q[4];matrix_quat(q,m,g->cycle);int valid=1;for(unsigned i=0;i<4;i++)if(!isfinite(q[i]))valid=0;if(fesetenv(&env))valid=0;if(!valid){*f=0x15a190;return GH_ACTOR_MOTION_UNSUPPORTED;}memcpy(out,q,16);return GH_ACTOR_MOTION_OK;}
