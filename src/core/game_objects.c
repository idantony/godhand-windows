#include "game_objects.h"
#include <stdlib.h>
#include <string.h>
struct gh_game_objects {
 uint32_t release_cursor,release_sound;uint8_t releasing;
 gh_stage_slots *slots;uint8_t records[80][0x5c];
 struct {const uint8_t *bytes;size_t size,file_extent;uint8_t file_extent_known;
  gh_r207_allocation allocation;gh_heap *heap;gh_dvd_jobs *dvd;uint32_t job;} source[80];
};
static uint32_t u32(const uint8_t *p){return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static void w32(uint8_t *p,uint32_t n){for(unsigned i=0;i<4;i++)p[i]=(uint8_t)(n>>(8*i));}
static int manager_valid(const gh_game_objects *s){return s&&s->slots&&s->slots->configured&&s->slots->count<=80&&s->slots->records==&s->records[0][0]&&s->slots->bytes>=(size_t)s->slots->count*0x5c;}
static int valid(const gh_game_objects *s,unsigned i){const uint8_t *p=s->slots->records+(size_t)i*0x5c;
 /* Actual1FF0D8: nonzero useCount, else (!(flags&4) && data!=NULL). */
 return u32(p+4)!=0 || (!(u32(p+8)&4)&&u32(p+0x10)!=0);
}
static int find(const gh_game_objects *s,uint32_t kind){for(uint32_t n=s->slots->count;n;n--)if(valid(s,n-1)&&u32(s->slots->records+(size_t)(n-1)*0x5c+0x20)==kind)return (int)(n-1);return -1;}
static uint32_t file_kind(uint32_t kind){uint32_t prefix=kind&0xff00;return kind&&prefix!=0x500&&prefix<=0x600?kind|0x40000000:UINT32_MAX;}
gh_object_status gh_game_objects_create(gh_stage_slots *slots,int room,gh_game_objects **out){
 gh_game_objects *s;if(!slots||!out||(room!=0&&room!=1)||slots->configured)return GH_OBJECT_ARGUMENT;*out=NULL;
 s=calloc(1,sizeof(*s));if(!s)return GH_OBJECT_MEMORY;s->slots=slots;slots->records=&s->records[0][0];slots->count=room?32:80;
 slots->bytes=(size_t)slots->count*0x5c;slots->heap_handle=NULL;slots->configured=1;*out=s;return GH_OBJECT_OK;
}
void gh_game_objects_destroy(gh_game_objects *s){if(!s)return;
 /* Native abort only: no async native DMA, cancel before actualownedfree.
  * Managers must be destroyed before their borrowed DVD/heap owners. */
 for(unsigned i=0;i<80;i++)if(s->source[i].allocation.pointer){if(s->source[i].dvd&&s->source[i].job)gh_dvd_cancel(s->source[i].dvd,s->source[i].job);(void)gh_heap_release(s->source[i].heap,s->source[i].allocation);}
if(s->slots->records==&s->records[0][0])memset(s->slots,0,sizeof(*s->slots));free(s);}
gh_object_status gh_game_objects_add_pinned_001fece0(gh_game_objects *s,uint32_t kind,const void *bytes,size_t size,int *original){
 int old,index=-1;uint8_t *p;
 if(original)*original=0;if(!manager_valid(s)||!bytes||!size)return GH_OBJECT_ARGUMENT;
 old=find(s,kind);if(old>=0){
  if(s->source[old].allocation.pointer)return GH_OBJECT_BUSY;
  gh_stage_slots one={s->slots->records+(size_t)old*0x5c,0x5c,1,NULL,1};
  if(gh_stage_slots_reset(&one,NULL)!=GH_STAGE_OK)return GH_OBJECT_BOUNDS;
  s->source[old].bytes=NULL;s->source[old].size=0;s->source[old].file_extent=0;s->source[old].file_extent_known=0;
 }
 for(unsigned i=0;i<s->slots->count;i++)if(!s->source[i].allocation.pointer&&!valid(s,i)&&!(u32(s->slots->records+(size_t)i*0x5c+8)&4)){index=(int)i;break;}
 if(index<0)return GH_OBJECT_CAPACITY;p=s->slots->records+(size_t)index*0x5c;
 /*1FF470 order: useCount1; file-kind mapping; data; ID; file; state2.
  * Native+10 is an owner-local handle(index+1), never a truncated pointer.
  * A canonical1FF028 reset clears it, so stale borrowed views are inactive. */
 w32(p+4,1);w32(p+0x10,(uint32_t)index+1);s->source[index].bytes=bytes;s->source[index].size=size;s->source[index].file_extent=0;s->source[index].file_extent_known=0;
 w32(p+0x20,kind);w32(p+0xc,file_kind(kind));w32(p,2);w32(p+4,0xffffff9c);
 if(original)*original=1;return GH_OBJECT_OK;
}
gh_object_status gh_game_objects_find(const gh_game_objects *s,uint32_t kind,gh_object_view *out){
 if(!manager_valid(s)||!out)return GH_OBJECT_ARGUMENT;int n=find(s,kind);if(n<0)return GH_OBJECT_MISSING;
 const uint8_t *p=s->slots->records+(size_t)n*0x5c;
 if(u32(p+0x10)!=(uint32_t)n+1)return GH_OBJECT_BOUNDS;
 out->bytes=s->source[n].bytes;out->size=s->source[n].size;out->kind=u32(p+0x20);out->state=u32(p);out->flags=u32(p+8);out->file_kind=u32(p+0xc);
 out->file_extent=s->source[n].file_extent;out->file_extent_known=s->source[n].file_extent_known;
 uint32_t count=u32(p+4);out->use_count=count<=INT32_MAX?(int32_t)count:(int32_t)((int64_t)count-4294967296LL);return GH_OBJECT_OK;
}
gh_object_status gh_game_objects_register_startup(gh_game_objects *s,gh_game_assets *assets){
 static const uint16_t ids[]={0x361,0x362,0x363,0x364,0x365,0x366,0x368,0x376,0x377,0x3b7,
 0x379,0x37b,0x37c,0x380,0x36d,0x375,0x3be,0x3c1,0x3c6,0x3c7,0x3c8,0x3d0,0x3d1,
 0x3d2,0x3d6,0x3d7,0x3d8,0x3d9,0x3da,0x3db,0x3dc,0x3c2,0x3c3,0x3c4,0x3df,0x3e0,
 0x3e1,0x3de,0x3e2,0x3e3,0x23f};
 static const uint8_t slots[]={26,27,28,29,30,31,32,34,35,33,36,37,38,39,40,41,2,4,23,24,
 3,10,11,25,12,13,14,16,17,18,19,20,21,22,9,5,6,7,8,15,42};
 if(!manager_valid(s)||!assets||!assets->work||s->slots->count!=80)return GH_OBJECT_ARGUMENT;
 if(!(assets->work->flags&0x400))return GH_OBJECT_NOT_READY;
 for(unsigned i=0;i<43;i++){
  unsigned slot=i<2?i:slots[i-2];uint32_t id=i<2?0x100+i:ids[i-2];int original;
  gh_object_status e=gh_game_objects_add_pinned_001fece0(s,id,assets->objects[slot].pointer,assets->object_sizes[slot],&original);
  if(e)return e;
  if(assets->object_file_sizes[slot]){e=gh_game_objects_set_file_extent(s,id,assets->objects[slot].pointer,assets->object_file_sizes[slot]);if(e)return e;}
 }
 assets->work->flags|=0x80;return GH_OBJECT_OK;
}

static int has_dependency(const uint8_t*p,uint32_t key){for(unsigned i=0;i<4;i++)if(u32(p+0x4c+i*4)==key)return 1;return 0;}
/* Actual1FF710 counts pre-existingFFFF only; removedmatches donotcountasempty.
 * Multiplematchingdependencies thereforedoNOTreturnlast-reference true. */
static int remove_dependency(uint8_t*p,uint32_t key){unsigned empty=0;int found=0;for(unsigned i=0;i<4;i++){uint8_t*q=p+0x4c+i*4;uint32_t old=u32(q);if(old==key){w32(q,0xffff);found=1;}else if(old==0xffff)empty++;}return empty==3&&found;}
gh_object_status gh_game_objects_update_001fe748(gh_game_objects*s,uint32_t*f){
 if(f)*f=0;if(!manager_valid(s))return GH_OBJECT_ARGUMENT;
 /* Onlypure scalarpaths commit. AnunknownIO/free consumer isdetectedbefore
  * owner mutation; thisnativepreflight isnot anexecutedpartialoriginalcall. */
 uint8_t plan[80][0x5c];memcpy(plan,s->slots->records,(size_t)s->slots->count*0x5c);
 for(uint32_t n=s->slots->count;n;n--){uint8_t*p=plan[n-1];uint32_t state=u32(p);
  if(state==1||state==7){if(f)*f=state==1?0x1ff238:0x1ff338;return GH_OBJECT_UNSUPPORTED;}
  if(state==5)w32(p,6);else if(state==6)w32(p,7);
  if(u32(p)==8&&has_dependency(p,0xfffe)){
   uint32_t kind=u32(p+0x20);(void)remove_dependency(p,0xfffe);
   for(uint32_t j=s->slots->count;j;j--){uint8_t*q=plan[j-1];int is_valid=u32(q+4)!=0||(!(u32(q+8)&4)&&u32(q+0x10)!=0);
    if(is_valid&&has_dependency(q,kind)&&remove_dependency(q,kind)){if(f)*f=0x1ff2e0;return GH_OBJECT_UNSUPPORTED;}
   }
  }
 }
 memcpy(s->slots->records,plan,(size_t)s->slots->count*0x5c);return GH_OBJECT_OK;
}


gh_object_status gh_game_objects_release_nonpinned_001fef30(gh_game_objects*s,gh_object_slot_release call,void*ctx,uint32_t*f){
 if(f)*f=0;if(!manager_valid(s)||!call)return GH_OBJECT_ARGUMENT;
 if(!s->releasing){s->release_cursor=s->slots->count;s->release_sound=0;s->releasing=1;}
 while(s->release_cursor){unsigned i=s->release_cursor-1;uint8_t*p=s->slots->records+(size_t)i*0x5c;
  if(u32(p+4)&0x80000000u){s->release_cursor--;continue;}
  /*1FF3A0 clears usecount/flags before the actualdata free. Borrowed pinned
   * views never enter this branch; unknown owned data needs original2A9680. */
  w32(p+4,0);w32(p+8,0);
  if(u32(p+0x10)){if(f)*f=0x2a9680;return GH_OBJECT_UNSUPPORTED;}
  w32(p+0xc,UINT32_MAX);w32(p+0x1c,0);w32(p+0x18,0);s->source[i].bytes=NULL;s->source[i].size=0;
  while(s->release_sound<10){uint8_t*q=p+0x24+s->release_sound*4;uint32_t v=u32(q);int32_t slot=v<=INT32_MAX?(int32_t)v:(int32_t)((int64_t)v-4294967296LL);
   gh_object_status e=call(ctx,slot,f);if(e)return e;w32(q,UINT32_MAX);s->release_sound++;
  }
  w32(p,8);s->release_sound=0;s->release_cursor--;
 }
 s->releasing=0;return GH_OBJECT_OK;
}
gh_object_status gh_game_objects_reset_records_001ff028(gh_game_objects*s,uint32_t*f){
 if(f)*f=0;if(!manager_valid(s))return GH_OBJECT_ARGUMENT;
 if(s->releasing){if(f)*f=0x1fef30;return GH_OBJECT_NOT_READY;}
 for(unsigned i=0;i<s->slots->count;i++)if(s->source[i].allocation.pointer){if(f)*f=0x2a9680;return GH_OBJECT_BUSY;}
 if(gh_stage_slots_reset(s->slots,s->slots->heap_handle)!=GH_STAGE_OK)return GH_OBJECT_BOUNDS;
 memset(s->source,0,sizeof(s->source));return GH_OBJECT_OK;
}


gh_object_status gh_game_objects_acquire_001fea48(gh_game_objects*s,uint32_t kind,gh_object_view*out){
 if(!manager_valid(s)||!out)return GH_OBJECT_ARGUMENT;int index=find(s,kind);if(index<0)return GH_OBJECT_MISSING;
 uint8_t*p=s->slots->records+(size_t)index*0x5c;
 if(u32(p+0x10)!=(uint32_t)index+1)return GH_OBJECT_BOUNDS;
 uint32_t count=u32(p+4);if(!(count&0x80000000u))w32(p+4,count+1u);w32(p,3);
 return gh_game_objects_find(s,kind,out);
}
gh_object_status gh_game_objects_release_reference_001ff0b0(gh_game_objects*s,uint32_t kind){
 if(!manager_valid(s))return GH_OBJECT_ARGUMENT;int index=find(s,kind);if(index<0)return GH_OBJECT_MISSING;
 uint8_t*p=s->slots->records+(size_t)index*0x5c;uint32_t count=u32(p+4);
 if(count&&!(count&0x80000000u)){count--;w32(p+4,count);}if(!count)w32(p,4);return GH_OBJECT_OK;
}

gh_object_status gh_game_objects_set_file_extent(gh_game_objects*s,uint32_t kind,const void*source,size_t extent){
 if(!manager_valid(s)||!source||!extent)return GH_OBJECT_ARGUMENT;int n=find(s,kind);if(n<0)return GH_OBJECT_MISSING;
 if(s->source[n].bytes!=source||extent>s->source[n].size)return GH_OBJECT_BOUNDS;
 s->source[n].file_extent=extent;s->source[n].file_extent_known=1;return GH_OBJECT_OK;
}
static gh_object_status resource_fail(uint32_t*f,uint32_t pc,gh_object_status e){if(f)*f=pc;return e;}
gh_object_status gh_game_objects_load_wait_001fe8a0(gh_object_resource_load*op,gh_game_objects*s,uint32_t kind,
 const gh_object_resource_bindings*b,int reference,gh_object_view*out,uint32_t*f){
 if(f)*f=0;if(!op||!manager_valid(s)||!b||!out||(reference!=0&&reference!=1))return GH_OBJECT_ARGUMENT;
 if(op->stage&&(op->manager!=s||op->kind!=kind||op->reference!=(uint8_t)reference))return GH_OBJECT_BUSY;
 if(op->stage==4)return gh_game_objects_find(s,kind,out);
 if(!op->stage){
  int found=find(s,kind);if(found>=0){gh_object_status e=gh_game_objects_acquire_001fea48(s,kind,out);if(e)return resource_fail(f,0x1fea48,e);op->manager=s;op->kind=kind;op->reference=(uint8_t)reference;op->stage=4;return GH_OBJECT_OK;}
  if(!b->dvd||!b->dvd->disc||!b->heap||!b->threads)return GH_OBJECT_ARGUMENT;
  int index=-1;for(unsigned i=0;i<s->slots->count;i++)if(!s->source[i].allocation.pointer&&!valid(s,i)&&!(u32(s->slots->records+(size_t)i*0x5c+8)&4)){index=(int)i;break;}
  if(index<0){*out=(gh_object_view){0};return GH_OBJECT_OK;} /* originalNULL capacity */
  char path[12];if(gh_game_object_path(kind,path)!=GH_GAME_OK)return resource_fail(f,0x307918,GH_OBJECT_UNSUPPORTED);
  size_t entry;gh_disc_entry file;if(gh_disc_find(b->dvd->disc,path,&entry)||gh_disc_entry_at(b->dvd->disc,entry,&file)||file.size>SIZE_MAX)return resource_fail(f,0x200cb0,GH_OBJECT_DISC);
  uint8_t*p=s->slots->records+(size_t)index*0x5c;
  uint32_t job=gh_dvd_read_alloc(b->dvd,path,&s->source[index].allocation,b->heap,0,0,0,0);if(!job)return resource_fail(f,0x200cb0,GH_OBJECT_DISC);
  s->source[index].bytes=s->source[index].allocation.pointer;s->source[index].size=((size_t)file.size+2047)&~(size_t)2047;
  s->source[index].file_extent=(size_t)file.size;s->source[index].file_extent_known=1;s->source[index].heap=b->heap;s->source[index].dvd=b->dvd;s->source[index].job=job;
  w32(p+0x10,(uint32_t)index+1);w32(p+0x14,0);w32(p+0x1c,job);w32(p+0xc,file_kind(kind));w32(p,1);w32(p+8,u32(p+8)|1);w32(p+0x20,kind);
  op->manager=s;op->kind=kind;op->index=(uint32_t)index;op->bindings=*b;op->reference=(uint8_t)reference;op->stage=1;
 }
 b=&op->bindings;uint8_t*p=s->slots->records+(size_t)op->index*0x5c;
 if(op->stage==1){
  while(gh_dvd_check(b->dvd,s->source[op->index].job))if(gh_task_threads_sleep(b->threads,1)!=GH_THREAD_OK)return resource_fail(f,0x2d5250,GH_OBJECT_THREAD);
  if(b->dvd->failed_id==s->source[op->index].job)return resource_fail(f,0x201098,GH_OBJECT_DISC);
  w32(p+8,u32(p+8)&~1u);op->stage=2;
 }
 if(op->stage==2){
  for(;;){int ready=1;
   if(u32(p+0x20)!=0xffff){for(unsigned i=0;i<10;i++){uint32_t raw=u32(p+0x24+i*4);int32_t slot=raw<=INT32_MAX?(int32_t)raw:(int32_t)((int64_t)raw-4294967296LL);
    if(slot==-1)continue;if(!b->sound_check)return resource_fail(f,0x2cb778,GH_OBJECT_UNSUPPORTED);
    int current=0;gh_object_status e=b->sound_check(b->sound_context,slot,&current,f);if(e)return e;if(current!=0&&current!=1)return resource_fail(f,0x2cb778,GH_OBJECT_ARGUMENT);if(!current){ready=0;break;}
   }}
   if(ready)break;if(gh_task_threads_sleep(b->threads,1)!=GH_THREAD_OK)return resource_fail(f,0x2d5250,GH_OBJECT_THREAD);
  }
  w32(p,2);op->stage=3;
 }
 if(op->stage==3){
  gh_object_status e;if(op->reference)e=gh_game_objects_acquire_001fea48(s,kind,out);
  else {w32(p+4,0xffffff9c);e=gh_game_objects_find(s,kind,out);}if(e)return resource_fail(f,0x1ff090,e);op->stage=4;
 }
 return gh_game_objects_find(s,kind,out);
}
