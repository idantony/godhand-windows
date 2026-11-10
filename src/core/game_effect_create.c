#include "game_effect_create.h"
#include "game_effects_frame.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <fenv.h>
typedef struct {const gh_title_player_view *actor,*selector;const gh_actor_model *model;uint32_t part_index,part_handle;
 const uint8_t *record;uint32_t serial;uint8_t associated,initialized;int32_t first_esp_child;} emitter_metadata;
typedef struct {gh_effect_view record,texture,secondary_texture;const gh_title_player_view *actor,*selector;uint32_t serial,emitter_serial;unsigned emitter_slot;uint8_t associated,initialized,type;int32_t parent_emitter,previous,next;uint32_t parent_serial;uint8_t target120_known_null;} esp_metadata;
struct gh_effect_native {gh_game_effects *owner;uint8_t *esp,*emitters;uint32_t serial;
 emitter_metadata slots[256];esp_metadata esps[1024];struct gh_effects_frame_globals *frame_globals;gh_game_scene_tables *frame_actors;};
static uint32_t w(const uint8_t*p){uint32_t v;memcpy(&v,p,4);return v;}
static void ww(uint8_t*p,uint32_t v){memcpy(p,&v,4);}
static void half(uint8_t*p,uint16_t v){memcpy(p,&v,2);}
static gh_effect_status fail(gh_effect_create_call*c,uint32_t pc,gh_effect_status e){c->frontier=pc;return e;}
static void vf0(uint8_t*p){memset(p,0,12);ww(p+12,0x3f800000);}
static void identity(uint8_t*p){memset(p,0,64);for(unsigned i=0;i<4;i++)ww(p+20*i,0x3f800000);}
/*300458 memset guarantees the cold parentless2B5C20 path. 2B6420's local
 * scale1/angle0/position0 compose identity before GetWorld* and detach;
 *2B6400 invalidates90 again. No live cNode hierarchy is generalized here. */
static void construct_type0(uint8_t*p){
 ww(p+0xf0,0x44b310);for(unsigned i=0;i<4;i++)ww(p+0x20+4*i,0x3f800000);vf0(p+0x30);vf0(p+0x40);vf0(p+0xa0);identity(p+0x50);identity(p+0xb0);ww(p+0x90,0);
 ww(p+0xf0,0x44fd58);for(unsigned i=0;i<4;i++){ww(p+0x1a0+4*i,0x3f800000);ww(p+0x1d0+4*i,0x3f800000);}vf0(p+0x1b0);vf0(p+0x1c0);ww(p+0x1e0,0x3f800000);p[0x1ec]=p[0x1ef]=0xff;vf0(p+0x200);ww(p+0x22c,0x3f800000);p[0x190]=0;
}
gh_effect_status gh_effect_create_begin(gh_effect_create_call*c,const gh_effect_create_request*r){if(!c||!r||(c->active&&!c->complete))return GH_EFFECT_ARGUMENT;*c=(gh_effect_create_call){.identity=c,.request=*r,.active=1};return GH_EFFECT_OK;}
gh_effect_status gh_effect_create_ambient_begin_00305e88(gh_effect_create_call*c,uint32_t index){if(!c)return GH_EFFECT_ARGUMENT;uint32_t bits=index+0x2200u;int32_t mode;memcpy(&mode,&bits,4);gh_effect_create_request request={.bank=3,.index=index,.actor=NULL,.mode=mode,.selector=-1};gh_effect_status e=gh_effect_create_begin(c,&request);if(!e)c->configuration_305e88=1;return e;}
/* Actual305FB0: locate group in SSD's first relative table, then choose the
 * same ordinal from its second table. Views stay within the owned SSD. */
