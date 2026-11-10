#include "game_actor_model_light.h"
#include "game_actor_light_normalize.h"
#include "game_actor_light_attachment.h"
#include <string.h>
#include <math.h>
#include <fenv.h>

static uint32_t word(const void *p){uint32_t n;memcpy(&n,p,4);return n;}
static unsigned half(const uint8_t *p){return (unsigned)p[0]|(unsigned)p[1]<<8;}
static float number(const void *p){float n;memcpy(&n,p,4);return n;}
static float sub(float a,float b){volatile float n=a-b;return n;}
static float add(float a,float b){volatile float n=a+b;return n;}
static float mul(float a,float b){volatile float n=a*b;return n;}
static float divide(float a,float b){volatile float n=a/b;return n;}
static gh_actor_model_status stop(uint32_t *pc,uint32_t n,gh_actor_model_status e){*pc=n;return e;}
static int normal_or_zero(float n){return isfinite(n)&&(n==0||fpclassify(n)==FP_NORMAL);}
/* 1F7E28: VMUL.xyz, VADDY.x, VADDZ.x, no square root. The separate
 * 101E28 ->1F7DF8 path returns SQRT of this same ordered sum. */
static gh_actor_model_status distance_squared(const float xyz[3],float *out,uint32_t *pc){
 float squares[3];
 for(unsigned k=0;k<3;k++){
  if(!normal_or_zero(xyz[k]))return stop(pc,0x1f7e28,GH_ACTOR_MODEL_UNSUPPORTED);
  squares[k]=mul(xyz[k],xyz[k]);
  if(!normal_or_zero(squares[k])||(xyz[k]!=0&&squares[k]==0))return stop(pc,0x1f7e28,GH_ACTOR_MODEL_UNSUPPORTED);
 }
 float xy=add(squares[0],squares[1]),sum=add(xy,squares[2]);
 if(!normal_or_zero(xy)||!normal_or_zero(sum))return stop(pc,0x1f7e28,GH_ACTOR_MODEL_UNSUPPORTED);
 *out=sum;return GH_ACTOR_MODEL_OK;
}

/* 2D9000 is signed arithmetic, including the original selector -1 alias at
 * owner+10. This is not an array-index clamp or a synthetic default record. */
static const uint8_t *record(const gh_game_world_light *s,int index){
 if(index==-1)return s->scalar+0x10;
 if(index<0||index>=256)return NULL;
 return s->scalar+0x80+(size_t)index*0x70;
}

static gh_actor_model_status directional(const gh_actor_model *model,const uint8_t *r,const gh_actor_light_bindings *b,const gh_game_world_light *s,unsigned area,
 const float source_position[3],float strength,float direction[3],float color[3],uint32_t *pc){
 uint32_t flags=word(r+0x58);
 if(flags&0x100000)return stop(pc,0x1f7ef0,GH_ACTOR_MODEL_UNSUPPORTED);

 unsigned kind=half(r);
 if(!b||!b->pivot_owner||!b->pivot_owner->initialized)return stop(pc,0x14dfd8,GH_ACTOR_MODEL_ARGUMENT);
 float point_direction[4]={0,0,0,0};
 if(kind==1){
  float point[3]={0,0,0},delta[3],squared;
  if(flags&0x1000000){
   if(model->root.bytes[0x2b4]){
    const gh_actor_part *part;
    if(gh_actor_model_part_at(model,0,&part))return stop(pc,0x14e1f4,GH_ACTOR_MODEL_UNSUPPORTED);
    memcpy(point,part->bytes+0xb0,12);
   }
  }else{
   if(!model->root.alias_f0)return stop(pc,0x14e264,GH_ACTOR_MODEL_UNSUPPORTED);
   memcpy(point,model->root.alias_f0,12);
  }
  for(unsigned k=0;k<3;k++)delta[k]=sub(point[k],source_position[k]);
  if(distance_squared(delta,&squared,pc))return stop(pc,0x101e28,GH_ACTOR_MODEL_UNSUPPORTED);
  /* Original conditional length>0 keeps initialized VF0 XYZ at +0 for
   * exact zero, unlike the unguarded30A370 signed-zero normalization. */
  if(squared>0){uint32_t f=0;if(gh_actor_light_normalize_0030a370(delta,point_direction,GH_STATIC_BINARY32_REFERENCE,&f))return stop(pc,0x101e28,GH_ACTOR_MODEL_UNSUPPORTED);}
 }
 for(unsigned k=0;k<3;k++){
  float d=number(r+0x20+4*k),c=number(r+0x30+4*k),p=number(b->pivot_owner->bytes+4*k);
  if(!isfinite(p)||(kind<=2&&(!isfinite(c)||(kind!=1&&!isfinite(d)))))return stop(pc,0x14dfd8,GH_ACTOR_MODEL_UNSUPPORTED);
  direction[k]=kind==1?point_direction[k]:(kind>2?p:d);color[k]=kind>2?p:sub(c,p);
  if(kind<2){float t=strength<0?0:(strength<1?strength:1);volatile float result=color[k]*t;color[k]=result;}
  if(!isfinite(color[k]))return stop(pc,0x14dfd8,GH_ACTOR_MODEL_UNSUPPORTED);
 }
 if(flags&0x2000000){
  float ratio=1;
  if(s->table.bytes&&area){
   const uint8_t *table=s->table.bytes;size_t size=s->table.size;
   if(size<16||word(table)>(size-16)/0x50)return stop(pc,0x2d9360,GH_ACTOR_MODEL_BOUNDS);
   for(uint32_t i=0;i<word(table);i++)if(table[0x12+(size_t)i*0x50]==area){ratio=number(table+0x14+(size_t)i*0x50);break;}
  }else if(area&&word(s->scalar+0x16130))return stop(pc,0x2d9360,GH_ACTOR_MODEL_UNSUPPORTED);
  if(!isfinite(ratio))return stop(pc,0x2d9360,GH_ACTOR_MODEL_UNSUPPORTED);
  for(unsigned k=0;k<3;k++){volatile float n=color[k]*ratio;color[k]=n;if(!isfinite(n))return stop(pc,0x14e47c,GH_ACTOR_MODEL_UNSUPPORTED);}
 }
 return GH_ACTOR_MODEL_OK;
}

