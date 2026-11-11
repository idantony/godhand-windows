#include "game_effects_frame.h"
#include "game_esp14.h"
#include "game_actor_matrix.h"
#include "game_effect_node.h"
#include "game_actor_two_bone.h"
#include <string.h>
#include <math.h>
#include <fenv.h>
static uint32_t w(const void*p){uint32_t n;memcpy(&n,p,4);return n;}
static unsigned h(const uint8_t*p){return p[0]|(unsigned)p[1]<<8;}
static void put(void*p,uint32_t n){memcpy(p,&n,4);}
static gh_effect_status fail(gh_game_effects_frame*s,uint32_t pc,gh_effect_status e){s->frontier=pc;return e;}
static int mapped(const uint8_t*b,size_t n,uint32_t address,uint8_t*out){
 if(n<52||memcmp(b,"\177ELF\1\1\1",7)||h(b+18)!=8)return 0;uint32_t table=w(b+28);unsigned stride=h(b+42),count=h(b+44);if(stride<32||count>128||table>n||count>(n-table)/stride)return 0;
 for(unsigned i=0;i<count;i++){const uint8_t*p=b+table+(size_t)i*stride;uint32_t off=w(p+4),base=w(p+8),files=w(p+16),memory=w(p+20);if(w(p)!=1||address<base||(uint64_t)address+4>(uint64_t)base+memory)continue;if(files>memory||off>n||files>n-off)return 0;memset(out,0,4);uint32_t delta=address-base;for(unsigned j=0;j<4;j++)if((uint64_t)delta+j<files)out[j]=b[off+delta+j];return 1;}return 0;
}
gh_effect_status gh_effects_frame_globals_from_elf(gh_effects_frame_globals*s,const void*raw,size_t n){if(!s||!raw)return GH_EFFECT_ARGUMENT;if(s->initialized)return GH_EFFECT_UNSUPPORTED;const uint8_t*b=raw;uint8_t v[4];if(!mapped(b,n,0x30f280,v))return GH_EFFECT_BOUNDS;if(w(v)!=0xe4401dc0)return GH_EFFECT_UNSUPPORTED;if(!mapped(b,n,0x292f44,v))return GH_EFFECT_BOUNDS;if(w(v)!=0xe46c1dc0)return GH_EFFECT_UNSUPPORTED;const uint32_t exp_addresses[]={0x45ead0,0x45ead4,0x45eae8,0x45eaec,0x45eaf0,0x45eaf4},exp_words[]={0x3f000000u,0xbf000000u,0x3f317180u,0xbf317180u,0x3717f7d1u,0xb717f7d1u};for(unsigned i=0;i<6;i++){if(!mapped(b,n,exp_addresses[i],v))return GH_EFFECT_BOUNDS;if(w(v)!=exp_words[i])return GH_EFFECT_UNSUPPORTED;}gh_effects_frame_globals next={0};uint32_t*fields[]={&next.factor_741dc0,&next.region_mask_741db0,&next.region_active_741db8,&next.emitter_count_3c3bc0};const uint32_t addresses[]={0x741dc0,0x741db0,0x741db8,0x3c3bc0};for(unsigned i=0;i<4;i++){if(!mapped(b,n,addresses[i],v))return GH_EFFECT_BOUNDS;*fields[i]=w(v);}for(unsigned i=0;i<32;i++){if(!mapped(b,n,0x61b540+i*4,v))return GH_EFFECT_BOUNDS;next.gaussian_offsets_61b540[i]=w(v);if(!mapped(b,n,0x61b5c0+i*4,v))return GH_EFFECT_BOUNDS;next.gaussian_weights_61b5c0[i]=w(v);}const uint32_t flare_addresses[]={0x61b660,0x61b664,0x61b668,0x3c3bb4,0x61b674};
 uint32_t *flare_words[]={next.flare_position_61b660,next.flare_position_61b660+1,
    next.flare_position_61b660+2,&next.flare_intensity_3c3bb4,&next.flare_setup_61b674};
 for(unsigned i=0;i<5;i++){
    if(!mapped(b,n,flare_addresses[i],v))return GH_EFFECT_BOUNDS;
    *flare_words[i]=w(v);
 }
 if(!mapped(b,n,0x3c3bb0,v))return GH_EFFECT_BOUNDS;
 next.flare_hold_3c3bb0=v[0];
 if(!mapped(b,n,0x61b670,v))return GH_EFFECT_BOUNDS;
 next.flare_active_61b670=v[0];
 next.flare_depth_61b672=(uint16_t)h(v+2);
 next.initialized=1;*s=next;return GH_EFFECT_OK;}