static gh_effect_status ambient_est(gh_effect_create_call*c){
 gh_effect_view ssd=c->owner->ssd;if(!ssd.bytes){if(ssd.size)return fail(c,0x305fb0,GH_EFFECT_BOUNDS);c->original_return=0;c->complete=1;return GH_EFFECT_OK;}if(ssd.size<24)return fail(c,0x305fb0,GH_EFFECT_BOUNDS);
 uint64_t group=(uint64_t)w(ssd.bytes+16)+16,est=(uint64_t)w(ssd.bytes+20)+16;if(group>ssd.size||16>ssd.size-group||est>ssd.size||16>ssd.size-est)return fail(c,0x305fb0,GH_EFFECT_BOUNDS);uint32_t count=w(ssd.bytes+(size_t)group+4);if(count>(ssd.size-(size_t)group-16)/4||count>(ssd.size-(size_t)est-16)/4)return fail(c,0x305fb0,GH_EFFECT_BOUNDS);
 for(unsigned i=0;i<count;i++){uint64_t row=group+w(ssd.bytes+(size_t)group+16+(size_t)i*4)+16;if(row>ssd.size||2>ssd.size-row)return fail(c,0x305fb0,GH_EFFECT_BOUNDS);if((uint32_t)(ssd.bytes[row]|(unsigned)ssd.bytes[row+1]<<8)!=c->request.index)continue;uint64_t start=est+w(ssd.bytes+(size_t)est+16+(size_t)i*4)+16;if(start>ssd.size||16>ssd.size-start)return fail(c,0x305fb0,GH_EFFECT_BOUNDS);size_t end=ssd.size;for(unsigned j=0;j<count;j++){uint64_t next=est+w(ssd.bytes+(size_t)est+16+(size_t)j*4)+16;if(next>start&&next<end)end=(size_t)next;}if(w(ssd.bytes+(size_t)start)!=0x545345){c->original_return=0;c->complete=1;return GH_EFFECT_OK;}c->est=(gh_effect_view){ssd.bytes+(size_t)start,end-(size_t)start};c->count=w(c->est.bytes+4);if(c->count>(c->est.size-16)/0x2f0)return fail(c,0x3066d0,GH_EFFECT_BOUNDS);c->config_flags=0x40;c->stage=1;return GH_EFFECT_OK;
 }
 c->original_return=0;c->complete=1;return GH_EFFECT_OK;
}
static gh_effect_status storage(gh_effect_create_call*c,const gh_effect_create_bindings*b){
 const gh_effect_release_bindings*s=&b->storage;gh_game_effects*e=s->effects;if(!e||!e->ready||!s->esp||s->esp_bytes<0x100180||!s->emitters||s->emitter_bytes<0x26060)return GH_EFFECT_ARGUMENT;
 if(c->owner&&c->owner!=e)return GH_EFFECT_ARGUMENT;c->owner=e;
 if(e->native&&(e->native->owner!=e||e->native->esp!=s->esp||e->native->emitters!=s->emitters))return GH_EFFECT_ARGUMENT;
 return GH_EFFECT_OK;
}
static gh_effect_status select_est(gh_effect_create_call*c,const gh_effect_create_bindings*b){
 uint32_t flags=w((const uint8_t*)b->work+0x5d8);if((flags&0x10000000u)||(!c->request.index&&c->request.bank!=3)){c->original_return=1;c->complete=1;return GH_EFFECT_OK;}
 if(c->request.bank==3){if(!c->configuration_305e88||c->request.actor||c->request.selector!=-1)return fail(c,0x305fb0,GH_EFFECT_UNSUPPORTED);return ambient_est(c);}
 gh_effect_resource*r=NULL;for(unsigned i=0;i<64;i++)if(c->owner->resources[i].kind==(int32_t)c->request.bank){r=&c->owner->resources[i];break;}
 if(!r||!r->tbl1.bytes||r->tbl1.size<4){c->original_return=0;c->complete=1;return GH_EFFECT_OK;}
 gh_effect_view table=r->tbl1;uint32_t count=w(table.bytes),index=c->request.index;if(count>(table.size-4)/4)return fail(c,0x306030,GH_EFFECT_BOUNDS);if(index>count){c->original_return=0;c->complete=1;return GH_EFFECT_OK;}
 uint32_t off=w(table.bytes+(size_t)index*4);if(!off){c->original_return=0;c->complete=1;return GH_EFFECT_OK;}if(off>table.size||16>table.size-off)return fail(c,0x306030,GH_EFFECT_BOUNDS);
 size_t end=table.size;for(unsigned i=1;i<=count;i++){uint32_t next=w(table.bytes+4*i);if(next>off&&next<end)end=next;}
 if(end<off+16||w(table.bytes+off)!=0x545345){c->original_return=0;c->complete=1;return GH_EFFECT_OK;}
 c->est=(gh_effect_view){table.bytes+off,end-off};c->count=w(c->est.bytes+4);if(c->count>(c->est.size-16)/0x2f0)return fail(c,0x3066d0,GH_EFFECT_BOUNDS);c->config_flags=(w((const uint8_t*)b->work+0x5e4)&0x40)?2:0;c->stage=1;return GH_EFFECT_OK;
}
static gh_effect_status allocate(gh_effect_create_call*c,const gh_effect_create_bindings*b,const uint8_t*r){
 /* Future type variants require their real constructor, before reserving a
  * slot; cannot silently instantiate defaulttype0 for unsupported subclasses. */
 if(r[0x128])return fail(c,r[0x128]==1?0x304d70:r[0x128]==2?0x304db0:r[0x128]==3?0x304df0:0x304d48,GH_EFFECT_UNSUPPORTED);
 gh_game_effects*e=c->owner;if(!e->native){e->native=calloc(1,sizeof(*e->native));if(!e->native)return fail(c,0x300458,GH_EFFECT_UNSUPPORTED);e->native->owner=e;e->native->esp=b->storage.esp;e->native->emitters=b->storage.emitters;}
 if(e->native->serial==UINT32_MAX)return fail(c,0x300458,GH_EFFECT_UNSUPPORTED);
 uint8_t*base=b->storage.emitters;unsigned slot=0;for(;slot<256;slot++)if(!(w(base+0x26000+(slot>>5)*4)&(0x80000000u>>(slot&31))))break;
 if(slot==256){c->original_return=0;c->complete=1;return GH_EFFECT_OK;}if(e->native->slots[slot].associated&&e->native->slots[slot].first_esp_child!=-1)return fail(c,0x300458,GH_EFFECT_UNSUPPORTED);uint32_t mask=0x80000000u>>(slot&31),off=(slot>>5)*4;
 ww(base+0x26000+off,w(base+0x26000+off)|mask);ww(base+0x26020+off,w(base+0x26020+off)&~mask);ww(base+0x26040+off,w(base+0x26040+off)&~mask);
 uint8_t*p=base+(size_t)slot*0x260;memset(p,0,0x260);construct_type0(p);c->slot=slot;c->serial=++e->native->serial;
 e->native->slots[slot]=(emitter_metadata){.actor=c->request.actor,.selector=c->request.selector==-1?c->request.actor:NULL,.record=r,.serial=c->serial,.associated=1,.first_esp_child=-1};c->stage=2;return GH_EFFECT_OK;
}
static gh_effect_status initialize_prefix(gh_effect_create_call*c,const gh_effect_create_bindings*b,const uint8_t*r){
 emitter_metadata*m=&c->owner->native->slots[c->slot];uint8_t*p=b->storage.emitters+(size_t)c->slot*0x260;
 if(!m->associated||m->serial!=c->serial||m->record!=r)return GH_EFFECT_ARGUMENT;
 if(c->request.selector!=-1)return fail(c,0x303a30,GH_EFFECT_UNSUPPORTED);
 ww(p+0x110,(uint32_t)c->request.mode);half(p+0x18c,(uint16_t)c->request.bank);half(p+0x18e,(uint16_t)c->request.index);
 ww(p+0x118,c->config_flags&~8u);/*303EFC/303F00 copies distinct config+48 verbatim.*/ww(p+0x1e8,c->config_flags);
 if(!r[0x28e]){if(!b->random)return fail(c,0x31cfe0,GH_EFFECT_UNSUPPORTED);if(c->random_identity&&c->random_identity!=b->random)return GH_EFFECT_ARGUMENT;c->random_identity=b->random;uint32_t seed;if(gh_game_mt_next_0030f348(b->random,&seed))return fail(c,0x31cfe0,GH_EFFECT_ARGUMENT);ww(p+0x220,seed);}else ww(p+0x220,r[0x28e]);c->stage=3;return GH_EFFECT_OK;
}
static gh_effect_status initialize_record(gh_effect_create_call*c,const gh_effect_create_bindings*b,const uint8_t*r){
 emitter_metadata*m=&c->owner->native->slots[c->slot];uint8_t*p=b->storage.emitters+(size_t)c->slot*0x260;
 /* Exact observed zero-vector domain of30A370: VDIV1/0 saturates finiteQ;
  * VMUL.xyz zero * finiteQ produces zero and destW is explicitlyzero.
  * No VU status flags are consumed by303A30. General norm pending. */
 for(unsigned i=0;i<3;i++)if(w(r+0x230+4*i)!=0)return fail(c,0x30a370,GH_EFFECT_UNSUPPORTED);
 if((w(r+0x23c)&0x80000000u)||(w(r+0x23c)&0x7f800000u)==0x7f800000u)return fail(c,0x30aba0,GH_EFFECT_UNSUPPORTED);
 /* Default config+75 is uninitialized padding in original306528. Raw215
  * stays initialhostzero; typedview marks it UNKNOWN, never an originalvalue
  * claim. It is not read by remaining303A30/type0Init. */
 memset(p+0x11c,0,12);memcpy(p+0x174,r+0x240,4);uint32_t negative=w(r+0x244)^0x80000000u;ww(p+0x178,negative);half(p+0x18a,(uint16_t)(w(r+0x19c)<<1));
 memcpy(p+0x30,r+0x1a4,12);memcpy(p+0x128,r+0x1b4,12);memcpy(p+0x14c,r+0x1d4,12);memcpy(p+0x158,r+0x1e4,12);memcpy(p+0x40,r+0x204,12);memcpy(p+0x164,r+0x214,12);
 int part=(int8_t)r[0xbe];if(part==-3){/* Original303A30 false -> original3005D8 marks pendingfree. */unsigned off=(c->slot>>5)*4;ww(b->storage.emitters+0x26020+off,w(b->storage.emitters+0x26020+off)|(0x80000000u>>(c->slot&31)));c->original_return=0;c->complete=1;return GH_EFFECT_OK;}
 if(part>=0){if(!b->model||!b->model->parts_ready||!c->request.actor||b->model->root.bytes!=c->request.actor->bytes)return fail(c,0x303a30,GH_EFFECT_UNSUPPORTED);const gh_actor_part*view;if(gh_actor_model_part_at(b->model,(unsigned)part,&view))return fail(c,0x303a30,GH_EFFECT_BOUNDS);m->model=b->model;m->part_index=(uint32_t)part;m->part_handle=view->handle;}
 else if(part==-2){if(!c->request.actor)return fail(c,0x303a30,GH_EFFECT_UNSUPPORTED);}
 else {
  /* Actual NULL-config, parentless identity cNode branch. All21 rows except
   * this one bind realparts. 2B64C0/147A48 computes identity; default draw
   * rotation147728/translation1476D0 alsoidentity. 147548 staysidentity;
   *147C88 mode0 zero-angle earlyleaves return(+0,-0,+0),147C60 XYZzero.
   * General unattached transforms remain explicit, not a bindpose fallback. */
  for(unsigned k=0;k<3;k++)if(w(p+0x30+4*k)!=0||w(p+0x40+4*k)!=0)return fail(c,0x2b61b0,GH_EFFECT_UNSUPPORTED);
  identity(p+0x50);identity(p+0xb0);ww(p+0x30,0);ww(p+0x34,0x80000000u);ww(p+0x38,0);memset(p+0x40,0,12);p[0x16]=0;ww(p+0x90,0);
 }
 half(p+0x186,(uint16_t)((uint16_t)(r[0xc0]|(unsigned)r[0xc1]<<8)+p[0x1ed]));uint16_t duration=(uint16_t)(r[0xc2]|(unsigned)r[0xc3]<<8);if(duration&&duration<0x8000)duration=(uint16_t)(duration+p[0x1ed]);half(p+0x188,duration);
 if((w(r+4)&0x48000)&&c->request.actor){if(c->request.actor->size<0x2f6)return fail(c,0x303a30,GH_EFFECT_BOUNDS);p[0x226]=c->request.actor->bytes[0x2f4];p[0x227]=c->request.actor->bytes[0x2f5];}
 c->stage=4;return GH_EFFECT_OK;
}
gh_effect_status gh_game_effect_create_00306528(gh_effect_create_call*c,const gh_effect_create_bindings*b){
 if(!c||!b||!b->work||!c->active||c->identity!=c)return GH_EFFECT_ARGUMENT;c->frontier=0;if(c->complete)return GH_EFFECT_OK;
 if(c->request.actor&&(!c->request.actor->bytes||c->request.actor->size<0x2b5))return GH_EFFECT_ARGUMENT;gh_effect_status e=storage(c,b);if(e)return e;
 if(!c->stage){e=select_est(c,b);if(e||c->complete)return e;}
 while(c->row<c->count){const uint8_t*r=c->est.bytes+16+(size_t)c->row*0x2f0;if(c->stage==1){if(r[0x136]){c->row++;continue;}if(!c->request.actor&&(int8_t)r[0xbe]!=-1&&(int8_t)r[0xbe]!=-4){c->original_return=0;c->complete=1;return GH_EFFECT_OK;}e=allocate(c,b,r);if(e||c->complete)return e;}
  if(c->stage==2){e=initialize_prefix(c,b,r);if(e||c->complete)return e;}
  if(c->stage==3){e=initialize_record(c,b,r);if(e||c->complete)return e;}
  if(c->stage==4){uint8_t*p=b->storage.emitters+(size_t)c->slot*0x260;/*actualtype0 virtual303A20*/memcpy(p+0x224,p+0x210,2);c->owner->native->slots[c->slot].initialized=1;c->row++;c->stage=1;}
 }
 c->complete=1;c->original_return=1;return GH_EFFECT_OK;
}
gh_effect_status gh_effect_create_actor_match(void*raw,int emitter,unsigned slot,const gh_title_player_view*a,int*match,uint32_t*f){
 gh_game_effects*e=raw;if(f)*f=0;if(!e||!match||!f||slot>=(emitter?256u:1024u))return GH_EFFECT_ARGUMENT;if(!emitter){gh_effect_esp_view v;gh_effect_status status=gh_effect_create_esp_at(e,slot,&v);if(status){*f=0x2feb80;return GH_EFFECT_UNSUPPORTED;}if(w(v.payload+0x108)){*f=0x2feb80;return GH_EFFECT_UNSUPPORTED;}*match=(!a&&!v.selector)||(a&&v.selector&&a->bytes==v.selector->bytes);return GH_EFFECT_OK;}if(!e->native){*f=0x3039e8;return GH_EFFECT_UNSUPPORTED;}if(e->native->owner!=e)return GH_EFFECT_ARGUMENT;emitter_metadata*m=&e->native->slots[slot];if(!m->associated){*f=0x3039e8;return GH_EFFECT_UNSUPPORTED;}*match=(!a&&!m->actor)||(a&&m->actor&&a->bytes==m->actor->bytes);return GH_EFFECT_OK;
}
void gh_effect_create_forget_on_owner_destroy(gh_game_effects*s){if(!s||!s->native)return;if(s->native->owner!=s)return;free(s->native);s->native=NULL;}

