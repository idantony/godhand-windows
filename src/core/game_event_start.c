/* Readable recovery of2965F0/296738/2967B0/308A38/2C5068.
 * MIT reconstruction reference: LICENSE.upstream. See private evidence. */
#include "game_event_start.h"
#include <string.h>
static uint32_t rd(const void*p){uint32_t x;memcpy(&x,p,4);return x;}
static void wr(void*p,uint32_t x){memcpy(p,&x,4);}
static uint8_t *work(const gh_event_start_bindings*b){return (uint8_t*)b->work;}
static gh_game_event_status fail(uint32_t*f,uint32_t at){if(f)*f=at;return GH_EVENT_UNSUPPORTED;}
static const gh_room_object_binding *binding(const gh_room_pools*p,uint32_t h){
 for(size_t i=0;i<p->object_slots;i++)if(p->objects[i].handle==h&&p->objects[i].object)return p->objects+i;
 return NULL;
}
static gh_game_event_status registry(const gh_event_start_bindings*b,uint32_t address,const gh_room_vector**v,uint32_t*f){
 if(!b||!b->pools)return GH_EVENT_ARGUMENT;
 if(gh_room_registry_view(b->pools,address,v)!=GH_POOL_OK)return fail(f,0x308a38);
 return GH_EVENT_OK;
}
static gh_game_event_status om(const gh_event_start_bindings*b,uint32_t handle,gh_title_player_view*out,uint32_t*f){
 if(!b->object_view)return fail(f,0x308a38);
 gh_game_event_status e=b->object_view(b->context,handle,out,f);if(e)return e;
 if(!out->bytes||out->size<0x5bc)return GH_EVENT_BOUNDS;
 return GH_EVENT_OK;
}
gh_game_event_status gh_event_clear_object_hidden_00308a38(const gh_event_start_bindings*b,uint32_t*f){
 if(f)*f=0;const gh_room_vector*v;gh_game_event_status e=registry(b,0x754c38,&v,f);if(e)return e;
 for(uint32_t i=0;i<v->count;i++){
  uint32_t h=rd((const uint8_t*)v->allocation.pointer+i*4);const gh_room_object_binding*x=binding(b->pools,h);
  if(!x||x->kind!=GH_ROOM_OBJECT_SCROLL_MESH||!b->meshes||x->owner!=b->meshes)return fail(f,0x14b6f8);
  gh_scene_mesh_object*o=x->object;
  if(!o->constructor_complete)return fail(f,0x14b6f8);
  if(!o->node)continue;
  /* Current typed constructor owns one408-byte node. Guest next pointers are
   * not dereferenced: a positive link requires an expanded native node owner. */
  uint8_t*n=o->node;if(rd(n+0x404))return fail(f,0x308a38);
  wr(n+0x380,rd(n+0x380)&~1u);
 }
 e=registry(b,0x754c58,&v,f);if(e)return e;
 for(uint32_t i=0;i<v->count;i++){
  gh_title_player_view p;e=om(b,rd((const uint8_t*)v->allocation.pointer+i*4),&p,f);if(e)return e;
  unsigned id=p.bytes[0x2fe]|(unsigned)p.bytes[0x2ff]<<8;
  if((id>=0x100&&id<0x1ff)||(id>=0x200&&id<0x300)||(id>=0x300&&id<0x500)||(id>=0x500&&id<0x600))wr(p.bytes+0x5a0,rd(p.bytes+0x5a0)&~2u);
 }
 return GH_EVENT_OK;
}
gh_game_event_status gh_event_hide_scenario_objects_002c5068(const gh_event_start_bindings*b,int hide,uint32_t*f){
 if(f)*f=0;const gh_room_vector*v;gh_game_event_status e=registry(b,0x754c58,&v,f);if(e)return e;
 for(uint32_t i=0;i<v->count;i++){
  gh_title_player_view p;e=om(b,rd((const uint8_t*)v->allocation.pointer+i*4),&p,f);if(e){if(f)*f=0x2c5068;return e;}
  unsigned id=p.bytes[0x2fe]|(unsigned)p.bytes[0x2ff]<<8;
  if(id>=0x300&&id<0x500){if(hide==1){if(!(rd(p.bytes+0x5b8)&0x40))wr(p.bytes+0x250,rd(p.bytes+0x250)|0x8000);}else wr(p.bytes+0x250,rd(p.bytes+0x250)&~0x8000u);}
 }
 return GH_EVENT_OK;
}
gh_game_event_status gh_event_start_002965f0(gh_event_start_state*s,const gh_event_start_bindings*b,int*done){
 if(!s||!b||!done||!b->event||!b->event->initialized||!b->config||!b->config->initialized||!b->work||!b->fade||!b->fade_context||!b->tables)return GH_EVENT_ARGUMENT;
 *done=0;s->frontier=0;if(s->event&&(s->event!=b->event||s->work!=b->work))return GH_EVENT_ARGUMENT;
 s->event=b->event;s->work=b->work;gh_game_event_prefix*p=&b->event->fields;uint8_t*w=work(b);gh_game_event_status e;
 if(!s->stage){
  if(p->phase==0){if(!(rd(w+0x58c)&0x80000000u)&&!(b->config->flags&14)){
    if(gh_screen_fade_start_00307208(b->fade,b->fade_context,0,10,0,0,0xff000000,15)!=GH_FADE_OK)return GH_EVENT_ARGUMENT;
   }p->phase++;}
  if(p->phase!=1)return GH_EVENT_OK;
  if(rd(w+0x58c)&0x80000000u)return GH_EVENT_OK;
  if(!(b->config->flags&14)&&!(gh_screen_fade_status_00307718(b->fade)&4))return GH_EVENT_OK;
  wr(w+0x5e0,rd(w+0x5e0)|0x02000000);s->stage=1;
 }
 if(s->stage==1){e=gh_event_clear_object_hidden_00308a38(b,&s->frontier);if(e)return e;s->stage=2;}
 if(s->stage==2){for(unsigned i=32;i--;)b->tables->scroll_link_words[i]=0;wr(w+0x5d8,rd(w+0x5d8)|0x00200000);s->stage=3;}
 if(s->stage==3){e=gh_event_hide_scenario_objects_002c5068(b,1,&s->frontier);if(e)return e;
  wr(w+0x5d8,rd(w+0x5d8)&0xcbffffff);wr(w+0x5e4,rd(w+0x5e4)|0x01000000);s->stage=4;}
 if(s->stage==4){if(gh_screen_fade_start_00307208(b->fade,b->fade_context,0,8,0,0xff000000,0xff000000,0)!=GH_FADE_OK)return GH_EVENT_ARGUMENT;s->stage=5;}
 if(!b->system_menu_start)return fail(&s->frontier,0x2b65b0);
 e=b->system_menu_start(b->context,&s->frontier);if(e)return e;
 p->unknown07=0;p->state=1;p->phase=0;p->unknown06=0;*done=1;memset(s,0,sizeof(*s));return GH_EVENT_OK;
}
