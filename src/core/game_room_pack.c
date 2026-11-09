#include "game_room_pack.h"
#include <stdio.h>
gh_room_pack_status gh_game_room_path(uint16_t room,char out[20]){
 if(!out)return GH_PACK_ARGUMENT;int n=snprintf(out,20,"st%x/r%x%02x.dat",(unsigned)(room>>8),(unsigned)(room>>8),(unsigned)(room&255));
 return n>0&&n<20?GH_PACK_OK:GH_PACK_ARGUMENT;
}
static gh_room_pack_status fail(gh_room_pack *s,gh_room_pack_status e,uint32_t entry,int detail){s->failed_entry=entry;s->detail=detail;return e;}
gh_room_pack_status gh_game_room_pack_load_0031cc48(gh_room_pack *s,uint16_t room){
 char path[20];size_t index;gh_disc_entry entry;gh_dvd_error e;
 if(!s||!s->dvd||!s->dvd->disc||!s->heap||!s->threads)return GH_PACK_ARGUMENT;
 if(s->allocation.pointer)return GH_PACK_BUSY;s->failed_entry=0;s->detail=0;s->loaded=0;
 if(gh_game_room_path(room,path)!=GH_PACK_OK)return GH_PACK_ARGUMENT;
 if(gh_disc_find(s->dvd->disc,path,&index)!=GH_DISC_OK||gh_disc_entry_at(s->dvd->disc,index,&entry)!=GH_DISC_OK)
  return fail(s,GH_PACK_DVD,0x200df8,GH_DVD_NOT_FOUND);
 s->file_bytes=(size_t)entry.size;s->room=room;
 s->job=gh_dvd_read_alloc_sized(s->dvd,path,&s->allocation,s->heap,0,&s->allocated_bytes,0,0);
 if(!s->job)return fail(s,GH_PACK_DVD,0x200df8,s->dvd->last_error);
 e=gh_dvd_check_wait_frame(s->dvd,s->job,s->threads);if(e)return fail(s,GH_PACK_DVD,0x201108,e);
 s->saved_pointer=s->allocation.pointer;s->saved_bytes=s->allocated_bytes;s->loaded=1;return GH_PACK_OK;
}
gh_room_pack_status gh_game_room_pack_release(gh_room_pack *s){
 if(!s||!s->heap)return GH_PACK_ARGUMENT;if(s->dvd&&s->job&&gh_dvd_check(s->dvd,s->job))return GH_PACK_BUSY;
 if(s->allocation.pointer&&gh_heap_release(s->heap,s->allocation)!=GH_MEM_OK)return GH_PACK_MEMORY;
 s->allocation=(gh_r207_allocation){0};s->saved_pointer=NULL;s->saved_bytes=0;s->file_bytes=0;s->allocated_bytes=0;s->job=0;s->loaded=0;return GH_PACK_OK;
}
gh_bundle_error gh_game_room_pack_bundle(const gh_room_pack *s,gh_bundle *out){
 if(!s||!s->loaded||!s->allocation.pointer||!out)return GH_BUNDLE_ARGUMENT;
 return gh_bundle_open(s->allocation.pointer,s->file_bytes,out);
}