gh_effect_status gh_effect_create_emitter_at(const gh_game_effects*s,unsigned slot,gh_effect_emitter_view*out){
 if(!s||!out||slot>=256||!s->native||s->native->owner!=s)return GH_EFFECT_ARGUMENT;
 const struct gh_effect_native*n=s->native;const emitter_metadata*m=&n->slots[slot];uint32_t mask=0x80000000u>>(slot&31);if(!m->associated||!(w(n->emitters+0x26000+(slot>>5)*4)&mask))return GH_EFFECT_UNSUPPORTED;
 const gh_actor_part*part=NULL;if(m->model){if(gh_actor_model_part_at(m->model,m->part_index,&part)||part->handle!=m->part_handle)return GH_EFFECT_UNSUPPORTED;}
 *out=(gh_effect_emitter_view){.record={m->record,0x2f0},.actor=m->actor,.selector=m->selector,.model=m->model,.part=part,.serial=m->serial,.initialized=m->initialized,.payload=n->emitters+(size_t)slot*0x260,.payload_size=0x260,.first_esp_child=m->first_esp_child};return GH_EFFECT_OK;
}

static void construct_esp_base(uint8_t*p,unsigned type){
 /* 2FFBD0 ->2E81D8 ->2FBCC8 ->2FBD98 ->2B5C20(NULL). */
 for(unsigned i=0;i<4;i++)ww(p+0x20+i*4,0x3f800000);vf0(p+0x30);ww(p+0x34,0x80000000u);vf0(p+0x40);vf0(p+0xa0);identity(p+0x50);identity(p+0xb0);
 const unsigned unit[]={0x188,0x18c,0x190,0x194,0x1f8,0x1fc,0x200,0x204,0x210,0x240};for(unsigned i=0;i<sizeof(unit)/sizeof(unit[0]);i++)ww(p+unit[i],0x3f800000);
 half(p+0x238,0xffff);p[0x246]=p[0x23e]=p[0x23f]=p[0x245]=1;vf0(p+0x280);vf0(p+0x290);ww(p+0x11c,0x06000000);ww(p+0xf0,type==6?0x44e6d8u:0x44f270u);p[0x102]=(uint8_t)type;
}
gh_effect_status gh_effect_create_esp_allocate_002faa88(const gh_effect_release_bindings*b,unsigned source_slot,uint32_t source_serial,unsigned*slot,uint32_t*serial,int*allocated,uint32_t*f){
 if(!b||!slot||!serial||!allocated||!f)return GH_EFFECT_ARGUMENT;*f=0;*allocated=0;*slot=0;*serial=0;
 gh_game_effects*e=b->effects;if(!e||!e->native||e->native->owner!=e||e->native->esp!=b->esp||e->native->emitters!=b->emitters||b->esp_bytes<0x100180||b->emitter_bytes<0x26060)return GH_EFFECT_ARGUMENT;
 gh_effect_emitter_view source;gh_effect_status status=gh_effect_create_emitter_at(e,source_slot,&source);if(status||source.serial!=source_serial||!source.initialized)return GH_EFFECT_ARGUMENT;
 if(source.actor||source.selector||source.model||source.part){*f=0x2faa88;return GH_EFFECT_UNSUPPORTED;}
 unsigned i;for(i=0;i<1024;i++)if(!(w(b->esp+0x100000+(i>>5)*4)&(UINT32_C(0x80000000)>>(i&31))))break;if(i==1024)return GH_EFFECT_OK;
 unsigned type=source.record.bytes[0x133];if(type!=6&&type!=0x1a){*f=type==0?0x2ffa68:type==0x1a?0x2ffed0:0x2faca8;return GH_EFFECT_UNSUPPORTED;}if(e->native->serial==UINT32_MAX){*f=0x2faa88;return GH_EFFECT_UNSUPPORTED;}
 uint8_t*p=b->esp+(size_t)i*0x400;memset(p,0,0x400);construct_esp_base(p,type);uint32_t id=++e->native->serial;esp_metadata*m=&e->native->esps[i];*m=(esp_metadata){.record=source.record,.actor=source.actor,.selector=source.selector,.serial=id,.emitter_serial=source_serial,.emitter_slot=source_slot,.associated=1,.type=(uint8_t)type,.parent_emitter=-1,.previous=-1,.next=-1,.target120_known_null=(uint8_t)(!source.actor&&!source.selector&&!w(source.payload+0x218))};unsigned off=(i>>5)*4;uint32_t mask=UINT32_C(0x80000000)>>(i&31);ww(b->esp+0x100000+off,w(b->esp+0x100000+off)|mask);ww(b->esp+0x100080+off,w(b->esp+0x100080+off)&~mask);ww(b->esp+0x100100+off,w(b->esp+0x100100+off)&~mask);*slot=i;*serial=id;*allocated=1;return GH_EFFECT_OK;
}
gh_effect_status gh_effect_create_esp_at(const gh_game_effects*e,unsigned slot,gh_effect_esp_view*out){
 if(!e||!out||slot>=1024||!e->native||e->native->owner!=e)return GH_EFFECT_ARGUMENT;const esp_metadata*m=&e->native->esps[slot];if(!m->associated||!(w(e->native->esp+0x100000+(slot>>5)*4)&(UINT32_C(0x80000000)>>(slot&31))))return GH_EFFECT_UNSUPPORTED;
 *out=(gh_effect_esp_view){.record=m->record,.texture=m->texture,.secondary_texture=m->secondary_texture,.actor=m->actor,.selector=m->selector,.payload=e->native->esp+(size_t)slot*0x400,.payload_size=0x400,.serial=m->serial,.emitter_serial=m->emitter_serial,.emitter_slot=m->emitter_slot,.initialized=m->initialized,.type=m->type,.globals=e->native->frame_globals,.scene_actors=e->native->frame_actors,.parent_emitter=m->parent_emitter,.previous=m->previous,.next=m->next,.parent_serial=m->parent_serial,.target120_known_null=m->target120_known_null};return GH_EFFECT_OK;
}
gh_effect_status gh_effect_create_esp_bind_textures(gh_game_effects*e,unsigned slot,uint32_t serial,gh_effect_view texture,gh_effect_view secondary){gh_effect_esp_view v;gh_effect_status status=gh_effect_create_esp_at(e,slot,&v);if(status||v.serial!=serial)return GH_EFFECT_ARGUMENT;e->native->esps[slot].texture=texture;e->native->esps[slot].secondary_texture=secondary;return GH_EFFECT_OK;}

