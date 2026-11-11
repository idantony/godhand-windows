#include "game_actor_model_trans.h"
#include <string.h>
#include <math.h>
#include <fenv.h>
static uint32_t word(const void*p){uint32_t n;memcpy(&n,p,4);return n;}
static uint16_t half(const void*p){uint16_t n;memcpy(&n,p,2);return n;}
static float value(const void*p){float n;memcpy(&n,p,4);return n;}
static void put(void*p,uint32_t n){memcpy(p,&n,4);}
static float add(float a,float b){volatile float x=a+b;return x;}
static float sub(float a,float b){volatile float x=a-b;return x;}
static float mul(float a,float b){volatile float x=a*b;return x;}
static float divide(float a,float b){volatile float x=a/b;return x;}
static int signed16(uint16_t n){return n<32768?(int)n:(int)n-65536;}
static gh_actor_model_status stop(uint32_t*f,uint32_t pc,gh_actor_model_status e){if(f)*f=pc;return e;}
static void point(float*out,const float*m,const float*p){for(unsigned r=0;r<4;r++){float x=mul(m[r],p[0]);x=add(x,mul(m[4+r],p[1]));x=add(x,mul(m[8+r],p[2]));out[r]=add(x,m[12+r]);}}
static int box(const uint8_t*p,const gh_game_pl_camera*camera,int*out){
 float basis[16],clip[16],combined[16],center[3],world[4],extent[3];memcpy(basis,p+0x80,64);memcpy(clip,camera->fields+0x80,64);
 for(unsigned k=0;k<16;k++)if(!isfinite(basis[k])||!isfinite(clip[k]))return 0;
 /* Original copies worldcenterXYZ and resets W1 before composing axes. */
 if(basis[3]!=0||basis[7]!=0||basis[11]!=0||basis[15]!=1)return 0;
 for(unsigned k=0;k<3;k++){center[k]=value(p+0x260+k*4);extent[k]=value(p+0x26c+k*4);if(!isfinite(center[k])||!isfinite(extent[k])||extent[k]<0)return 0;}
 point(world,basis,center);memcpy(basis+12,world,12);basis[15]=1;
 for(unsigned c=0;c<4;c++)for(unsigned r=0;r<4;r++){float x=mul(clip[r],basis[c*4]);x=add(x,mul(clip[4+r],basis[c*4+1]));x=add(x,mul(clip[8+r],basis[c*4+2]));combined[c*4+r]=add(x,mul(clip[12+r],basis[c*4+3]));if(!isfinite(combined[c*4+r]))return 0;}
 unsigned common=63;for(unsigned i=0;i<8;i++){float corner[3],v[4];for(unsigned k=0;k<3;k++)corner[k]=(i&(1u<<k))?extent[k]:-extent[k];point(v,combined,corner);unsigned flags=0;float limit=fabsf(v[3]);if(!isfinite(limit))return 0;for(unsigned k=0;k<3;k++){if(!isfinite(v[k]))return 0;if(v[k]>limit)flags|=1u<<(k*2);if(v[k]<-limit)flags|=2u<<(k*2);}common&=flags;}*out=!common;return 1;
}
gh_actor_model_status gh_actor_model_box_visible_001f8ad8(const gh_actor_model*m,const gh_game_pl_camera*c,int*out,uint32_t*f){
 if(f)*f=0;if(!m||!m->root.bytes||m->root.size<0x278||!c||!c->constructed||!out)return GH_ACTOR_MODEL_ARGUMENT;
 fenv_t env;if(feholdexcept(&env))return GH_ACTOR_MODEL_UNSUPPORTED;if(fesetround(FE_TONEAREST)){fesetenv(&env);return GH_ACTOR_MODEL_UNSUPPORTED;}int valid=box(m->root.bytes,c,out);if(fesetenv(&env))valid=0;return valid?GH_ACTOR_MODEL_OK:stop(f,0x1f8ad8,GH_ACTOR_MODEL_UNSUPPORTED);
}
static int elfword(const gh_actor_trans_bindings*b,uint32_t pc,uint32_t*out){
 const uint8_t*p=b->elf;size_t n=b->elf_bytes;if(!p||n<52||memcmp(p,"\177ELF\1\1\1",7)||half(p+18)!=8)return 0;uint32_t ph=word(p+28);unsigned stride=half(p+42),count=half(p+44),found=0;if(stride<32||count>128||ph>n||(size_t)count*stride>n-ph)return 0;
 for(unsigned i=0;i<count;i++){const uint8_t*q=p+ph+(size_t)i*stride;uint32_t off=word(q+4),va=word(q+8),size=word(q+16);if(word(q)!=1)continue;if(off>n||size>n-off)return 0;if(va<=pc&&(uint64_t)pc+4<=(uint64_t)va+size){if(found++)return 0;*out=word(p+(size_t)off+(size_t)(pc-va));}}return found==1;
}
static gh_actor_model_status method(gh_actor_trans_call*c,unsigned offset,uint32_t*entry,uint32_t*f){uint32_t vt=word(c->model->root.bytes+0x214),delta,first;if(vt>UINT32_MAX-offset-8||!elfword(&c->bindings,vt+offset,&delta)||!elfword(&c->bindings,vt+offset+4,entry)||!elfword(&c->bindings,*entry,&first))return stop(f,0x14a3e8,GH_ACTOR_MODEL_UNSUPPORTED);if((uint16_t)delta)return stop(f,*entry,GH_ACTOR_MODEL_UNSUPPORTED);return GH_ACTOR_MODEL_OK;}
static gh_actor_model_status suppress(gh_actor_trans_call*c,uint32_t entry,int*out,uint32_t*f){
 const uint8_t*p=c->model->root.bytes,*w=(const uint8_t*)c->bindings.work;uint32_t flags=word(w+0x5e0);*out=0;
 if(entry==0x2b8978&&word(p+0x214)==0x44b840u){
  /* cObjSimple effect model's Trans predicate reads only Work+5E0. */
  *out=(flags&0x10000000u)!=0;
  return GH_ACTOR_MODEL_OK;
 }
 if(entry==0x107f20){if(flags&0x80000000u){*out=1;return GH_ACTOR_MODEL_OK;}entry=0x2a7448;}
 else if(entry==0x1004c8){if(flags&0x200000u){*out=1;return GH_ACTOR_MODEL_OK;}entry=0x14b470;}
 else if(entry==0x1b80c0){if(flags&0x20000000u){*out=1;return GH_ACTOR_MODEL_OK;}entry=0x2a7448;}
 if(entry==0x2a7448){if(c->model->root.size<0x5a4)return GH_ACTOR_MODEL_BOUNDS;if(word(p+0x5a0)&3u){*out=1;return GH_ACTOR_MODEL_OK;}entry=0x14b470;}
 if(entry!=0x14b470)return stop(f,entry,GH_ACTOR_MODEL_UNSUPPORTED);
 if((flags&0x800000u)||(word(p+0x250)&2u)){*out=1;return GH_ACTOR_MODEL_OK;}
 if(word(p+0x250)&0x10000u)return GH_ACTOR_MODEL_OK;
 if(!c->bindings.camera||!c->bindings.camera->constructed)return stop(f,0x1f8ad8,GH_ACTOR_MODEL_UNSUPPORTED);int visible=0;if(!box(p,c->bindings.camera,&visible))return stop(f,0x1f8ad8,GH_ACTOR_MODEL_UNSUPPORTED);*out=!visible;return GH_ACTOR_MODEL_OK;
}
static gh_actor_model_status callee(gh_actor_trans_call*c,uint32_t pc,uint32_t*f){
 c->pending=(gh_actor_trans_request){.entry=pc,.parity=c->parity,.model=c->model,.shadow=c->bindings.shadow,.first_node=c->first_node,.depth=c->depth,.far_mode=(uint8_t)(c->near_limit<c->distance),.camera=c->bindings.camera};
 if(!c->bindings.callee)return stop(f,pc,GH_ACTOR_MODEL_UNSUPPORTED);uint32_t frontier=0;gh_actor_model_status e=c->bindings.callee(c->bindings.context,&c->pending,&frontier);return e?stop(f,frontier?frontier:pc,e):GH_ACTOR_MODEL_OK;
}
static gh_actor_model_status shadow_prefix(gh_actor_trans_call*c,uint32_t*f){
 const uint8_t*p=c->model->root.bytes;const gh_boot_work*w=c->bindings.work;if(word((const uint8_t*)w+0x5e0)&0x400000u)return GH_ACTOR_MODEL_OK;
 if(c->model->root.size<0x4ac)return GH_ACTOR_MODEL_BOUNDS;
 if((word(p+0x4a8)&0x80000000u)!=0||(word(p+0x250)&2u))return GH_ACTOR_MODEL_OK;float alpha=value(p+0x24c);if(!isfinite(alpha))return stop(f,0x14efa8,GH_ACTOR_MODEL_UNSUPPORTED);if(alpha<=.3f)return GH_ACTOR_MODEL_OK;
 /* 44C/450 removedguestwords are not NULL evidence. */
 const gh_player_shadow*s=c->bindings.shadow;if(!s||!s->initialized||!s->object||s->object->bytes!=p||!s->packets[0]||!s->packets[1]||!s->pattern_original)return stop(f,0x14efa8,GH_ACTOR_MODEL_UNSUPPORTED);
 uintptr_t base=(uintptr_t)p,first=(uintptr_t)s->packets[0],second=(uintptr_t)s->packets[1];size_t size=c->model->root.size;
 if(first<base||first-base>size||0x750>size-(first-base)||second<base||second-base>size||0x750>size-(second-base)||second-first!=0x750)return stop(f,0x14efa8,GH_ACTOR_MODEL_BOUNDS);
 return callee(c,0x14efa8,f);
}
static int same(const gh_actor_trans_bindings*a,const gh_actor_trans_bindings*b){return a->work==b->work&&a->camera==b->camera&&a->world_light==b->world_light&&a->depth_shift==b->depth_shift&&a->elf==b->elf&&a->elf_bytes==b->elf_bytes&&a->shadow==b->shadow&&a->callee==b->callee&&a->context==b->context&&a->numeric_policy==b->numeric_policy;}
static gh_actor_model_status run(gh_actor_trans_call*c,uint32_t*f){
 uint8_t*p=c->model->root.bytes;const uint8_t*w=(const uint8_t*)c->bindings.work;gh_actor_model_status e;uint32_t entry=0;
 if(c->phase==0){c->parity=word(w+0x210)&1u;if(word(p+0x2c0)==10){
  if(!c->bindings.camera||!c->bindings.camera->constructed||!c->bindings.depth_shift||!c->model->root.alias_f0)return stop(f,0x31a6b0,GH_ACTOR_MODEL_UNSUPPORTED);
  uintptr_t base=(uintptr_t)p,alias=(uintptr_t)c->model->root.alias_f0;if(alias<base||alias-base>c->model->root.size||12>c->model->root.size-(alias-base))return stop(f,0x31a6b0,GH_ACTOR_MODEL_BOUNDS);
  float matrix[16],position[3],v[4];memcpy(matrix,c->bindings.camera->fields,64);memcpy(position,c->model->root.alias_f0,12);for(unsigned k=0;k<16;k++)if(!isfinite(matrix[k]))return stop(f,0x31a6b0,GH_ACTOR_MODEL_UNSUPPORTED);for(unsigned k=0;k<3;k++)if(!isfinite(position[k]))return stop(f,0x31a6b0,GH_ACTOR_MODEL_UNSUPPORTED);point(v,matrix,position);float z=mul(v[3],10);if(!isfinite(z)||z>=2147483648.0f)return stop(f,0x31a6b0,GH_ACTOR_MODEL_UNSUPPORTED);uint32_t depth=z>=0?(uint32_t)z:0;uint16_t result=(uint16_t)(depth>>(*c->bindings.depth_shift&31u));memcpy(p+0x2ac,&result,2);
 }c->phase=1;}
 if(c->phase==1){e=method(c,0x30,&entry,f);if(e)return e;int skip=0;e=suppress(c,entry,&skip,f);if(e)return e;if(skip){c->complete=1;return GH_ACTOR_MODEL_OK;}c->phase=2;}
 if(c->phase==2){if(word(p+0x254)&0x8000000u){e=callee(c,0x14d828,f);if(e)return e;}c->phase=3;}
 if(c->phase==3){c->distance=signed16(half(p+0x2ac))+(int)(int8_t)p[0x2b5]+50;c->depth=c->distance;e=method(c,0x38,&entry,f);if(e)return e;
  if(entry==0x12bbe0)e=shadow_prefix(c,f);else if(entry==0x14e4c8){uint32_t first,delay;if(!elfword(&c->bindings,entry,&first)||!elfword(&c->bindings,entry+4,&delay)||first!=0x03e00008||delay)return stop(f,entry,GH_ACTOR_MODEL_UNSUPPORTED);}else e=callee(c,entry,f);if(e)return e;c->phase=4;
 }
 if(c->phase==4){put(p+0x250,word(p+0x250)&~0x200000u);size_t mesh=0;if(gh_actor_model_node_at(c->model,0,&c->first_node,&mesh))return stop(f,0x14b6f8,GH_ACTOR_MODEL_UNSUPPORTED);c->first_mesh=mesh;c->phase=5;}
 if(c->phase==5){uint32_t flags=word(c->first_node+0x380);int distance=c->distance,max=signed16(half(p+0x2ae));
  if(!(flags&0x2000u)){
   if(word(p+0x254)&0x80000000u){int begin=max-(int)p[0x2b0];if(begin<distance){if(!p[0x2b0])return stop(f,0x14a3e8,GH_ACTOR_MODEL_UNSUPPORTED);float a=mul(sub(1,divide((float)(distance-begin),(float)p[0x2b0])),255);if(!isfinite(a)||a>=256)return stop(f,0x14a3e8,GH_ACTOR_MODEL_UNSUPPORTED);p[0x2b7]=a>=0?(uint8_t)(int32_t)a:0;}if(max<distance)p[0x2b7]=0;if(distance<=begin)p[0x2b7]=255;}
   else{float target=max<=distance?0:255;float a=add(mul(p[0x2b7],.8f),mul(target,.19999999f));if(!isfinite(a)||a>=2147483648.0f)return stop(f,0x14a3e8,GH_ACTOR_MODEL_UNSUPPORTED);int n=(int32_t)a;if(n<0)n=0;if(n>254)n=255;if(n<5)n=0;else if(n>249)n=255;p[0x2b7]=(uint8_t)n;}
  }
  c->near_limit=half(p+0x2b2);
  if(!(flags&0x2000u)&&(word(p+0x254)&0x40000000u)&&value(p+0x2b8)>0){float fade=value(p+0x2b8),den=fade<2?fade:2,a=add(divide(sub(divide((float)signed16(half(p+0x2ac)),10),fade),den),1);if(!isfinite(a))return stop(f,0x14a3e8,GH_ACTOR_MODEL_UNSUPPORTED);if(a<0)a=0;if(a>=1)a=1;p[0x2bc]=(uint8_t)(int32_t)mul(mul(mul(a,a),a),255);}
  if(!(word(w+0x598)&0x100000u)){if(!(flags&0x2000u)&&distance< -50){c->complete=1;return GH_ACTOR_MODEL_OK;}if(c->depth<1)c->depth=1;if(max<c->depth&&!(word(w+0x5e4)&0x20000000u)){c->depth=max;if(!(flags&0x2000u)&&!p[0x2b7]){c->complete=1;return GH_ACTOR_MODEL_OK;}}if(c->depth>1022)c->depth=1022;if(word(w+0x5e4)&0x20000000u)p[0x2b7]=255;}
  else{p[0x2b7]=255;if(c->depth>1022)c->depth=1022;}c->phase=6;
 }
 if(c->phase==6){gh_model_mesh mesh;if(gh_model_mesh_at(&c->model->script,c->first_mesh,&mesh))return stop(f,0x14b6f8,GH_ACTOR_MODEL_BOUNDS);
  unsigned offset=mesh.data_num<2?0x20:0x28;
  if(mesh.data_num>=2&&!(word(c->first_node+0x380)&0x2000u)){if(!c->bindings.world_light||!c->bindings.world_light->ready)return stop(f,0x604714,GH_ACTOR_MODEL_UNSUPPORTED);float far_mode=value(c->bindings.world_light->scalar+0x14);if(!isfinite(far_mode))return stop(f,0x604714,GH_ACTOR_MODEL_UNSUPPORTED);if(mul(far_mode,10)<(float)signed16(half(p+0x2ac))){c->complete=1;return GH_ACTOR_MODEL_OK;}}
  e=method(c,offset,&entry,f);if(e)return e;c->pending.entry=entry;c->phase=7;
 }
 if(c->phase==7){uint32_t pc=c->pending.entry;e=callee(c,pc,f);if(e)return e;c->phase=8;}
 /* 28C/294 auxiliary packetrefs require a native semantic owner; rawzero may
  * also denote removedguestpointers. Do not silently skip their twoOTnodes. */
 if(c->phase==8){e=callee(c,0x31a600,f);if(e)return e;c->complete=1;c->phase=9;}
 return GH_ACTOR_MODEL_OK;
}
gh_actor_model_status gh_actor_model_trans_0014a3e8(gh_actor_trans_call*c,gh_actor_model*m,const gh_actor_trans_bindings*b,uint32_t*f){
 if(f)*f=0;if(!c||!m||!b||!m->bound||m->native_identity!=m||!m->root.bytes||m->root.size<0x2d0||!b->work||m->work!=b->work||b->numeric_policy!=GH_STATIC_BINARY32_REFERENCE)return GH_ACTOR_MODEL_ARGUMENT;
 if(c->active){if(c->model!=m||!same(&c->bindings,b)||c->root_identity!=m->root.bytes||c->alias_identity!=m->root.alias_f0)return GH_ACTOR_MODEL_BUSY;}
 else{memset(c,0,sizeof(*c));c->model=m;c->bindings=*b;c->root_identity=m->root.bytes;c->alias_identity=m->root.alias_f0;c->active=1;}
 if(c->complete)return GH_ACTOR_MODEL_OK;fenv_t env;if(feholdexcept(&env))return GH_ACTOR_MODEL_UNSUPPORTED;if(fesetround(FE_TONEAREST)){fesetenv(&env);return GH_ACTOR_MODEL_UNSUPPORTED;}gh_actor_model_status e=run(c,f);if(fesetenv(&env))return GH_ACTOR_MODEL_UNSUPPORTED;return e;
}
