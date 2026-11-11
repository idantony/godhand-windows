#include "game_actor_motion_step.h"
#include "game_actor_matrix.h"
#include "game_actor_sequence.h"
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
static unsigned motion_flags_allowed(const gh_actor_model *model){
 const uint8_t *p=model->root.bytes;
 /* Original 24C3C8 sets 434 bit 8 for the ordinary frontal reaction.
  * Sample tests bit 2, sequence tests bit 20, and advance tests 1/4/10. */
 if(model->root.size<0x568||half(p+0x2fe)!=0x202||p[0x2f5])
  return 0x40u;
 const uint32_t dependency=rd(p+0x564);
 const int stamp19=model->root.size>=0x642&&dependency==0x203&&half(p+0x640)==19&&
    model->work&&(uint16_t)model->work->room==0x100;
 /* 25E790 uses the same bit while sampling either opening actor's death clip. */
 const int stamp20=model->root.size>=0x642&&dependency==0x200&&half(p+0x640)==20&&
    model->work&&(uint16_t)model->work->room==0x100;
 const int allowed=(p[0x2f4]==1&&(dependency==0x20a||stamp19||stamp20))||
                   (p[0x2f4]==2&&(stamp19||stamp20));
 return allowed?0x48u:0x40u;
}
static gh_actor_motion_status track_value(gh_actor_motion*s,unsigned index,float time,int cached,float*out,uint32_t*f){
 gh_motion_track t;if(gh_motion_track_at(&s->motion,index,&t))return GH_ACTOR_MOTION_BOUNDS;
 if(cached&&(t.method==1||t.method==3||t.method==5)&&t.keys<=1){*out=0;return GH_ACTOR_MOTION_OK;}
 gh_motion_cache cache;gh_motion_status e=gh_motion_sample_reference(&s->motion,index,time,out,&cache,f);if(e)return fail(f,*f?*f:0x159ad0);if(cached)s->caches[index]=cache;return GH_ACTOR_MOTION_OK;
}
gh_actor_motion_status gh_actor_motion_sample_00158328(gh_actor_motion*s,gh_static_transform_policy policy,uint32_t*f){
 if(!s||!s->bound||!s->complete||!s->b.model||!f)return GH_ACTOR_MOTION_ARGUMENT;*f=0;gh_actor_model*m=s->b.model;uint8_t*p=m->root.bytes;
 if(policy!=GH_STATIC_BINARY32_REFERENCE||(half(p+0x434)&~motion_flags_allowed(m)))return fail(f,0x158328);if(m->part_count>255)return GH_ACTOR_MOTION_BOUNDS;
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
gh_actor_motion_status gh_actor_motion_step_00157c18(gh_actor_motion*s,float rate,gh_static_transform_policy policy,int*ended,uint32_t*f){
 if(!s||!s->bound||!s->complete||!s->b.model||!s->b.work||!ended||!f)return GH_ACTOR_MOTION_ARGUMENT;*f=0;*ended=0;uint8_t*p=s->b.model->root.bytes;uint32_t rate_bits;memcpy(&rate_bits,&rate,4);
 if(policy!=GH_STATIC_BINARY32_REFERENCE||(half(p+0x434)&~motion_flags_allowed(s->b.model))||!isfinite(rate)||rate<0)return fail(f,0x157c18);
 if(s->step_stage&&rate_bits!=s->step_rate_bits)return GH_ACTOR_MOTION_BUSY;
 fenv_t env;if(feholdexcept(&env))return fail(f,0x157c18);if(fesetround(FE_TONEAREST)){(void)fesetenv(&env);return fail(f,0x157c18);}gh_actor_motion_status result=GH_ACTOR_MOTION_OK;
 if(!s->step_stage){result=gh_actor_motion_sequence_002c7fd0(s,rate,policy,f);if(result)goto finish;s->step_rate_bits=rate_bits;s->step_stage=4;}
 if(s->step_stage==4){if(!isfinite(rf(p+0x444))||!isfinite(rf(p+0x440))||rf(p+0x444)<0||rf(p+0x440)<0){result=fail(f,0x157c18);goto finish;}float remaining=rf(p+0x438),duration=rf(p+0x43c);if(!isfinite(remaining)||!isfinite(duration)||remaining<0||duration<0){result=fail(f,0x157c18);goto finish;}float dt=mul(mul(rf(p+0x444),rf(p+0x440)),s->b.work->phase569==1?mul(rate,.5f):rate);if(!isfinite(dt)){result=fail(f,0x157c18);goto finish;}float next=sub(remaining,dt);if(next<0)next=0;float ratio=0;if(next>0){if(!(duration>0)){result=fail(f,0x157c18);goto finish;}volatile float q=next/duration;ratio=q;if(!isfinite(ratio)){result=fail(f,0x157c18);goto finish;}}s->step_dt=dt;s->step_ratio=ratio;s->step_rate_bits=rate_bits;wf(p+0x438,next);s->step_stage=1;}
 if(s->step_stage==1){/*158328 reads PREVIOUS27C. Only aftercomplete sample store nextfactor. */result=gh_actor_motion_sample_00158328(s,policy,f);if(result)goto finish;wf(p+0x27c,s->step_ratio);s->step_stage=2;}
 if(s->step_stage==2){float delta[4]={0},angles[4]={0},rotation[16],translated[3];for(unsigned j=0;j<3;j++)delta[j]=sub(rf(p+0x310+j*4),rf(p+0x320+j*4));delta[3]=1;memcpy(angles,p+0x340,16);angles[1]=-angles[1];if(gh_actor_matrix_rotation_00147728(angles,0,rotation,policy,f)){result=GH_ACTOR_MOTION_UNSUPPORTED;goto finish;}
 for(unsigned row=0;row<3;row++){translated[row]=add(add(add(mul(rotation[row],delta[0]),mul(rotation[4+row],delta[1])),mul(rotation[8+row],delta[2])),rotation[12+row]);if(!isfinite(translated[row])){result=fail(f,0x150830);goto finish;}}
 float dif[4],wrapped[4],scale_delta[3];for(unsigned j=0;j<4;j++)dif[j]=sub(rf(p+0x340+j*4),rf(p+0x350+j*4));if(gh_actor_euler_adjust_001f7da0(dif,wrapped,policy,f)){result=GH_ACTOR_MOTION_UNSUPPORTED;goto finish;}for(unsigned j=0;j<3;j++){scale_delta[j]=sub(rf(p+0x370+j*4),rf(p+0x380+j*4));if(!isfinite(scale_delta[j])){result=fail(f,0x157c18);goto finish;}}
 memcpy(p+0x330,translated,12);memcpy(p+0x320,p+0x310,12);memcpy(p+0x360,wrapped,12);memcpy(p+0x350,p+0x340,16);memcpy(p+0x390,scale_delta,12);memcpy(p+0x380,p+0x370,12);s->step_stage=3;}
 if(s->step_stage==3){float dt=s->step_dt,next=add(rf(p+0x430),dt);if(dt==1){float adjusted=add(next,.0001f);if(adjusted>2147483520.0f){result=fail(f,0x157c18);goto finish;}next=(float)(int32_t)adjusted;}if(!isfinite(next)){result=fail(f,0x157c18);goto finish;}float last=(float)(s->motion.frames-1);if(next>last){if(s->motion.flags&1){float excess=sub(next,last);if(excess<0)excess=0;if(excess>last)excess=last;next=excess;memset(p+0x320,0,12);memset(p+0x350,0,12);}else next=last;if(p[0x436]!=255)p[0x436]++;*ended=1;}wf(p+0x430,next);s->step_stage=0;}
finish:if(fesetenv(&env))return fail(f,0x157c18);return result;
}
