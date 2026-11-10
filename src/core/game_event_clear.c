/* MIT reconstruction reference LICENSE.upstream; actual ELF evidence in
 * analysis/game-event-clear-evidence.json. */
#include "game_event_clear.h"
#include <string.h>
static uint32_t rd(const void*p){uint32_t x;memcpy(&x,p,4);return x;}
static void wr(void*p,uint32_t x){memcpy(p,&x,4);}
static gh_game_event_status fail(uint32_t*f,uint32_t address){*f=address;return GH_EVENT_UNSUPPORTED;}
gh_game_event_status gh_event_player_clear_0012c418(gh_event_player_clear*s,const gh_event_clear_bindings*b){
 if(!s||!b||!b->player_abort||!b->player_abort->player)return GH_EVENT_ARGUMENT;
 gh_title_player_view*p=&b->player_abort->player->view;if(!p->bytes||p->size<0x1660)return GH_EVENT_BOUNDS;
 if(s->player&&s->player!=p->bytes)return GH_EVENT_ARGUMENT;s->player=p->bytes;s->frontier=0;
 if(!s->stage){if(rd(p->bytes+0x15f4)&2){wr(p->bytes+0x15f4,rd(p->bytes+0x15f4)&~2u);s->stage=1;}else s->stage=2;}
 if(s->stage==1){if(!b->camera_reset_0013b9b8_then_00138d40)return fail(&s->frontier,0x13b9b8);
  gh_game_event_status e=b->camera_reset_0013b9b8_then_00138d40(b->context,&s->frontier);if(e)return e;s->stage=2;}
 if(s->stage==2){gh_player_abort_status e=gh_game_player_abort_00126770(&s->abort,b->player_abort);
  if(e){s->frontier=s->abort.frontier;s->stage=2;return e==GH_PLAYER_ABORT_UNSUPPORTED?GH_EVENT_UNSUPPORTED:GH_EVENT_CALLEE;}s->stage=3;}
 if(s->stage==3){memset(p->bytes+0x2f4,0,4);s->stage=4;}
 return fail(&s->frontier,0x2a8578);
}
static gh_game_event_status enemies(gh_event_clear_state*s,const gh_event_clear_bindings*b){
 if(!b->init||!b->pools)return GH_EVENT_ARGUMENT;
 /* Original manager500 is a positive linked actor list, not all64 slots.
  * Native nonempty ownership requires290770, never a blanket reset. */
 if(rd(b->init->enemies+0x500)||b->init->enemy_top)return fail(&s->frontier,0x290770);
 const gh_room_vector*v;if(gh_room_registry_view(b->pools,0x754c58,&v)!=GH_POOL_OK)return fail(&s->frontier,0x2908c0);
 while(s->index<v->count){uint32_t handle=rd((const uint8_t*)v->allocation.pointer+s->index*4);gh_title_player_view p={0};
  if(!b->object_view)return fail(&s->frontier,0x2908c0);gh_game_event_status e=b->object_view(b->context,handle,&p,&s->frontier);if(e)return e;
  if(!p.bytes||p.size<0x300)return GH_EVENT_BOUNDS;
  if((p.bytes[0x2fe]|(unsigned)p.bytes[0x2ff]<<8)==0x364){if(!b->special_remove_00312708)return fail(&s->frontier,0x312708);e=b->special_remove_00312708(b->context,&s->frontier);if(e)return e;}
  s->index++;
 }
 return GH_EVENT_OK;
}
gh_game_event_status gh_event_clear_00296818(gh_event_clear_state*s,const gh_event_clear_bindings*b,int*done){
 if(!s||!b||!done||!b->event||!b->event->initialized||!b->config||!b->config->initialized)return GH_EVENT_ARGUMENT;
 *done=0;s->frontier=0;if(s->event&&s->event!=b->event)return GH_EVENT_ARGUMENT;s->event=b->event;gh_game_event_status e;
 if(!s->stage){
  if(b->player_clear_0012c418)e=b->player_clear_0012c418(b->context,&s->frontier);
  else{e=gh_event_player_clear_0012c418(&s->player,b);s->frontier=s->player.frontier;}
  if(e)return e;s->stage=1;
 }
 gh_game_event_prefix*p=&b->event->fields;
 if(!(b->config->flags&1)){
  switch(p->phase){
   case 0:e=enemies(s,b);if(e)return e;p->phase++;break;
   case 6:if(b->config->flags&0x400)goto complete;/* fall through */
   case 1:case 2:case 3:case 4:case 5:case 7:p->phase++;break;
   case 8:{if(!b->room_objects)return fail(&s->frontier,0x1fef30);
    gh_object_status o=GH_OBJECT_OK;if(s->stage==1){o=gh_game_objects_release_nonpinned_001fef30(b->room_objects,b->release_sound_slot,b->context,&s->frontier);if(o){s->detail=o;if(!s->frontier)s->frontier=0x1fef30;return GH_EVENT_CALLEE;}s->stage=2;}
    o=gh_game_objects_reset_records_001ff028(b->room_objects,&s->frontier);if(o){s->detail=o;if(!s->frontier)s->frontier=0x1ff028;return GH_EVENT_CALLEE;}goto complete;}
   default:break;
  }
  memset(s,0,sizeof(*s));return GH_EVENT_OK;
 }
complete:
 p->unknown07=0;p->state=2;p->phase=0;p->unknown06=0;*done=1;memset(s,0,sizeof(*s));return GH_EVENT_OK;
}
