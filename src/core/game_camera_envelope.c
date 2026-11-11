#include "game_camera_envelope.h"
#include "id_layout_rotation.h"
#include <string.h>
#include <math.h>
#include <fenv.h>
static uint32_t word(const uint8_t*p){uint32_t v;memcpy(&v,p,4);return v;}
static float number(const uint8_t*p){float v;memcpy(&v,p,4);return v;}
static float bits(uint32_t n){float x;memcpy(&x,&n,4);return x;}
static void put(uint8_t*p,uint32_t n){memcpy(p,&n,4);}
static void store(uint8_t*p,float v){memcpy(p,&v,4);}
static float add(float a,float b){volatile float v=a+b;return v;}
static float mul(float a,float b){volatile float v=a*b;return v;}
static float divide(float a,float b){volatile float v=a/b;return v;}
static float blend(float a,float b,float w){return add(mul(a,w),mul(b,add(1,-w)));}
static gh_camera_status fail(uint32_t*f,uint32_t pc,gh_camera_status e){if(f)*f=pc;return e;}
static int read_elf(const uint8_t*b,size_t n,uint32_t va,void*out,size_t size){if(!b||n<52||memcmp(b,"\177ELF\1\1\1",7)||b[18]!=8||b[19])return 0;size_t table=word(b+28);unsigned step=b[42]|(unsigned)b[43]<<8,count=b[44]|(unsigned)b[45]<<8;if(step<32||count>128||table>n||(size_t)step*count>n-table)return 0;unsigned found=0;for(unsigned i=0;i<count;i++){const uint8_t*p=b+table+(size_t)i*step;if(word(p)!=1)continue;uint32_t off=word(p+4),base=word(p+8),files=word(p+16),mem=word(p+20);if(files>mem||off>n||files>n-off||(uint64_t)base+mem>UINT64_C(0x100000000))return 0;if(base<=va&&(uint64_t)va+size<=(uint64_t)base+files){if(found++)return 0;memcpy(out,b+off+(va-base),size);}}return found==1;}
gh_camera_status gh_game_camera_envelope_from_elf(gh_camera_envelope*s,const void*raw,size_t n,uint32_t*f){if(f)*f=0;if(!s||!raw)return GH_CAMERA_ARGUMENT;if(s->loaded)return GH_CAMERA_BUSY;gh_camera_envelope e={0};const uint8_t*b=raw;
 if(!read_elf(b,n,0x3bd540,&e.mode,4)||!read_elf(b,n,0x3bd548,e.offset,sizeof(e.offset))||!read_elf(b,n,0x3bd5f0,e.angle,sizeof(e.angle))||!read_elf(b,n,0x3bd698,e.distance,sizeof(e.distance))||!read_elf(b,n,0x3bd544,&e.smoothing,4)||!read_elf(b,n,0x3bd6d0,&e.low_offset,4)||!read_elf(b,n,0x3bd6d4,&e.high_offset,4)||!read_elf(b,n,0x42c350,e.atan_coeff,sizeof(e.atan_coeff)))return fail(f,0x139ff8,GH_CAMERA_ARGUMENT);
 for(unsigned i=0;i<14;i++){for(unsigned j=0;j<3;j++)if(!isfinite(e.offset[i][j])||!isfinite(e.angle[i][j]))return GH_CAMERA_NUMERIC;if(!isfinite(e.distance[i]))return GH_CAMERA_NUMERIC;}for(unsigned i=0;i<8;i++)if(!isfinite(e.atan_coeff[i]))return GH_CAMERA_NUMERIC;
 if(!isfinite(e.smoothing)||!isfinite(e.low_offset)||!isfinite(e.high_offset))return GH_CAMERA_NUMERIC;e.loaded=1;*s=e;return GH_CAMERA_OK;
}
gh_camera_status gh_game_camera_envelope_bind_model(gh_camera_envelope*s,const gh_actor_model*m){if(!s||!s->loaded)return GH_CAMERA_ARGUMENT;s->player_model=m;return GH_CAMERA_OK;}
/* Actual1F8180 bounded native reference, coefficients borrowed from42C350.
 * Neither libc atan nor a generic look-at replacement is used. */