static gh_effect_status abort_esp_native(gh_game_effects*e,unsigned slot,uint32_t serial,int ready_allowed){gh_effect_esp_view v;gh_effect_status status=gh_effect_create_esp_at(e,slot,&v);if(status||v.serial!=serial||(!ready_allowed&&v.initialized)||v.actor||v.selector)return GH_EFFECT_ARGUMENT;if(!((v.type==6&&w(v.payload+0xf0)==0x44e6d8u)||(v.type==0x1a&&w(v.payload+0xf0)==0x44f270u)))return GH_EFFECT_UNSUPPORTED;if(v.parent_emitter>=0){status=gh_effect_create_esp_unlink_002b5ec0(e,slot,serial);if(status)return status;}unsigned off=(slot>>5)*4;uint32_t keep=~(UINT32_C(0x80000000)>>(slot&31));for(unsigned bank=0;bank<3;bank++)ww(e->native->esp+0x100000+bank*0x80+off,w(e->native->esp+0x100000+bank*0x80+off)&keep);e->native->esps[slot]=(esp_metadata){0};return GH_EFFECT_OK;}
gh_effect_status gh_effect_create_esp_abort_partial(gh_game_effects*e,unsigned slot,uint32_t serial){return abort_esp_native(e,slot,serial,0);}

