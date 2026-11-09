#include "game_room_pack.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"line%d %s\n",__LINE__,#x);exit(1);}}while(0)
typedef struct {gh_room_pack pack;uint16_t room;int returned;gh_room_pack_status status;} context;
static void load(void *p,int32_t arg){context*c=p;(void)arg;c->status=gh_game_room_pack_load_0031cc48(&c->pack,c->room);c->returned=1;}
int main(int argc,char**argv){
 gh_disc *d;gh_heap *heap;gh_dvd_jobs dvd;gh_task_record32 records[4]={0};gh_task_manager manager={.records=records,.capacity=4,.count=4,.current_index=-1};gh_task_threads *threads;char path[20];
 CHECK(argc==2&&gh_disc_open(argv[1],&d)==GH_DISC_OK);CHECK(gh_heap_create(32*1024*1024,&heap)==GH_MEM_OK);
 gh_dvd_jobs_init(&dvd,d);CHECK(gh_task_threads_create(&manager,&threads)==GH_THREAD_OK);
 CHECK(gh_game_room_path(0x20,path)==GH_PACK_OK&&!strcmp(path,"st0/r020.dat"));
 CHECK(gh_game_room_path(0x207,path)==GH_PACK_OK&&!strcmp(path,"st2/r207.dat"));
 CHECK(gh_game_room_path(0xffff,path)==GH_PACK_OK&&!strcmp(path,"stff/rffff.dat"));
 for(unsigned i=0;i<2;i++){
  context c={.pack={.dvd=&dvd,.heap=heap,.threads=threads},.room=i?0x207:0x20};int32_t slot;unsigned visits=0;gh_bundle bundle;gh_bundle_entry rht;
  CHECK(gh_task_threads_start(threads,2,load,&c,0,&slot)==GH_THREAD_OK&&slot==2);
  while(!c.returned&&visits<10){CHECK(gh_tasks_run_matching_002d52a0(&manager).status==GH_TASK_OK);
   if(!visits){CHECK(c.pack.job&&c.pack.allocation.pointer&&!c.pack.loaded);CHECK(gh_game_room_pack_release(&c.pack)==GH_PACK_BUSY);}
   CHECK(gh_dvd_pump(&dvd)==GH_DVD_OK);visits++;
  }
  CHECK(c.returned&&c.status==GH_PACK_OK&&c.pack.loaded&&visits==3);
  CHECK(c.pack.saved_pointer==c.pack.allocation.pointer&&c.pack.saved_bytes==c.pack.allocated_bytes&&!(c.pack.allocated_bytes&2047));
  CHECK(c.pack.allocated_bytes>=c.pack.file_bytes&&c.pack.allocated_bytes-c.pack.file_bytes<2048);
  CHECK(gh_game_room_path(c.room,path)==GH_PACK_OK);size_t index;gh_disc_entry entry;CHECK(gh_disc_find(d,path,&index)==GH_DISC_OK&&gh_disc_entry_at(d,index,&entry)==GH_DISC_OK);
  void *direct=malloc((size_t)entry.size);CHECK(direct&&gh_disc_read(d,index,0,direct,(size_t)entry.size)==GH_DISC_OK);
  CHECK(!memcmp(c.pack.allocation.pointer,direct,(size_t)entry.size));free(direct);
  CHECK(gh_game_room_pack_bundle(&c.pack,&bundle)==GH_BUNDLE_OK&&gh_bundle_find(&bundle,"RHT",0,&rht)==GH_BUNDLE_OK);
  printf("room%03x truepack %zu bytes, allocated%u, entries%u, RHT%zu, visits%u\n",c.room,c.pack.file_bytes,c.pack.allocated_bytes,bundle.count,rht.size,visits);
  CHECK(gh_game_room_pack_load_0031cc48(&c.pack,c.room)==GH_PACK_BUSY);
  CHECK(gh_game_room_pack_release(&c.pack)==GH_PACK_OK&&gh_heap_used(heap)==0);
 }
 context missing={.pack={.dvd=&dvd,.heap=heap,.threads=threads},.room=0xffff};int32_t slot;
 CHECK(gh_task_threads_start(threads,2,load,&missing,0,&slot)==GH_THREAD_OK);
 CHECK(gh_tasks_run_matching_002d52a0(&manager).status==GH_TASK_OK&&missing.returned&&missing.status==GH_PACK_DVD&&!missing.pack.loaded&&missing.pack.failed_entry==0x200df8);
 CHECK(gh_task_threads_destroy(threads)==GH_THREAD_OK&&gh_heap_destroy(heap)==GH_MEM_OK);gh_disc_close(d);
 puts("PASS actual room020/207 nativeDVD+fiber load/backup, RHT views, exact waits and ownership/errors");return 0;
}