static float atan_original(const gh_camera_envelope*s,float y,float x){float ay=fabsf(y),ax=fabsf(x),pi=bits(0x40490fdb);if(ax<bits(0x3727c5ac))return ay<bits(0x3727c5ac)?0:(y<0?bits(0xbfc90fdb):bits(0x3fc90fdb));if(ay<bits(0x3727c5ac))return x<0?(y<0?-pi:pi):0;float r=divide(add(ay,-ax),add(ay,ax)),r2=mul(r,r),p=r,result=mul(r,s->atan_coeff[0]);for(unsigned i=1;i<8;i++){p=mul(p,r2);result=add(result,mul(p,s->atan_coeff[i]));}result=add(result,bits(0x3f490fdb));return x<0?(y<0?add(result,-pi):add(pi,-result)):(y<0?-result:result);}
static gh_camera_status aim(const gh_camera_envelope*s,gh_game_pl_camera*c,uint32_t*f){float d[3];for(unsigned i=0;i<3;i++){d[i]=add(number(c->fields+0x210+i*4),-number(c->fields+0x200+i*4));if(!isfinite(d[i]))return fail(f,0x138358,GH_CAMERA_NUMERIC);}float yaw=(d[0]==0&&d[2]==0)?0:atan_original(s,d[0],d[2]),sn,cs;if(gh_id_sincos_reference(-yaw,&sn,&cs))return fail(f,0x30a7b8,GH_CAMERA_UNSUPPORTED);
 /* 138358 uses actual MtxInitRotY then30A2B0, including explicit zero terms. */
 float ry=add(add(add(mul(0,d[0]),mul(1,d[1])),mul(0,d[2])),0),rz=add(add(add(mul(-sn,d[0]),mul(0,d[1])),mul(cs,d[2])),0);float pitch=(ry==0&&rz==0)?0:-atan_original(s,ry,rz);if(!isfinite(pitch)||!isfinite(yaw))return fail(f,0x1f8180,GH_CAMERA_NUMERIC);store(c->fields+0x220,pitch);store(c->fields+0x224,yaw);put(c->fields+0x228,0);return GH_CAMERA_OK;}
static gh_camera_status resolve_target(const gh_player_target_bindings *binding,
 const gh_title_player_view *player,uint32_t handle,gh_title_player_view *target,
 uint32_t entry,uint32_t *f)
{
 if(!binding||binding->player!=player||!binding->resolve)
  return fail(f,entry,GH_CAMERA_UNSUPPORTED);
 uint32_t pc=0;
 gh_player_target_status status=binding->resolve(binding->context,handle,target,&pc);
 if(status)return fail(f,pc?pc:entry,GH_CAMERA_UNSUPPORTED);
 if(!target->bytes||target->size<0x568)return fail(f,entry,GH_CAMERA_UNSUPPORTED);
 return GH_CAMERA_OK;
}

