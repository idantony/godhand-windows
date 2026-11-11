#include "game_esp_trail_draw.h"
#include "game_scene_at_geometry.h"
#include "../assets/static_mesh_vu_numeric.h"
#include <fenv.h>
#include <math.h>
#include <string.h>
static uint32_t word(const void*p){uint32_t n;memcpy(&n,p,4);return n;}
static float num(const void*p){float n;memcpy(&n,p,4);return n;}
static float add(float a,float b){volatile float n=a+b;return n;}
static float mul(float a,float b){volatile float n=a*b;return n;}
static float divf(float a,float b){volatile float n=a/b;return n;}
static gh_effect_status reject(uint32_t *f,uint32_t pc,gh_effect_status s){*f=pc;return s;}
/* 3C3560 from SLUS_215.03, verified read-only cubic basis; multiplication
 * and Horner evaluation follow 2D4DC8 and 2D4EF8. */
static void centre(const float *history,unsigned index,unsigned stride,float out[3]){
 if(stride==1){memcpy(out,history+4*index,12);return;}
 static const float basis[16]={
  1,-0x1.d55556p+0f,1,-0x1.555556p-3f, 0,3,-2.5f,.5f,
  0,-1.5f,2,-.5f, 0,0x1.555556p-2f,-.5f,0x1.555556p-3f};
 unsigned base=index/stride;
 float t=divf((float)(index%stride),(float)stride);
 if(base){base--;t=add(t,1);}
 for(unsigned axis=0;axis<3;axis++){
  float coeff[4];
  for(unsigned k=0;k<4;k++){
   float n=mul(basis[k],history[4*base+axis]);
   for(unsigned j=1;j<4;j++)n=add(n,mul(basis[4*j+k],history[4*(base+j)+axis]));
   coeff[k]=n;
  }
  out[axis]=add(mul(add(mul(add(mul(coeff[3],t),coeff[2]),t),coeff[1]),t),coeff[0]);
 }
}
static int norm(float x[3]){
 float s=add(add(mul(x[0],x[0]),mul(x[1],x[1])),mul(x[2],x[2]));
 if(!isfinite(s)||s<0)return 0;
 if(s==0){x[0]=x[1]=x[2]=0;return 1;}
 volatile float r=sqrtf(s);
 float q;
 if(!gh_static_vu_reciprocal(r,&q))return 0;
 for(unsigned k=0;k<3;k++)x[k]=mul(x[k],q);
 return 1;
}
static int xyz(const float view[16],const float p[3],float out[4]){
 return !gh_scene_at_apply_world_001508d8(view,p,out);
}
static gh_effect_status prepare(const gh_effects_trans_bindings*b,unsigned slot,
 const gh_effect_esp_view*v,gh_effect_trans_packet*out,uint32_t*f){
 if(!b||!v||!out||!v->payload)return GH_EFFECT_ARGUMENT;
 const uint8_t *p=v->payload;
 if(v->type!=9||!v->initialized||!gh_effect_esp_trail_is_bound(b->storage.effects,v)||
    v->payload_size!=0x400||word(p+0xf0)!=0x44e8c8)return reject(f,0x2ec398,GH_EFFECT_ARGUMENT);
 if(word(p+0x11c)&0x800000u){*out=(gh_effect_trans_packet){0};return GH_EFFECT_OK;}
 const unsigned stride=word(p+0x2e4);
 const uint32_t pc=stride==1?0x2ec398:0x2edf70,flags=word(p+0x11c),mode=word(p+0x2e0);
 const gh_display_state*d=b->display;
 if(!d||!d->initialized||!b->camera||!b->init||!b->camera->constructed||
    b->camera->identity!=b->init->active_camera||b->camera->policy!=GH_CAMERA_NATIVE_BINARY32_REFERENCE||
    b->cvt_policy!=GH_FRAME_CHOP_REFERENCE||b->policy!=GH_STATIC_BINARY32_REFERENCE)
  return reject(f,0x147528,GH_EFFECT_UNSUPPORTED);
 if(!stride||(mode&~2u)||!v->target120_known_null||word(p+0x120)||word(p+0x114)||
    !(flags&0x1000000u)||(int8_t)p[0x101]>2)
  return reject(f,pc,GH_EFFECT_UNSUPPORTED);
 const float *history=NULL;
 unsigned count=0;
 if(gh_effect_create_esp_trail_view(b->storage.effects,slot,v->serial,&history,&count)||
    count<2||count>26||(stride!=1&&((count-1)/stride?((count-1)/stride+2):3)>=count))
  return reject(f,0x2edea0,GH_EFFECT_UNSUPPORTED);
 float camera[16],eye[3],points[26][3];
 memcpy(camera,b->camera->fields,64);memcpy(eye,b->camera->fields+0x210,12);
 for(unsigned i=0;i<count;i++)centre(history,i,stride,points[i]);
 const float bias=num(p+0x254);
 float head[3],direction[3],projected[4];
 for(unsigned j=0;j<3;j++){head[j]=history[j];direction[j]=add(head[j],-eye[j]);}
 if(!isfinite(bias))return reject(f,pc,GH_EFFECT_UNSUPPORTED);
 if(bias!=0){
  if(!norm(direction))return reject(f,0x101e28,GH_EFFECT_UNSUPPORTED);
  for(unsigned j=0;j<3;j++)head[j]=add(head[j],mul(direction[j],bias));
 }
 if(!xyz(camera,head,projected))return reject(f,0x30a2b0,GH_EFFECT_UNSUPPORTED);
 const float divisor=projected[3];
 float q;
 if(!gh_static_vu_reciprocal(divisor,&q)||divisor>=2147483520.0f)
  return reject(f,0x30a468,GH_EFFECT_UNSUPPORTED);
 const gh_stage_material *material=NULL;
 gh_effect_status status=gh_effect_sprite_material_00151398(b->textures,v->texture,1,&material,f);
 if(status)return status;
 uint32_t colour=0;
 for(unsigned j=0;j<4;j++){
  float c=mul(num(p+0x188+4*j),128);
  if(!isfinite(c))return reject(f,pc,GH_EFFECT_UNSUPPORTED);
  if(c<0)c=0;else if(c>255)c=255;
  if(j==3&&p[0x24c])c=mul(c,divf((float)((int)p[0x24c]-(int)p[0x24d]),p[0x24c]));
  if(c<0||c>255)return reject(f,pc,GH_EFFECT_UNSUPPORTED);
  colour|=(uint32_t)c<<(8*j);
 }
 const float first_width=num(p+0x24),last_width=num(p+0x20);
 float width=first_width,step=divf(add(last_width,-first_width),(float)count);
 if(mode&2)step=add(step,step);
 const float u=(flags&0x10000000u)?-1:1, direction_v=(flags&0x08000000u)?1:-1;
 unsigned kick_history=6;
 gh_effect_trans_vertex prior[2]={0};
 gh_effect_trans_packet packet={0};
 packet.original_callee=pc;packet.slot=slot;packet.serial=v->serial;
 packet.kind=GH_EFFECT_PASS_SPRITE;packet.source=GH_EFFECT_SURFACE_MATERIAL;
 packet.destination=GH_EFFECT_SURFACE_SCENE_COLOR;packet.prim=0x54;
 packet.writes_tex0=1;packet.tex0=material->source_tex0;packet.material=material;
 packet.inherited_sampler=1;packet.writes_clamp=1;
 packet.frame_mask=(word(v->record.bytes+4)&0x1000u)?0:0xff000000u;
 packet.test=(flags&0x1000u)?0x3000d:0x5000d;
 packet.zbuf=UINT64_C(0xa0000f8)|((flags&0x80000u)?0:UINT64_C(1)<<32);
 packet.restores_scene_target=1;packet.restore_zbuf=0xa0000f8;
 static const uint64_t alpha[]={0x8000000044,0x8000000048,0x80000000a1,0x8000000041,0x8000000049,0x8000000068,0x8000000009,0x8000000046,0x80000000a4,0x8000000042,0x8000000006};
 if(p[0x247]>10)return reject(f,0x3002b0,GH_EFFECT_UNSUPPORTED);
 packet.alpha=alpha[p[0x247]];
 unsigned bank=2;
 unsigned depth=divisor>970?1020u:(unsigned)(divisor<1?1:(int)divisor)+50u;
 if(flags&0x2000u){bank=3;depth=flags&0x4000u?1:2;}
 else if(flags&0x4000u){bank=3;depth=5;}
 packet.order=(gh_ui_order_group){.bank=bank,.depth=depth,
  .identity=((uint64_t)v->serial<<32)|((uint64_t)slot<<8)};
 for(unsigned i=0;i<count;i++){
  if(i){width=add(width,(mode&2)&&i>=count/2?-step:step);if((mode&2)&&i>=count/2&&width<0)width=0;}
  const float *point=points[i],*next=points[i?i:1],*prev=points[i?i-1:0];
  float tangent[3],ray[3],normal[3];
  for(unsigned j=0;j<3;j++){tangent[j]=add(next[j],-prev[j]);ray[j]=add(point[j],-eye[j]);}
  for(unsigned j=0;j<3;j++){unsigned k=(j+1)%3,l=(j+2)%3;normal[j]=add(mul(ray[k],tangent[l]),-mul(ray[l],tangent[k]));}
  if(!norm(normal)||!isfinite(width))return reject(f,0x30a370,GH_EFFECT_UNSUPPORTED);
  gh_effect_trans_quad segment={0};
  for(unsigned side=0;side<2;side++){
   float corner[3],clip[4];
   for(unsigned j=0;j<3;j++)corner[j]=add(point[j],mul(side?width:-width,normal[j]));
   if(!xyz(camera,corner,clip))return reject(f,0x30a2b0,GH_EFFECT_UNSUPPORTED);
   int32_t fixed[3];
   for(unsigned j=0;j<3;j++)if(!gh_static_vu_ftoi4(mul(clip[j],q),fixed+j))
    return reject(f,0x30a570,GH_EFFECT_UNSUPPORTED);
   int32_t x=fixed[0]>>4,y=fixed[1]>>4;
   if(fixed[2]<0||x<0x400-d->screen_width/2||x>0xc00+d->screen_width-d->screen_width/2||
      y<d->gs_y_origin-0x400||y>d->gs_y_origin+d->screen_lines+0x400)kick_history|=1;
   const gh_effect_trans_vertex current={.x12_4=fixed[0],.y12_4=fixed[1],.z=(uint32_t)fixed[2]>>4,
    .rgba=colour,.q=1,.s=side?u:0,.t=divf(mul(direction_v,(float)i),(float)(count-1))};
   if(!(kick_history&7u)&&i){
    unsigned k=segment.vertex_count;
    segment.vertices[k]=prior[side?1:0];segment.vertices[k+1]=prior[side?0:1];
    segment.vertices[k+2]=current;segment.vertex_count+=3;
   }
   prior[0]=prior[1];prior[1]=current;
   kick_history=(kick_history&0x7fu)<<1;
  }
  if(segment.vertex_count)packet.quads[packet.quad_count++]=segment;
 }
 *out=packet;
 return GH_EFFECT_OK;
}
gh_effect_status gh_esp09_draw_002ec398(const gh_effects_trans_bindings*b,unsigned slot,
 const gh_effect_esp_view*v,gh_effect_trans_packet*out,uint32_t*f){
 if(!f)return GH_EFFECT_ARGUMENT;
 *f=0;
 fenv_t env;
 if(feholdexcept(&env))return reject(f,0x2ec398,GH_EFFECT_UNSUPPORTED);
 if(fesetround(FE_TONEAREST)){(void)fesetenv(&env);return reject(f,0x2ec398,GH_EFFECT_UNSUPPORTED);}
 gh_effect_status s=prepare(b,slot,v,out,f);
 if(fesetenv(&env))return reject(f,0x2ec398,GH_EFFECT_UNSUPPORTED);
 return s;
}
