/* Actual static type0 virtual Move318B58 and dispatch helpers. All borrowed
 * storage is canonical; numerical policy is finite binary32 nearest reference. */
#include "game_object_step.h"
#include "game_object_live_trans.h"
#include <string.h>
#include <math.h>
#include <fenv.h>
static uint32_t w(const uint8_t*p){uint32_t n;memcpy(&n,p,4);return n;}
static uint16_t h(const uint8_t*p){uint16_t n;memcpy(&n,p,2);return n;}
static float rf(const uint8_t*p){float f;memcpy(&f,p,4);return f;}
static void ww(uint8_t*p,uint32_t n){memcpy(p,&n,4);}
static void wf(uint8_t*p,float n){memcpy(p,&n,4);}
static float add(float a,float b){volatile float v=a+b;return v;}
static float mul(float a,float b){volatile float v=a*b;return v;}
static float divf(float a,float b){volatile float v=a/b;return v;}
static int32_t s32(uint32_t n){return n<UINT32_C(0x80000000)?(int32_t)n:-(int32_t)(UINT32_MAX-n)-1;}
static gh_object_step_status stop(uint32_t*f,uint32_t pc,gh_object_step_status e){if(f)*f=pc;return e;}
static int elf_read(const uint8_t*b,size_t n,uint32_t addr,void*out,size_t count){
 if(!b||n<52||memcmp(b,"\177ELF\1\1\1",7)||h(b+18)!=8)return 0;
 size_t ph=w(b+28);unsigned stride=h(b+42),num=h(b+44),found=0;
 if(stride<32||num>128||ph>n||(size_t)num*stride>n-ph)return 0;
 for(unsigned i=0;i<num;i++){const uint8_t*p=b+ph+(size_t)i*stride;if(w(p)!=1)continue;
  uint32_t off=w(p+4),base=w(p+8),files=w(p+16),memory=w(p+20);
  if(files>memory||off>n||files>n-off||(uint64_t)base+memory>UINT64_C(0x100000000))return 0;
  if(base<=addr&&(uint64_t)addr+count<=(uint64_t)base+memory){
   if(found++)return 0;memset(out,0,count);uint32_t delta=addr-base;
   if(delta<files){size_t take=files-delta;if(take>count)take=count;memcpy(out,b+off+delta,take);}
  }
 }
 return found!=0;
}
gh_object_step_status gh_game_object_step_bind_static(gh_game_object_step*s,const gh_object_static_bindings*b,const void*elf,size_t n){
 if(!s||!s->constructed||!b||!b->meshes||!b->scene_at)return GH_OBJSTEP_ARGUMENT;
 if(s->move_stage||s->trans_stage)return GH_OBJSTEP_ARGUMENT;
 if(!s->globals_loaded){uint32_t count;uint64_t alpha[8];
  if(!elf_read(elf,n,0x5caffc,&count,4)||!elf_read(elf,n,0x3bd718,alpha,sizeof(alpha)))return GH_OBJSTEP_ARGUMENT;
  s->moved_objects_5caffc=count;memcpy(s->alpha_presets,alpha,sizeof(alpha));
  s->depth_shift=0; /* actual31A0E8 raw31A210 SBzero,+B58 */
  s->globals_loaded=1;
 }
 s->static_objects=*b;s->static_bound=1;return GH_OBJSTEP_OK;
}
gh_object_step_status gh_object_static_lookup(gh_game_object_step*s,const gh_room_pools*pools,uint32_t handle,gh_scene_mesh_object**out,size_t*index,uint32_t*f,uint32_t pc){
 if(!s||!s->static_bound||!s->globals_loaded||!s->static_objects.meshes||s->static_objects.meshes->pools!=pools)return stop(f,pc,GH_OBJSTEP_UNSUPPORTED);
 void*raw;if(gh_room_object_lookup(pools,handle,GH_ROOM_OBJECT_SCROLL_MESH,s->static_objects.meshes,&raw)!=GH_POOL_OK)return stop(f,pc,GH_OBJSTEP_UNSUPPORTED);
 gh_scene_meshes*m=s->static_objects.meshes;
 for(size_t i=0;i<m->count;i++){const gh_scene_mesh_object*o;
  if(gh_scene_meshes_object_at(m,i,&o)==GH_MESH_OK&&o==raw){if(!o->constructor_complete)return stop(f,0x3187d8,GH_OBJSTEP_UNSUPPORTED);*out=raw;*index=i;return GH_OBJSTEP_OK;}}
 return stop(f,pc,GH_OBJSTEP_UNSUPPORTED);
}
static int point_valid(const uint8_t*p){for(unsigned i=0;i<3;i++)if(!isfinite(rf(p+4*i)))return 0;return 1;}
static int near_box(const uint8_t*model,const float*point,float padding,int*out){
 float local[3],center[3];for(unsigned i=0;i<3;i++){local[i]=rf(model+0x260+4*i);if(!isfinite(local[i])||!isfinite(point[i]))return 0;}
 for(unsigned row=0;row<3;row++){float v=mul(rf(model+0x80+row*4),local[0]);v=add(v,mul(rf(model+0x90+row*4),local[1]));v=add(v,mul(rf(model+0xa0+row*4),local[2]));center[row]=add(v,rf(model+0xb0+row*4));if(!isfinite(center[row]))return 0;}
 *out=1;for(unsigned i=0;i<3;i++){float radius=add(rf(model+0x26c+4*i),padding);if(!isfinite(radius)||radius<0)return 0;if(point[i]>add(center[i],radius)||point[i]<add(center[i],-radius))*out=0;}return 1;
}
gh_object_step_status gh_object_static_move_one(gh_game_object_step*s,gh_scene_mesh_object*o,const gh_boot_work*work,uint32_t*f){
 uint8_t*p=o->allocation.pointer,*node=o->node;const uint8_t*d=o->descriptor.script.bytes+o->descriptor.mesh.descriptor_offset,*raw=(const uint8_t*)work;
 uint32_t info=w(d+0x34),flags=w(p+0x350);gh_object_step_status result=GH_OBJSTEP_OK;uint32_t frontier=0;
 fenv_t env;if(feholdexcept(&env))return stop(f,0x318b58,GH_OBJSTEP_UNSUPPORTED);if(fesetround(FE_TONEAREST)){fesetenv(&env);return stop(f,0x318b58,GH_OBJSTEP_UNSUPPORTED);}
 if(!s->move_object_stage){
  if(!p[0x32c]){frontier=0x3187d8;goto unsupported;}
  float alpha=rf(p+0x24c);if(!isfinite(alpha)||alpha<0||alpha>1){frontier=0x319d60;goto unsupported;}
  uint64_t test;
  if(info&0x08000000u)test=0x5100d;
  else if(info&0x800000u)test=0x5000d;
  else if(info&0x20000u)test=0x5000d;
  else if(info&0x4000u)test=0x51001;
  else test=0x5100d;
  if((info&0x08000000u)||(info&0x20000u)||!(info&(0x800000u|0x4000u))){int32_t ref=(int32_t)mul(alpha,128)-1;if(ref<1)ref=0;test|=(uint64_t)(uint32_t)ref<<4;}
  o->test=test; /*319D60->155D60 writes bothparityTESTs*/
  if(w(raw+0x5d8)&0x08000000u){p[0x2b6]=255;o->node_program[0]=o->node_program[1]=0x8000000e;o->object_program[0]=o->object_program[1]=14;frontier=0x318b58;goto unsupported;}
  if(flags&0x80000000u)goto done;
  const uint8_t*side=s->init->side_counter;int32_t count=s32(w(side+8)),progress=s32(w(side+12));
  float factor=1;
  if(count>0)factor=add(1,-mul(divf((float)d[0x3d],255),divf((float)progress,(float)count)));
  else if(side[4])factor=add(1,-divf((float)d[0x3d],255));
  for(unsigned i=0;i<3;i++)wf(p+0x240+4*i,mul(rf(p+0x310+4*i),factor));
  if(count>0){float a=rf(p+0x31c);if(info&0x100u)a=mul(a,divf((float)progress,(float)count));wf(p+0x24c,a);}
  else if(side[4])ww(p+0x24c,w(p+0x31c));
  /*319970 actualR100 has six nonzero linear UV rates, no sinusoidal or
   * texture-matrix variant; preserve precise frontier for those other paths.*/
  float dt=mul(rf(raw+0x574),rf((uint8_t*)&s->ratio10_bits));
  int active_uv=rf(p+0x324)!=0||rf(p+0x328)!=0;
  for(unsigned axis=0;axis<2;axis++){float speed=rf(p+0x324+axis*4);if(!isfinite(speed)||!isfinite(dt)){frontier=0x319970;goto unsupported;}
   if(active_uv){if(info&(0x10000u|(axis?0x400u:0x800u))){frontier=0x319970;goto unsupported;}
    float value=add(rf(node+0x38c+axis*4),mul(speed,dt));if(!isfinite(value)){frontier=0x319970;goto unsupported;}
    if(speed<0){if(value< -1)value=add(value,1);}else if(value>1)value=add(value,-1);wf(node+0x38c+axis*4,value);
   }
  }
  s->moved_objects_5caffc++;p[0x2b6]=255;s->move_object_stage=1;
 }
 if(info&0x80000u){frontier=0x2da468;goto unsupported;}
 if(flags&0x10000000u){frontier=0x150128;goto unsupported;}
 if(d[0x4a]&&d[0x4b]){frontier=0x1f7fb8;goto unsupported;}
 unsigned blend=(w(d+0x3c)>>4)&15;if(blend){if(blend>=8){frontier=0x148f28;goto unsupported;}o->alpha=s->alpha_presets[blend];}
 p[0x2be]=14;
 const gh_object_static_bindings*b=&s->static_objects;
 if(!b->camera||!b->camera->constructed||!b->player||!b->player->bytes||!b->player->alias_f0||b->player->size<0xc0){frontier=0x2db6b0;goto unsupported;}
 uintptr_t start=(uintptr_t)b->player->bytes,point=(uintptr_t)b->player->alias_f0;
 if(point<start||point-start>b->player->size-12||!point_valid(b->camera->fields+0x210)||!point_valid(b->player->alias_f0)){frontier=0x2db6b0;goto unsupported;}
 if(!(flags&0x08000000u)){float camera[3],player[3];for(unsigned i=0;i<3;i++){camera[i]=rf(b->camera->fields+0x210+4*i);player[i]=rf(b->player->alias_f0+4*i);}int near_camera,near_player;
  float radius=(float)h(raw+0x5d4);if(!near_box(p,camera,radius,&near_camera)||!near_box(p,player,radius,&near_player)){frontier=0x1f9070;goto unsupported;}
  if(near_camera||near_player)p[0x2be]=10;
 }
 if((raw[0x58c]&0x80u)||(info&0x40000000u))p[0x2be]=10;
 if(info&0x10000u){frontier=0x148200;goto unsupported;}
 if((info&0x200000u)&&w(raw+0x608)){frontier=0x318b58;goto unsupported;}
 /*319960/319968 are actualjrra/nop, verified from this ELF.*/
 done:s->move_object_stage=0;goto finish;
 unsupported:result=GH_OBJSTEP_UNSUPPORTED;
 finish:if(fesetenv(&env))return stop(f,0x318b58,GH_OBJSTEP_UNSUPPORTED);if(result)return stop(f,frontier,result);return GH_OBJSTEP_OK;
}
static int s16(uint16_t v){return v<32768?(int)v:(int)v-65536;}
static int s8(uint8_t v){return v<128?(int)v:(int)v-256;}
static void point4(float*out,const float*m,const float*p){for(unsigned r=0;r<4;r++){float v=mul(m[r],p[0]);v=add(v,mul(m[4+r],p[1]));v=add(v,mul(m[8+r],p[2]));out[r]=add(v,m[12+r]);}}
/* Actual1F8AD8's two 24-bit CLIP histories test the same plane for all8
 * corners. Restricted finite native-reference math, -absW..absW on XYZ. */
