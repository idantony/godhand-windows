#include "game_scenario_soft_event.h"
#include <string.h>
static uint16_t h(const uint8_t*p){return (uint16_t)(p[0]|(unsigned)p[1]<<8);}
static uint32_t w(const uint8_t*p){return (uint32_t)h(p)|(uint32_t)h(p+2)<<16;}
static uint64_t dw(const uint8_t*p){return w(p)|(uint64_t)w(p+4)<<32;}
static void wh(uint8_t*p,uint16_t v){p[0]=(uint8_t)v;p[1]=(uint8_t)(v>>8);}
static void ww(uint8_t*p,uint32_t v){for(unsigned i=0;i<4;i++)p[i]=(uint8_t)(v>>(i*8));}
static void wd(uint8_t*p,uint64_t v){for(unsigned i=0;i<8;i++)p[i]=(uint8_t)(v>>(i*8));}
gh_soft_event_status gh_scenario_clear_pad_0031b410(gh_boot_work*s){
 if(!s)return GH_SOFT_EVENT_ARGUMENT;uint8_t*p=(uint8_t*)s+0x178;uint64_t mask=UINT64_C(0x000f00807ff00000);
 wd(p+64,dw(p+64)&mask);wd(p+56,dw(p+56)&mask);wd(p,dw(p+24)&mask);wd(p+40,dw(p+40)&mask);wd(p+48,dw(p+48)&mask);
 p[0x6b]=0x80;p[0x7b]=0;p[0x68]=p[0x69]=p[0x6a]=0x80;p[0x6f]=p[0x6e]=p[0x6c]=p[0x6d]=0;memset(p+0x70,0,12);return GH_SOFT_EVENT_OK;
}
gh_soft_event_status gh_scenario_stop_input_0031afa0(gh_boot_work*s){
 if(!s)return GH_SOFT_EVENT_ARGUMENT;
 uint8_t *raw=(uint8_t*)s;
 ww(raw+0x5d8,w(raw+0x5d8)|0x2000000);
 return gh_scenario_clear_pad_0031b410(s);
}
static gh_soft_event_status stop(gh_soft_event_start*s,uint32_t pc,gh_soft_event_status e){s->frontier=pc;return e;}
static gh_soft_event_status leaf(gh_soft_event_start *s,gh_soft_event_leaf action,
 void *context,uint32_t original)
{
 if(!action)return stop(s,original,GH_SOFT_EVENT_UNSUPPORTED);
 uint32_t frontier=0;
 gh_soft_event_status status=action(context,&frontier);
 return status?stop(s,frontier?frontier:original,status):GH_SOFT_EVENT_OK;
}
static gh_soft_event_status event_result(gh_soft_event_start *s,
 gh_game_event_status status,uint32_t frontier,uint32_t original)
{
 if(!status)return GH_SOFT_EVENT_OK;
 return stop(s,frontier?frontier:original,
   status==GH_EVENT_UNSUPPORTED?GH_SOFT_EVENT_UNSUPPORTED:GH_SOFT_EVENT_CALLEE);
}
static gh_soft_event_status kind0_step(gh_soft_event_start *s,const gh_soft_event_bindings *b,
 uint8_t *raw)
{
 const gh_soft_event_kind0_bindings *kind=b->kind0;
 uint32_t frontier=0;
 gh_game_event_status e;
 switch(s->stage){
 case 4:
  ww(raw+0x5d8,w(raw+0x5d8)|0x40000);
  ww(raw+0x5e4,w(raw+0x5e4)|0x4010000);
  return GH_SOFT_EVENT_OK;
 case 5:
  if(!kind->room_objects)return stop(s,0x308a38,GH_SOFT_EVENT_UNSUPPORTED);
  e=gh_event_clear_object_hidden_00308a38(kind->room_objects,&frontier);
  return event_result(s,e,frontier,0x308a38);
 case 6:
  if(!kind->room_objects||!kind->room_objects->tables)return stop(s,0x308d40,GH_SOFT_EVENT_UNSUPPORTED);
  memset(kind->room_objects->tables->scroll_link_words,0,
         sizeof(kind->room_objects->tables->scroll_link_words));
  return GH_SOFT_EVENT_OK;
 case 7:return leaf(s,kind->suspend_enemies,b->context,0x2948e8);
 case 8:
  if(!kind->room_objects)return stop(s,0x2c5068,GH_SOFT_EVENT_UNSUPPORTED);
  e=gh_event_hide_scenario_objects_002c5068(kind->room_objects,1,&frontier);
  return event_result(s,e,frontier,0x2c5068);
 case 9:return leaf(s,kind->suspend_effects,b->context,0x2fa470);
 case 10:return leaf(s,kind->suspend_player,b->context,0x12c7d0);
 case 11:
  ww(raw+0x5e4,w(raw+0x5e4)|0x1000000);
  return GH_SOFT_EVENT_OK;
 default:return GH_SOFT_EVENT_ARGUMENT;
 }
}

