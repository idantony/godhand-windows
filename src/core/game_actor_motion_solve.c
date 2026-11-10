#include "game_actor_motion_solve.h"
#include <string.h>
#include <math.h>
#include <fenv.h>
static uint32_t rd(const uint8_t*p){uint32_t n;memcpy(&n,p,4);return n;}
static void wr(uint8_t*p,uint32_t n){memcpy(p,&n,4);}
static float rf(const uint8_t*p){float x;memcpy(&x,p,4);return x;}
static void wf(uint8_t*p,float x){memcpy(p,&x,4);}
static float add(float a,float b){volatile float x=a+b;return x;}
static float sub(float a,float b){volatile float x=a-b;return x;}
static float mul(float a,float b){volatile float x=a*b;return x;}
static gh_actor_motion_status fail(gh_motion_solve*s,uint32_t*f,uint32_t pc){s->frontier=*f=pc;return GH_ACTOR_MOTION_UNSUPPORTED;}
static void transform(float*out,const uint8_t*m,const uint8_t*v){for(unsigned row=0;row<3;row++)out[row]=add(add(add(mul(rf(m+row*4),rf(v)),mul(rf(m+16+row*4),rf(v+4))),mul(rf(m+32+row*4),rf(v+8))),rf(m+48+row*4));out[3]=1;}
static void product(uint8_t*out,const uint8_t*a,const uint8_t*b){float m[16];for(unsigned col=0;col<4;col++)for(unsigned row=0;row<4;row++){float v=mul(rf(a+row*4),rf(b+col*16));v=add(v,mul(rf(a+16+row*4),rf(b+col*16+4)));v=add(v,mul(rf(a+32+row*4),rf(b+col*16+8)));m[col*4+row]=add(v,mul(rf(a+48+row*4),rf(b+col*16+12)));}memcpy(out,m,64);}
/* Actual30A3F8 transposes3x3 evenfornon-unit scale; nevergeneralinverse.
 * TranslationW is preserved by its XYZ-only VU masks. */
static void transpose_inverse(uint8_t*out,const uint8_t*in){float m[16]={0};for(unsigned col=0;col<3;col++)for(unsigned row=0;row<3;row++)m[col*4+row]=rf(in+row*16+col*4);
 for(unsigned row=0;row<3;row++)m[12+row]=sub(0,add(add(mul(m[row],rf(in+48)),mul(m[4+row],rf(in+52))),mul(m[8+row],rf(in+56))));m[15]=rf(in+60);memcpy(out,m,64);}
