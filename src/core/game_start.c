/* Readable recovery from actual patched ELF; see game-start-evidence.json. */
#include "game_start.h"
#include <string.h>
static gh_game_status error(gh_game_assets *s,gh_game_status e,uint32_t entry,int detail){
 s->error=e;s->failed_entry=entry;s->detail=detail;return e;
}
gh_game_status gh_game_object_path(uint32_t id,char out[12]){
 static const char *const prefixes[]={"sc","pl","em","om","ol","ef","el"};
 static const char hex[]="0123456789abcdef";const char *p;
 if(!out || (id>>8)>=7)return GH_GAME_ARGUMENT;p=prefixes[id>>8];
 out[0]=out[3]=p[0];out[1]=out[4]=p[1];out[2]='/';
 out[5]=hex[(id>>4)&15];out[6]=hex[id&15];memcpy(out+7,".dat",5);return GH_GAME_OK;
}
static gh_game_status wait_job(gh_game_assets *s,uint32_t job){
 gh_dvd_error e=gh_dvd_check_wait_frame(s->dvd,job,s->threads);
 return e?error(s,GH_GAME_DISC,0x201108,(int)e):GH_GAME_OK;
}
static gh_game_status load(gh_game_assets *s,const char *path,gh_r207_allocation *out,
 size_t *size,uint32_t *job){
 size_t index;gh_disc_entry entry;uint32_t id;
 if(out->pointer)return error(s,GH_GAME_MEMORY,0x200cb0,GH_MEM_BUSY);
 if(gh_disc_find(s->dvd->disc,path,&index)!=GH_DISC_OK ||
  gh_disc_entry_at(s->dvd->disc,index,&entry)!=GH_DISC_OK)
  return error(s,GH_GAME_DISC,0x200cb0,GH_DVD_NOT_FOUND);
 id=gh_dvd_read_alloc(s->dvd,path,out,s->heap,*job,0,0,0);
 if(!id)return error(s,GH_GAME_DISC,0x200cb0,(int)s->dvd->last_error);
 *job=id;*size=(size_t)entry.size;return GH_GAME_OK;
}
/* Native synchronous equivalent of200B88 into a caller-owned fixed buffer.
 * Keeps requested explicit job token; no pending DMA is fabricated. Its
 * following201108 still yields the original unconditional one extra visit. */
static gh_game_status load_fixed(gh_game_assets *s,uint32_t object,unsigned slot,
 uint32_t *job){
 char path[12];gh_disc_entry e;size_t index;uint64_t bytes;
 if(gh_game_object_path(object,path)!=GH_GAME_OK)return error(s,GH_GAME_ARGUMENT,0x307918,0);
 if(gh_disc_find(s->dvd->disc,path,&index)!=GH_DISC_OK || gh_disc_entry_at(s->dvd->disc,index,&e)!=GH_DISC_OK)
  return error(s,GH_GAME_DISC,0x200b88,GH_DVD_NOT_FOUND);
 bytes=(e.size+2047)&~UINT64_C(2047);
 if(!s->objects[slot].pointer || bytes>s->object_sizes[slot])return error(s,GH_GAME_BOUNDS,0x200b88,0);
 if(gh_disc_read_blocks(s->dvd->disc,index,0,s->objects[slot].pointer,(size_t)(bytes>>11))!=GH_DISC_OK)
  return error(s,GH_GAME_DISC,0x200b88,GH_DVD_IO);
 s->object_file_sizes[slot]=(size_t)e.size;
 if(!*job){++s->dvd->id_counter;if(!s->dvd->id_counter)s->dvd->id_counter=1;*job=s->dvd->id_counter;}
 return GH_GAME_OK;
}
#define TRY(expr) do{gh_game_status e=(expr);if(e)return e;}while(0)
gh_game_status gh_game_assets_reload_player_0030d8e0(gh_game_assets *s,uint8_t costume)
{
 /* The boot and case3 dispatch tables choose the same player file. */
 static const uint16_t models[]={0x100,0x102,0x105,0x10f,0x107,0x110,0x10e,0x111};
 static const uint16_t motions[]={0x101,0x101,0x106,0x106,0x108,0x108,0x112,0x112};
 if(!s||!s->dvd||!s->dvd->disc||!s->threads)return GH_GAME_ARGUMENT;
 s->error=GH_GAME_OK;s->failed_entry=0;s->detail=0;
 unsigned index=costume<8?costume:0;
 uint32_t job=0;
 TRY(load_fixed(s,models[index],0,&job));
 TRY(wait_job(s,job));
 /* The motion read reuses the model's original job token. */
 TRY(load_fixed(s,motions[index],1,&job));
 return wait_job(s,job);
}

