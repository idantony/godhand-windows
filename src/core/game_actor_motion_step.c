#include "game_actor_motion_step.h"
#include "game_actor_matrix.h"
#include <string.h>
#include <math.h>
#include <fenv.h>
static uint32_t rd(const uint8_t*p){uint32_t x;memcpy(&x,p,4);return x;}
static unsigned half(const uint8_t*p){return p[0]|(unsigned)p[1]<<8;}
static void wr(uint8_t*p,uint32_t x){memcpy(p,&x,4);}
static float rf(const uint8_t*p){float x;memcpy(&x,p,4);return x;}
static void wf(uint8_t*p,float x){memcpy(p,&x,4);}
static float add(float a,float b){volatile float x=a+b;return x;}
static float sub(float a,float b){volatile float x=a-b;return x;}
static float mul(float a,float b){volatile float x=a*b;return x;}
static gh_actor_motion_status fail(uint32_t*f,uint32_t a){*f=a;return GH_ACTOR_MOTION_UNSUPPORTED;}
static gh_actor_motion_status track_value(gh_actor_motion*s,unsigned index,float time,int cached,float*out,uint32_t*f){
 gh_motion_track t;if(gh_motion_track_at(&s->motion,index,&t))return GH_ACTOR_MOTION_BOUNDS;
 if(cached&&(t.method==1||t.method==3||t.method==5)&&t.keys<=1){*out=0;return GH_ACTOR_MOTION_OK;}
 gh_motion_cache cache;gh_motion_status e=gh_motion_sample_reference(&s->motion,index,time,out,&cache,f);if(e)return fail(f,*f?*f:0x159ad0);if(cached)s->caches[index]=cache;return GH_ACTOR_MOTION_OK;
}
gh_actor_motion_status gh_actor_motion_sample_00158328(gh_actor_motion*s,gh_static_transform_policy policy,uint32_t*f){
 if(!s||!s->bound||!s->complete||!s->b.model||!f)return GH_ACTOR_MOTION_ARGUMENT;*f=0;gh_actor_model*m=s->b.model;uint8_t*p=m->root.bytes;
 if(policy!=GH_STATIC_BINARY32_REFERENCE||half(p+0x434))return fail(f,0x158328);if(m->part_count>255)return GH_ACTOR_MOTION_BOUNDS;
 float time=rf(p+0x430),blend=rf(p+0x27c);if(!isfinite(time)||!isfinite(blend)||time<0||time>s->motion.frames-1)return fail(f,0x158328);
 int indices[256][9];for(unsigned i=0;i<256;i++)for(unsigned j=0;j<9;j++)indices[i][j]=-1;
 for(unsigned k=0;k<s->motion.count;k++){gh_motion_track t;if(gh_motion_track_at(&s->motion,k,&t))return GH_ACTOR_MOTION_BOUNDS;
  if(t.part==0x7f)break;if(t.channel>8||(t.part!=0xff&&t.part>=m->part_count))return fail(f,0x158328);unsigned owner=t.part==0xff?255:t.part;if(indices[owner][t.channel]>=0)return fail(f,0x158328);indices[owner][t.channel]=(int)k;
 }
 float values[256][9];for(unsigned i=0;i<256;i++)for(unsigned j=0;j<9;j++)if(indices[i][j]>=0){gh_actor_motion_status e=track_value(s,(unsigned)indices[i][j],time,(i==255&&j<3)||(i!=255&&j>=3&&j<6),&values[i][j],f);if(e)return e;}
 for(unsigned j=0;j<9;j++){float v=indices[255][j]>=0?values[255][j]:(j<6?0:1);wf(p+(j<3?0x310+j*4:j<6?0x340+(j-3)*4:0x370+(j-6)*4),v);}
 float angle[4],wrapped[4];memcpy(angle,p+0x340,16);if(gh_actor_euler_adjust_001f7da0(angle,wrapped,policy,f))return GH_ACTOR_MOTION_UNSUPPORTED;memcpy(p+0x340,wrapped,12);
 for(unsigned i=0;i<m->part_count;i++){
  gh_actor_part*part=&m->parts[i];uint8_t*q=part->bytes;if(!q||!part->alias_d0)return GH_ACTOR_MOTION_ARGUMENT;uint32_t flags=rd(q+0x154);
  if(flags&0x2000)flags|=0x400000;if(flags&0x8000)flags|=0x200000;flags&=~0x80u;wr(q+0x154,flags);if(flags&1)continue;
  for(unsigned j=0;j<9;j++){
   if((j<3&&(flags&0x200))||(j>=3&&j<6&&(flags&0x100))||(j>=6&&(flags&0x400)))continue;
   uint8_t*target=j<3?part->alias_d0+j*4:j<6?q+0x100+(j-3)*4:q+0x110+(j-6)*4;
   if(indices[i][j]>=0){wf(target,values[i][j]);flags|=0x80;wr(q+0x154,flags);}
   else if(j<3&&blend>0&&!(flags&0x100000))memcpy(target,q+0xe0+j*4,4);
   else if(j>=3&&j<6&&(flags&0x8000))wr(target,0);
  }
 }
 return GH_ACTOR_MOTION_OK;
}
static gh_actor_motion_status empty_sequence(gh_actor_motion*s,uint32_t*f){
 if(!s->sequence_valid)return GH_ACTOR_MOTION_OK; /* actual2C7FD0(NULL) */
 const uint8_t*b=s->b.resource;size_t n=s->b.resource_bytes,o=s->sequence_offset;if(o>n||n-o<0x20)return GH_ACTOR_MOTION_BOUNDS;const uint8_t*seq=b+o;
 if(memcmp(seq,"SEQ\0",4))return GH_ACTOR_MOTION_BOUNDS;for(unsigned j=0;j<4;j++)if(half(seq+10+j*4))return fail(f,0x2c7fd0);
 uint8_t*p=s->b.model->root.bytes;if(rd(p+0x418))return fail(f,0x2c7fd0);
 if(!(half(p+0x434)&0x20))memset(p+0x410,0,8);p[0x3ac]=p[0x3ad]=0;if(!(p[0x420]&1))p[0x3ae]=p[0x3af]=0;wr(p+0x41c,rd(p+0x430));wr(p+0x3a8,rd(p+0x440));return GH_ACTOR_MOTION_OK;
}
gh_actor_motion_status gh_actor_motion_step_00157c18(gh_actor_motion*s,float rate,gh_static_transform_policy policy,int*ended,uint32_t*f){
 if(!s||!s->bound||!s->complete||!s->b.model||!s->b.work||!ended||!f)return GH_ACTOR_MOTION_ARGUMENT;*f=0;*ended=0;uint8_t*p=s->b.model->root.bytes;
 if(policy!=GH_STATIC_BINARY32_REFERENCE||half(p+0x434)||!isfinite(rate)||rate<0||rf(p+0x438)!=0)return fail(f,0x157c18);
 if(!isfinite(rf(p+0x444))||!isfinite(rf(p+0x440))||rf(p+0x444)<0||rf(p+0x440)<0)return fail(f,0x157c18);
 gh_actor_motion_status result=empty_sequence(s,f);if(result)return result;
 fenv_t env;if(fegetenv(&env)||fesetround(FE_TONEAREST))return fail(f,0x157c18);
 float dt=mul(mul(rf(p+0x444),rf(p+0x440)),s->b.work->phase569==1?mul(rate,.5f):rate);if(!isfinite(dt)){result=fail(f,0x157c18);goto finish;}
 wf(p+0x438,0);wf(p+0x27c,0);result=gh_actor_motion_sample_00158328(s,policy,f);if(result)goto finish;
 float delta[4]={0},angles[4]={0},rotation[16];for(unsigned j=0;j<3;j++)delta[j]=sub(rf(p+0x310+j*4),rf(p+0x320+j*4));delta[3]=1;memcpy(p+0x320,p+0x310,12);angles[1]=-rf(p+0x344);
 if(gh_actor_matrix_rotation_00147728(angles,0,rotation,policy,f)){result=GH_ACTOR_MOTION_UNSUPPORTED;goto finish;}
 for(unsigned row=0;row<3;row++){float v=add(add(add(mul(rotation[row],delta[0]),mul(rotation[4+row],delta[1])),mul(rotation[8+row],delta[2])),rotation[12+row]);wf(p+0x330+row*4,v);}
 float dif[4],wrapped[4];for(unsigned j=0;j<4;j++)dif[j]=sub(rf(p+0x340+j*4),rf(p+0x350+j*4));if(gh_actor_euler_adjust_001f7da0(dif,wrapped,policy,f)){result=GH_ACTOR_MOTION_UNSUPPORTED;goto finish;}memcpy(p+0x360,wrapped,12);memcpy(p+0x350,p+0x340,16);
 for(unsigned j=0;j<3;j++)wf(p+0x390+j*4,sub(rf(p+0x370+j*4),rf(p+0x380+j*4)));memcpy(p+0x380,p+0x370,12);
 float next=add(rf(p+0x430),dt);if(dt==1){float adjusted=add(next,.0001f);if(adjusted>2147483520.0f){result=fail(f,0x157c18);goto finish;}next=(float)(int32_t)adjusted;}wf(p+0x430,next);
 float last=(float)(s->motion.frames-1);if(next>last){
  if(s->motion.flags&1){float excess=sub(next,last);if(excess<0)excess=0;if(excess>last)excess=last;wf(p+0x430,excess);memset(p+0x320,0,12);memset(p+0x350,0,12);}else wf(p+0x430,last);
  if(p[0x436]!=255)p[0x436]++;*ended=1;
 }
finish:
 if(fesetenv(&env))return fail(f,0x157c18);return result;
}
