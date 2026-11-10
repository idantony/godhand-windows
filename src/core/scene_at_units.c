#include "scene_at_units.h"
#include <stdlib.h>
#include <string.h>
#include <fenv.h>
typedef struct {gh_at_native_unit view;gh_at_link next;gh_r207_allocation owned;uint8_t linked;} node;
struct gh_scene_at_units {gh_heap*heap;const void*source;node*nodes;size_t count,capacity;uint8_t*copies[4];uint32_t phase,jump_index,jump_slot,pending_handle;uint16_t room;uint8_t reference;};
static uint16_t h(const uint8_t*p){return (uint16_t)(p[0]|(unsigned)p[1]<<8);}
static uint32_t w(const uint8_t*p){return (uint32_t)h(p)|(uint32_t)h(p+2)<<16;}
static void wh(uint8_t*p,uint16_t v){p[0]=(uint8_t)v;p[1]=(uint8_t)(v>>8);}
static void ww(uint8_t*p,uint32_t v){for(unsigned i=0;i<4;i++)p[i]=(uint8_t)(v>>(i*8));}
static gh_scene_at_status fail(gh_scene_at_manager*s,uint32_t pc,int detail,gh_scene_at_status e){s->frontier=pc;s->detail=detail;return e;}
static node*resolve(struct gh_scene_at_units*t,uint32_t handle){return handle&&handle<=t->count?&t->nodes[handle-1]:NULL;}
static int own(gh_scene_at_manager*s,gh_at_native_unit v){node*n=s&&s->native_units?resolve(s->native_units,v.handle):NULL;return n&&n->view.bytes==v.bytes&&v.size==0x9c;}
static node*next_node(gh_scene_at_manager*s,gh_at_link link){for(unsigned guard=0;guard<17;guard++){if(link.kind==GH_AT_LINK_END)return NULL;if(link.kind==GH_AT_LINK_UNIT)return resolve(s->native_units,link.index);if(link.kind!=GH_AT_LINK_SLOT||link.index>=16)return NULL;link=s->heads[link.index];}return NULL;}
static void insert(gh_scene_at_manager*s,node*n,unsigned group){n->next=s->heads[group];s->heads[group]=(gh_at_link){GH_AT_LINK_UNIT,n->view.handle};n->linked=1;/* raw+0 cannot contain hostpointer */ww(n->view.bytes,0);}
gh_scene_at_status gh_scene_at_get_jump_002c2ae0(gh_scene_at_manager*s,uint32_t index,gh_at_native_unit*out){
 if(!s||!out||!s->native_units)return GH_AT_ARGUMENT;*out=(gh_at_native_unit){0};node*n=next_node(s,(gh_at_link){GH_AT_LINK_SLOT,15});size_t guard=0;
 while(n){if(++guard>s->native_units->count)return GH_AT_BOUNDS;uint8_t*p=n->view.bytes;if(p[0x35]==1&&p[0x79]==index){*out=n->view;return GH_AT_OK;}n=next_node(s,n->next);}return GH_AT_OK;
}
static gh_scene_at_status reset_units(gh_scene_at_manager*s){node*n=next_node(s,(gh_at_link){GH_AT_LINK_SLOT,15});size_t guard=0;while(n){if(++guard>s->native_units->count)return GH_AT_BOUNDS;uint8_t*p=n->view.bytes;switch(p[0x35]){
 case 1:if(!(p[0x3b]&8)){if(p[0x7a]==1){p[0x3b]=0;p[0x3a]=0x31;p[0x39]=0;}else{p[0x3b]=(p[0x3b]&0x80)|8;p[0x3a]=0x20;p[0x39]=4;}}else if(p[0x7a]==1){p[0x3b]=0;p[0x3a]=0x31;p[0x39]=0;}break;
 case 4:if(!(p[0x3b]&8)){if(h(p+0x64))return fail(s,0x31ce38,0,GH_AT_UNSUPPORTED);p[0x3b]=(p[0x3b]&0x80)|8;p[0x3a]=2;p[0x39]=1;ww(p+0x68,0);}break;
 case 5:p[0x3b]=1;break;case 8:p[0x3b]=0x31;break;
 case 9:if(p[5]==1&&(p[0x34]&1))return fail(s,0x2c0e20,0,GH_AT_UNSUPPORTED);break;
 case 10:if(!(p[0x3b]&8)){p[0x3b]=(p[0x3b]&0x80)|8;p[0x3a]=2;p[0x39]=4;}break;
 case 11:break;default:break;}
 n=next_node(s,n->next);}return GH_AT_OK;}