gh_effect_status gh_effect_create_bind_frame_globals(gh_game_effects*e,struct gh_effects_frame_globals*g){if(!e||!e->native||e->native->owner!=e||!g||!g->initialized)return GH_EFFECT_ARGUMENT;if(e->native->frame_globals&&e->native->frame_globals!=g)return GH_EFFECT_ARGUMENT;e->native->frame_globals=g;return GH_EFFECT_OK;}
gh_effect_status gh_effect_create_esp_commit_type6(gh_game_effects*e,unsigned slot,uint32_t serial){gh_effect_esp_view v;gh_effect_status status=gh_effect_create_esp_at(e,slot,&v);if(status||v.serial!=serial||v.type!=6||v.initialized||w(v.payload+0xf0)!=0x44e6d8u||!v.globals||!v.globals->initialized||v.actor||v.selector)return GH_EFFECT_ARGUMENT;e->native->esps[slot].initialized=1;return GH_EFFECT_OK;}
gh_effect_status gh_effect_create_esp_abort_native(gh_game_effects*e,unsigned slot,uint32_t serial){return abort_esp_native(e,slot,serial,1);}

gh_effect_status gh_effect_create_bind_frame_actors(gh_game_effects*e,gh_game_scene_tables*actors){if(!e||!e->native||e->native->owner!=e||!actors)return GH_EFFECT_ARGUMENT;if(e->native->frame_actors&&e->native->frame_actors!=actors)return GH_EFFECT_ARGUMENT;e->native->frame_actors=actors;return GH_EFFECT_OK;}
gh_effect_status gh_effect_create_esp_commit_1a(gh_game_effects*e,unsigned slot,uint32_t serial){gh_effect_esp_view v;gh_effect_status status=gh_effect_create_esp_at(e,slot,&v);if(status||v.serial!=serial||v.type!=0x1a||v.initialized||w(v.payload+0xf0)!=0x44f270u||!v.globals||!v.globals->initialized||!v.scene_actors||v.actor||v.selector)return GH_EFFECT_ARGUMENT;e->native->esps[slot].initialized=1;return GH_EFFECT_OK;}

