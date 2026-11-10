#include "game_event_module.h"
#include <string.h>
static uint32_t word(const uint8_t*p){uint32_t n;memcpy(&n,p,4);return n;}
static gh_game_event_status fail(gh_event_module*s,uint32_t pc,int detail,gh_game_event_status e){s->frontier=pc;s->detail=detail;return e;}
gh_game_event_status gh_game_event_module_begin_00297378(gh_event_module*s,gh_game_event*event,
 const gh_boot_work*work,gh_dvd_jobs*dvd,gh_heap*heap){
 if(!s||!event||!event->initialized)return GH_EVENT_ARGUMENT;s->frontier=0;s->detail=0;
 if(event->fields.flags&0x80)return GH_EVENT_OK;
 if(!work||!dvd||!dvd->disc||!heap)return GH_EVENT_ARGUMENT;
 if(s->requested||s->allocation.pointer)return fail(s,0x297378,0,GH_EVENT_UNSUPPORTED);
 event->fields.resource_token=0;
 if(word((const uint8_t*)work+0x594)&0x400)return fail(s,0x200cb0,0,GH_EVENT_UNSUPPORTED);
 const char*path="dll/debugoptimize/event.rel";size_t index;gh_disc_entry entry;
 if(gh_disc_find(dvd->disc,path,&index)||gh_disc_entry_at(dvd->disc,index,&entry)||entry.size>SIZE_MAX)return fail(s,0x200cb0,0,GH_EVENT_BOUNDS);
 s->event=event;s->dvd=dvd;s->heap=heap;s->file_bytes=(size_t)entry.size;
 s->job=gh_dvd_read_alloc_sized(dvd,path,&s->allocation,heap,0,&s->allocated_bytes,0,0);
 event->fields.load_handle=s->job;
 if(!s->job)return fail(s,0x200cb0,dvd->last_error,GH_EVENT_CALLEE);
 s->requested=1;return GH_EVENT_OK;
}
gh_game_event_status gh_game_event_module_poll_00297428(gh_event_module*s,int*done){
 if(!s||!done||!s->event||!s->event->initialized||!s->dvd)return GH_EVENT_ARGUMENT;
 s->frontier=0;s->detail=0;*done=gh_dvd_check(s->dvd,s->event->fields.load_handle)==0;
 if(s->requested&&s->event->fields.load_handle!=s->job)return fail(s,0x297428,0,GH_EVENT_UNSUPPORTED);
 if(s->requested&&s->dvd->failed_id==s->job)return fail(s,0x201098,s->dvd->last_error,GH_EVENT_CALLEE);
 if(*done&&s->requested)s->loaded=1;return GH_EVENT_OK;
}
gh_game_event_status gh_game_event_module_abort(gh_event_module*s){
 if(!s)return GH_EVENT_ARGUMENT;
 if(s->job&&s->dvd&&gh_dvd_check(s->dvd,s->job))
  if(!gh_dvd_cancel(s->dvd,s->job))return fail(s,0x201018,0,GH_EVENT_CALLEE);
 if(s->allocation.pointer){if(!s->heap)return GH_EVENT_ARGUMENT;
  gh_mem_error e=gh_heap_release(s->heap,s->allocation);if(e)return fail(s,0x30ee08,e,GH_EVENT_CALLEE);}
 memset(s,0,sizeof(*s));return GH_EVENT_OK;
}
