#include "game_effect_create.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <fenv.h>
typedef struct {const gh_title_player_view *actor,*selector;const gh_actor_model *model;uint32_t part_index,part_handle;
 const uint8_t *record;uint32_t serial;uint8_t associated,initialized;} emitter_metadata;
struct gh_effect_native {gh_game_effects *owner;uint8_t *esp,*emitters;uint32_t serial;
 emitter_metadata slots[256];};
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
static gh_effect_status storage(gh_effect_create_call*c,const gh_effect_create_bindings*b){
 const gh_effect_release_bindings*s=&b->storage;gh_game_effects*e=s->effects;if(!e||!e->ready||!s->esp||s->esp_bytes<0x100180||!s->emitters||s->emitter_bytes<0x26060)return GH_EFFECT_ARGUMENT;
 if(c->owner&&c->owner!=e)return GH_EFFECT_ARGUMENT;c->owner=e;
 if(e->native&&(e->native->owner!=e||e->native->esp!=s->esp||e->native->emitters!=s->emitters))return GH_EFFECT_ARGUMENT;
 return GH_EFFECT_OK;
}
static gh_effect_status select_est(gh_effect_create_call*c,const gh_effect_create_bindings*b){
 uint32_t flags=w((const uint8_t*)b->work+0x5d8);if((flags&0x10000000u)||(!c->request.index&&c->request.bank!=3)){c->original_return=1;c->complete=1;return GH_EFFECT_OK;}
 if(c->request.bank==3)return fail(c,0x305fb0,GH_EFFECT_UNSUPPORTED);
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
 if(slot==256){c->original_return=0;c->complete=1;return GH_EFFECT_OK;}uint32_t mask=0x80000000u>>(slot&31),off=(slot>>5)*4;
 ww(base+0x26000+off,w(base+0x26000+off)|mask);ww(base+0x26020+off,w(base+0x26020+off)&~mask);ww(base+0x26040+off,w(base+0x26040+off)&~mask);
 uint8_t*p=base+(size_t)slot*0x260;memset(p,0,0x260);construct_type0(p);c->slot=slot;c->serial=++e->native->serial;
 e->native->slots[slot]=(emitter_metadata){.actor=c->request.actor,.selector=c->request.selector==-1?c->request.actor:NULL,.record=r,.serial=c->serial,.associated=1};c->stage=2;return GH_EFFECT_OK;
}
static gh_effect_status initialize_prefix(gh_effect_create_call*c,const gh_effect_create_bindings*b,const uint8_t*r){
 emitter_metadata*m=&c->owner->native->slots[c->slot];uint8_t*p=b->storage.emitters+(size_t)c->slot*0x260;
 if(!m->associated||m->serial!=c->serial||m->record!=r)return GH_EFFECT_ARGUMENT;
 if(c->request.selector!=-1)return fail(c,0x303a30,GH_EFFECT_UNSUPPORTED);
 ww(p+0x110,(uint32_t)c->request.mode);half(p+0x18c,(uint16_t)c->request.bank);half(p+0x18e,(uint16_t)c->request.index);
 ww(p+0x118,c->config_flags);
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
 gh_game_effects*e=raw;if(f)*f=0;if(!e||!match||!f||slot>=(emitter?256u:1024u))return GH_EFFECT_ARGUMENT;if(!emitter){*f=0x2feb80;return GH_EFFECT_UNSUPPORTED;}if(!e->native){*f=0x3039e8;return GH_EFFECT_UNSUPPORTED;}if(e->native->owner!=e)return GH_EFFECT_ARGUMENT;emitter_metadata*m=&e->native->slots[slot];if(!m->associated){*f=0x3039e8;return GH_EFFECT_UNSUPPORTED;}*match=(!a&&!m->actor)||(a&&m->actor&&a->bytes==m->actor->bytes);return GH_EFFECT_OK;
}
void gh_effect_create_forget_on_owner_destroy(gh_game_effects*s){if(!s||!s->native)return;if(s->native->owner!=s)return;free(s->native);s->native=NULL;}

gh_effect_status gh_effect_create_emitter_at(const gh_game_effects*s,unsigned slot,gh_effect_emitter_view*out){
 if(!s||!out||slot>=256||!s->native||s->native->owner!=s)return GH_EFFECT_ARGUMENT;
 const struct gh_effect_native*n=s->native;const emitter_metadata*m=&n->slots[slot];uint32_t mask=0x80000000u>>(slot&31);if(!m->associated||!(w(n->emitters+0x26000+(slot>>5)*4)&mask))return GH_EFFECT_UNSUPPORTED;
 const gh_actor_part*part=NULL;if(m->model){if(gh_actor_model_part_at(m->model,m->part_index,&part)||part->handle!=m->part_handle)return GH_EFFECT_UNSUPPORTED;}
 *out=(gh_effect_emitter_view){.record={m->record,0x2f0},.actor=m->actor,.selector=m->selector,.model=m->model,.part=part,.serial=m->serial,.initialized=m->initialized};return GH_EFFECT_OK;
}