gh_effect_status gh_effect_create_esp_attach_emitter_002b5d58(gh_game_effects*e,unsigned slot,uint32_t serial,unsigned parent,uint32_t parent_serial){
 gh_effect_esp_view v;gh_effect_status status=gh_effect_create_esp_at(e,slot,&v);if(status||v.serial!=serial)return GH_EFFECT_ARGUMENT;esp_metadata*m=&e->native->esps[slot];
 if(m->parent_emitter>=0){gh_effect_emitter_view existing;if(gh_effect_create_emitter_at(e,(unsigned)m->parent_emitter,&existing)||existing.serial!=m->parent_serial)return GH_EFFECT_ARGUMENT;return GH_EFFECT_OK;}
 gh_effect_emitter_view p;if(gh_effect_create_emitter_at(e,parent,&p)||p.serial!=parent_serial)return GH_EFFECT_ARGUMENT;if(m->previous!=-1||m->next!=-1||w(v.payload+4)||w(v.payload+0xc)||w(v.payload+0x10)||w(p.payload+8))return GH_EFFECT_UNSUPPORTED;
 int32_t previous=-1,current=p.first_esp_child;unsigned walked=0;while(current!=-1){if(current<0||current>=1024||++walked>1024)return GH_EFFECT_ARGUMENT;gh_effect_esp_view child;if(gh_effect_create_esp_at(e,(unsigned)current,&child)||child.parent_emitter!=(int32_t)parent||child.parent_serial!=parent_serial||child.previous!=previous)return GH_EFFECT_ARGUMENT;previous=current;current=child.next;}
 m->parent_emitter=(int32_t)parent;m->parent_serial=parent_serial;m->previous=previous;m->next=-1;if(previous==-1)e->native->slots[parent].first_esp_child=(int32_t)slot;else e->native->esps[previous].next=(int32_t)slot;return GH_EFFECT_OK;
}
gh_effect_status gh_effect_create_esp_unlink_002b5ec0(gh_game_effects*e,unsigned slot,uint32_t serial){
 gh_effect_esp_view v;gh_effect_status status=gh_effect_create_esp_at(e,slot,&v);if(status||v.serial!=serial)return GH_EFFECT_ARGUMENT;if(v.parent_emitter<0)return GH_EFFECT_OK;gh_effect_emitter_view p;if(gh_effect_create_emitter_at(e,(unsigned)v.parent_emitter,&p)||p.serial!=v.parent_serial)return GH_EFFECT_ARGUMENT;
 if(v.previous< -1||v.next< -1||v.previous>=1024||v.next>=1024)return GH_EFFECT_ARGUMENT;if(v.previous==-1){if(p.first_esp_child!=(int32_t)slot)return GH_EFFECT_ARGUMENT;}else {gh_effect_esp_view previous;if(gh_effect_create_esp_at(e,(unsigned)v.previous,&previous)||previous.next!=(int32_t)slot||previous.parent_emitter!=v.parent_emitter||previous.parent_serial!=v.parent_serial)return GH_EFFECT_ARGUMENT;}
 if(v.next>=0){gh_effect_esp_view next;if(gh_effect_create_esp_at(e,(unsigned)v.next,&next)||next.previous!=(int32_t)slot||next.parent_emitter!=v.parent_emitter||next.parent_serial!=v.parent_serial)return GH_EFFECT_ARGUMENT;e->native->esps[v.next].previous=v.previous;}
 if(v.previous==-1)e->native->slots[v.parent_emitter].first_esp_child=v.next;else e->native->esps[v.previous].next=v.next;esp_metadata*m=&e->native->esps[slot];m->parent_emitter=m->previous=m->next=-1;m->parent_serial=0;return GH_EFFECT_OK;
}