gh_effect_status gh_effects_frame_set_speed_00292f38(gh_effects_frame_globals*g,gh_game_init_state*init,uint32_t*ratio,uint32_t bits){if(!g||!g->initialized||!init||!ratio)return GH_EFFECT_ARGUMENT;put(init->enemies+0x548,bits);g->factor_741dc0=bits;*ratio=bits;return GH_EFFECT_OK;}
static int same(const gh_effects_frame_bindings*a,const gh_effects_frame_bindings*b){return a->storage.effects==b->storage.effects&&a->storage.esp==b->storage.esp&&a->storage.emitters==b->storage.emitters&&a->storage.esp_bytes==b->storage.esp_bytes&&a->storage.emitter_bytes==b->storage.emitter_bytes&&a->storage.actor_match==b->storage.actor_match&&a->storage.context==b->storage.context&&a->work==b->work&&a->init==b->init&&a->camera==b->camera&&a->display==b->display&&a->current_player==b->current_player&&a->globals==b->globals&&a->profile==b->profile&&a->clock==b->clock&&a->clock_context==b->clock_context&&a->atan_coeff==b->atan_coeff&&a->policy==b->policy&&a->model_effect==b->model_effect&&(a->actors==b->actors||(!a->actors&&b->actors));}
static gh_effect_status emitter_view(gh_game_effects_frame*s,unsigned i,gh_effect_emitter_view*out){gh_effect_status e=gh_effect_create_emitter_at(s->b.storage.effects,i,out);if(e)return fail(s,0x300628,GH_EFFECT_UNSUPPORTED);if(!out->initialized||out->payload!=s->b.storage.emitters+(size_t)i*0x260||out->payload_size!=0x260||out->record.size!=0x2f0)return fail(s,0x303a30,GH_EFFECT_UNSUPPORTED);if(s->emitter_stage&&out->serial!=s->emitter_serial)return fail(s,0x3029c0,GH_EFFECT_ARGUMENT);return GH_EFFECT_OK;}
/* Actual type0 destructor3028E8(...,2) only changes its vtable; no operator
 * delete and no invented particle cleanup. Active bit is cleared by caller. */
static gh_effect_status destroy_emitter(gh_game_effects_frame*s,unsigned i){gh_effect_emitter_view v;gh_effect_status e=emitter_view(s,i,&v);if(e)return e;if(v.first_esp_child!=-1)return fail(s,0x3028e8,GH_EFFECT_UNSUPPORTED);if(w(v.payload+0xf0)!=0x44fd58)return fail(s,0x3028e8,GH_EFFECT_UNSUPPORTED);put(v.payload+0xf0,0x44b310);return GH_EFFECT_OK;}
static float number(const void*p){float n;memcpy(&n,p,4);return n;}
static void store_number(void*p,float n){memcpy(p,&n,4);}
static int clock_integer(float value,int32_t *result)
{
 if(!isfinite(value)||value < -2147483648.0f||value>=2147483648.0f)return 0;
 *result=(int32_t)value;
 return 1;
}
static int scaled_emitter_clock(const gh_game_effects_frame *s,const uint8_t *p,const uint8_t *r)
{
 return (!(w(r+4)&0x2000u)&&(w(p+0x114)&0x01000000u))||
        (w((const uint8_t*)s->b.work+0x5e4)&0x20000u);
}
/* 2FA350 and 2FA3E0 round the existing clock up in centiframe units, then
 * add or subtract the chopped product of the live effect and Work rates. */
