#include "game_event_module.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define C(x) do{if(!(x)){fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x);exit(1);}}while(0)
int main(int argc,char**argv){
 C(argc==2);gh_disc*disc=NULL;C(!gh_disc_open(argv[1],&disc));gh_heap*heap=NULL;C(!gh_heap_create(1<<20,&heap));
 gh_dvd_jobs dvd;gh_dvd_jobs_init(&dvd,disc);gh_boot_work work={0};gh_game_event event={.initialized=1};gh_event_module state={0};
 event.fields.flags=0x80;event.fields.load_handle=123;event.fields.resource_token=456;
 C(!gh_game_event_module_begin_00297378(&state,&event,NULL,NULL,NULL)&&event.fields.load_handle==123&&event.fields.resource_token==456&&!state.requested&&!gh_heap_used(heap));
 event.fields.flags=0;uint32_t debug=0x400;memcpy((uint8_t*)&work+0x594,&debug,4);
 C(gh_game_event_module_begin_00297378(&state,&event,&work,&dvd,heap)==GH_EVENT_UNSUPPORTED&&state.frontier==0x200cb0&&!event.fields.resource_token&&!state.requested);
 debug=0;memcpy((uint8_t*)&work+0x594,&debug,4);C(!gh_game_event_module_begin_00297378(&state,&event,&work,&dvd,heap)&&state.requested&&state.job==event.fields.load_handle&&state.allocation.pointer&&state.file_bytes==80431&&state.allocated_bytes==81920&&!event.fields.resource_token);
 int done=1;C(!gh_game_event_module_poll_00297428(&state,&done)&&!done&&!state.loaded);
 unsigned steps=0;while(gh_dvd_check(&dvd,state.job)){C(steps++<10&&!gh_dvd_pump(&dvd));}C(!gh_game_event_module_poll_00297428(&state,&done)&&done&&state.loaded&&!memcmp(state.allocation.pointer,"SNR2",4));
 size_t index;gh_disc_entry entry;C(!gh_disc_find(disc,"dll/debugoptimize/event.rel",&index)&&!gh_disc_entry_at(disc,index,&entry));void*reference=malloc((size_t)entry.size);C(reference&&!gh_disc_read(disc,index,0,reference,(size_t)entry.size)&&!memcmp(reference,state.allocation.pointer,(size_t)entry.size));free(reference);
 uint32_t job=state.job;void*allocation=state.allocation.pointer;C(gh_game_event_module_begin_00297378(&state,&event,&work,&dvd,heap)==GH_EVENT_UNSUPPORTED&&state.job==job&&state.allocation.pointer==allocation);
 event.fields.load_handle++;C(gh_game_event_module_poll_00297428(&state,&done)==GH_EVENT_UNSUPPORTED&&state.frontier==0x297428);event.fields.load_handle=job;
 C(!gh_game_event_module_abort(&state)&&!gh_heap_used(heap)&&!state.allocation.pointer&&!state.requested);
 C(!gh_game_event_module_begin_00297378(&state,&event,&work,&dvd,heap)&&gh_dvd_check(&dvd,state.job));job=state.job;C(!gh_game_event_module_abort(&state)&&!gh_dvd_check(&dvd,job)&&!gh_heap_used(heap));
 gh_event_module zero={.event=&event,.dvd=&dvd};event.fields.load_handle=0;C(!gh_game_event_module_poll_00297428(&zero,&done)&&done&&!zero.loaded);
 C(!gh_heap_destroy(heap));gh_disc_close(disc);puts("PASS real80431B Event.rel asyncDVD/FSTalias/sectorpadding/fullindependentread/poll0/borrowidentity/cancel+abort; no RELlink/constructors/playbackack");return 0;
}