gh_effect_status gh_effect_create_esp_pause_002fbe50(gh_game_effects*e,unsigned slot,uint32_t serial,uint32_t*f){if(!f)return GH_EFFECT_ARGUMENT;*f=0;gh_effect_esp_view v;gh_effect_status status=gh_effect_create_esp_at(e,slot,&v);if(status||v.serial!=serial||!v.initialized)return GH_EFFECT_ARGUMENT;if((v.type!=6&&v.type!=0x1a)||v.actor||v.selector){*f=0x2fbe50;return GH_EFFECT_UNSUPPORTED;}
 /* Supported constructors create no ESPchildren and the only supportedattach
  * API accepts emitterparents. Unknown rawchild/parent tokens remainstrict. */
 if(w(v.payload+4)||w(v.payload+8)||w(v.payload+0xc)||w(v.payload+0x10)){*f=0x2b5cf0;return GH_EFFECT_UNSUPPORTED;}status=gh_effect_create_esp_unlink_002b5ec0(e,slot,serial);if(status){*f=0x2b5ec0;return status;}unsigned off=(slot>>5)*4;uint32_t bit=UINT32_C(0x80000000)>>(slot&31);ww(e->native->esp+0x100080+off,w(e->native->esp+0x100080+off)|bit);return GH_EFFECT_OK;}