static int emitter_clock(const gh_game_effects_frame *s,float old,int direction,int scaled,float *result)
{
 if(!scaled){
  *result=old+(float)direction;
  return isfinite(old)&&isfinite(*result);
 }
 const volatile float hundred=old*100.0f;
 const volatile float speed=number(&s->b.globals->factor_741dc0)*number((const uint8_t*)s->b.work+0x574);
 const volatile float scaled_speed=speed*100.0f;
 int32_t whole,delta;
 if(!clock_integer(hundred,&whole)||!clock_integer(scaled_speed,&delta))return 0;
 uint32_t bits=(uint32_t)whole+(uint32_t)(hundred!=(float)whole);
 bits+=direction>0?(uint32_t)delta:0u-(uint32_t)delta;
 int32_t signed_clock;
 memcpy(&signed_clock,&bits,sizeof(signed_clock));
 const volatile float frame=(float)signed_clock/100.0f;
 *result=frame;
 return 1;
}
static gh_effect_status emitter_transform(gh_game_effects_frame*s,uint8_t*p,const uint8_t*r){
 if((int8_t)r[0]<0)return fail(s,0x3033a8,GH_EFFECT_UNSUPPORTED);
 if(gh_esp14_nested_sprite_is_bound(s->emitter.record,s->emitter.actor,s->emitter.selector)){
  uint32_t f=0;
  gh_effect_status status=gh_esp14_nested_emitter_pose_00302bb4(s->b.storage.effects,
     s->emitter_index,&s->emitter,s->b.atan_coeff,s->b.policy,&f);
  return status?fail(s,f?f:0x302bb4,status):GH_EFFECT_OK;
 }
 if(!s->b.atan_coeff)return fail(s,0x147c88,GH_EFFECT_ARGUMENT);
 float world[16],angles[3];
 uint32_t pc=0;
 gh_effect_status status;
 const gh_effect_emitter_view *v=&s->emitter;
 if(gh_effect_event_part_is_bound(v->record,v->actor,v->selector)||gh_effect_stamp19_death_part_is_bound(v)||
    gh_effect_knockdown_emitter_is_bound(v)||gh_effect_em00_attack_emitter_is_bound(v)||gh_effect_stamp19_attack_part_is_bound(v)){
  if(!v->model||!v->part||v->model->root.bytes!=v->actor->bytes||!v->part->bytes||
     w(p+4)||w(p+8)||w(p+0x10))return fail(s,0x302b58,GH_EFFECT_UNSUPPORTED);
  /* 303A30 binds the selected part to +104 and the root actor to +108.
   * 302B58 takes root orientation and the selected part's translation. */
  if(w(r+4)&0x400000u){
   memcpy(world,v->actor->bytes+0x80,sizeof(world));
   memcpy(world+12,v->part->bytes+0xb0,12);
   world[15]=1;
  }else{
   /* 302BB4's ordinary positive-part branch copies that part's full basis. */
   memcpy(world,v->part->bytes+0x80,sizeof(world));
  }
  float offset[16],rotation[3],translation[3];
  static const float unit[3]={1,1,1};
  memcpy(rotation,p+0x1b0,12);
  memcpy(translation,p+0x1c0,12);
  if(gh_actor_matrix_compose_00147a48(translation,rotation,unit,0,offset,s->b.policy,&pc)||
     gh_actor_matrix_product_00147548(world,offset,world,s->b.policy,&pc))
    return fail(s,pc?pc:0x302bc8,GH_EFFECT_UNSUPPORTED);
 }else{
  if(v->actor||v->part||w(p+4)||w(p+8)||w(p+0x10))return fail(s,0x2b61b0,GH_EFFECT_UNSUPPORTED);
  status=gh_effect_node_world_matrix_native(s->b.storage.effects,1,s->emitter_index,
    s->emitter_serial,world,s->b.policy,&pc);
  if(status)return fail(s,pc,status);
 }
 if(gh_actor_matrix_extract_euler_00147c88(world,s->b.atan_coeff,angles,s->b.policy,&pc))
   return fail(s,pc?pc:0x147c88,GH_EFFECT_UNSUPPORTED);
 status=gh_effect_node_set_euler_native(s->b.storage.effects,1,s->emitter_index,s->emitter_serial,angles,&pc);
 if(status)return fail(s,pc,status);
 status=gh_effect_node_set_position_native(s->b.storage.effects,1,s->emitter_index,s->emitter_serial,world+12,&pc);
 if(status)return fail(s,pc,status);
 put(p+0x114,w(p+0x114)|0x800000u);
 return GH_EFFECT_OK;
}
static gh_effect_status emitter_update(gh_game_effects_frame*s,unsigned i){
 gh_effect_emitter_view current;gh_effect_status e=emitter_view(s,i,&current);if(e)return e;uint8_t*p=current.payload;const uint8_t*r=current.record.bytes;
 if(s->emitter_stage==1){s->emitter=current;s->emitter_serial=current.serial;if(w(p+0xf0)!=0x44fd58)return fail(s,0x3029c0,GH_EFFECT_UNSUPPORTED);if(w(p+0x114)&0x04000000u)return GH_EFFECT_OK;
  uint32_t flags=w(p+0x114);put(p+0x114,(w(p+0x118)&0x40)?flags&~0x01000000u:flags|0x01000000u);if(w(r+4)&0x2000)put(p+0x114,w(p+0x114)&~0x01000000u);
  if(w(p+0x100)||w(p+0x104)||w(p+0x108)||w(p+0x10c))return fail(s,0x3029c0,GH_EFFECT_UNSUPPORTED);
  if(current.actor){if(!current.actor->bytes||current.actor->size<0x254)return fail(s,0x3029c0,GH_EFFECT_BOUNDS);if(((w(r+4)&0x48000)&&current.actor->size>0x2f4&&current.actor->bytes[0x2f4]==2)||(w(current.actor->bytes+0x250)&0x80000000u)){if(w(current.actor->bytes+0x250)&0x80000000u){uint16_t z=0;memcpy(p+0x18a,&z,2);}put(p+0x114,w(p+0x114)|0x40000000u);}}
  s->emitter_stage=2;
 }
 if(s->emitter_stage==2){float time=number(p+0x180);if(!isfinite(time))return fail(s,0x302ab0,GH_EFFECT_UNSUPPORTED);if((float)(int16_t)h(p+0x186)<=time){e=emitter_transform(s,p,r);if(e)return e;}s->emitter_stage=3;}
 if(s->emitter_stage==3){
  if(w(p+0x114)&0x40000000u){if(current.first_esp_child>=0){gh_effect_esp_view child;if(gh_effect_create_esp_at(s->b.storage.effects,(unsigned)current.first_esp_child,&child)||child.parent_emitter!=(int32_t)i||child.parent_serial!=current.serial)return fail(s,0x2b5e58,GH_EFFECT_UNSUPPORTED);return GH_EFFECT_OK;}if(!gh_effect_event_part_is_bound(current.record,current.actor,current.selector)&&!gh_effect_stamp19_death_part_is_bound(&current)&&!gh_effect_knockdown_emitter_is_bound(&current)&&!gh_effect_em00_attack_emitter_is_bound(&current)&&
    !gh_effect_stamp19_attack_part_is_bound(&current)&&
  (((current.actor||current.selector)&&!gh_esp14_nested_sprite_is_bound(current.record,current.actor,current.selector))||current.model||current.part))return fail(s,0x2b5cf0,GH_EFFECT_UNSUPPORTED);
  if(w(p+8))return fail(s,0x2b5e58,GH_EFFECT_UNSUPPORTED);if(w(p+4))return fail(s,0x2b5cf0,GH_EFFECT_UNSUPPORTED);unsigned off=(i>>5)*4;uint8_t*mask=s->b.storage.emitters+0x26020+off;put(mask,w(mask)|(UINT32_C(0x80000000)>>(i&31)));return GH_EFFECT_OK;}
  const int scaled=scaled_emitter_clock(s,p,r);
  float time,elapsed;
  if(!emitter_clock(s,number(p+0x180),1,scaled,&time)||
     !emitter_clock(s,number(p+0x228),1,scaled,&elapsed))
   return fail(s,scaled?0x2fa350:0x302c18,GH_EFFECT_UNSUPPORTED);
  store_number(p+0x180,time);
  store_number(p+0x228,elapsed);
  s->emitter_stage=4;
 }
 if(s->emitter_stage==4){
  int32_t time,elapsed;if(!clock_integer(number(p+0x180),&time)||!clock_integer(number(p+0x228),&elapsed))return fail(s,0x3035d0,GH_EFFECT_UNSUPPORTED);
  unsigned duration=h(p+0x224);if(duration&&(int16_t)(uint16_t)elapsed>=(int32_t)duration){put(p+0x114,w(p+0x114)|0x40000000u);return GH_EFFECT_OK;}
  s->emitter_spawn_count=0;
  if((int16_t)h(p+0x186)<time){if((int16_t)h(p+0x188)>0&&(int16_t)h(p+0x188)<time){put(p+0x114,w(p+0x114)|0x40000000u);return GH_EFFECT_OK;}
   if(w(r)&0x10000u)return fail(s,0x31d098,GH_EFFECT_UNSUPPORTED);
   if(!(w(p+0x114)&0x800000u)){e=emitter_transform(s,p,r);if(e)return e;}
   const int scaled=scaled_emitter_clock(s,p,r);
   const int spawn=(int8_t)r[0xbc];
   if(!r[0x137]&&!r[0xbd]){
    if(!scaled){
     if(spawn>0)s->emitter_spawn_count=spawn;
    }else{
     float interval;
     if(!emitter_clock(s,number(p+0x22c),1,1,&interval))
      return fail(s,0x2fa350,GH_EFFECT_UNSUPPORTED);
     store_number(p+0x22c,interval<1?interval:0);
     if(interval>=1&&spawn>0)s->emitter_spawn_count=spawn;
    }
   }else{
    float interval;
    if(!emitter_clock(s,number(p+0x17c),-1,scaled,&interval))
     return fail(s,scaled?0x2fa3e0:0x303820,GH_EFFECT_UNSUPPORTED);
    if(interval<=0){
     interval=(float)r[0xbd];
     const int variation=(int8_t)r[0x137];
     if(variation){
      const int duration=(int16_t)h(p+0x188);
      if(!duration)return fail(s,0x3038b0,GH_EFFECT_UNSUPPORTED);
      const volatile float progress=number(p+0x180)/(float)duration;
      const volatile float adjusted=progress*(float)variation;
      int32_t offset;
      if(!clock_integer(adjusted,&offset))return fail(s,0x3038b0,GH_EFFECT_UNSUPPORTED);
      interval+=(float)offset;
      if(interval<0)interval=0;
     }
     if(spawn>0)s->emitter_spawn_count=spawn;
    }
    store_number(p+0x17c,interval);
   }
  }s->emitter_stage=5;
 }
 if(s->emitter_stage==5){while(s->emitter_spawn_count>0){if(!s->esp_started){e=gh_esp_create_begin_00300e30(&s->esp_call,i,s->emitter_serial);if(e)return fail(s,0x300e30,e);s->esp_started=1;}gh_esp_create_bindings b={.storage=s->b.storage,.random=s->b.init?&s->b.init->random:NULL,.work=s->b.work,.policy=s->b.policy,.globals=s->b.globals,.actors=s->b.actors,.atan_coeff=s->b.atan_coeff,.model_effect=s->b.model_effect,.camera=s->b.camera,
 .active_camera=s->b.init?s->b.init->active_camera:NULL};e=gh_game_esp_create_00300e30(&s->esp_call,&b);if(e)return fail(s,s->esp_call.frontier?s->esp_call.frontier:0x300e30,e);if(!s->esp_call.complete)return fail(s,0x300e30,GH_EFFECT_ARGUMENT);s->esp_started=0;s->emitter_spawn_count--;}}
 return GH_EFFECT_OK;
}
static gh_effect_status emitters(gh_game_effects_frame*s){uint8_t*base=s->b.storage.emitters;uint32_t sys=w((const uint8_t*)s->b.work+0x5e4);for(;s->emitter_index<256;s->emitter_index++){
 unsigned i=s->emitter_index,off=(i>>5)*4;if(s->emitter_stage){gh_effect_status e=emitter_update(s,i);if(e)return e;s->emitter_stage=0;continue;}uint32_t mask=UINT32_C(0x80000000)>>(i&31);if(!(w(base+0x26000+off)&mask))continue;
 if(w(base+0x26040+off)&mask){gh_effect_status e=destroy_emitter(s,i);if(e)return e;put(base+0x26000+off,w(base+0x26000+off)&~mask);continue;}
 if(w(base+0x26020+off)&mask){put(base+0x26040+off,w(base+0x26040+off)|mask);continue;}
 gh_effect_emitter_view v;gh_effect_status e=emitter_view(s,i,&v);if(e)return e;uint8_t*p=v.payload;int allowed=1;
 if(!(w(p+0x114)&0x40000000u)){int actor_active=0;if(v.actor){if(!v.actor->bytes||v.actor->size<0x254)return fail(s,0x300628,GH_EFFECT_BOUNDS);if(w(v.actor->bytes+0x250)&0x8000u)continue;actor_active=1;}if(sys&0x10000u){uint32_t flags=w(p+0x118);allowed=(flags&2)||((flags&0x40)?(sys&0x4000000u)!=0:actor_active);}}
 if(!allowed)continue;
 if(!s->emitter_stage){s->emitter_count++;s->emitter=v;s->emitter_serial=v.serial;s->emitter_stage=1;}
 e=emitter_update(s,i);if(e)return e;s->emitter_stage=0;
 }s->b.globals->emitter_count_3c3bc0=s->emitter_count;return GH_EFFECT_OK;}
