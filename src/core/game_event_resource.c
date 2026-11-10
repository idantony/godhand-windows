#include "game_event_resource.h"
#include <stdio.h>
#include <string.h>
static gh_game_event_status fail(gh_event_resource*s,uint32_t pc,int detail,gh_game_event_status e){s->frontier=pc;s->detail=detail;return e;}

gh_game_event_status gh_game_event_resource_path_00297540(int32_t number,char out[32]){
 if(!out)return GH_EVENT_ARGUMENT;
 int count=snprintf(out,32,"event/ev%02x.dat",(unsigned)(uint32_t)number);
 return count<0||count>=32?GH_EVENT_BOUNDS:GH_EVENT_OK;
}

gh_game_event_status gh_game_event_resource_begin_00297540(gh_event_resource*s,
 gh_game_event*event,gh_dvd_jobs*dvd,gh_heap*heap){
 if(!s||!event||!event->initialized||!dvd||!dvd->disc||!heap)return GH_EVENT_ARGUMENT;
 s->frontier=0;s->detail=0;
 if(s->requested||s->allocation.pointer)return fail(s,0x297540,0,GH_EVENT_UNSUPPORTED);
 char path[32];gh_game_event_status e=gh_game_event_resource_path_00297540(event->fields.data_number,path);if(e)return e;
 size_t index;gh_disc_entry entry;gh_disc_error de=gh_disc_find(dvd->disc,path,&index);
 if(de!=GH_DISC_OK&&de!=GH_DISC_NOT_FOUND)return fail(s,0x200cb0,de,GH_EVENT_CALLEE);
 if(!de&&(gh_disc_entry_at(dvd->disc,index,&entry)||entry.size>SIZE_MAX))return fail(s,0x200cb0,0,GH_EVENT_BOUNDS);
 s->event=event;s->dvd=dvd;s->heap=heap;s->data_number=event->fields.data_number;
 s->file_bytes=de?0:(size_t)entry.size;event->fields.text_token=0;
 s->job=gh_dvd_read_alloc_sized(dvd,path,&s->allocation,heap,0,&s->allocated_bytes,0,0);
 event->fields.load_handle=s->job;
 if(s->allocation.pointer)event->fields.flags|=0x100;
 if(!s->job&&dvd->last_error!=GH_DVD_NOT_FOUND)return fail(s,0x200cb0,dvd->last_error,GH_EVENT_CALLEE);
 s->requested=1;return GH_EVENT_OK;
}

gh_game_event_status gh_game_event_resource_poll_002975d0(gh_event_resource*s,int*done){
 if(!s||!done||!s->event||!s->event->initialized||!s->dvd)return GH_EVENT_ARGUMENT;
 s->frontier=0;s->detail=0;
 if(s->requested&&(s->event->fields.load_handle!=s->job||s->event->fields.data_number!=s->data_number))return fail(s,0x2975d0,0,GH_EVENT_UNSUPPORTED);
 *done=gh_dvd_check(s->dvd,s->event->fields.load_handle)==0;
 if(s->job&&s->dvd->failed_id==s->job)return fail(s,0x201098,s->dvd->last_error,GH_EVENT_CALLEE);
 if(*done&&s->requested&&s->allocation.pointer)s->loaded=1;
 return GH_EVENT_OK;
}

gh_game_event_status gh_game_event_resource_abort(gh_event_resource*s){
 if(!s)return GH_EVENT_ARGUMENT;
 if(s->job&&s->dvd&&gh_dvd_check(s->dvd,s->job)&&!gh_dvd_cancel(s->dvd,s->job))return fail(s,0x201018,0,GH_EVENT_CALLEE);
 if(s->allocation.pointer){if(!s->heap)return GH_EVENT_ARGUMENT;gh_mem_error e=gh_heap_release(s->heap,s->allocation);if(e)return fail(s,0x30ee08,e,GH_EVENT_CALLEE);}
 memset(s,0,sizeof(*s));return GH_EVENT_OK;
}