static const uint8_t*parent_matrix(const gh_actor_model*m,unsigned index){const gh_actor_part*p=m->parts+index;if(p->parent_is_model)return m->root.bytes+0x80;if(p->parent<0||(unsigned)p->parent>=m->part_count)return NULL;return m->parts[p->parent].bytes?m->parts[p->parent].bytes+0x80:NULL;}gh_actor_motion_status gh_actor_motion_solve_00152a20(gh_motion_solve*s,const gh_motion_solve_bindings*b,gh_static_transform_policy policy,uint32_t*f){
 if(!s||!b||!b->model||!b->model->parts_ready||!b->work||!b->actors||!f)return GH_ACTOR_MOTION_ARGUMENT;*f=0;
 gh_actor_model*m=b->model;if(m->part_count>255||!m->root.bytes||m->root.size<0x4a0||!m->root.alias_f0)return GH_ACTOR_MOTION_ARGUMENT;
 if(s->model&&(s->model!=m||s->work!=b->work||s->actors!=b->actors))return GH_ACTOR_MOTION_BUSY;s->model=m;s->work=b->work;s->actors=b->actors;
 if(policy!=GH_STATIC_BINARY32_REFERENCE)return fail(s,f,0x152a20);uint8_t*p=m->root.bytes;
 fenv_t env;if(fegetenv(&env)||fesetround(FE_TONEAREST))return fail(s,f,0x152a20);gh_actor_motion_status result=GH_ACTOR_MOTION_OK;
 while(s->index<m->part_count){
  unsigned i=s->index;uint8_t*a=m->parts[i].bytes;if(!a){result=GH_ACTOR_MOTION_ARGUMENT;break;}
  uint32_t flags=rd(a+0x154);
  if(!s->stage){if(!(flags&0x2000)||!(flags&0x400000)){s->index++;continue;}
   if(flags&0x1000){result=fail(s,f,0x152a20);break;}
   if(i+2>=m->part_count){result=GH_ACTOR_MOTION_BOUNDS;break;}uint8_t*c=m->parts[i+2].bytes;if(!c||!m->parts[i+1].bytes||!m->parts[i+2].alias_d0||!m->parts[i+2].alias_f0){result=GH_ACTOR_MOTION_ARGUMENT;break;}wr(a+0x154,flags&~0x400000u);
   if(rd(c+0x154)&0x200000)transform(s->target,p+0x80,m->parts[i+2].alias_d0);
   else{unsigned target=i+2;if(flags&0x4000)target++;if(target>=m->part_count||!m->parts[target].alias_f0){result=GH_ACTOR_MOTION_BOUNDS;break;}memcpy(s->target,m->parts[target].alias_f0,12);s->target[3]=1;}
   s->stage=1;
  }
  if(rd((uint8_t*)b->work+0x58c)&1){result=fail(s,f,0x15f700);break;}
  uint8_t*middle=m->parts[i+1].bytes;gh_actor_part*last=&m->parts[i+2];
  if(s->stage==1){
   if(!(rd(p+0x258)&0x80000000u)){
    memset(&s->request,0,sizeof(s->request));for(unsigned j=0;j<3;j++)s->request.from[j]=s->request.to[j]=s->hit[j]=s->target[j];s->request.from[3]=s->request.to[3]=s->hit[3]=1;
    float root_y=rf(m->root.alias_f0+4);s->request.from[1]=add(root_y,mul(add(rf(a+0x158),rf(middle+0x158)),.6f));s->request.to[1]=sub(root_y,add(mul(rf(p+0x448),rf(middle+0x124)),mul(rf(last->alias_d0+4),rf(p+0x114))));
    s->request.mask=2;s->request.segmented=1;s->request.hit_position=s->hit;
    uint32_t pc=0;gh_scene_ray_status ray=b->ray?b->ray(b->context,b->actors,&s->request,&s->original_hit,&pc):gh_scene_ray_query_00131e10(b->actors,&s->request,&s->original_hit,&pc);
    if(ray){result=fail(s,f,pc?pc:0x131e10);break;}s->stage=2;
   }else s->stage=3;
  }
  if(s->stage==2){
   if(!(rd(p+0x258)&0x40000000u)){
    float correction=sub(add(sub(s->hit[1],s->target[1]),mul(rf(last->alias_d0+4),rf(p+0x114))),rf(a+0x1b0));float dy=sub(rf(m->root.alias_f0+4),rf(p+0x494));
    if(fabsf(dy)<=.01f)correction=mul(correction,correction>=0?.28f:.7f);if(correction<-.3f)correction=-.3f;if(correction>.15f)correction=.15f;
    float accumulated=add(rf(a+0x1b0),correction);wf(a+0x1b0,accumulated);s->target[1]=add(s->target[1],sub(accumulated,mul(rf(last->alias_d0+4),rf(p+0x114))));
   }else{wr(a+0x1b0,0);s->target[1]=s->hit[1];}
   if(s->original_hit){float y=add(s->target[1],mul(rf(last->alias_d0+4),rf(p+0x114)));if(s->target[1]<add(s->target[1],rf(last->alias_d0+4)))s->target[1]=y;}
   s->stage=3;
  }
  if(s->stage==3){
   unsigned cindex=i+2;uint8_t*c=last->bytes;const uint8_t*parent=parent_matrix(m,cindex);if(!parent){result=GH_ACTOR_MOTION_BOUNDS;break;}
   if((rd(a+0x154)&0x4000)&&(i+3>=m->part_count||!m->parts[i+3].alias_d0)){result=GH_ACTOR_MOTION_BOUNDS;break;}
   uint8_t inverse[64];product(c+0x80,p+0x80,c);
   if(rd(a+0x154)&0x4000){
    /* Actual153808 uses1F8350 rotation-only XYZW, so temporaryF0.W can0.
     * Preserve that rawmatrix semantics, do not normalizeaffineW byhabit. */
    const uint8_t*v=m->parts[i+3].alias_d0;float direction[4];for(unsigned row=0;row<4;row++)direction[row]=add(add(mul(rf(c+0x80+row*4),rf(v)),mul(rf(c+0x90+row*4),rf(v+4))),mul(rf(c+0xa0+row*4),rf(v+8)));
    memcpy(last->alias_f0,direction,16);for(unsigned row=0;row<3;row++)wf(last->alias_f0+row*4,sub(s->target[row],direction[row]));
   }else memcpy(last->alias_f0,s->target,12);
   transpose_inverse(inverse,parent);product(c,inverse,c+0x80);s->stage=4;
  }
  if(s->stage==4){
   if(rd(a+0x154)&0x4000){unsigned child=i+3;uint8_t*q=m->parts[child].bytes;const uint8_t*parent=parent_matrix(m,child);if(!q||!parent){result=GH_ACTOR_MOTION_BOUNDS;break;}
    if((rd(p+0x250)&0x40004)==0x40000){result=fail(s,f,0x1501d0);break;}
    product(q+0x80,parent,q);if(rd(q+0x154)&0x800){result=fail(s,f,0x159fc8);break;}
   }s->stage=5;
  }
  result=fail(s,f,0x153158);break;
 }
 if(!result)memset(s,0,sizeof(*s));if(fesetenv(&env))return fail(s,f,0x152a20);return result;
}