gh_effect_status gh_effect_create_esp_destroy_hidden_native(gh_game_effects*e,unsigned slot,uint32_t serial,uint32_t*f){if(!f)return GH_EFFECT_ARGUMENT;*f=0;gh_effect_esp_view v;gh_effect_status status=gh_effect_create_esp_at(e,slot,&v);if(status||v.serial!=serial||!v.initialized)return GH_EFFECT_ARGUMENT;unsigned off=(slot>>5)*4;uint32_t bit=UINT32_C(0x80000000)>>(slot&31);if(!(w(e->native->esp+0x100100+off)&bit))return GH_EFFECT_ARGUMENT;if((v.type!=6&&v.type!=0x1a)||v.actor||v.selector||v.parent_emitter!=-1||v.previous!=-1||v.next!=-1||w(v.payload+4)||w(v.payload+8)||w(v.payload+0xc)||w(v.payload+0x10)){*f=0x2fbe28;return GH_EFFECT_UNSUPPORTED;}if(w(v.payload+0xf0)!=(v.type==6?0x44e6d8u:0x44f270u)){*f=0x2fbe28;return GH_EFFECT_UNSUPPORTED;}
 /* Actualvirtual dtor(flags2): concreteVT ->44FAD0 ->44B310; bit1clear
  * means original30F5A0 is not called. Caller clearsactive onlyafterreturn. */
 ww(v.payload+0xf0,0x44fad0u);ww(v.payload+0xf0,0x44b310u);e->native->esps[slot].associated=0;return GH_EFFECT_OK;}
