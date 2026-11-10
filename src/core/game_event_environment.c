#include "game_event_environment.h"
#include <string.h>
static uint32_t word(const uint8_t*p){return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static unsigned half(const uint8_t*p){return p[0]|(unsigned)p[1]<<8;}
static gh_game_event_status span(const void *data,size_t bytes,uint32_t address,size_t width,
 const uint8_t **file,uint8_t *cold){
 const uint8_t*b=data;if(!b)return GH_EVENT_ARGUMENT;
 if(bytes<52||memcmp(b,"\177ELF\1\1\1",7)||half(b+18)!=8)return GH_EVENT_FORMAT;
 size_t table=word(b+28);unsigned stride=half(b+42),count=half(b+44);
 if(stride<32||count>128||table>bytes||count>(bytes-table)/stride)return GH_EVENT_BOUNDS;
 const uint8_t*found=NULL;unsigned matches=0;
 for(unsigned i=0;i<count;i++){
  const uint8_t*p=b+table+(size_t)i*stride;uint32_t base=word(p+8),files=word(p+16),memory=word(p+20),offset=word(p+4);
  if(word(p)!=1||base>address||(uint64_t)address+width>(uint64_t)base+memory)continue;
  if(++matches>1||files>memory||offset>bytes||files>bytes-offset)return GH_EVENT_BOUNDS;
  size_t delta=address-base;
  if(delta<=files&&width<=files-delta)found=b+offset+delta;
  if(cold){memset(cold,0,width);if(delta<files){size_t take=files-delta;if(take>width)take=width;memcpy(cold,b+offset+delta,take);}}
 }
 if(matches!=1||(!cold&&!found))return GH_EVENT_BOUNDS;
 if(file)*file=found;return GH_EVENT_OK;
}
gh_game_event_status gh_game_event_config_from_elf(gh_game_event_config*s,const void*elf,size_t bytes){
 if(!s||s->initialized)return GH_EVENT_ARGUMENT;uint8_t raw[8];
 gh_game_event_status e=span(elf,bytes,0x586b30,8,NULL,raw);if(e)return e;
 gh_game_event_config next={.record_address=word(raw),.flags=word(raw+4),.initialized=1};
 if(next.record_address){e=span(elf,bytes,next.record_address,12,&next.record,NULL);if(e)return e;}
 *s=next;return GH_EVENT_OK;
}
gh_game_event_status gh_game_event_config_set_00299320(gh_game_event_config*s,int32_t number,const void*elf,size_t bytes){
 if(!s||!s->initialized)return GH_EVENT_ARGUMENT;
 uint32_t address=0x3c0550u+(uint32_t)number*12u;const uint8_t*record=NULL;
 gh_game_event_status e=span(elf,bytes,address,12,&record,NULL);if(e)return e;
 s->record=record;s->record_address=address;s->flags=word(record+4);return GH_EVENT_OK;
}
gh_game_event_status gh_game_event_reset_00297660(gh_game_event*s){
 if(!s||!s->initialized)return GH_EVENT_ARGUMENT;
 s->fields.text_token=0;s->fields.state=0;s->fields.phase=0;s->fields.unknown06=0;s->fields.unknown07=0;
 s->fields.flags=0;s->fields.data_number=0;return GH_EVENT_OK;
}
gh_game_event_status gh_game_event_init_environment_002975f8(gh_game_event*s,gh_game_event_config*config,int32_t number,const void*elf,size_t bytes){
 if(!s||!s->initialized||!config||!config->initialized)return GH_EVENT_ARGUMENT;
 if(s->fields.flags&2)return GH_EVENT_OK;
 gh_game_event_config next=*config;gh_game_event_status e=gh_game_event_config_set_00299320(&next,number,elf,bytes);if(e)return e;
 e=gh_game_event_reset_00297660(s);if(e)return e;s->fields.data_number=number;*config=next;s->fields.flags|=2;
 return GH_EVENT_OK;
}
gh_game_event_status gh_game_event_play_start_00296458(const gh_event_play_start_bindings*b,int32_t number,uint32_t*frontier){
 if(frontier)*frontier=0;if(!b||!b->event||!b->config||!b->work)return GH_EVENT_ARGUMENT;
 gh_game_event_status e=gh_game_event_init_environment_002975f8(b->event,b->config,number,b->elf,b->elf_bytes);
 if(e){if(frontier)*frontier=0x2975f8;return e;}
 uint8_t*w=(uint8_t*)b->work;uint32_t flags=word(w+0x5e4)|0x20000000u;memcpy(w+0x5e4,&flags,4);
 gh_game_event_prefix*s=&b->event->fields;s->skip_ok=0;s->flags|=1;
 const gh_title_player_view*p=b->player;
 if(!p||!p->bytes){if(frontier)*frontier=0x2db6b0;return GH_EVENT_UNSUPPORTED;}
 uintptr_t base=(uintptr_t)p->bytes,pos=(uintptr_t)p->alias_f0;
 if(p->size<0x550||pos<base||pos-base>p->size||12>p->size-(pos-base))return GH_EVENT_BOUNDS;
 memcpy(s->position_bits,p->alias_f0,12);memcpy(&s->angle_bits,p->bytes+0x104,4);
 uint32_t timer=0x42700000;memcpy(p->bytes+0x54c,&timer,4);
 s->unknown07=s->state=s->phase=s->unknown06=0;
 if(b->config->flags&8){
  if(!b->fade||!b->display||!b->display->initialized){if(frontier)*frontier=0x307208;return GH_EVENT_UNSUPPORTED;}
  gh_screen_fade_context c={b->display->screen_width,b->display->gs_y_origin,word(w+0x564),word(w+0x5e0)};
  gh_fade_error f=gh_screen_fade_start_00307208(b->fade,&c,0,10,0,0xff000000,0xff000000,0);
  if(f){if(frontier)*frontier=0x307208;return GH_EVENT_CALLEE;}
 }
 return GH_EVENT_OK;
}
