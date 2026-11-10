#include "game_event_resource.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define C(x) do{if(!(x)){fprintf(stderr,"FAIL%d %s\n",__LINE__,#x);exit(1);}}while(0)
int main(int argc,char**argv){
 C(argc==2);gh_disc*disc;C(!gh_disc_open(argv[1],&disc));gh_heap*heap;C(!gh_heap_create(2*1024*1024,&heap));
 char path[32];C(!gh_game_event_resource_path_00297540(10,path)&&!strcmp(path,"event/ev0a.dat"));
 C(!gh_game_event_resource_path_00297540(0x100,path)&&!strcmp(path,"event/ev100.dat"));
 C(!gh_game_event_resource_path_00297540(-1,path)&&!strcmp(path,"event/evffffffff.dat"));
 gh_dvd_jobs dvd;gh_dvd_jobs_init(&dvd,disc);gh_game_event event={.initialized=1};event.fields.data_number=1;event.fields.flags=0x81;event.fields.text_token=123;
 gh_event_resource state={0};C(!gh_game_event_resource_begin_00297540(&state,&event,&dvd,heap));
 C(state.requested&&!state.loaded&&state.job==event.fields.load_handle&&state.allocation.pointer&&state.file_bytes==873872&&state.allocated_bytes==874496&&event.fields.flags==0x181&&!event.fields.text_token);
 int done=1;C(!gh_game_event_resource_poll_002975d0(&state,&done)&&!done);
 C(gh_game_event_resource_begin_00297540(&state,&event,&dvd,heap)==GH_EVENT_UNSUPPORTED&&state.frontier==0x297540);
 event.fields.data_number=2;C(gh_game_event_resource_poll_002975d0(&state,&done)==GH_EVENT_UNSUPPORTED&&state.frontier==0x2975d0);event.fields.data_number=1;
 unsigned pumps=0;while(gh_dvd_check(&dvd,state.job))C(pumps++<5&&!gh_dvd_pump(&dvd));
 C(!gh_game_event_resource_poll_002975d0(&state,&done)&&done&&state.loaded);
 size_t index;gh_disc_entry entry;C(!gh_disc_find(disc,"event/ev01.dat",&index)&&!gh_disc_entry_at(disc,index,&entry));
 uint8_t*reference=malloc((size_t)entry.size);C(reference&&!gh_disc_read(disc,index,0,reference,(size_t)entry.size)&&!memcmp(reference,state.allocation.pointer,(size_t)entry.size));free(reference);
 const uint8_t prefix[]={1,10,9,0,0x60,0,0,0,0xe0,0,0,0,0x30,1,0,0};C(!memcmp(state.allocation.pointer,prefix,16));
 uint32_t flags=event.fields.flags;C(!gh_game_event_resource_abort(&state)&&!gh_heap_used(heap)&&event.fields.flags==flags);
 C(!gh_game_event_resource_begin_00297540(&state,&event,&dvd,heap));uint32_t job=state.job;C(gh_dvd_check(&dvd,job)&&!gh_game_event_resource_abort(&state)&&!gh_dvd_check(&dvd,job)&&!gh_heap_used(heap));
 event.fields.data_number=-1;event.fields.flags=0;C(!gh_game_event_resource_begin_00297540(&state,&event,&dvd,heap)&&state.requested&&!state.job&&!state.allocation.pointer&&!event.fields.load_handle&&!event.fields.flags&&dvd.last_error==GH_DVD_NOT_FOUND);
 C(!gh_game_event_resource_poll_002975d0(&state,&done)&&done&&!state.loaded&&!gh_game_event_resource_abort(&state));
 C(!gh_heap_destroy(heap));gh_disc_close(disc);
 puts("PASS actual873872B ev01 customEvent data asyncDVD/hexpath/fullindependentread/pending/identity/cancel/handle0-missing; no parser/movie/playback acknowledgement");return 0;
}