static gh_camera_status finish(gh_camera_envelope*s,gh_camera_follow*follow,
 const gh_player_target_bindings *targets,const gh_game_init_state *init,
 gh_frame_random *random,uint32_t*f){
 if(!s||!s->loaded||!follow||!follow->bound||!follow->camera||!follow->player||!follow->player->bytes||follow->player->size<GH_TITLE_PLAYER_BYTES||follow->stage!=6)return GH_CAMERA_ARGUMENT;
 gh_game_pl_camera*c=follow->camera;const uint8_t*p=follow->player->bytes;uint8_t*b=c->fields;if(c->policy!=GH_CAMERA_NATIVE_BINARY32_REFERENCE)return fail(f,0x139ff8,GH_CAMERA_UNSUPPORTED);if(s->stage&&s->pending!=c)return GH_CAMERA_BUSY;
 if(!s->stage){
  /* The local expanded center/rotated envelopes have no persistent read or
   * outgoing ray consumer in this no-target domain. This omits dead scratch,
   * not a collision hit/miss or an original world operation. */
  if(word(b+0x528)&2)return fail(f,0x13a600,GH_CAMERA_UNSUPPORTED);
  float amount=add(mul(number(b+0x510),15),31);if(amount>45)amount=45;float fov=add(mul(number(b+0x250),bits(0x3f666666)),mul(amount,bits(0x3dcccccd)));if(!isfinite(fov))return fail(f,0x13a6e0,GH_CAMERA_NUMERIC);store(b+0x250,fov);s->pending=c;s->stage=1;
 }
 const uint32_t target_handle=word(p+0x640);
 if(s->stage>1&&s->target_handle!=target_handle)return GH_CAMERA_BUSY;
 const int special=(p[0x2f4]==0&&p[0x2f5]==6)||p[0x2f4]==1;
 const int immediate=special||word(b+0x518);
 const float weight=immediate?0:s->smoothing;
 if(s->stage==1){
  float height=0,part_y=0,player_y=number(follow->player->alias_f0+4);
  int active=0;
  if(target_handle){
   gh_title_player_view target={0};
   gh_camera_status status=resolve_target(targets,follow->player,target_handle,&target,0x13a70c,f);
   if(status)return status;
   if(!(word(target.bytes+0x250)&0xe0000000u)){
    if(target.original_vtable214!=0x4479c8||
       ((unsigned)target.bytes[0x2fe]|(unsigned)target.bytes[0x2ff]<<8)!=0x202)
     return fail(f,0x13a73c,GH_CAMERA_UNSUPPORTED);
    float point[3];
    if(gh_enemy_target_position_00275978(&target,point))return fail(f,0x275978,GH_CAMERA_UNSUPPORTED);
    const float x=add(point[0],-number(follow->player->alias_f0));
    const float z=add(point[2],-number(follow->player->alias_f0+8));
    const float distance=add(mul(x,x),mul(z,z));
    if(!isfinite(distance)||!isfinite(player_y))return fail(f,0x1f7eb0,GH_CAMERA_NUMERIC);
    if(distance<9){
     if(gh_enemy_target_position_00275978(&target,point))return fail(f,0x275978,GH_CAMERA_UNSUPPORTED);
     height=mul(add(point[1],-player_y),0.5f);
    }
    active=1;
   }
  }
  if(p[0x2b4]){
   /* Original cModel.partNum!=0 -> model.parts[0] -> part F0 world Y.
    * This is an ordinary skeletal-model path, not a carried-object flag. */
   const gh_actor_model*m=s->player_model;const gh_actor_part*part=NULL;
   if(!m||!m->bound||!m->parts_ready||m->root.bytes!=p||m->root.alias_f0!=follow->player->alias_f0||m->part_count!=p[0x2b4]||m->parts_allocated!=m->part_count||gh_actor_model_part_at(m,0,&part)!=GH_ACTOR_MODEL_OK||!part||!part->bytes||part->alias_f0!=part->bytes+0xb0)return fail(f,0x13a7d8,GH_CAMERA_UNSUPPORTED);
   part_y=number(part->alias_f0+4);if(!isfinite(part_y)||!isfinite(player_y))return fail(f,0x13a7f0,GH_CAMERA_NUMERIC);
   float candidate=add(add(part_y,-1.5f),-player_y);if(height<candidate)height=candidate;
  }
  if(!isfinite(height))return fail(f,0x13a7f0,GH_CAMERA_NUMERIC);
  if(height<0)height=0;if(height>3)height=3;store(b+0x510,height);
  if(immediate)put(b+0x51c,0);
  s->target_handle=target_handle;
  s->player_y=player_y;s->part_y=part_y;
  s->stage=active?2:4;
 }
 if(s->stage==2){
  if(!word(b+0x51c)){
   uint8_t draw;
   if(!random||gh_frame_random_byte_0031cef8(random,&draw))
    return fail(f,0x31cef8,GH_CAMERA_UNSUPPORTED);
   s->mode=draw&1u;
  }
  put(b+0x51c,15);
  s->stage=3;
 }
 if(s->stage==3){
  const uint32_t focus_handle=word(p+0x690);
  if(focus_handle){
   gh_title_player_view excluded={0};
   gh_camera_status status=resolve_target(targets,follow->player,focus_handle,&excluded,0x291010,f);
   if(status)return status;
   if(!init||c->identity!=&init->camera)return fail(f,0x291010,GH_CAMERA_UNSUPPORTED);
   uint32_t pc=0;
   gh_player_target other={0};
   gh_player_target_status result=gh_player_enemy_target_excluding_00291010(targets,init,8,
       bits(0x40490fdb),&excluded,&other,&pc);
   if(result)return fail(f,pc?pc:0x291010,GH_CAMERA_UNSUPPORTED);
   if(other.handle&&!s->mode)s->mode=1;
  }else s->mode=0;
  if((p[0x2f4]==0&&p[0x2f5]==0x11)||number(b+0x510)>1)s->mode=1;
  /* 12CA60 uses the same first native player part examined for height. */
  if((word(p+0x15f4)&0x200000u)||(p[0x2b4]&&add(s->player_y,0.8f)>s->part_y)){
   s->mode=9;
   put(b+0x51c,0);
  }
  s->stage=4;
 }
 if(s->stage==4){
  uint32_t flags=word(p+0x15f4);if(flags&0x100000){s->mode=8;put(b+0x51c,15);}if(flags&0x1000000){s->mode=13;put(b+0x51c,15);}if(flags&0x1000){s->mode=4;put(b+0x51c,1);}if(s->mode>13)s->mode=0;
  uint32_t time=word(b+0x51c);float result[7];unsigned offsets[7]={0x400,0x404,0x408,0x410,0x414,0x418,0x420};
  if(time){put(b+0x51c,time-1);for(unsigned i=0;i<3;i++){result[i]=blend(number(b+0x400+i*4),s->offset[s->mode][i],weight);result[i+3]=blend(number(b+0x410+i*4),s->angle[s->mode][i],weight);}result[6]=blend(number(b+0x420),s->distance[s->mode],weight);}
  else{for(unsigned i=0;i<3;i++)result[i]=blend(number(b+0x400+i*4),number(b+0x440+i*4),weight);result[3]=blend(number(b+0x410),number(b+0x450),weight);result[5]=blend(number(b+0x418),number(b+0x458),weight);result[6]=blend(number(b+0x420),number(b+0x460),weight);
   int idle=p[0x2f4]==0&&(p[0x2f5]==0||p[0x2f5]==10),state3=p[0x2f4]==0&&p[0x2f5]==3;float offset=s->low_offset;if(word(p+0x690)&&(idle||state3||special))offset=s->high_offset;result[4]=blend(number(b+0x414),add(number(b+0x454),offset),weight);
  }
  for(unsigned i=0;i<7;i++)if(!isfinite(result[i]))return fail(f,0x13ab80,GH_CAMERA_NUMERIC);for(unsigned i=0;i<7;i++)store(b+offsets[i],result[i]);s->stage=5;
 }
 gh_camera_status e=aim(s,c,f);if(e)return e;follow->stage=0;follow->frontier=0;s->stage=0;s->pending=NULL;s->target_handle=0;return GH_CAMERA_OK;
}
gh_camera_status gh_game_camera_finish_follow_target_00139c88(gh_camera_envelope*s,gh_camera_follow*follow,
 const gh_player_target_bindings *targets,const gh_game_init_state *init,gh_frame_random *random,uint32_t*f){
 if(f)*f=0;fenv_t env;if(feholdexcept(&env))return GH_CAMERA_NUMERIC;
 if(fesetround(FE_TONEAREST)){(void)fesetenv(&env);return GH_CAMERA_NUMERIC;}
 gh_camera_status e=finish(s,follow,targets,init,random,f);if(fesetenv(&env))return GH_CAMERA_NUMERIC;return e;
}
gh_camera_status gh_game_camera_finish_follow_00139c88(gh_camera_envelope*s,gh_camera_follow*follow,uint32_t*f){
 return gh_game_camera_finish_follow_target_00139c88(s,follow,NULL,NULL,NULL,f);
}