static gh_effect_status ambient_step(gh_game_effects_frame*s){
 if(!s->b.init)return fail(s,0x31cfe0,GH_EFFECT_ARGUMENT);
 if(!s->ambient_started){gh_effect_status e=gh_effect_create_ambient_begin_00305e88(&s->ambient_call,s->ambient_index);if(e)return fail(s,0x305e88,e);s->ambient_started=1;}
 gh_effect_create_bindings b={.storage=s->b.storage,.work=s->b.work,.random=&s->b.init->random,.model=NULL};gh_effect_status e=gh_game_effect_create_00306528(&s->ambient_call,&b);if(e)return fail(s,s->ambient_call.frontier?s->ambient_call.frontier:0x306528,e);if(!s->ambient_call.complete)return fail(s,0x306528,GH_EFFECT_ARGUMENT);s->ambient_started=0;s->ambient_pending=0;return GH_EFFECT_OK;
}
static gh_effect_status regions(gh_game_effects_frame*s){gh_game_effects*effects=s->b.storage.effects;gh_effects_frame_globals*g=s->b.globals;gh_effect_view ear=effects->ear;
 if(!s->region_stage){g->region_active_741db8=0;if(!ear.bytes){if(ear.size)return fail(s,0x305c20,GH_EFFECT_BOUNDS);s->region_stage=2;}else{g->region_mask_741db0=0;const gh_title_player_view*p=s->b.current_player;if(!p||!p->bytes||p->size<0x218||!p->alias_f0)return fail(s,0x2db6b0,GH_EFFECT_UNSUPPORTED);if(w(p->bytes+0x214)!=0x41d220)return fail(s,0x305c20,GH_EFFECT_UNSUPPORTED);uintptr_t start=(uintptr_t)p->bytes,alias=(uintptr_t)p->alias_f0;if(alias<start||alias-start>p->size||12>p->size-(alias-start))return fail(s,0x2a9458,GH_EFFECT_ARGUMENT);memcpy(s->region_position,p->alias_f0,12);for(unsigned j=0;j<3;j++)if(!isfinite(s->region_position[j]))return fail(s,0x1f83e8,GH_EFFECT_UNSUPPORTED);volatile float y=s->region_position[1]+.5f;s->region_position[1]=y;if(!isfinite(y))return fail(s,0x1f83e8,GH_EFFECT_UNSUPPORTED);s->region_stage=1;}}
 if(s->region_stage==1){if(ear.size<16||w(ear.bytes)>(ear.size-16)/0x50)return fail(s,0x305c20,GH_EFFECT_BOUNDS);unsigned count=w(ear.bytes);for(;s->region_index<count;s->region_index++){const uint8_t*r=ear.bytes+16+(size_t)s->region_index*0x50;int inside;gh_scene_at_status a=gh_scene_at_region_contains_001f83e8(r+0x20,s->region_position,&inside);if(a)return fail(s,0x1f83e8,GH_EFFECT_UNSUPPORTED);uint32_t flags=w(r+4);if((flags&2)?!inside:inside)g->region_mask_741db0|=UINT32_C(1)<<(r[2]&31);if(!(flags&2)&&inside&&(flags&1))g->region_active_741db8=1;}s->region_stage=2;}
 if(s->region_stage==2){gh_effect_view ssd=effects->ssd;if(!ssd.bytes){if(ssd.size)return fail(s,0x305c20,GH_EFFECT_BOUNDS);s->region_stage=4;return GH_EFFECT_OK;}if(ssd.size<20)return fail(s,0x305c20,GH_EFFECT_BOUNDS);uint64_t offset=(uint64_t)w(ssd.bytes+16)+16;if(offset>ssd.size||16>ssd.size-offset)return fail(s,0x305c20,GH_EFFECT_BOUNDS);s->ssd_offset=(size_t)offset;s->ssd_count=w(ssd.bytes+s->ssd_offset+4);if(s->ssd_count>(ssd.size-s->ssd_offset-16)/4)return fail(s,0x305c20,GH_EFFECT_BOUNDS);s->region_stage=3;}
 if(s->region_stage==3){gh_effect_view ssd=effects->ssd;if(s->ambient_pending==1){gh_effect_status e=ambient_step(s);if(e)return e;s->ssd_index++;}if(!s->b.init)return fail(s,0x291fc0,GH_EFFECT_ARGUMENT);
  for(;s->ssd_index<s->ssd_count;s->ssd_index++){uint64_t pos=(uint64_t)s->ssd_offset+w(ssd.bytes+s->ssd_offset+16+s->ssd_index*4)+16;if(pos>ssd.size||6>ssd.size-pos)return fail(s,0x305c20,GH_EFFECT_BOUNDS);const uint8_t*r=ssd.bytes+(size_t)pos;unsigned flags=h(r+4),group=h(r),region=h(r+2);uint32_t mask=UINT32_C(1)<<(s->ssd_index&31);int enemy=s->b.init->enemies[0x518]!=0;int enable=!(flags&(enemy?2u:1u))&&(effects->cached408&(UINT32_C(1)<<(group&31)))&&(region==0xffffu||((effects->cached40c&(UINT32_C(1)<<(region&31)))&&(g->region_mask_741db0&(UINT32_C(1)<<(region&31)))));
   if(enable){if(!(effects->field414&mask)){effects->field414|=mask;s->ambient_pending=1;s->ambient_index=(uint32_t)s->ssd_index;gh_effect_status e=ambient_step(s);if(e)return e;}}
   else if(s->ambient_pending==2||(effects->field414&mask)){if(!s->ambient_pending){effects->field414&=~mask;s->ambient_pending=2;s->ambient_index=(uint32_t)s->ssd_index;}unsigned slot;uint32_t pc=0;gh_effect_status e=gh_game_effect_release_00306200(&s->b.storage,NULL,(int32_t)(s->ssd_index+0x2200),2,&pc,&slot);if(e)return fail(s,pc?pc:0x306200,e);s->ambient_pending=0;}
  }s->region_stage=4;
 }return GH_EFFECT_OK;
}
gh_effect_status gh_effects_frame_refresh_regions_00305c20(gh_game_effects_frame*s,
 const gh_effects_frame_bindings*b){
 if(!s||!b||!b->storage.effects||!b->storage.effects->ready||!b->globals||!b->globals->initialized||
    !b->work||!b->init)return GH_EFFECT_ARGUMENT;
 if(s->stage||(s->bound&&(s->identity!=s||!same(&s->b,b))))return GH_EFFECT_ARGUMENT;
 if(!s->bound){s->identity=s;s->b=*b;s->bound=1;}
 s->frontier=0;
 fenv_t environment;
 if(feholdexcept(&environment))return fail(s,0x305c20,GH_EFFECT_UNSUPPORTED);
 if(fesetround(FE_TONEAREST)){(void)fesetenv(&environment);return fail(s,0x305c20,GH_EFFECT_UNSUPPORTED);}
 gh_effect_status status=regions(s);
 if(fesetenv(&environment))return fail(s,0x305c20,GH_EFFECT_UNSUPPORTED);
 return status;
}
static int esp_update_class(const gh_effect_esp_view *view)
{
 const uint32_t vtable=w(view->payload+0xf0);
 switch(view->type){
 case 0: return vtable==0x44fad0;
 case 6: return vtable==0x44e6d8;
 case 7: return vtable==0x44e770;
 case 8: return vtable==0x44e820&&gh_effect_r003_esp08_row(view->record);
 case 9: return vtable==0x44e8c8;
 case 0x12: return vtable==0x44eb70;
 case 0x13: return vtable==0x44ec68;
 case 0x14: return vtable==GH_ESP14_VTABLE;
 case 0x18: return vtable==0x44f078;
 case 0x1a: return vtable==0x44f270;
 default: return 0;
 }
}
static gh_effect_status esps(gh_game_effects_frame*s){uint8_t*base=s->b.storage.esp;for(;s->esp_index<1024;s->esp_index++){unsigned i=s->esp_index,off=(i>>5)*4;uint32_t mask=UINT32_C(0x80000000)>>(i&31);if(!(w(base+0x100000+off)&mask))continue;if(w(base+0x100100+off)&mask){gh_effect_esp_view hidden;if(gh_effect_create_esp_at(s->b.storage.effects,i,&hidden))return fail(s,0x2fb0e8,GH_EFFECT_UNSUPPORTED);uint32_t f=0;gh_effect_status status;
 if(hidden.type==0x14){
  const gh_esp14_provider *provider=s->b.model_effect;
  if(!provider||!provider->destroy_hidden)return fail(s,0x2f27e8,GH_EFFECT_UNSUPPORTED);
  status=provider->destroy_hidden(provider->context,&hidden,i,&f);
 }else status=gh_effect_create_esp_destroy_hidden_native(s->b.storage.effects,i,hidden.serial,&f);if(status)return fail(s,f?f:0x2fb0e8,status);put(base+0x100000+off,w(base+0x100000+off)&~mask);continue;}if(w(base+0x100080+off)&mask){put(base+0x100100+off,w(base+0x100100+off)|mask);continue;}gh_effect_esp_view v={0};if(gh_effect_create_esp_at(s->b.storage.effects,i,&v)||v.payload!=base+(size_t)i*0x400||!v.initialized||v.globals!=s->b.globals||!esp_update_class(&v))return fail(s,0x2fb2b0,GH_EFFECT_UNSUPPORTED);if(!s->esp_stage){/* Original2FB0E8 gate precedes VT+24=2FBE88; native pointer
 * tokens are resolved only through SAME creator metadata. */
 if(((v.actor||v.selector)&&!gh_esp14_nested_sprite_is_bound(v.record,v.actor,v.selector)&&!gh_effect_event_part_is_bound(v.record,v.actor,v.selector)&&!gh_effect_em00_attack_esp_is_bound(s->b.storage.effects,&v)&&!gh_effect_knockdown_esp_is_bound(s->b.storage.effects,&v))||w(v.payload+0x114))
    return fail(s,0x2fb0e8,GH_EFFECT_UNSUPPORTED);
 if(v.actor&&(w(v.actor->bytes+0x250)&0x8000u))continue;
 uint32_t work_flags=w((const uint8_t*)s->b.work+0x5e4),config=w(v.payload+0x104);
 if((work_flags&0x10000u)&&!(config&2u)&&!((config&0x40u)?(work_flags&0x4000000u)!=0:v.actor!=NULL))continue;
 s->esp_count++;s->esp_stage=1;}
 /* Original wrapper2FBE88 counts the call but skips virtualUpdate whenfrozen. */
 if(!s->esp_update.identity&&(w(v.payload+0x11c)&0x10000u)){s->esp_stage=0;continue;}gh_esp_update_bindings b={.storage=s->b.storage,.work=s->b.work,.globals=s->b.globals,.atan_coeff=s->b.atan_coeff,.policy=s->b.policy,.camera=s->b.camera,.display=s->b.display,
 .active_camera=s->b.init?s->b.init->active_camera:NULL};gh_effect_status status=gh_game_esp_update_002fbed0(&s->esp_update,&b,i,v.serial);if(status)return fail(s,s->esp_update.frontier,status);if(!s->esp_update.complete)return fail(s,0x2fbed0,GH_EFFECT_ARGUMENT);
 if(v.type==0x14){
  const gh_esp14_provider *provider=s->b.model_effect;
  if(!provider||!provider->move)return fail(s,0x2f2250,GH_EFFECT_UNSUPPORTED);
  uint32_t frontier=0;
  status=provider->move(provider->context,&v,i,!s->esp_update.retired,&frontier);
  if(status)return fail(s,frontier?frontier:0x2f2250,status);
 }
 s->esp_update=(gh_esp_update_call){0};s->esp_stage=0;}s->b.profile->effect_count_3c3bb8=s->esp_count;return GH_EFFECT_OK;}