gh_soft_event_status gh_scenario_start_soft_event_002c39b0(gh_soft_event_start *s,
 const gh_soft_event_bindings *b,uint8_t kind)
{
 if(!s||!b||!b->work||!b->scenario.bytes||b->scenario.size<0xea||!b->scenario.manager||
    !b->scenario.manager->records||b->scenario.manager->count>b->scenario.manager->capacity)
  return GH_SOFT_EVENT_ARGUMENT;
 if(kind!=4&&(kind!=0||!b->kind0))return stop(s,0x2c39b0,GH_SOFT_EVENT_UNSUPPORTED);
 if(s->stage&&(s->scenario_bytes!=b->scenario.bytes||s->manager!=b->scenario.manager||
               s->work!=b->work||s->kind!=kind))return GH_SOFT_EVENT_ARGUMENT;
 uint8_t *p=b->scenario.bytes,*raw=(uint8_t*)b->work;
 gh_task_manager *m=b->scenario.manager;
 if(!s->stage){
  s->scenario_bytes=p;s->manager=m;s->work=b->work;s->kind=kind;
  uint16_t depth=(uint16_t)(h(p+0xe8)+1);wh(p+0xe8,depth);
  int32_t signed_depth=depth<=32767?depth:(int32_t)depth-65536;
  if(signed_depth>=2){s->frontier=0;return GH_SOFT_EVENT_OK;}
  ww(p+0xe0,kind);s->stage=1;
 }
 if(s->stage==1){
  ww(raw+0x5e4,w(raw+0x5e4)|0x40000040);
  if(gh_scenario_stop_input_0031afa0(b->work))return GH_SOFT_EVENT_ARGUMENT;
  s->stage=2;
 }
 if(s->stage==2){
  ww(raw+0x5d8,w(raw+0x5d8)|0x500000);
  ww(raw+0x5e0,w(raw+0x5e0)|0x2100000);
  s->stage=3;
 }
 if(s->stage==3){
  gh_soft_event_status status=leaf(s,b->player_abort_00126770,b->context,0x126770);
  if(status)return status;
  s->stage=4;
 }
 if(kind==0){
  while(s->stage<12){
   gh_soft_event_status status=kind0_step(s,b,raw);
   if(status)return status;
   ++s->stage;
  }
 }else if(s->stage==4){
  ww(raw+0x5e4,w(raw+0x5e4)|0x80000);
  ww(raw+0x5d8,w(raw+0x5d8)|0xf4000000);
  s->stage=5;
 }
 if(m->current_index>=0){
  if((uint32_t)m->current_index>=m->count||m->current!=&m->records[m->current_index])
   return stop(s,0x2c39b0,GH_SOFT_EVENT_ARGUMENT);
  m->current->attributes|=1;
 }
 m->flags|=1;
 s->stage=0;s->frontier=0;
 return GH_SOFT_EVENT_OK;
}

gh_soft_event_status gh_soft_event_suspend_player_0012c7d0(const gh_title_player_view *player,
 const gh_title_player_view *const related[4],uint8_t known_mask,uint32_t *frontier)
{
 if(!frontier)return GH_SOFT_EVENT_ARGUMENT;
 *frontier=0x12c7d0;
 if(!player||!player->bytes||player->size<GH_TITLE_PLAYER_BYTES||!related||
    player->original_vtable214!=0x41d220||w(player->bytes+0x214)!=0x41d220)
  return GH_SOFT_EVENT_ARGUMENT;
 static const uint32_t sites[]={0x12c858,0x12c878,0x12c898,0x12c8b8};
 static const unsigned slots[]={0,0x698,0x6a0,0x6a8};
 for(unsigned i=0;i<4;i++){
  *frontier=sites[i];
  if(!(known_mask&(1u<<i)))return GH_SOFT_EVENT_UNSUPPORTED;
  if(!related[i]){
   if(slots[i]&&w(player->bytes+slots[i]))return GH_SOFT_EVENT_UNSUPPORTED;
   continue;
  }
  if(!related[i]->bytes||related[i]->size<0x254||
     related[i]->original_vtable214!=0x41b6a0||w(related[i]->bytes+0x214)!=0x41b6a0)
   return GH_SOFT_EVENT_UNSUPPORTED;
 }
 ww(player->bytes+0x250,w(player->bytes+0x250)|0x8000);
 ww(player->bytes+0x5a0,w(player->bytes+0x5a0)|8);
 for(unsigned i=0;i<4;i++)if(related[i])
  ww(related[i]->bytes+0x250,w(related[i]->bytes+0x250)|0x8000);
 *frontier=0;
 return GH_SOFT_EVENT_OK;
}

gh_soft_event_status gh_soft_event_set_effect_suspend_002fa470(const gh_event_start_bindings *b,
 int suspend,uint32_t *frontier)
{
 if(!frontier)return GH_SOFT_EVENT_ARGUMENT;
 *frontier=0x2fa470;
 if(!b||!b->pools)return GH_SOFT_EVENT_ARGUMENT;
 const gh_room_vector *v;
 if(gh_room_registry_view(b->pools,0x754c30,&v)||
    (v->count&&!v->allocation.pointer)||v->count>v->capacity)
  return GH_SOFT_EVENT_ARGUMENT;
 if(v->count&&!b->object_view)return GH_SOFT_EVENT_UNSUPPORTED;
 for(uint32_t i=0;i<v->count;i++){
  gh_title_player_view actor;
  uint32_t nested=0;
  uint32_t handle=w((const uint8_t *)v->allocation.pointer+(size_t)i*4);
  gh_game_event_status e=b->object_view(b->context,handle,&actor,&nested);
  if(e){if(nested)*frontier=nested;return e==GH_EVENT_UNSUPPORTED?GH_SOFT_EVENT_UNSUPPORTED:GH_SOFT_EVENT_CALLEE;}
  if(!actor.bytes||actor.size<0x300)return GH_SOFT_EVENT_ARGUMENT;
  unsigned kind=h(actor.bytes+0x2fe);
  if(kind>=0x500&&kind<0x600){
   const uint32_t flags=w(actor.bytes+0x250);
   ww(actor.bytes+0x250,suspend?flags|0x8000u:flags&~0x8000u);
  }
 }
 *frontier=0;
 return GH_SOFT_EVENT_OK;
}

gh_soft_event_status gh_soft_event_suspend_effects_002fa470(const gh_event_start_bindings *b,
 uint32_t *frontier)
{
 return gh_soft_event_set_effect_suspend_002fa470(b,1,frontier);
}
