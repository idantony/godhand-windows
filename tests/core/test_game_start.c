#include "game_start.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"line%d %s\n",__LINE__,#x);exit(1);}}while(0)
static uint32_t le(const uint8_t *p){return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static uint16_t shortle(const uint8_t *p){return (uint16_t)((unsigned)p[0]|(unsigned)p[1]<<8);}
static const uint8_t *at(const uint8_t *elf,size_t size,uint32_t addr,size_t bytes){
 uint32_t off=le(elf+28);uint16_t stride=shortle(elf+42),count=shortle(elf+44);
 for(unsigned i=0;i<count;i++){
  CHECK((uint64_t)off+(uint64_t)i*stride+32<=size);
  const uint8_t *ph=elf+off+(size_t)i*stride;uint32_t v=le(ph+8),n=le(ph+16),f=le(ph+4);
  if(le(ph)==1 && addr>=v && addr-v<=n && bytes<=n-(addr-v)){
   CHECK((uint64_t)f+addr-v+bytes<=size);return elf+f+addr-v;
  }
 }
 CHECK(0);return 0;
}
static uint8_t *read_local(const char *path,size_t *size){
 FILE *f=fopen(path,"rb");long n;uint8_t *b;CHECK(f);CHECK(!fseek(f,0,SEEK_END));
 n=ftell(f);CHECK(n>0);*size=(size_t)n;rewind(f);b=malloc(*size);CHECK(b);
 CHECK(fread(b,1,*size,f)==*size);fclose(f);return b;
}
static void compare(gh_disc *d,const char *path,gh_r207_allocation a,size_t capacity){
 size_t index;gh_disc_entry e;uint8_t *b;
 CHECK(gh_disc_find(d,path,&index)==GH_DISC_OK && gh_disc_entry_at(d,index,&e)==GH_DISC_OK);
 CHECK(a.pointer && e.size<=capacity);b=malloc((size_t)e.size);CHECK(b);
 CHECK(gh_disc_read(d,index,0,b,(size_t)e.size)==GH_DISC_OK);
 CHECK(!memcmp(a.pointer,b,(size_t)e.size));free(b);
}
static gh_game_assets assets;static int returned;static gh_game_status result;
static void loader(void *ctx,int32_t costume){(void)ctx;result=gh_game_assets_0030d5c8(&assets,(uint8_t)costume);returned=1;}
static gh_game_loop loop;static unsigned calls;static gh_game_status loop_result;
static int pre(void *ctx,gh_game_loop *s){(void)ctx;(void)s;calls++;return 0;}
static int initialize(void *ctx,gh_game_loop *s){(void)ctx;calls++;s->work->flags&=~0x80000000u;return 0;}
static void loop_entry(void *ctx,int32_t arg){(void)arg;loop_result=gh_game_main_002a5998(&loop,ctx);}
int main(int argc,char **argv){
 gh_disc *disc;gh_heap *heap;gh_task_threads *threads;gh_dvd_jobs dvd;gh_boot_work work={0};
 gh_task_record32 records[4]={0};gh_task_manager manager={records,4,0,-1,4,0,0,0};
 int32_t slot;unsigned ticks;size_t elfsize;uint8_t *elf;const uint8_t *table;char path[12];
 CHECK(argc==3);elf=read_local(argv[2],&elfsize);
 CHECK(gh_disc_open(argv[1],&disc)==GH_DISC_OK && gh_heap_create(32*1024*1024,&heap)==GH_MEM_OK);
 CHECK(gh_task_threads_create(&manager,&threads)==GH_THREAD_OK);gh_dvd_jobs_init(&dvd,disc);
 work.flags=0x40000;assets.work=&work;assets.dvd=&dvd;assets.heap=heap;assets.threads=threads;
 CHECK(gh_task_threads_start(threads,2,loader,0,0,&slot)==GH_THREAD_OK && slot==2);
 for(ticks=0;ticks<1000 && !returned;ticks++){
  CHECK(gh_tasks_run_matching_002d52a0(&manager).status==GH_TASK_OK);
  CHECK(gh_dvd_pump(&dvd)==GH_DVD_OK);
 }
 CHECK(returned && result==GH_GAME_OK && work.flags==0x12400);
 compare(disc,"etc/core.dat",assets.core,assets.core_size);
 compare(disc,"etc/debugres/roomjmptbl.dat",assets.room_jump,assets.room_jump_size);
 /* Independent actual ELF pointer/records, not a second copied source list. */
 table=at(elf,elfsize,le(at(elf,elfsize,0x3c3cc4,4)),42*8);
 for(unsigned i=0;i<41;i++){
  uint32_t id=le(table+i*8),dest=le(table+i*8+4);CHECK(dest<43);
  CHECK(gh_game_object_path(id,path)==GH_GAME_OK);
  compare(disc,path,assets.objects[dest],assets.object_sizes[dest]);
 }
 CHECK(le(table+41*8)==65535);
 compare(disc,"pl/pl00.dat",assets.objects[0],assets.object_sizes[0]);
 compare(disc,"pl/pl01.dat",assets.objects[1],assets.object_sizes[1]);
 CHECK(gh_game_assets_release(&assets)==GH_GAME_OK && gh_heap_used(heap)==0);
 /* Original flags skip previously loaded groups; no read/allocation invented. */
 work.flags=0x40000|0x2000|0x400|0x10000;returned=0;
 CHECK(gh_task_threads_start(threads,2,loader,0,0,&slot)==GH_THREAD_OK);
 CHECK(gh_tasks_run_matching_002d52a0(&manager).status==GH_TASK_OK);
 CHECK(returned && result==GH_GAME_OK && gh_heap_used(heap)==0);
 /* Missing real sound operation fails, retains in-progress4000, no false done. */
 work.flags=0;returned=0;CHECK(gh_task_threads_start(threads,2,loader,0,0,&slot)==GH_THREAD_OK);
 CHECK(gh_tasks_run_matching_002d52a0(&manager).status==GH_TASK_OK);
 CHECK(returned && result==GH_GAME_UNAVAILABLE && assets.failed_entry==0x2ca798 && work.flags==0x4000);
 /* Real fixed-buffer loads for each original costume dispatch branch. */
 {static const char *const model[]={"pl/pl00.dat","pl/pl02.dat","pl/pl05.dat","pl/pl0f.dat",
  "pl/pl07.dat","pl/pl10.dat","pl/pl0e.dat","pl/pl11.dat"};
  static const char *const motion[]={"pl/pl01.dat","pl/pl01.dat","pl/pl06.dat","pl/pl06.dat",
  "pl/pl08.dat","pl/pl08.dat","pl/pl12.dat","pl/pl12.dat"};
  for(int costume=0;costume<8;costume++){
   work.flags=0x40000|0x2000|0x400;returned=0;
   CHECK(gh_task_threads_start(threads,2,loader,0,costume,&slot)==GH_THREAD_OK);
   for(unsigned visit=0;visit<20 && !returned;visit++){
    CHECK(gh_tasks_run_matching_002d52a0(&manager).status==GH_TASK_OK);
    CHECK(gh_dvd_pump(&dvd)==GH_DVD_OK);
   }
   CHECK(returned && result==GH_GAME_OK);
   compare(disc,model[costume],assets.objects[0],assets.object_sizes[0]);
   compare(disc,motion[costume],assets.objects[1],assets.object_sizes[1]);
   CHECK(gh_game_assets_release(&assets)==GH_GAME_OK && gh_heap_used(heap)==0);
  }
 }
 /* Suspended loader retains its buffer until explicit queue cancellation. */
 work.flags=0x40000;returned=0;
 CHECK(gh_task_threads_start(threads,2,loader,0,0,&slot)==GH_THREAD_OK);
 CHECK(gh_tasks_run_matching_002d52a0(&manager).status==GH_TASK_OK);
 CHECK(!returned && gh_game_assets_release(&assets)==GH_GAME_THREAD);
 CHECK(gh_task_threads_cancel(threads,2)==GH_THREAD_OK);
 CHECK(gh_dvd_cancel(&dvd,dvd.jobs[dvd.current].id));
 CHECK(gh_game_assets_release(&assets)==GH_GAME_OK && gh_heap_used(heap)==0);
 /* Dispatcher boundary test only: handlers here are test-only trace adapters. */
 loop.work=&work;loop.threads=threads;gh_game_platform platform={0};
 platform.dvd_cover=pre;platform.modal_input=pre;platform.states[0]=initialize;
 CHECK(gh_task_threads_start(threads,0,loop_entry,&platform,0,&slot)==GH_THREAD_OK);
 CHECK(gh_tasks_run_matching_002d52a0(&manager).status==GH_TASK_OK);
 CHECK(calls==3 && records[0].running==1);
 CHECK(gh_tasks_run_matching_002d52a0(&manager).status==GH_TASK_OK);
 CHECK(loop_result==GH_GAME_OK && records[0].running==0);
 platform.dvd_cover=0;CHECK(gh_task_threads_start(threads,0,loop_entry,&platform,0,&slot)==GH_THREAD_OK);
 CHECK(gh_tasks_run_matching_002d52a0(&manager).status==GH_TASK_OK);
 CHECK(loop_result==GH_GAME_UNAVAILABLE && loop.failed_entry==0x2a6ca8);
 CHECK(gh_task_threads_destroy(threads)==GH_THREAD_OK && gh_heap_destroy(heap)==GH_MEM_OK);
 gh_disc_close(disc);free(elf);
 printf("PASS actual30D5C8 resources/player buffers in%u visits; original2A5998 dispatch boundary tests\n",ticks);return 0;
}