static gh_actor_model_status run(gh_actor_model *m,gh_game_world_light *s,
 const gh_actor_light_bindings *b,uint32_t *pc){
 uint8_t *root=m->root.bytes;
 if(word(root+0x250)&2)return GH_ACTOR_MODEL_OK;
 uint32_t count=word(s->scalar+0x78);
 if(count>256)return stop(pc,0x2d8ff8,GH_ACTOR_MODEL_BOUNDS);
 int selected[3]={-1,-1,-1},least=0,reflection=-1,shadow=-1;
 float strength[3]={0,0,0},least_strength=-1,reflect_strength=-1,shadow_strength=-1;
 float positions[3][3]={{0}},reflection_position[3]={0},shadow_position[3]={0},camera_matrix[16];
 int camera_written=0;
 /* Preserve ordered source distance and attenuation, then rank candidates.
  * Attached/animated records remain explicit separate dependencies. */
 for(unsigned i=0;i<count;i++){
  const uint8_t *r=record(s,(int)i);unsigned kind=half(r);if(kind==255)break;
  uint32_t flags=word(r+0x58);
  if(!word(r+0x50)&&word(r+0x4c)!=root[0x2cc])continue;
  if(flags&0x40000){
   const void *owner=s->native_light_owners[i];
   if(!owner){if(word(r+0x54))return stop(pc,0x14d954,GH_ACTOR_MODEL_UNSUPPORTED);continue;}
   if(owner!=root)continue;
  }else if(!(flags&word(root+0x2d0)))continue;
  float source_position[4];memcpy(source_position,r+0x10,16);
  if(flags&0x4000000){
   int written=0;float scratch[16];
   gh_actor_model_status e=gh_actor_light_attachment_0014d9a4(m,b,(flags&0x1000000)!=0,source_position,source_position,scratch,&written,GH_STATIC_BINARY32_REFERENCE,pc);if(e)return e;
   if(written){memcpy(camera_matrix,scratch,64);camera_written=1;}
  }
  /* Actual read-only 41EA88 is FLT_MAX (verified in evidence). */
  float value=0;const uint32_t maximum=0x7f7fffff;
  if(kind<2){
   if(!m->root.alias_f0)return stop(pc,0x14daa4,GH_ACTOR_MODEL_UNSUPPORTED);
   float delta[3],squared;
   for(unsigned k=0;k<3;k++)delta[k]=sub(number(m->root.alias_f0+k*4),source_position[k]);
   gh_actor_model_status e=distance_squared(delta,&squared,pc);if(e)return e;
   float a=number(r+0x40),bcoef=number(r+0x44),c=number(r+0x48);
   if(!normal_or_zero(a)||!normal_or_zero(bcoef)||!normal_or_zero(c))return stop(pc,0x14dad8,GH_ACTOR_MODEL_UNSUPPORTED);
   float radius=sqrtf(squared),quadratic=mul(a,squared),linear=mul(bcoef,radius),denominator=add(quadratic,linear);
   if(!normal_or_zero(radius)||!normal_or_zero(quadratic)||!normal_or_zero(linear)||!normal_or_zero(denominator))return stop(pc,0x14daf0,GH_ACTOR_MODEL_UNSUPPORTED);
   if(denominator==0)return stop(pc,0x14daf4,GH_ACTOR_MODEL_UNSUPPORTED);
   value=add(divide(1,denominator),c);
   if(!normal_or_zero(value))return stop(pc,0x14dafc,GH_ACTOR_MODEL_UNSUPPORTED);
  }
  if(kind==2||(flags&0x8000000))memcpy(&value,&maximum,4);
  if(flags&0x400000){if((word(root+0x254)&0x20000000)&&reflect_strength<value){reflection=(int)i;reflect_strength=value;memcpy(reflection_position,source_position,12);}}
  else if(flags&0x200000){if(shadow_strength<value){shadow=(int)i;shadow_strength=value;memcpy(shadow_position,source_position,12);}}
  else if(least_strength<value){
   selected[least]=(int)i;strength[least]=value;least_strength=value;memcpy(positions[least],source_position,12);
   for(unsigned k=0;k<3;k++){
    if(selected[k]<0){least_strength=strength[k];least=(int)k;}
    else if(strength[k]<least_strength){least_strength=strength[k];least=(int)k;}
   }
  }
 }
 float directions[3][3],colors[3][3],reflect_dir[3],reflect_color[3],shadow_dir[3],shadow_color[3];
 unsigned used=0;
 for(unsigned k=0;k<3;k++)if(selected[k]>=0){
  gh_actor_model_status e=directional(m,record(s,selected[k]),b,s,root[0x2cc],positions[k],strength[k],directions[used],colors[used],pc);if(e)return e;used++;
 }
 int has_reflection=(word(root+0x254)&0x20000000)!=0;
 if(has_reflection){
  if(reflection<0){int best=0;float best_value=-1;for(unsigned k=0;k<3;k++)if(strength[k]>best_value){best_value=strength[k];best=(int)k;}reflection=selected[best];reflect_strength=strength[best];memcpy(reflection_position,positions[best],12);}
  gh_actor_model_status e=directional(m,record(s,reflection),b,s,root[0x2cc],reflection_position,reflect_strength,reflect_dir,reflect_color,pc);if(e)return e;
 }
 if(shadow>=0){gh_actor_model_status e=directional(m,record(s,shadow),b,s,root[0x2cc],shadow_position,shadow_strength,shadow_dir,shadow_color,pc);if(e)return e;}
 /* Commit only after every required producer was validated. The original
  * writes XYZ only: neighboring W words must survive. */
 for(unsigned k=0;k<3;k++)root[0x2cd+k]=(uint8_t)selected[k];
 for(unsigned k=0;k<used;k++){memcpy(s->scalar+0x161a0+k*16,directions[k],12);memcpy(s->scalar+0x161d0+k*16,colors[k],12);}
 for(unsigned k=used;k<3;k++)memset(s->scalar+0x161d0+k*16,0,12);
 if(has_reflection){memcpy(s->scalar+0x16200,reflect_dir,12);memcpy(s->scalar+0x16210,reflect_color,12);}
 if(camera_written)memcpy(s->scalar+0x16240,camera_matrix,64);
 if(shadow>=0){memcpy(s->scalar+0x16220,shadow_dir,12);memcpy(s->scalar+0x16230,shadow_color,12);}
 else{
  const uint32_t d[3]={0xbe23d70a,0xbf733333,0xbe851eb8},c[4]={0x3f000000,0x3f000000,0x3f000000,0x3f800000};
  memcpy(s->scalar+0x16220,d,12);memcpy(s->scalar+0x16230,c,16);
 }
 return GH_ACTOR_MODEL_OK;
}

gh_actor_model_status gh_actor_model_light_0014d828(gh_actor_model *m,gh_game_world_light *s,
 const gh_actor_light_bindings *b,gh_static_transform_policy policy,uint32_t *pc){
 uint32_t local=0;if(!pc)pc=&local;*pc=0;
 if(!m||m->native_identity!=m||!m->ready||!m->root.bytes||m->root.size<0x2d4||!s||!s->constructed||!s->ready||policy!=GH_STATIC_BINARY32_REFERENCE)return GH_ACTOR_MODEL_ARGUMENT;
 fenv_t env;if(feholdexcept(&env))return GH_ACTOR_MODEL_UNSUPPORTED;
 if(fesetround(FE_TONEAREST)){fesetenv(&env);return GH_ACTOR_MODEL_UNSUPPORTED;}
 gh_actor_model_status e=run(m,s,b,pc);
 if(fesetenv(&env))return GH_ACTOR_MODEL_UNSUPPORTED;
 return e;
}
