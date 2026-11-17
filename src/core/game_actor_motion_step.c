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
static unsigned motion_flags_allowed(const gh_actor_motion *s){
 const gh_actor_model *model=s->b.model;
 const uint8_t *p=model->root.bytes;
 const unsigned admitted=gh_actor_motion_begin_flags(&s->b)|(s->flags&2u);
 if(admitted&2u)return admitted|8u;
 /* Original 24C3C8 sets 434 bit 8 for the ordinary frontal reaction.
  * Sample tests bit 2, sequence tests bit 20, and advance tests 1/4/10. */
 if(model->root.size<0x568)return 0x40u;
 const unsigned kind=half(p+0x2fe);
 const uint32_t dependency=rd(p+0x564);
 if(model->work&&model->work->room==0x103){
  if(kind==0x204&&model->root.original_vtable214==0x4479c8&&
     model->root.size>0x1650&&p[0x2f4]==1&&(!p[0x2f5]||p[0x2f5]==3||p[0x2f5]==4)){
   const unsigned stamp=half(p+0x640);
   const unsigned appearance=p[0x1650];
   /* 25002C and 251614 set the same bit for the two original boss recoil clips. */
   if((!p[0x2f5]&&appearance==24&&((dependency==0x200&&(stamp==2||stamp==3))||
       (dependency==0x203&&(stamp==6||stamp==7))))||
      (appearance==15&&((dependency==0x214&&stamp==0)||(dependency==0x215&&stamp==1))))
    return 0x48u;
  }
  /* The lethal 11D4B0 and 11C608 phases set bit 8 before moveMotion. */
  if(kind==0x100&&model->root.original_vtable214==0x41d220&&p[0x2f4]==1&&
     ((p[0x2f5]==0xf&&p[0x2f6]==1)||(p[0x2f5]==0xb&&p[0x2f6]==5))&&
     !half(p+0x54a))return 0x48u;
 }
 /* 11C608's paired lethal clip sets the original sample bit before Step. */
 if(kind==0x100&&model->work&&model->work->room==0x100&&
    model->root.original_vtable214==0x41d220&&half(p+0x2f4)==0x0b01&&
    p[0x2f6]==5&&!half(p+0x54a))return 0x48u;
 /* The dep0250 frontal Give07 reaction reaches the same original bit 8 write. */
 if(kind==0x203&&dependency==0x250&&model->work&&model->work->room==0x104&&
    model->root.original_vtable214==0x4479c8&&model->root.size>0x1651&&
    rd(p+0x1644)==0x80000&&!half(p+0x640)&&!p[0x1650]&&!p[0x1651]&&
    p[0x2f4]==1&&!p[0x2f5]&&p[0x2f6]==1&&!p[0x2f7])return 0x48u;
 if(kind!=0x202)return 0x40u;
 /* 25FA18 sets the same sample bit for the R100 transform death clip. */
 if(model->root.size>=0x642&&dependency==0x200&&half(p+0x640)==8&&
    model->work&&(uint16_t)model->work->room==0x100&&p[0x2f4]==2&&p[0x2f5]==3)
  return 0x48u;
 if(p[0x2f5])return 0x40u;
 const int stamp19=model->root.size>=0x642&&dependency==0x203&&half(p+0x640)==19&&
    model->work&&(uint16_t)model->work->room==0x100;
 /* 25E790 uses the same bit while sampling either opening actor's death clip. */
 const int stamp20=model->root.size>=0x642&&dependency==0x200&&half(p+0x640)==20&&
    model->work&&(uint16_t)model->work->room==0x100;
 const int r102_em00=dependency==0x200&&model->work&&(uint16_t)model->work->room==0x102;
 const int r102_death=r102_em00&&model->root.size>0x1650&&!half(p+0x54a)&&
    ((half(p+0x640)==0x10&&!p[0x1650])||(half(p+0x640)==11&&p[0x1650]==5));
 const int r102_em03=dependency==0x203&&model->root.size>0x1650&&model->work&&
    model->work->room==0x102&&half(p+0x640)==8&&!p[0x1650];
 const int at0b_em02=dependency==0x202&&model->root.size>=0x16c8&&model->work&&
    model->work->room==0x102&&half(p+0x640)==0x17&&!p[0x1650]&&rd(p+0x16c4)==2&&
    p[0x2f6]==1&&!p[0x2f7];
 const int em27_reaction=dependency==0x227&&model->root.size>0x1650&&model->work&&
    p[0x2f6]==1&&!p[0x2f7]&&
    ((model->work->room==0x102&&half(p+0x640)==4&&!p[0x1650])||
     (model->work->room==0x101&&half(p+0x640)==9&&p[0x1650]==4));
 const unsigned stamp=model->root.size>0x1650?half(p+0x640):UINT16_MAX;
 const int r101_wave=model->root.size>0x1650&&model->work&&model->work->room==0x101&&
    p[0x1650]==6&&p[0x2f6]==1&&!p[0x2f7]&&
    ((stamp==0x16&&dependency==0x200)||(stamp==0x20&&dependency==0x203));
 const int allowed=(p[0x2f4]==1&&(dependency==0x20a||stamp19||stamp20||r102_em00||r102_em03||
                       em27_reaction||at0b_em02||r101_wave))||
                   (p[0x2f4]==2&&(stamp19||stamp20||r102_death||(r102_em03&&!half(p+0x54a))));
 return allowed?0x48u:0x40u;
}
static gh_actor_motion_status track_value(gh_actor_motion*s,unsigned index,float time,int cached,float*out,uint32_t*f){
 gh_motion_track t;if(gh_motion_track_at(&s->motion,index,&t))return GH_ACTOR_MOTION_BOUNDS;
 if(cached&&(t.method==1||t.method==3||t.method==5)&&t.keys<=1){*out=0;return GH_ACTOR_MOTION_OK;}
 gh_motion_cache cache;gh_motion_status e=gh_motion_sample_reference(&s->motion,index,time,out,&cache,f);if(e)return fail(f,*f?*f:0x159ad0);if(cached)s->caches[index]=cache;return GH_ACTOR_MOTION_OK;
}
gh_actor_motion_status gh_actor_motion_sample_00158328(gh_actor_motion*s,gh_static_transform_policy policy,uint32_t*f){
 if(!s||!s->bound||!s->complete||!s->b.model||!f)return GH_ACTOR_MOTION_ARGUMENT;*f=0;gh_actor_model*m=s->b.model;uint8_t*p=m->root.bytes;
 if(policy!=GH_STATIC_BINARY32_REFERENCE||(half(p+0x434)&~motion_flags_allowed(s)))return fail(f,0x158328);if(m->part_count>255)return GH_ACTOR_MOTION_BOUNDS;
 float time=rf(p+0x430),blend=rf(p+0x27c);if(!isfinite(time)||!isfinite(blend)||time<0||time>s->motion.frames-1)return fail(f,0x158328);
 int indices[256][9];for(unsigned i=0;i<256;i++)for(unsigned j=0;j<9;j++)indices[i][j]=-1;
 for(unsigned k=0;k<s->motion.count;k++){gh_motion_track t;if(gh_motion_track_at(&s->motion,k,&t))return GH_ACTOR_MOTION_BOUNDS;
  if(t.part==0x7f)break;if(t.channel>8||(t.part!=0xff&&t.part>=m->part_count))return fail(f,0x158328);unsigned owner=t.part==0xff?255:t.part;if(indices[owner][t.channel]>=0)return fail(f,0x158328);indices[owner][t.channel]=(int)k;
 }
 float values[256][9];for(unsigned i=0;i<256;i++)for(unsigned j=0;j<9;j++)if(indices[i][j]>=0){gh_actor_motion_status e=track_value(s,(unsigned)indices[i][j],time,(i==255&&j<3)||(i!=255&&j>=3&&j<6),&values[i][j],f);if(e)return e;}
 for(unsigned j=0;j<9;j++){float v=indices[255][j]>=0?values[255][j]:(j<6?0:1);wf(p+(j<3?0x310+j*4:j<6?0x340+(j-3)*4:0x370+(j-6)*4),v);}
 const int mirror=(half(p+0x434)&2u)!=0;
 if(mirror)wf(p+0x310,-rf(p+0x310));
 float angle[4],wrapped[4];memcpy(angle,p+0x340,16);if(gh_actor_euler_adjust_001f7da0(angle,wrapped,policy,f))return GH_ACTOR_MOTION_UNSUPPORTED;
 if(mirror){wrapped[1]=-wrapped[1];wrapped[2]=-wrapped[2];}
 memcpy(p+0x340,wrapped,12);
 for(unsigned i=0;i<m->part_count;i++){
  gh_actor_part*part=&m->parts[i];
  uint8_t*q=part->bytes;
  if(!q||!part->alias_d0)return GH_ACTOR_MOTION_ARGUMENT;
  uint32_t flags=rd(q+0x154);
  if(flags&0x2000)flags|=0x400000;
  if(flags&0x8000)flags|=0x200000;
  wr(q+0x154,flags);
  /* 1586D4 uses typed part14C for the target, including original self links.
   * The source flags independently mask the reflected rotation channels. */
  const int mirror_part=mirror&&(part->mirror_is_self||part->mirror>=0)&&!(flags&0x00800000u);
  const int mirror_angles=mirror_part&&!(flags&0x03000000u);
  if(mirror_part&&!part->mirror_is_self){
   if((unsigned)part->mirror>=m->part_count)return fail(f,0x158730);
   part=&m->parts[part->mirror];
   q=part->bytes;
   if(!q||!part->alias_d0)return GH_ACTOR_MOTION_ARGUMENT;
   flags=rd(q+0x154);
  }
  flags&=~0x80u;
  wr(q+0x154,flags);
  if(flags&1)continue;
  for(unsigned j=0;j<9;j++){
   if((j<3&&(flags&0x200))||(j>=3&&j<6&&(flags&0x100))||(j>=6&&(flags&0x400)))continue;
   uint8_t*target=j<3?part->alias_d0+j*4:j<6?q+0x100+(j-3)*4:q+0x110+(j-6)*4;
   if(indices[i][j]>=0){
    const int negate=(j==0&&mirror_part)||((j==4||j==5)&&mirror_angles);
    wf(target,negate?-values[i][j]:values[i][j]);
    flags|=0x80;
    wr(q+0x154,flags);
   }
   else if(j<3&&blend>0&&!(flags&0x100000))memcpy(target,q+0xe0+j*4,4);
   else if(j>=3&&j<6&&(flags&0x8000))wr(target,0);
  }
 }
 return GH_ACTOR_MOTION_OK;
}
gh_actor_motion_status gh_actor_motion_step_00157c18(gh_actor_motion*s,float rate,gh_static_transform_policy policy,int*ended,uint32_t*f){
 if(!s||!s->bound||!s->complete||!s->b.model||!s->b.work||!ended||!f)return GH_ACTOR_MOTION_ARGUMENT;*f=0;*ended=0;uint8_t*p=s->b.model->root.bytes;uint32_t rate_bits;memcpy(&rate_bits,&rate,4);
 if(policy!=GH_STATIC_BINARY32_REFERENCE||(half(p+0x434)&~motion_flags_allowed(s))||!isfinite(rate)||rate<0)return fail(f,0x157c18);
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