static int visible_box(const uint8_t*model,const gh_game_pl_camera*camera,int*out){
 float basis[16],view[16],combined[16],center[3],world[4],extent[3];
 memcpy(basis,model+0x80,64);memcpy(view,camera->fields+0x80,64);
 for(unsigned i=0;i<3;i++){center[i]=rf(model+0x260+4*i);extent[i]=rf(model+0x26c+4*i);if(!isfinite(center[i])||!isfinite(extent[i])||extent[i]<0)return 0;}
 point4(world,basis,center);memcpy(basis+12,world,16);
 for(unsigned c=0;c<4;c++)for(unsigned r=0;r<4;r++){float v=mul(view[r],basis[c*4]);v=add(v,mul(view[4+r],basis[c*4+1]));v=add(v,mul(view[8+r],basis[c*4+2]));combined[c*4+r]=add(v,mul(view[12+r],basis[c*4+3]));if(!isfinite(combined[c*4+r]))return 0;}
 unsigned common=63;
 for(unsigned corner=0;corner<8;corner++){float p[3],clip[4];for(unsigned i=0;i<3;i++)p[i]=(corner&(1u<<i))?extent[i]:-extent[i];point4(clip,combined,p);
  unsigned flags=0;float limit=fabsf(clip[3]);if(!isfinite(limit))return 0;for(unsigned i=0;i<3;i++){if(!isfinite(clip[i]))return 0;if(clip[i]>limit)flags|=1u<<(i*2);if(clip[i]<-limit)flags|=2u<<(i*2);}common&=flags;
 }
 *out=!common;return 1;
}
static gh_object_step_status trans_prelude(gh_game_object_step*s,gh_scene_mesh_object*o,const gh_boot_work*work,int*draw,uint32_t*f){
 *draw=0;uint8_t*p=o->allocation.pointer;const uint8_t*raw=(const uint8_t*)work,*node=o->node,*d=o->descriptor.script.bytes+o->descriptor.mesh.descriptor_offset;
 const gh_object_static_bindings*b=&s->static_objects;uint32_t frontier=0;gh_object_step_status result=GH_OBJSTEP_OK;
 if(!b->camera||!b->camera->constructed)return stop(f,0x31a6b0,GH_OBJSTEP_UNSUPPORTED);
 fenv_t saved;if(feholdexcept(&saved))return stop(f,0x14a3e8,GH_OBJSTEP_UNSUPPORTED);if(fesetround(FE_TONEAREST)){fesetenv(&saved);return stop(f,0x14a3e8,GH_OBJSTEP_UNSUPPORTED);}
 if(w(p+0x2c0)!=10){frontier=0x14a3e8;goto unsupported;}
 float camera[16],pos[3],value[4];memcpy(camera,b->camera->fields,64);for(unsigned i=0;i<3;i++)pos[i]=rf(p+0xb0+i*4);point4(value,camera,pos);
 float depthf=mul(value[3],10);if(!isfinite(depthf)||depthf>=2147483648.0f){frontier=0x31a6b0;goto unsupported;}
 uint32_t z=depthf>=0?(uint32_t)(int32_t)depthf:0;z>>=s->depth_shift&31u;uint16_t short_depth=(uint16_t)z;memcpy(p+0x2ac,&short_depth,2);
 /*319848 returns true to suppress rendering.*/
 if(w(raw+0x5e0)&0x08000000u)goto done;
 int skip=0;
 if(!(raw[0x58d]&0x80u)){
  const uint8_t*side=s->init->side_counter;
  if(s32(w(side+8))<=0){if((!side[4]&&(d[0x35]&1u))||(side[4]&&(w(d+0x34)&0x200u)))skip=1;}
  if((!p[0x32d]&&side[5]==1)||(p[0x32d]&&side[5]==0)||(w(p+0x250)&2u))skip=1;
  if(!skip&&!(w(p+0x250)&0x10000u)){int visible;if(!visible_box(p,b->camera,&visible)){frontier=0x1f8ad8;goto unsupported;}skip=!visible||(w(raw+0x58c)&0x04000000u);}
 }
 if(skip)goto done;
 if(w(p+0x254)&0x08000000u){frontier=0x14d828;goto unsupported;}
 ww(p+0x250,w(p+0x250)&~0x200000u); /*14E4C8 preceding this isactualJRRA*/
 if(p[0x2b1]!=1){frontier=0x14b6f8;goto unsupported;}
 int distance=s16(h(p+0x2ac))+s8(p[0x2b5])+50,depth=distance;
 if(!(w(node+0x380)&0x2000u)){
  if(w(p+0x254)&0x80000000u){int near_start=s16(h(p+0x2ae))-(int)p[0x2b0];
   if(near_start<distance){if(!p[0x2b0]){frontier=0x14a3e8;goto unsupported;}float a=mul(add(1,-divf((float)(distance-near_start),(float)p[0x2b0])),255);if(!isfinite(a)||a>255){frontier=0x14a3e8;goto unsupported;}p[0x2b7]=a>=0?(uint8_t)(int32_t)a:0;}
   if(s16(h(p+0x2ae))<distance)p[0x2b7]=0;if(distance<=near_start)p[0x2b7]=255;
  }else{float target=s16(h(p+0x2ae))<=distance?0:255;float mixed=add(mul((float)p[0x2b7],.8f),mul(target,.19999999f));int32_t a=(int32_t)mixed;if(a<0)a=0;if(a>254)a=255;if(a<5)a=0;else if(a>249)a=255;p[0x2b7]=(uint8_t)a;}
 }
 /* Constructor254 bit40000000 uses finite distance fade at2B8.*/
 if(!(w(node+0x380)&0x2000u)&&(w(p+0x254)&0x40000000u)&&rf(p+0x2b8)>0){float fade=rf(p+0x2b8),den=fade<2?fade:2;float a=add(divf(add(divf((float)s16(h(p+0x2ac)),10),-fade),den),1);if(a<0)a=0;if(a>=1)a=1;float cube=mul(mul(mul(a,a),a),255);if(!isfinite(cube)){frontier=0x14a3e8;goto unsupported;}p[0x2bc]=(uint8_t)(int32_t)cube;}
 if(!(w(raw+0x598)&0x100000u)){
  if(!(w(node+0x380)&0x2000u)&&distance< -50)goto done;
  if(depth<1)depth=1;int max=s16(h(p+0x2ae));
  if(max<depth&&!(w(raw+0x5e4)&0x20000000u)){depth=max;if(!(w(node+0x380)&0x2000u)&&!p[0x2b7])goto done;}
  if(depth>1022)depth=1022;if(w(raw+0x5e4)&0x20000000u)p[0x2b7]=255;
 }else{p[0x2b7]=255;if(depth>1022)depth=1022;}
 if(o->descriptor.mesh.data_num!=1){frontier=0x14ac90;goto unsupported;}
 memset(&s->pending,0,sizeof(s->pending));s->pending.object=o;s->pending.handle=o->handle;
 s->pending.depth=(w(node+0x380)&0x4080u)?(uint16_t)depth:13;s->pending.bank=(w(node+0x380)&0x4080u)?2:1;
 s->pending.test=o->test;s->pending.alpha=o->alpha;s->pending.tex1=o->tex1;s->pending.clamp=o->clamp;
 unsigned selected=p[0x2be];
 if(selected==254){selected=(w(node+0x380)&0x1000000u)?14:10;if(w(p+0x250)&0x2000u)selected=20;
  if((w(node+0x380)&0x20000000u)&&rf(p+0x24c)==1&&p[0x2b6]==255&&p[0x2b7]==255&&p[0x2bc]==255)selected=30;
  if((w(d+0x34)&0x200000u)&&w(raw+0x608))selected=34;
 }
 if(selected>35){frontier=0x14bef8;goto unsupported;}s->pending.selected_program=selected;*draw=1;
 done:goto finish;
 unsupported:result=GH_OBJSTEP_UNSUPPORTED;
 finish:if(fesetenv(&saved))return stop(f,0x14a3e8,GH_OBJSTEP_UNSUPPORTED);return result?stop(f,frontier,result):GH_OBJSTEP_OK;
}
/*312DB8 is a linear identity find. Registry vectors are public mutable host
 * owners and their binding sidecar has no membership mask: removing these
 * two lists does not unregister/free the object still in globalvector0. */