static void wf(uint8_t*p,float f){uint32_t bits;memcpy(&bits,&f,4);ww(p,bits);}
static gh_scene_at_status jump_box(gh_scene_at_manager*s,node*n,const uint8_t*r){
 if(!s->native_units->reference)return fail(s,0x1f88a8,0,GH_AT_UNSUPPORTED);
 fenv_t old;if(feholdexcept(&old))return fail(s,0x1f88a8,0,GH_AT_UNSUPPORTED);if(fesetround(FE_TONEAREST)){(void)fesetenv(&old);return fail(s,0x1f88a8,0,GH_AT_UNSUPPORTED);}
 uint32_t bits=0x3c23d70a;float factor;memcpy(&factor,&bits,4);volatile float x=(float)(int16_t)h(r+12)*factor,y=(float)(int16_t)h(r+14)*factor,z=(float)(int16_t)h(r+16)*factor;
 uint8_t*p=n->view.bytes;memset(p,0,0x9c);p[0x35]=1;p[0x3b]=2;p[0x38]=3;p[0x3a]=0x31;wh(p+0x36,0xffff);p[0x34]=3;p[0x39]=8;p[0x54]=0x31;
 int angle=(int16_t)(uint16_t)(h(r+18)+180);if(angle>180)angle=(int16_t)(uint16_t)(angle-360);p[0x45]=(uint8_t)(angle/2);p[0x46]=0x28;
 p[4]=1;p[5]=1;wf(p+8,y);wf(p+12,3.0f);wf(p+16,1.5f);volatile float xm=x-1.5f,xp=x+1.5f,zm=z-1.5f,zp=z+1.5f;wf(p+20,xm);wf(p+24,zm);wf(p+28,xm);wf(p+32,zp);wf(p+36,xp);wf(p+40,zp);wf(p+44,xp);wf(p+48,zm);
 if(fesetenv(&old))return fail(s,0x1f88a8,0,GH_AT_UNSUPPORTED);return GH_AT_OK;
}
gh_scene_at_status gh_scene_at_initialize_units_002c0f90(gh_scene_at_manager*s,const gh_bundle*b,gh_heap*heap,uint16_t room,int reference){
 if(!s||!b||!heap||(reference!=0&&reference!=1))return GH_AT_ARGUMENT;
 struct gh_scene_at_units*t=s->native_units;
 if(!t){t=calloc(1,sizeof(*t));if(!t)return GH_AT_BOUNDS;t->heap=heap;t->source=b->bytes;t->room=room;t->reference=(uint8_t)reference;s->native_units=t;s->ready=0;s->frontier=0;s->detail=0;
  static const char*names[]={"SCA","MEH","SCI","SCL"};size_t count=8;
  s->units[0]=(gh_at_unit_view){0};s->units[1]=(gh_at_unit_view){0};for(unsigned g=0;g<16;g++)s->heads[g]=(gh_at_link){g?GH_AT_LINK_SLOT:GH_AT_LINK_END,g?g-1:0};
  for(unsigned g=0;g<4;g++){gh_bundle_entry e={0};gh_bundle_error be=gh_bundle_find(b,names[g],0,&e);if(be&&be!=GH_BUNDLE_NOT_FOUND)return fail(s,0x31ccf0,g,GH_AT_BOUNDS);s->headers[g]=e;if(!e.bytes||e.size<8||h(e.bytes+4)!=0x100)continue;unsigned n=h(e.bytes+6);if(e.size<16||(size_t)n>(e.size-16)/0x9c)return fail(s,0x2c0f90,g,GH_AT_BOUNDS);count+=n;}
  t->nodes=calloc(count,sizeof(*t->nodes));if(!t->nodes)return GH_AT_BOUNDS;t->capacity=count;
  for(unsigned g=0;g<4;g++){gh_bundle_entry e=s->headers[g];if(!e.bytes||e.size<8||h(e.bytes+4)!=0x100)continue;unsigned count_g=h(e.bytes+6);if(count_g){t->copies[g]=malloc((size_t)count_g*0x9c);if(!t->copies[g])return GH_AT_BOUNDS;memcpy(t->copies[g],e.bytes+16,(size_t)count_g*0x9c);}s->units[g]=(gh_at_unit_view){t->copies[g],(size_t)count_g*0x9c,(uint16_t)count_g};size_t base=t->count;for(unsigned i=0;i<count_g;i++){node*n=&t->nodes[t->count++];n->view=(gh_at_native_unit){t->copies[g]+(size_t)i*0x9c,0x9c,(uint32_t)t->count};if(g)wh(n->view.bytes+0x36,(uint16_t)(h(n->view.bytes+0x36)+g*0x80));if(n->view.bytes[0x39]>=16)return fail(s,0x2c3040,i,GH_AT_BOUNDS);}
   for(unsigned k=0;k<count_g;k++){unsigned i=g==1?k:count_g-1-k;node*n=&t->nodes[base+i];insert(s,n,n->view.bytes[0x39]);}}
  for(unsigned i=0;i<8;i++)s->jump_slots[i]=(gh_at_unit_view){0};t->phase=1;
 }
 if(t->source!=b->bytes||t->heap!=heap||t->room!=room)return GH_AT_ARGUMENT;if(!t->phase)return fail(s,s->frontier?s->frontier:0x2c0f90,s->detail,GH_AT_BOUNDS);t->reference=(uint8_t)reference;
 if(t->phase==1){gh_bundle_entry jump={0};gh_bundle_error be=gh_bundle_find(b,"JMP",0,&jump);if(be&&be!=GH_BUNDLE_NOT_FOUND)return fail(s,0x31ccf0,be,GH_AT_BOUNDS);if(jump.bytes){if(jump.size<4||w(jump.bytes)>(jump.size-4)/20)return fail(s,0x2c12c8,0,GH_AT_BOUNDS);
  while(t->jump_index<w(jump.bytes)&&t->jump_slot<8){uint32_t i=t->jump_index;gh_at_native_unit existing;gh_scene_at_status e=gh_scene_at_get_jump_002c2ae0(s,i,&existing);if(e)return e;if(!existing.bytes){node*n=t->pending_handle?resolve(t,t->pending_handle):NULL;if(!n){if(t->count==t->capacity)return GH_AT_BOUNDS;n=&t->nodes[t->count];if(gh_heap_allocate(heap,0x9c,16,&n->owned))return fail(s,0x30f550,i,GH_AT_BOUNDS);n->view=(gh_at_native_unit){n->owned.pointer,0x9c,(uint32_t)t->count+1};t->count++;t->pending_handle=n->view.handle;s->jump_slots[t->jump_slot]=(gh_at_unit_view){n->view.bytes,0x9c,1};}
   const uint8_t*r=jump.bytes+4+(size_t)i*20;if(h(r+8)!=room||r[10]!=i){e=jump_box(s,n,r);if(e)return e;n->view.bytes[0x79]=(uint8_t)i;insert(s,n,4);t->jump_slot++;}t->pending_handle=0;
  }t->jump_index++;}}
  t->phase=2;
 }
 if(t->phase==2){gh_scene_at_status e=reset_units(s);if(e)return e;t->phase=3;s->field60=s->field68=s->field6c=s->field70=0;s->field64=0;s->ready=1;s->frontier=0;}
 return GH_AT_OK;
}
gh_scene_at_status gh_scene_at_set_enabled(gh_scene_at_manager*s,gh_at_native_unit v,int enabled,int*original){if(!s||!original)return GH_AT_ARGUMENT;*original=0;if(!s->headers[0].bytes||!v.bytes)return GH_AT_OK;if(!own(s,v))return GH_AT_ARGUMENT;if(v.bytes[0x35]==9)return fail(s,enabled?0x2c0e20:0x2c0e60,0,GH_AT_UNSUPPORTED);if(enabled)v.bytes[0x34]|=1;else v.bytes[0x34]&=0xfe;*original=1;return GH_AT_OK;}
gh_scene_at_status gh_scene_at_set_jump_info_002c2750(gh_scene_at_manager*s,gh_at_native_unit v,uint8_t state,uint32_t fn,uint32_t arg,uint8_t trigger){if(!s)return GH_AT_ARGUMENT;if(!v.bytes)return GH_AT_OK;if(!own(s,v))return GH_AT_ARGUMENT;uint8_t*p=v.bytes;if(trigger){if(!p[0x4e])p[0x4e]=p[0x3b];p[0x3b]=trigger;}ww(p+0x40,arg);p[0x44]=state;ww(p+0x3c,fn);return GH_AT_OK;}
gh_scene_at_status gh_scene_at_units_release(gh_scene_at_manager*s){if(!s)return GH_AT_ARGUMENT;struct gh_scene_at_units*t=s->native_units;if(!t)return GH_AT_OK;for(size_t i=0;i<t->count;i++)if(t->nodes[i].owned.pointer&&gh_heap_release(t->heap,t->nodes[i].owned))return GH_AT_BOUNDS;for(unsigned i=0;i<4;i++)free(t->copies[i]);free(t->nodes);free(t);s->native_units=NULL;s->ready=0;for(unsigned i=0;i<4;i++)s->units[i]=(gh_at_unit_view){0};for(unsigned i=0;i<8;i++)s->jump_slots[i]=(gh_at_unit_view){0};return GH_AT_OK;}
/* Strict typed traversal of the SAME original tagged unit lists. */
static gh_scene_at_status walk_checked(gh_scene_at_manager*s,gh_at_link link,gh_at_native_unit*out){
 *out=(gh_at_native_unit){0};for(unsigned depth=0;depth<17;depth++){
  if(link.kind==GH_AT_LINK_END)return GH_AT_OK;
  if(link.kind==GH_AT_LINK_SLOT){if(link.index>=16)return GH_AT_BOUNDS;link=s->heads[link.index];continue;}
  if(link.kind!=GH_AT_LINK_UNIT||!s->native_units)return GH_AT_BOUNDS;node*n=resolve(s->native_units,link.index);if(!n||!n->linked||!n->view.bytes||n->view.size!=0x9c)return GH_AT_BOUNDS;*out=n->view;return GH_AT_OK;
 }return GH_AT_BOUNDS;
}
gh_scene_at_status gh_scene_at_first_unit_002c30c0(gh_scene_at_manager*s,gh_at_native_unit*out){if(!s||!out||!s->ready)return GH_AT_ARGUMENT;return walk_checked(s,(gh_at_link){GH_AT_LINK_SLOT,15},out);}
gh_scene_at_status gh_scene_at_next_unit_002c30c8(gh_scene_at_manager*s,gh_at_native_unit current,gh_at_native_unit*out){if(!s||!out||!own(s,current))return GH_AT_ARGUMENT;node*n=resolve(s->native_units,current.handle);return walk_checked(s,n->next,out);}
size_t gh_scene_at_native_unit_count(const gh_scene_at_manager*s){return s&&s->native_units?s->native_units->count:0;}
