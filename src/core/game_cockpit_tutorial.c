#include "game_cockpit_tutorial.h"
#include <stdio.h>
#include <string.h>
static gh_tutorial_status fail(gh_cockpit_tutorial*s,uint32_t pc,gh_tutorial_status e){s->frontier=pc;return e;}
static gh_tutorial_status idfail(gh_cockpit_tutorial*s,uint32_t pc,gh_id_error e){s->id_error=e;return fail(s,pc,GH_TUTORIAL_ID);}
static int contains(const gh_cockpit_tutorial*s,const void*p){uintptr_t a=(uintptr_t)s->resource.pointer,v=(uintptr_t)p;return p&&s->resource.pointer&&v>=a&&v-a<s->image.size;}
gh_tutorial_status gh_cockpit_tutorial_room_init_00147028(gh_cockpit_tutorial*s,const gh_tutorial_bindings*b){
 static const char*const formats[]={"id/%s.idd","id/local/US/%s_US.idd","id/local/EN/%s_EN.idd","id/local/FR/%s_FR.idd","id/local/GE/%s_GE.idd","id/local/IT/%s_IT.idd","id/local/SP/%s_SP.idd"};
 if(!s||!b||!b->cockpit||!b->dvd||!b->dvd->disc||!b->room_heap||!b->id||!b->fonts||!b->packed||!b->work)return GH_TUTORIAL_ARGUMENT;
 gh_cockpit_tutorial_view view;gh_id_error e=gh_game_cockpit_get_tutorial(b->cockpit,&view);if(e)return idfail(s,0x147028,e);if(view.manager!=b->id)return GH_TUTORIAL_ARGUMENT;
 if(!s->bound){s->b=*b;s->bound=1;}else if(s->b.cockpit!=b->cockpit||s->b.dvd!=b->dvd||s->b.room_heap!=b->room_heap||s->b.id!=b->id||s->b.fonts!=b->fonts||s->b.packed!=b->packed||s->b.work!=b->work)return GH_TUTORIAL_ARGUMENT;
 if(s->complete)return GH_TUTORIAL_OK;
 if(s->stage==0){if(view.data_present){if(!s->resource.pointer||!s->pair_bound)return fail(s,0x147028,GH_TUTORIAL_BUSY);s->stage=5;}else{
   if(s->resource.pointer||b->id->pair[0x1d].bound)return fail(s,0x200cb0,GH_TUTORIAL_BUSY);
   size_t index;gh_disc_entry entry;gh_disc_error found=GH_DISC_NOT_FOUND;int lang=b->work->language;
   if((unsigned)lang<7){(void)snprintf(s->path,sizeof(s->path),formats[lang],"idTutorial");found=gh_disc_find(b->dvd->disc,s->path,&index);if(found!=GH_DISC_OK&&found!=GH_DISC_NOT_FOUND)return fail(s,0x2acfa8,GH_TUTORIAL_DVD);}
   if(found!=GH_DISC_OK){(void)snprintf(s->path,sizeof(s->path),formats[0],"idTutorial");if(gh_disc_find(b->dvd->disc,s->path,&index))return fail(s,0x2acfa8,GH_TUTORIAL_DVD);}
   if(gh_disc_entry_at(b->dvd->disc,index,&entry)||entry.size>SIZE_MAX)return fail(s,0x200cb0,GH_TUTORIAL_DVD);
   s->job=gh_dvd_read_alloc(b->dvd,s->path,&s->resource,b->room_heap,0,0,0,0);if(!s->job)return fail(s,0x200cb0,b->dvd->last_error==GH_DVD_MEMORY?GH_TUTORIAL_MEMORY:GH_TUTORIAL_DVD);
   s->image=(gh_bundle_entry){0};s->image.bytes=s->resource.pointer;s->image.size=(size_t)entry.size;
   e=gh_game_cockpit_tutorial_mark_resource(b->cockpit,s->job);if(e)return idfail(s,0x200cb0,e);s->stage=1;
  }}
 if(s->stage==1){while(gh_dvd_check(b->dvd,s->job))if(gh_dvd_pump(b->dvd)!=GH_DVD_OK)return fail(s,0x201160,GH_TUTORIAL_DVD);if(b->dvd->failed_id==s->job)return fail(s,0x201160,GH_TUTORIAL_DVD);s->stage=2;}
 if(s->stage==2){if(gh_id_set_data_002accf8(b->id,0x1d,&s->image))return fail(s,0x2accf8,GH_TUTORIAL_ID);s->pair_bound=1;s->stage=3;}
 if(s->stage==3){e=gh_game_cockpit_tutorial_begin_base(b->cockpit);if(e)return idfail(s,gh_game_cockpit_frontier(b->cockpit)?gh_game_cockpit_frontier(b->cockpit):0x2a9ea0,e);s->stage=4;}
 if(s->stage==4){e=gh_packed_message_bind(view.base,b->id,b->fonts,b->packed,b->language,b->work->language,0x1d,1,&s->packed_return);if(e)return idfail(s,0x2a9f30,e);s->stage=5;}
 if(s->stage==5){e=gh_game_cockpit_tutorial_finish_00147028(b->cockpit);if(e)return idfail(s,gh_game_cockpit_frontier(b->cockpit)?gh_game_cockpit_frontier(b->cockpit):0x2abbe8,e);s->complete=1;s->completed_calls++;s->stage=6;s->frontier=0;}
 return GH_TUTORIAL_OK;
}
gh_tutorial_status gh_cockpit_tutorial_next_call(gh_cockpit_tutorial*s){if(!s||!s->bound)return GH_TUTORIAL_ARGUMENT;if(!s->complete)return GH_TUTORIAL_BUSY;s->complete=0;s->stage=0;s->frontier=0;return GH_TUTORIAL_OK;}
gh_tutorial_status gh_cockpit_tutorial_release(gh_cockpit_tutorial*s){
 if(!s)return GH_TUTORIAL_ARGUMENT;if(!s->bound)return GH_TUTORIAL_OK;
 if(!s->resource.pointer){memset(s,0,sizeof(*s));return GH_TUTORIAL_OK;}
 if(s->job&&gh_dvd_check(s->b.dvd,s->job))if(!gh_dvd_cancel(s->b.dvd,s->job))return fail(s,0x201018,GH_TUTORIAL_DVD);
 gh_id_error e=gh_game_cockpit_tutorial_detach(s->b.cockpit);if(e)return idfail(s,0x2aa078,e);
 if(s->pair_bound&&contains(s,s->b.id->pair[0x1d].data.bytes))memset(&s->b.id->pair[0x1d],0,sizeof(s->b.id->pair[0x1d]));
 if(contains(s,s->b.fonts->slots[3].mst)||contains(s,s->b.fonts->slots[3].msa))memset(&s->b.fonts->slots[3],0,sizeof(s->b.fonts->slots[3]));
 if(contains(s,s->b.packed->cursor.bytes))memset(&s->b.packed->cursor,0,sizeof(s->b.packed->cursor));
 if(s->resource.pointer&&gh_heap_release(s->b.room_heap,s->resource))return fail(s,0x30ee40,GH_TUTORIAL_MEMORY);
 memset(s,0,sizeof(*s));return GH_TUTORIAL_OK;
}