static gh_object_step_status detach_static_before_destructor(gh_game_object_step*s,gh_scene_mesh_object*o,uint32_t*f){
 gh_room_pools*p=s->static_objects.meshes->pools;if(h((uint8_t*)o->allocation.pointer+0x2fe))return stop(f,0x312db8,GH_OBJSTEP_UNSUPPORTED);
 size_t found[2];const unsigned indices[]={1,2};
 for(unsigned k=0;k<2;k++){gh_room_vector*v=&p->vectors[indices[k]];if(v->count>v->capacity||(v->count&&!v->allocation.pointer))return stop(f,0x312db8,GH_OBJSTEP_POOLS);
  found[k]=v->count;unsigned matches=0;for(size_t i=0;i<v->count;i++)if(w((uint8_t*)v->allocation.pointer+i*4)==o->handle){found[k]=i;matches++;}
  if(matches>1||(!k&&!matches))return stop(f,0x312db8,GH_OBJSTEP_POOLS);
 }
 for(unsigned k=0;k<2;k++){gh_room_vector*v=&p->vectors[indices[k]];if(found[k]<v->count){uint8_t*b=v->allocation.pointer;memmove(b+found[k]*4,b+(found[k]+1)*4,((size_t)v->count-found[k]-1)*4);v->count--;}}
 return GH_OBJSTEP_OK;
}
gh_object_step_status gh_game_objects_trans_static_0030f650(gh_game_object_step*s,const gh_room_pools*pools,const gh_boot_work*work,uint32_t*f){
 if(f)*f=0;if(!s||!s->constructed||!s->init||!pools||!work)return GH_OBJSTEP_ARGUMENT;
 const gh_room_vector*list;if(gh_room_registry_view(pools,0x754c30,&list)!=GH_POOL_OK)return stop(f,0x30f650,GH_OBJSTEP_POOLS);
 if(!s->trans_stage){s->trans_index=0;s->trans_phase=0;s->trans_stage=1;s->trans_pools=pools;s->trans_work=work;}
 else if(s->trans_pools!=pools||s->trans_work!=work)return GH_OBJSTEP_ARGUMENT;
 for(;s->trans_index<list->count;){
  uint32_t handle=w((uint8_t*)list->allocation.pointer+s->trans_index*4);gh_scene_mesh_object*o;size_t index;
  gh_object_step_status e;
  if((s->trans_phase&&s->pending.handle!=handle)||(s->trans_live_phase&&s->trans_live_handle!=handle))return stop(f,0x30f69c,GH_OBJSTEP_POOLS);
  if(gh_object_live_trans_handle(pools,handle)){if(s->trans_phase)return stop(f,0x30f69c,GH_OBJSTEP_POOLS);int erased=0;e=gh_object_live_trans_one_0030f650(s,pools,work,handle,&erased,f);if(e)return e;if(!erased)s->trans_index++;continue;}
  if(s->trans_live_phase)return stop(f,0x30f69c,GH_OBJSTEP_POOLS);
  e=gh_object_static_lookup(s,pools,handle,&o,&index,f,0x30f69c);if(e)return e;
  uint8_t*p=o->allocation.pointer;const gh_object_static_bindings*b=&s->static_objects;
  if(s->trans_phase==5)return stop(f,0x31a018,GH_OBJSTEP_UNSUPPORTED);
  if(!s->trans_phase){uint32_t flags=w(p+0x250);
   if(flags&0x80000000u){if(flags&0x20000000u){e=detach_static_before_destructor(s,o,f);if(e)return e;s->pending.object=o;s->pending.handle=handle;s->trans_phase=5;return stop(f,0x31a018,GH_OBJSTEP_UNSUPPORTED);}ww(p+0x250,flags|((flags&0x40000000u)?0x20000000u:0x40000000u));s->trans_index++;continue;}
   if(flags&0x8000u){s->trans_index++;continue;}
   ww(p+0x250,flags|0x200000u);
   if(flags&0x80000u)return stop(f,0x14a910,GH_OBJSTEP_UNSUPPORTED);
   if(!b->scene_at||b->scene_at->field68)return stop(f,0x2c3190,GH_OBJSTEP_UNSUPPORTED);
   int draw;e=trans_prelude(s,o,work,&draw,f);if(e)return e;
   if(!draw){s->trans_index++;continue;}s->pending.object_index=index;s->trans_phase=1;
  }
  if(s->trans_phase==1){
   if(!b->camera||!b->projection)return stop(f,0x14d0e8,GH_OBJSTEP_UNSUPPORTED);
   gh_static_model_constants model;uint32_t frontier=0;gh_static_mesh_status r=gh_static_model_constants_0014d0e8(o,b->camera,b->projection,GH_STATIC_BINARY32_REFERENCE,&model,&frontier);
   if(r)return stop(f,frontier?frontier:0x14d0e8,GH_OBJSTEP_UNSUPPORTED);
   memcpy(s->pending.packet.inputs.screen_matrix,model.screen,64);memcpy(s->pending.packet.inputs.clip_matrix,model.clip,64);memcpy(s->pending.packet.projection,model.projection,64);
   if(w((uint8_t*)o->node+0x380)&1u){o->object_program[w((const uint8_t*)work+0x210)&1u]=0x80000000u;s->pending.kind=GH_OBJECT_SETUP;s->trans_phase=4;continue;}
   if(!b->world_light)return stop(f,0x153b00,GH_OBJSTEP_UNSUPPORTED);
   r=gh_static_node_constants_00153b00(o,work,b->world_light,&model,GH_STATIC_BINARY32_REFERENCE,&s->pending.packet,&frontier);
   if(r)return stop(f,frontier?frontier:0x153b00,GH_OBJSTEP_UNSUPPORTED);
   if(s->pending.packet.fog_written)o->fog_rgb=s->pending.packet.fog_rgb;
   o->object_program[w((const uint8_t*)work+0x210)&1u]=0x80000000u;
   s->trans_material=0;s->pending.material_count=o->descriptor.submesh_count;s->trans_phase=2;
  }
  if(s->trans_phase==2){
   if(!b->textures)return stop(f,0x148d30,GH_OBJSTEP_UNSUPPORTED);
   for(;s->trans_material<o->descriptor.submesh_count;s->trans_material++){
    const gh_stage_material*material;gh_scene_mesh_status r=gh_scene_meshes_material_00148d30(b->meshes,index,(unsigned)s->trans_material,b->textures,&material);
    if(r)return stop(f,b->meshes->frontier?b->meshes->frontier:0x148d30,GH_OBJSTEP_UNSUPPORTED);s->pending.materials[s->trans_material]=material;
   }
   unsigned parity=w((const uint8_t*)work+0x210)&1u;o->object_program[parity]=0x80000000u;o->node_program[parity]=s->pending.selected_program;
   s->pending.kind=GH_OBJECT_NODE_GEOMETRY;s->trans_phase=3;
  }
  if(s->trans_phase==3){
   if(!b->emit)return stop(f,0x14b810,GH_OBJSTEP_UNSUPPORTED);
   uint32_t frontier=0;e=b->emit(b->context,&s->pending,&frontier);if(e)return stop(f,frontier?frontier:0x14b810,e);
   s->pending.kind=GH_OBJECT_SETUP;s->trans_phase=4;
  }
  if(s->trans_phase==4){
   if(!b->emit)return stop(f,0x14c6b8,GH_OBJSTEP_UNSUPPORTED);
   uint32_t frontier=0;e=b->emit(b->context,&s->pending,&frontier);if(e)return stop(f,frontier?frontier:0x14c6b8,e);
   s->trans_index++;s->trans_phase=0;
  }
 }
 if(s->trans_phase||s->trans_live_phase)return stop(f,0x30f69c,GH_OBJSTEP_POOLS);
 s->trans_stage=0;return GH_OBJSTEP_OK;
}
