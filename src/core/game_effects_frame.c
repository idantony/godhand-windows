#include "game_effects_frame.h"
#include "game_actor_matrix.h"
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
gh_effect_status gh_effects_frame_globals_from_elf(gh_effects_frame_globals*s,const void*raw,size_t n){if(!s||!raw)return GH_EFFECT_ARGUMENT;if(s->initialized)return GH_EFFECT_UNSUPPORTED;const uint8_t*b=raw;uint8_t v[4];if(!mapped(b,n,0x30f280,v))return GH_EFFECT_BOUNDS;if(w(v)!=0xe4401dc0)return GH_EFFECT_UNSUPPORTED;if(!mapped(b,n,0x292f44,v))return GH_EFFECT_BOUNDS;if(w(v)!=0xe46c1dc0)return GH_EFFECT_UNSUPPORTED;gh_effects_frame_globals next={0};uint32_t*fields[]={&next.factor_741dc0,&next.region_mask_741db0,&next.region_active_741db8,&next.emitter_count_3c3bc0};const uint32_t addresses[]={0x741dc0,0x741db0,0x741db8,0x3c3bc0};for(unsigned i=0;i<4;i++){if(!mapped(b,n,addresses[i],v))return GH_EFFECT_BOUNDS;*fields[i]=w(v);}for(unsigned i=0;i<32;i++){if(!mapped(b,n,0x61b540+i*4,v))return GH_EFFECT_BOUNDS;next.gaussian_offsets_61b540[i]=w(v);if(!mapped(b,n,0x61b5c0+i*4,v))return GH_EFFECT_BOUNDS;next.gaussian_weights_61b5c0[i]=w(v);}next.initialized=1;*s=next;return GH_EFFECT_OK;}
gh_effect_status gh_effects_frame_set_speed_00292f38(gh_effects_frame_globals*g,gh_game_init_state*init,uint32_t*ratio,uint32_t bits){if(!g||!g->initialized||!init||!ratio)return GH_EFFECT_ARGUMENT;put(init->enemies+0x548,bits);g->factor_741dc0=bits;*ratio=bits;return GH_EFFECT_OK;}
static int same(const gh_effects_frame_bindings*a,const gh_effects_frame_bindings*b){return a->storage.effects==b->storage.effects&&a->storage.esp==b->storage.esp&&a->storage.emitters==b->storage.emitters&&a->storage.esp_bytes==b->storage.esp_bytes&&a->storage.emitter_bytes==b->storage.emitter_bytes&&a->storage.actor_match==b->storage.actor_match&&a->storage.context==b->storage.context&&a->work==b->work&&a->init==b->init&&a->current_player==b->current_player&&a->globals==b->globals&&a->profile==b->profile&&a->clock==b->clock&&a->clock_context==b->clock_context&&a->atan_coeff==b->atan_coeff&&a->policy==b->policy;}
static gh_effect_status emitter_view(gh_game_effects_frame*s,unsigned i,gh_effect_emitter_view*out){gh_effect_status e=gh_effect_create_emitter_at(s->b.storage.effects,i,out);if(e)return fail(s,0x300628,GH_EFFECT_UNSUPPORTED);if(!out->initialized||out->payload!=s->b.storage.emitters+(size_t)i*0x260||out->payload_size!=0x260||out->record.size!=0x2f0)return fail(s,0x303a30,GH_EFFECT_UNSUPPORTED);if(s->emitter_stage&&out->serial!=s->emitter_serial)return fail(s,0x3029c0,GH_EFFECT_ARGUMENT);return GH_EFFECT_OK;}
/* Actual type0 destructor3028E8(...,2) only changes its vtable; no operator
 * delete and no invented particle cleanup. Active bit is cleared by caller. */
