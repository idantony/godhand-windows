#include "game_actor_shadow_draw.h"
#include "game_actor_two_bone.h"
#include "game_actor_matrix.h"
#include "sdk_sine.h"
#include <string.h>
#include <math.h>
#include <fenv.h>
static uint32_t word(const void*p){uint32_t n;memcpy(&n,p,4);return n;}
static uint16_t half(const void*p){uint16_t n;memcpy(&n,p,2);return n;}
static float value(const void*p){float n;memcpy(&n,p,4);return n;}
static void putf(void*p,float n){memcpy(p,&n,4);}
static float add(float a,float b){volatile float x=a+b;return x;}
static float sub(float a,float b){volatile float x=a-b;return x;}
static float mul(float a,float b){volatile float x=a*b;return x;}
static float divide(float a,float b){volatile float x=a/b;return x;}
static float clamp(float x,float max){if(x<0)x=0;if(x>=max)x=max;return x;}
static float dot3(const float*a,const float*b){return add(add(mul(a[0],b[0]),mul(a[1],b[1])),mul(a[2],b[2]));}
static void cross(const float*a,const float*b,float*out){out[0]=sub(mul(a[1],b[2]),mul(a[2],b[1]));out[1]=sub(mul(a[2],b[0]),mul(a[0],b[2]));out[2]=sub(mul(a[0],b[1]),mul(a[1],b[0]));}
static gh_actor_model_status stop(uint32_t*f,uint32_t pc,gh_actor_model_status e){if(f)*f=pc;return e;}
static int read_elf(const void*raw,size_t n,uint32_t pc,void*out,size_t bytes){
 const uint8_t*p=raw;if(!p||n<52||memcmp(p,"\177ELF\1\1\1",7)||half(p+18)!=8)return 0;uint32_t ph=word(p+28);unsigned stride=half(p+42),count=half(p+44),found=0;if(stride<32||count>128||ph>n||(size_t)count*stride>n-ph)return 0;
 for(unsigned i=0;i<count;i++){const uint8_t*q=p+ph+(size_t)i*stride;uint32_t off=word(q+4),base=word(q+8),size=word(q+16);if(word(q)!=1)continue;if(off>n||size>n-off)return 0;if(base<=pc&&(uint64_t)pc+bytes<=(uint64_t)base+size){if(found++)return 0;memcpy(out,p+(size_t)off+(size_t)(pc-base),bytes);}}return found==1;
}
gh_actor_model_status gh_shadow_templates_from_elf(gh_shadow_templates*s,const void*elf,size_t bytes){if(!s||!elf)return GH_ACTOR_MODEL_ARGUMENT;if(s->initialized)return GH_ACTOR_MODEL_BUSY;uint8_t pattern[64];if(!read_elf(elf,bytes,0x3c3f68,pattern,64))return GH_ACTOR_MODEL_BOUNDS;unsigned count=0;for(unsigned i=0;i<8;i++){const uint8_t*p=pattern+i*8;if(!isfinite(value(p))||value(p)<0||!p[5]||p[5]>12)return GH_ACTOR_MODEL_UNSUPPORTED;count+=p[5];}if(count!=12)return GH_ACTOR_MODEL_UNSUPPORTED;memset(s,0,sizeof(*s));memcpy(s->player_pattern,pattern,64);s->initialized=1;s->identity=s;return GH_ACTOR_MODEL_OK;}
static void point(float*out,const float*m,const float*p){for(unsigned r=0;r<4;r++){float n=mul(m[r],p[0]);n=add(n,mul(m[4+r],p[1]));n=add(n,mul(m[8+r],p[2]));out[r]=add(n,mul(m[12+r],p[3]));}}
static gh_actor_model_status project(gh_shadow_quad*q,const gh_game_pl_camera*c,gh_static_transform_policy policy,uint32_t*f){
 float rotation[16],translation[16]={0},scale[16]={0},temporary[16],model[16],clip[16],screen[16];uint32_t pc=0;
 if(gh_actor_quaternion_matrix_001478a8(q->quaternion,rotation,policy,&pc))return stop(f,pc?pc:0x1478a8,GH_ACTOR_MODEL_UNSUPPORTED);
 for(unsigned k=0;k<4;k++){translation[k*5]=1;scale[k*5]=k==0||k==2?q->size:1;}memcpy(translation+12,q->position,12);
 if(gh_actor_matrix_product_00147548(translation,rotation,temporary,policy,&pc)||gh_actor_matrix_product_00147548(temporary,scale,model,policy,&pc))return stop(f,pc?pc:0x147ab8,GH_ACTOR_MODEL_UNSUPPORTED);
 float camera_clip[16];memcpy(camera_clip,c->fields+0xc0,64);if(gh_actor_matrix_product_00147548(camera_clip,model,clip,policy,&pc))return stop(f,pc?pc:0x15ec60,GH_ACTOR_MODEL_UNSUPPORTED);
 static const float unit[4][4]={{1,0,1,1},{1,0,-1,1},{-1,0,1,1},{-1,0,-1,1}};
 for(unsigned i=0;i<4;i++){float p[4];point(p,clip,unit[i]);for(unsigned k=0;k<4;k++)if(!isfinite(p[k]))return stop(f,0x2a38e8,GH_ACTOR_MODEL_UNSUPPORTED);float w=fabsf(p[3]);for(unsigned k=0;k<3;k++)if(p[k]>w||p[k]<-w){q->enabled=0;memset(q->xy_fixed4,0,sizeof(q->xy_fixed4));memset(q->z_integer,0,sizeof(q->z_integer));return GH_ACTOR_MODEL_OK;}}
 float camera_screen[16];memcpy(camera_screen,c->fields,64);if(gh_actor_matrix_product_00147548(camera_screen,model,screen,policy,&pc))return stop(f,pc?pc:0x15ec60,GH_ACTOR_MODEL_UNSUPPORTED);
 for(unsigned i=0;i<4;i++){float p[4];point(p,screen,unit[i]);if(!isfinite(p[3])||!(p[3]>0))return stop(f,0x30ab48,GH_ACTOR_MODEL_UNSUPPORTED);float inv=divide(1,p[3]);for(unsigned k=0;k<3;k++){float n=mul(p[k],inv);if(k<2)n=mul(n,16);if(!isfinite(n)||n<=-2147483648.0f||n>=2147483648.0f)return stop(f,0x30ab48,GH_ACTOR_MODEL_UNSUPPORTED);if(k<2)q->xy_fixed4[i][k]=(int32_t)n;else q->z_integer[i]=(int32_t)n;}}
 q->enabled=1;return GH_ACTOR_MODEL_OK;
}
gh_actor_model_status gh_shadow_quad_project_0015ec60(gh_shadow_quad*q,const gh_game_pl_camera*c,gh_static_transform_policy policy,uint32_t*f){if(f)*f=0;if(!q||!c||!c->constructed||policy!=GH_STATIC_BINARY32_REFERENCE)return GH_ACTOR_MODEL_ARGUMENT;for(unsigned k=0;k<3;k++)if(!isfinite(q->position[k]))return GH_ACTOR_MODEL_ARGUMENT;if(!isfinite(q->size)||q->size<0)return GH_ACTOR_MODEL_ARGUMENT;gh_shadow_quad out=*q;fenv_t env;if(feholdexcept(&env))return GH_ACTOR_MODEL_UNSUPPORTED;if(fesetround(FE_TONEAREST)){fesetenv(&env);return GH_ACTOR_MODEL_UNSUPPORTED;}gh_actor_model_status e=project(&out,c,policy,f);if(fesetenv(&env))return GH_ACTOR_MODEL_UNSUPPORTED;if(!e)*q=out;return e;}
static gh_actor_model_status align(const float*up,const float*normal,float*out,uint32_t*f){float d=dot3(up,normal);if(d< -1)d= -1;if(d>=1)d=1;float n=sqrtf(add(add(add(d,1),d),1));if(!isfinite(n)||!(n>0))return stop(f,0x15a878,GH_ACTOR_MODEL_UNSUPPORTED);float v[3];cross(up,normal,v);float inv=divide(1,n);for(unsigned k=0;k<3;k++)out[k]=mul(v[k],inv);out[3]=mul(n,.5f);return GH_ACTOR_MODEL_OK;}
static gh_actor_model_status interpolate(float t,const float*a,const float*b,float*out,gh_static_transform_policy policy,uint32_t*f){
 float dot=add(add(add(mul(a[0],b[0]),mul(a[1],b[1])),mul(a[2],b[2])),mul(a[3],b[3])),sign=dot<0?-1:1;dot=mul(dot,sign);float first=sub(1,t),second=t;
 if(dot<.99999f){float sine=sqrtf(sub(1,mul(dot,dot))),angle;if(!(sine>0)||gh_game_angle_001f8180(sine,dot,&angle,GH_CAMERA_NATIVE_BINARY32_REFERENCE,f))return stop(f,*f?*f:0x15a6a0,GH_ACTOR_MODEL_UNSUPPORTED);float inv=divide(1,sine),v;if(gh_sdk_sine_003a19f8(mul(t,angle),&v,policy,f))return GH_ACTOR_MODEL_UNSUPPORTED;second=mul(v,inv);if(gh_sdk_sine_003a19f8(mul(sub(1,t),angle),&v,policy,f))return GH_ACTOR_MODEL_UNSUPPORTED;first=mul(v,inv);}
 second=mul(second,sign);for(unsigned k=0;k<4;k++){out[k]=add(mul(a[k],first),mul(b[k],second));if(!isfinite(out[k]))return stop(f,0x15a6a0,GH_ACTOR_MODEL_UNSUPPORTED);}return GH_ACTOR_MODEL_OK;
}
static void disabled(gh_shadow_call*c,unsigned slot){memset(c->group.quads+slot,0,sizeof(c->group.quads[slot]));uint8_t*p=c->bindings.shadow->packets[c->group.parity]+0x80+slot*0x90;uint64_t rgba=UINT64_C(0x3f80000000000000),zero=0;memcpy(p,&rgba,8);const unsigned offsets[]={0x20,0x40,0x60,0x80};for(unsigned k=0;k<4;k++)memcpy(p+offsets[k],&zero,8);}
static void publish_quad(gh_shadow_call*c,unsigned slot){gh_shadow_quad*q=&c->pending;if(!q->enabled){disabled(c,slot);return;}c->group.quads[slot]=*q;uint8_t*p=c->bindings.shadow->packets[c->group.parity]+0x80+slot*0x90;uint64_t rgba=UINT64_C(0x3f80000000ffffff)|((uint64_t)q->alpha<<24);memcpy(p,&rgba,8);for(unsigned k=0;k<4;k++){uint64_t packed=(uint64_t)(int64_t)q->xy_fixed4[k][0]|((uint64_t)(int64_t)q->xy_fixed4[k][1]<<16)|((uint64_t)(uint32_t)q->z_integer[k]<<32);memcpy(p+0x20+k*0x20,&packed,8);}}
static int same(const gh_shadow_bindings*a,const gh_shadow_bindings*b){return a->model==b->model&&a->shadow==b->shadow&&a->work==b->work&&a->camera==b->camera&&a->world_light==b->world_light&&a->actors==b->actors&&a->up_owner==b->up_owner&&a->pivot_owner==b->pivot_owner&&a->templates==b->templates&&a->emit==b->emit&&a->emit_context==b->emit_context&&a->ray==b->ray&&a->ray_context==b->ray_context&&a->policy==b->policy;}
static gh_actor_model_status ray(gh_shadow_call*c,int*hit,uint32_t*f){gh_scene_ray_status e=c->bindings.ray?c->bindings.ray(c->bindings.ray_context,c->bindings.actors,&c->ray,hit,f):gh_scene_ray_query_00131e10(c->bindings.actors,&c->ray,hit,f);return e?stop(f,*f?*f:0x131e10,GH_ACTOR_MODEL_UNSUPPORTED):GH_ACTOR_MODEL_OK;}
static gh_actor_model_status run(gh_shadow_call*c,uint32_t*f){
 const gh_shadow_bindings*b=&c->bindings;uint8_t*p=b->model->root.bytes;const uint8_t*w=(const uint8_t*)b->work;gh_actor_model_status e;
 if(c->phase==0){
  if((word(w+0x5e0)&0x400000u)||(word(p+0x4a8)&0x80000000u)||(word(p+0x250)&2u)||value(p+0x24c)<=.3f){c->complete=1;return GH_ACTOR_MODEL_OK;}
  if(!b->shadow||!b->shadow->initialized||!b->shadow->object||b->shadow->object->bytes!=p||b->shadow->pattern_original!=0x3c3f68||!b->templates||!b->templates->initialized||b->templates->identity!=b->templates)return stop(f,0x14efa8,GH_ACTOR_MODEL_UNSUPPORTED);
  /* 14EFA8 reads the active camera and the independent global Up vector. */
  if(!b->camera||!b->camera->constructed||!b->world_light||!b->world_light->constructed||!b->up_owner||!b->up_owner->bound||!b->pivot_owner||!b->pivot_owner->initialized||!b->actors)return stop(f,0x14efa8,GH_ACTOR_MODEL_UNSUPPORTED);
  uintptr_t base=(uintptr_t)p,a=(uintptr_t)b->shadow->packets[0],z=(uintptr_t)b->shadow->packets[1];if(a<base||a-base>b->model->root.size||0x750>b->model->root.size-(a-base)||z<base||z-base>b->model->root.size||0x750>b->model->root.size-(z-base)||z-a!=0x750)return GH_ACTOR_MODEL_BOUNDS;
  float scale[3],pos[3],distance[3];memcpy(scale,p+0x110,12);memcpy(pos,b->model->root.alias_f0,12);for(unsigned k=0;k<3;k++){c->direction[k]=value(b->world_light->scalar+0x16220+k*4);distance[k]=sub(value(b->camera->fields+0x210+k*4),pos[k]);if(!isfinite(scale[k])||!isfinite(pos[k])||!isfinite(distance[k])||!isfinite(c->direction[k])||!isfinite(b->up_owner->up_global[k]))return stop(f,0x14efa8,GH_ACTOR_MODEL_UNSUPPORTED);}
  c->scale=sqrtf(dot3(scale,scale));c->ray_length=mul(c->scale,1.3f);float len=sqrtf(dot3(c->direction,c->direction));if(!isfinite(c->scale)||!isfinite(len))return stop(f,0x101e28,GH_ACTOR_MODEL_UNSUPPORTED);if(len>0){float inv=divide(1,len);for(unsigned k=0;k<3;k++)c->direction[k]=mul(c->direction[k],inv);}else memset(c->direction,0,12);
  c->group=(gh_shadow_group){.owner=b->shadow,.parity=word(w+0x210)&1u,.bank=2,.tex0=UINT64_C(0x2003800599422600),.tex1=0x60,.alpha=UINT64_C(0x8000000044),.test=0x5040c,.clamp=0,.primitive=0x5c};
  memcpy(c->current_point,pos,12);memcpy(c->current_quat,b->pivot_owner->bytes,16);c->current_alpha=1;c->current_size=c->scale;c->current_valid=c->previous_valid=1;
  c->far_mode=!(word(p+0x4a8)&0x20000000u)&&dot3(distance,distance)>=100;c->phase=c->far_mode?10:1;
 }
 while(c->slot<12){
  if(c->phase==1){if(!c->remaining){if(c->row>=8)return stop(f,0x14f2bc,GH_ACTOR_MODEL_BOUNDS);const uint8_t*pattern=b->templates->player_pattern+c->row*8;c->row_count=c->remaining=pattern[5];if(!c->remaining||c->remaining>12)return stop(f,0x14f2bc,GH_ACTOR_MODEL_UNSUPPORTED);c->previous_valid=c->current_valid;memcpy(c->previous_point,c->current_point,12);memcpy(c->previous_quat,c->current_quat,16);c->previous_alpha=c->current_alpha;c->previous_size=c->current_size;
    unsigned part_index=pattern[4];if(part_index==255||part_index>=b->model->part_count)c->invalid_part=1;
    const gh_actor_part*part=NULL;if(!c->invalid_part&&gh_actor_model_part_at(b->model,part_index,&part))return stop(f,0x150128,GH_ACTOR_MODEL_UNSUPPORTED);
    if(c->invalid_part){c->current_valid=0;c->phase=3;}
    else {memcpy(c->from,part->alias_f0,12);for(unsigned k=0;k<3;k++)c->current_point[k]=add(c->from[k],mul(c->direction[k],c->ray_length));c->ray=(gh_scene_ray_request){.mask=2,.flags=0,.segmented=1,.hit_position=c->current_point,.hit_normal=c->normal};memcpy(c->ray.from,c->from,12);memcpy(c->ray.to,c->current_point,12);c->ray.from[3]=c->ray.to[3]=1;c->phase=2;}
   }else c->phase=3;
  }
  if(c->phase==2){if(word(w+0x5d8)&0x1000u)return stop(f,0x135ec8,GH_ACTOR_MODEL_UNSUPPORTED);int hit=0;e=ray(c,&hit,f);if(e)return e;c->current_valid=(uint8_t)(hit!=0);c->phase=hit?20:3;}
  if(c->phase==20){if(!(c->ray_length>0))return stop(f,0x14f4a0,GH_ACTOR_MODEL_UNSUPPORTED);e=align(b->up_owner->up_global,c->normal,c->current_quat,f);if(e)return e;const uint8_t*pattern=b->templates->player_pattern+c->row*8;float fade=clamp(sub(1,divide(sub(c->from[1],c->current_point[1]),c->ray_length)),1);c->current_alpha=mul(mul(mul((float)pattern[6],fade),value(b->world_light->scalar+0x16230)),value(p+0x24c));c->current_size=mul(value(pattern),c->scale);c->current_point[1]=add(c->current_point[1],.02f);c->phase=3;}
  if(c->phase==3){c->remaining--;c->phase=21;}
  if(c->phase==21){if(c->invalid_part||!c->current_valid||!c->previous_valid){disabled(c,c->slot);c->phase=5;}
   else {float t=divide((float)c->remaining,(float)c->row_count);c->pending=(gh_shadow_quad){0};e=interpolate(t,c->current_quat,c->previous_quat,c->pending.quaternion,b->policy,f);if(e)return e;for(unsigned k=0;k<3;k++)c->pending.position[k]=add(c->current_point[k],mul(sub(c->previous_point[k],c->current_point[k]),t));c->pending.size=mul(add(c->current_size,mul(sub(c->previous_size,c->current_size),t)),.5f);float alpha=clamp(add(c->current_alpha,mul(sub(c->previous_alpha,c->current_alpha),t)),128);if(!isfinite(alpha)||!isfinite(c->pending.size))return stop(f,0x14f5a0,GH_ACTOR_MODEL_UNSUPPORTED);c->pending.alpha=(uint8_t)(int32_t)alpha;c->phase=4;}
  }
  if(c->phase==4){e=gh_shadow_quad_project_0015ec60(&c->pending,b->camera,b->policy,f);if(e)return e;publish_quad(c,c->slot);c->phase=5;}
  if(c->phase==5){c->slot++;if(!c->remaining)c->row++;c->phase=1;}
  if(c->phase==10){memcpy(c->from,b->model->root.alias_f0,12);memcpy(c->current_point,c->from,12);c->current_point[1]=sub(c->current_point[1],c->ray_length);c->ray=(gh_scene_ray_request){.mask=2,.segmented=1,.hit_position=c->current_point,.hit_normal=c->normal};memcpy(c->ray.from,c->from,12);memcpy(c->ray.to,c->current_point,12);c->ray.from[3]=c->ray.to[3]=1;c->phase=11;}
  if(c->phase==11){int hit=0;e=ray(c,&hit,f);if(e)return e;c->phase=hit?12:14;}
  if(c->phase==12){putf(p+0x470,c->current_point[1]);if(!(c->ray_length>0))return stop(f,0x14f7b0,GH_ACTOR_MODEL_UNSUPPORTED);e=align(b->up_owner->up_global,c->normal,c->pending.quaternion,f);if(e)return e;float alpha=mul(mul(mul(clamp(sub(1,divide(sub(c->current_point[1],c->from[1]),c->ray_length)),1),128),value(b->world_light->scalar+0x16230)),value(p+0x24c));if(!isfinite(alpha))return stop(f,0x14f7b0,GH_ACTOR_MODEL_UNSUPPORTED);c->pending.alpha=(uint8_t)(int32_t)clamp(alpha,128);c->pending.size=mul(c->scale,.55f);memcpy(c->pending.position,c->current_point,12);c->phase=13;}
  if(c->phase==13){e=gh_shadow_quad_project_0015ec60(&c->pending,b->camera,b->policy,f);if(e)return e;publish_quad(c,0);c->slot=1;c->phase=14;}
  if(c->phase==14){for(;c->slot<12;c->slot++)disabled(c,c->slot);c->phase=6;}
 }
 if(c->phase!=6){int depth=(int)(int16_t)half(p+0x2ac)+(int)(int8_t)p[0x2b5]+52;if(depth<=0)depth=1;if(depth>1021)depth=1022;c->group.depth=(uint32_t)depth;c->group.ready=1;c->phase=6;}
 /* Far-mode also needs the common originaltail. */
 if(!c->group.ready){int depth=(int)(int16_t)half(p+0x2ac)+(int)(int8_t)p[0x2b5]+52;if(depth<=0)depth=1;if(depth>1021)depth=1022;c->group.depth=(uint32_t)depth;c->group.ready=1;}
 if(!b->emit)return stop(f,0x31a600,GH_ACTOR_MODEL_UNSUPPORTED);uint32_t frontier=0;e=b->emit(b->emit_context,&c->group,&frontier);if(e)return stop(f,frontier?frontier:0x31a600,e);c->complete=1;return GH_ACTOR_MODEL_OK;
}
gh_actor_model_status gh_actor_shadow_draw_0014efa8(gh_shadow_call*c,const gh_shadow_bindings*b,uint32_t*f){
 uint32_t local=0;if(!f)f=&local;*f=0;if(!c||!b||!b->model||!b->model->bound||b->model->native_identity!=b->model||!b->model->root.bytes||b->model->root.size<0x4ac||!b->model->root.alias_f0||!b->work||b->model->work!=b->work||b->policy!=GH_STATIC_BINARY32_REFERENCE)return GH_ACTOR_MODEL_ARGUMENT;
 if(c->active){if(c->identity!=c||c->root_identity!=b->model->root.bytes||c->alias_identity!=b->model->root.alias_f0||!same(&c->bindings,b))return GH_ACTOR_MODEL_BUSY;}else{memset(c,0,sizeof(*c));c->bindings=*b;c->active=1;c->identity=c;c->root_identity=b->model->root.bytes;c->alias_identity=b->model->root.alias_f0;}
 if(c->complete)return GH_ACTOR_MODEL_OK;fenv_t env;if(feholdexcept(&env))return GH_ACTOR_MODEL_UNSUPPORTED;if(fesetround(FE_TONEAREST)){fesetenv(&env);return GH_ACTOR_MODEL_UNSUPPORTED;}gh_actor_model_status e=run(c,f);if(fesetenv(&env))return GH_ACTOR_MODEL_UNSUPPORTED;return e;
}