static gh_effect_status update_body(gh_game_effects_frame*s,const gh_effects_frame_bindings*b){
 if(!s||!b||!b->work||!b->profile||!b->clock)return GH_EFFECT_ARGUMENT;if(s->bound&&s->identity!=s)return fail(s,0x30f228,GH_EFFECT_ARGUMENT);if(s->bound&&s->stage&&!same(&s->b,b))return fail(s,0x30f228,GH_EFFECT_ARGUMENT);if(s->stage&&!s->b.actors&&b->actors)s->b.actors=b->actors;if(!s->stage){s->identity=s;s->b=*b;s->bound=1;}s->frontier=0;
 if(!s->stage){if(!b->profile->effect_stage){b->profile->effect_profile_752d00=b->clock(b->clock_context);b->profile->effect_count_3c3bb8=0;b->profile->effect_stage=1;}s->stage=(w((const uint8_t*)b->work+0x5d8)&0x10000000u)?5:1;}
 if(s->stage!=5){const gh_effect_release_bindings*t=&b->storage;if(!b->globals||!b->globals->initialized||!t->effects||!t->effects->ready||!t->esp||t->esp_bytes<0x100180||!t->emitters||t->emitter_bytes<0x26060)return fail(s,0x300628,GH_EFFECT_ARGUMENT);if(b->policy!=GH_STATIC_BINARY32_REFERENCE)return fail(s,0x300628,GH_EFFECT_UNSUPPORTED);}
 if(s->stage==1){gh_effect_status e=emitters(s);if(e)return e;s->stage=2;}
 if(s->stage==2){uint32_t flags=w((const uint8_t*)b->work+0x5e4);if((!(flags&0x20000000u)&&!(flags&0x40u))||(flags&0x4000000u)){gh_effect_status e=regions(s);if(e)return e;}s->stage=3;}
 if(s->stage==3){for(;s->pool_index<32;s->pool_index++)if(b->storage.effects->pool_words[s->pool_index])return fail(s,0x306e38,GH_EFFECT_UNSUPPORTED);s->stage=4;}
 if(s->stage==4){gh_effect_status e=esps(s);if(e)return e;
  const gh_esp14_provider *provider=s->b.model_effect;
  if(provider&&provider->collect_retired){
   uint32_t frontier=0;
   e=provider->collect_retired(provider->context,&frontier);
   if(e)return fail(s,frontier?frontier:0x30f5a0,e);
  }
  b->globals->factor_741dc0=0x3f800000;s->stage=5;
 }
 if(s->stage==5){b->profile->effect_profile_752d00-=b->clock(b->clock_context);b->profile->effect_stage=0;gh_effects_frame_bindings saved=s->b;*s=(gh_game_effects_frame){.b=saved,.identity=s,.bound=1};}
 return GH_EFFECT_OK;
}
gh_effect_status gh_game_effects_frame_update_0030f228(gh_game_effects_frame*s,const gh_effects_frame_bindings*b){fenv_t env;if(feholdexcept(&env))return GH_EFFECT_UNSUPPORTED;if(fesetround(FE_TONEAREST)){(void)fesetenv(&env);return GH_EFFECT_UNSUPPORTED;}gh_effect_status e=update_body(s,b);if(fesetenv(&env))return GH_EFFECT_UNSUPPORTED;return e;}