static gh_effect_status destroy_emitter(gh_game_effects_frame*s,unsigned i){gh_effect_emitter_view v;gh_effect_status e=emitter_view(s,i,&v);if(e)return e;if(w(v.payload+0xf0)!=0x44fd58)return fail(s,0x3028e8,GH_EFFECT_UNSUPPORTED);put(v.payload+0xf0,0x44b310);return GH_EFFECT_OK;}
static float number(const void*p){float n;memcpy(&n,p,4);return n;}
static void store_number(void*p,float n){memcpy(p,&n,4);}
/* Original CVT.W.S is currently admitted only for integral, in-range values;
 * fractional EE conversion policy is not guessed for a new timing domain. */
static int exact_integer(float n,int32_t*out){if(!isfinite(n)||n < -2147483648.0f||n>=2147483648.0f)return 0;int32_t i=(int32_t)n;if((float)i!=n)return 0;*out=i;return 1;}
static gh_effect_status parentless_transform(gh_game_effects_frame*s,uint8_t*p,const uint8_t*r){
 if((int8_t)r[0]<0)return fail(s,0x3033a8,GH_EFFECT_UNSUPPORTED);
 if(s->emitter.actor||s->emitter.part||w(p+4)||w(p+8)||w(p+0x10))return fail(s,0x2b61b0,GH_EFFECT_UNSUPPORTED);
 if(!s->b.atan_coeff)return fail(s,0x147c88,GH_EFFECT_ARGUMENT);
 float local[16],world[16],position[3],angles[3],scale[3];uint32_t pc=0;
 memcpy(position,p+0x40,12);memcpy(angles,p+0x30,12);memcpy(scale,p+0x20,12);
 if(!w(p+0x90)){if(!p[0x16]){if(gh_actor_matrix_compose_00147a48(position,angles,scale,p[0x15],local,s->b.policy,&pc))return fail(s,pc?pc:0x147a48,GH_EFFECT_UNSUPPORTED);}else memcpy(local,p+0x50,64);memcpy(world,local,64);}else {memcpy(local,p+0x50,64);memcpy(world,p+0xb0,64);}
 float out[3];if(gh_actor_matrix_extract_euler_00147c88(world,s->b.atan_coeff,out,s->b.policy,&pc))return fail(s,pc?pc:0x147c88,GH_EFFECT_UNSUPPORTED);
 /* 2B6420 commits local/world cache; 2B6230 then2B6288 each invalidate
  * local16 and world90 before writing XYZ, leaving vector W untouched. */
 if(!w(p+0x90)){if(!p[0x16]){memcpy(p+0x50,local,64);p[0x16]=1;}memcpy(p+0xb0,world,64);put(p+0x90,w(p+0x90)|1);}
 p[0x16]=0;put(p+0x90,w(p+0x90)&~1u);memcpy(p+0x30,out,12);memcpy(p+0x40,world+12,12);put(p+0x114,w(p+0x114)|0x800000u);return GH_EFFECT_OK;
}
static gh_effect_status emitter_update(gh_game_effects_frame*s,unsigned i){
 gh_effect_emitter_view current;gh_effect_status e=emitter_view(s,i,&current);if(e)return e;uint8_t*p=current.payload;const uint8_t*r=current.record.bytes;
 if(s->emitter_stage==1){s->emitter=current;s->emitter_serial=current.serial;if(w(p+0xf0)!=0x44fd58)return fail(s,0x3029c0,GH_EFFECT_UNSUPPORTED);if(w(p+0x114)&0x04000000u)return GH_EFFECT_OK;
  uint32_t flags=w(p+0x114);put(p+0x114,(w(p+0x118)&0x40)?flags&~0x01000000u:flags|0x01000000u);if(w(r+4)&0x2000)put(p+0x114,w(p+0x114)&~0x01000000u);
  if(w(p+0x100)||w(p+0x104)||w(p+0x108)||w(p+0x10c))return fail(s,0x3029c0,GH_EFFECT_UNSUPPORTED);
  if(current.actor){if(!current.actor->bytes||current.actor->size<0x254)return fail(s,0x3029c0,GH_EFFECT_BOUNDS);if(((w(r+4)&0x48000)&&current.actor->size>0x2f4&&current.actor->bytes[0x2f4]==2)||(w(current.actor->bytes+0x250)&0x80000000u)){if(w(current.actor->bytes+0x250)&0x80000000u){uint16_t z=0;memcpy(p+0x18a,&z,2);}put(p+0x114,w(p+0x114)|0x40000000u);}}
  s->emitter_stage=2;
 }
 if(s->emitter_stage==2){float time=number(p+0x180);if(!isfinite(time))return fail(s,0x302ab0,GH_EFFECT_UNSUPPORTED);if((float)(int16_t)h(p+0x186)<=time){e=parentless_transform(s,p,r);if(e)return e;}s->emitter_stage=3;}
 if(s->emitter_stage==3){
  if(w(p+0x114)&0x40000000u){if(w(p+8))return fail(s,0x2b5e58,GH_EFFECT_UNSUPPORTED);if(w(p+4))return fail(s,0x2b5cf0,GH_EFFECT_UNSUPPORTED);unsigned off=(i>>5)*4;uint8_t*mask=s->b.storage.emitters+0x26020+off;put(mask,w(mask)|(UINT32_C(0x80000000)>>(i&31)));return GH_EFFECT_OK;}
  if(((!(w(r+4)&0x2000))&&(w(p+0x114)&0x1000000u))||(w((const uint8_t*)s->b.work+0x5e4)&0x20000u))return fail(s,0x2fa350,GH_EFFECT_UNSUPPORTED);
  float a=number(p+0x180),b=number(p+0x228);if(!isfinite(a)||!isfinite(b)||!isfinite(a+1.0f)||!isfinite(b+1.0f))return fail(s,0x302c18,GH_EFFECT_UNSUPPORTED);store_number(p+0x180,a+1.0f);store_number(p+0x228,b+1.0f);s->emitter_stage=4;
 }
 if(s->emitter_stage==4){
  int32_t time,elapsed;if(!exact_integer(number(p+0x180),&time)||!exact_integer(number(p+0x228),&elapsed))return fail(s,0x3035d0,GH_EFFECT_UNSUPPORTED);
  unsigned duration=h(p+0x224);if(duration&&(int16_t)(uint16_t)elapsed>=(int32_t)duration){put(p+0x114,w(p+0x114)|0x40000000u);return GH_EFFECT_OK;}
  s->emitter_spawn_count=0;
  if((int16_t)h(p+0x186)<time){if((int16_t)h(p+0x188)>0&&(int16_t)h(p+0x188)<time){put(p+0x114,w(p+0x114)|0x40000000u);return GH_EFFECT_OK;}
   if(w(r)&0x10000u)return fail(s,0x31d098,GH_EFFECT_UNSUPPORTED);
   if(!(w(p+0x114)&0x800000u)){e=parentless_transform(s,p,r);if(e)return e;}
   if(!r[0x137]&&!r[0xbd]){if((int8_t)r[0xbc]>0)s->emitter_spawn_count=(int8_t)r[0xbc];}
   else {float rate=number(p+0x17c);if(!isfinite(rate)||!isfinite(rate-1.0f))return fail(s,0x303820,GH_EFFECT_UNSUPPORTED);rate-=1.0f;if(rate<=0){rate=(float)r[0xbd];if(r[0x137])return fail(s,0x3038b0,GH_EFFECT_UNSUPPORTED);if((int8_t)r[0xbc]>0)s->emitter_spawn_count=(int8_t)r[0xbc];}store_number(p+0x17c,rate);}
  }s->emitter_stage=5;
 }
 if(s->emitter_stage==5){while(s->emitter_spawn_count>0){if(!s->esp_started){e=gh_esp_create_begin_00300e30(&s->esp_call,i,s->emitter_serial);if(e)return fail(s,0x300e30,e);s->esp_started=1;}gh_esp_create_bindings b={.storage=s->b.storage,.random=s->b.init?&s->b.init->random:NULL,.work=s->b.work,.policy=s->b.policy,.globals=s->b.globals};e=gh_game_esp_create_00300e30(&s->esp_call,&b);if(e)return fail(s,s->esp_call.frontier?s->esp_call.frontier:0x300e30,e);if(!s->esp_call.complete)return fail(s,0x300e30,GH_EFFECT_ARGUMENT);s->esp_started=0;s->emitter_spawn_count--;}}
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
static gh_effect_status esps(gh_game_effects_frame*s){uint8_t*base=s->b.storage.esp;for(;s->esp_index<1024;s->esp_index++){unsigned i=s->esp_index,off=(i>>5)*4;uint32_t mask=UINT32_C(0x80000000)>>(i&31);if(!(w(base+0x100000+off)&mask))continue;if(w(base+0x100100+off)&mask)return fail(s,0x2fb0e8,GH_EFFECT_UNSUPPORTED);if(w(base+0x100080+off)&mask){put(base+0x100100+off,w(base+0x100100+off)|mask);continue;}return fail(s,0x2fb2b0,GH_EFFECT_UNSUPPORTED);}s->b.profile->effect_count_3c3bb8=s->esp_count;return GH_EFFECT_OK;}
static gh_effect_status update_body(gh_game_effects_frame*s,const gh_effects_frame_bindings*b){
 if(!s||!b||!b->work||!b->profile||!b->clock)return GH_EFFECT_ARGUMENT;if(s->bound&&s->identity!=s)return fail(s,0x30f228,GH_EFFECT_ARGUMENT);if(s->bound&&s->stage&&!same(&s->b,b))return fail(s,0x30f228,GH_EFFECT_ARGUMENT);if(!s->stage){s->identity=s;s->b=*b;s->bound=1;}s->frontier=0;
 if(!s->stage){if(!b->profile->effect_stage){b->profile->effect_profile_752d00=b->clock(b->clock_context);b->profile->effect_count_3c3bb8=0;b->profile->effect_stage=1;}s->stage=(w((const uint8_t*)b->work+0x5d8)&0x10000000u)?5:1;}
 if(s->stage!=5){const gh_effect_release_bindings*t=&b->storage;if(!b->globals||!b->globals->initialized||!t->effects||!t->effects->ready||!t->esp||t->esp_bytes<0x100180||!t->emitters||t->emitter_bytes<0x26060)return fail(s,0x300628,GH_EFFECT_ARGUMENT);if(b->policy!=GH_STATIC_BINARY32_REFERENCE)return fail(s,0x300628,GH_EFFECT_UNSUPPORTED);}
 if(s->stage==1){gh_effect_status e=emitters(s);if(e)return e;s->stage=2;}
 if(s->stage==2){uint32_t flags=w((const uint8_t*)b->work+0x5e4);if((!(flags&0x20000000u)&&!(flags&0x40u))||(flags&0x4000000u)){gh_effect_status e=regions(s);if(e)return e;}s->stage=3;}
 if(s->stage==3){for(;s->pool_index<32;s->pool_index++)if(b->storage.effects->pool_words[s->pool_index])return fail(s,0x306e38,GH_EFFECT_UNSUPPORTED);s->stage=4;}
 if(s->stage==4){gh_effect_status e=esps(s);if(e)return e;b->globals->factor_741dc0=0x3f800000;s->stage=5;}
 if(s->stage==5){b->profile->effect_profile_752d00-=b->clock(b->clock_context);b->profile->effect_stage=0;gh_effects_frame_bindings saved=s->b;*s=(gh_game_effects_frame){.b=saved,.identity=s,.bound=1};}
 return GH_EFFECT_OK;
}
gh_effect_status gh_game_effects_frame_update_0030f228(gh_game_effects_frame*s,const gh_effects_frame_bindings*b){fenv_t env;if(feholdexcept(&env))return GH_EFFECT_UNSUPPORTED;if(fesetround(FE_TONEAREST)){(void)fesetenv(&env);return GH_EFFECT_UNSUPPORTED;}gh_effect_status e=update_body(s,b);if(fesetenv(&env))return GH_EFFECT_UNSUPPORTED;return e;}