gh_game_status gh_game_assets_0030d5c8(gh_game_assets *s,uint8_t costume){
 /* Original records at default pointer3C3CC4->4505D8, terminatingFFFF.
  * These are recovered logical object IDs/destination slots, not file bytes. */
 static const uint16_t ids[]={0x361,0x362,0x363,0x364,0x365,0x366,0x368,0x376,0x377,0x3b7,
 0x379,0x37b,0x37c,0x380,0x36d,0x375,0x3be,0x3c1,0x3c6,0x3c7,0x3c8,0x3d0,0x3d1,
 0x3d2,0x3d6,0x3d7,0x3d8,0x3d9,0x3da,0x3db,0x3dc,0x3c2,0x3c3,0x3c4,0x3df,0x3e0,
 0x3e1,0x3de,0x3e2,0x3e3,0x23f};
 static const uint8_t slots[]={26,27,28,29,30,31,32,34,35,33,36,37,38,39,40,41,2,4,23,24,
 3,10,11,25,12,13,14,16,17,18,19,20,21,22,9,5,6,7,8,15,42};
 uint32_t job=0,flags;char path[12];
 if(!s || !s->work || !s->dvd || !s->dvd->disc || !s->heap || !s->threads)return GH_GAME_ARGUMENT;
 s->error=GH_GAME_OK;s->failed_entry=0;s->detail=0;
 s->work->flags|=0x4000;
 if(!(s->work->flags&0x40000)){
  if(!s->sound_startup)return error(s,GH_GAME_UNAVAILABLE,0x2ca798,0);
  {int e=s->sound_startup(s->sound_context);if(e)return error(s,GH_GAME_UNAVAILABLE,0x2ca798,e);}
 }
 flags=s->work->flags;s->work->flags&=~0x40000u;
 if(!(flags&0x2000)){
  TRY(load(s,"etc/core.dat",&s->core,&s->core_size,&job));
  TRY(wait_job(s,job));TRY(wait_job(s,job));
  TRY(load(s,"etc/debugres/RoomJmpTbl.dat",&s->room_jump,&s->room_jump_size,&job));
 }
 flags=s->work->flags;s->work->flags|=0x2000;
 if(!(flags&0x400)){
  for(unsigned i=0;i<sizeof(ids)/sizeof(ids[0]);i++){
   TRY(wait_job(s,job));TRY(gh_game_object_path(ids[i],path));
   TRY(load(s,path,&s->objects[slots[i]],&s->object_sizes[slots[i]],&job));s->object_file_sizes[slots[i]]=s->object_sizes[slots[i]];
  }
  TRY(wait_job(s,job));
 }
 if(!(s->work->flags&0x10000)){
  for(unsigned i=0;i<2;i++){
   size_t size=i?0x19800:0x1e6800;gh_mem_error e;
   if(s->objects[i].pointer)return error(s,GH_GAME_MEMORY,0x30ee08,GH_MEM_BUSY);
   e=gh_heap_allocate(s->heap,size,0x40,&s->objects[i]);
   if(e)return error(s,GH_GAME_MEMORY,0x30ee08,(int)e);s->object_sizes[i]=size;
  }
  TRY(gh_game_assets_reload_player_0030d8e0(s,costume));
 }
 s->work->flags=(s->work->flags|0x10400u)&~0x4000u;return GH_GAME_OK;
}
gh_game_status gh_game_assets_release(gh_game_assets *s){
 if(!s || !s->heap)return GH_GAME_ARGUMENT;
 if(s->dvd)for(unsigned i=0;i<GH_DVD_JOB_COUNT;i++)if(s->dvd->jobs[i].state)return GH_GAME_THREAD;
 if(s->core.pointer && gh_heap_release(s->heap,s->core)!=GH_MEM_OK)return GH_GAME_MEMORY;
 s->core=(gh_r207_allocation){0};
 if(s->room_jump.pointer && gh_heap_release(s->heap,s->room_jump)!=GH_MEM_OK)return GH_GAME_MEMORY;
 s->room_jump=(gh_r207_allocation){0};
 for(unsigned i=0;i<43;i++){if(s->objects[i].pointer && gh_heap_release(s->heap,s->objects[i])!=GH_MEM_OK)return GH_GAME_MEMORY;s->objects[i]=(gh_r207_allocation){0};s->object_file_sizes[i]=0;}
 return GH_GAME_OK;
}
static gh_game_status loop_call(gh_game_loop *s,const gh_game_platform *p,gh_game_operation fn,uint32_t entry){
 s->failed_entry=entry;if(!fn)return GH_GAME_UNAVAILABLE;
 s->platform_error=fn(p->context,s);return s->platform_error?GH_GAME_UNAVAILABLE:GH_GAME_OK;
}
gh_game_status gh_game_main_002a5998(gh_game_loop *s,const gh_game_platform *p){
 static const uint32_t states[]={0x2a5b18,0x2a5bb8,0x2a5d40,0x2a5ec0,0x2a6690,0x2a67d0};
 if(!s || !s->work || !s->threads || !p)return GH_GAME_ARGUMENT;
 s->work->flags|=0x80000000u;memset(s->game_state,0,4); /*2A5B00*/
 while(s->work->flags&0x80000000u){
  TRY(loop_call(s,p,p->dvd_cover,0x2a6ca8));
  TRY(loop_call(s,p,p->modal_input,0x2a6da8));
  if(s->game_state[0]<6)TRY(loop_call(s,p,p->states[s->game_state[0]],states[s->game_state[0]]));
  if(gh_task_threads_sleep(s->threads,1)!=GH_THREAD_OK){s->failed_entry=0x2d5aa0;return GH_GAME_THREAD;}
 }
 s->failed_entry=0;return GH_GAME_OK; /*native wrapper implements final task exit*/
}
