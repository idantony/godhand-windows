#include "game_room_state.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"line%d %s\n",__LINE__,#x);exit(1);}}while(0)
static uint32_t u32(const uint8_t*p){return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static void w32(uint8_t*p,uint32_t v){for(unsigned i=0;i<4;i++)p[i]=(uint8_t)(v>>(8*i));}
static const uint8_t *at(const uint8_t*b,size_t size,uint32_t a,size_t n){
 unsigned count=b[44]|(unsigned)b[45]<<8;uint32_t ph=u32(b+28);
 for(unsigned i=0;i<count;i++){CHECK((uint64_t)ph+i*32+32<=size);const uint8_t*p=b+ph+i*32;uint32_t v=u32(p+8),z=u32(p+16),o=u32(p+4);
 if(u32(p)==1&&a>=v&&a-v<=z&&n<=z-(a-v)){CHECK((uint64_t)o+a-v+n<=size);return b+o+a-v;}}
 CHECK(0);return NULL;
}
typedef struct {gh_task_threads *threads;int count;} task_ctx;
static void sleeping(void*p,int32_t arg){task_ctx*c=p;c->count+=arg;CHECK(gh_task_threads_sleep(c->threads,1)==GH_THREAD_OK);c->count+=1000;}
int main(int argc,char**argv){
 CHECK(argc==2);FILE*f=fopen(argv[1],"rb");CHECK(f&&!fseek(f,0,SEEK_END));long length=ftell(f);CHECK(length>0);rewind(f);
 uint8_t *elf=malloc((size_t)length);CHECK(elf&&fread(elf,1,(size_t)length,f)==(size_t)length);fclose(f);
 gh_game_prepare *p=malloc(sizeof(*p));uint8_t *expect=malloc(sizeof(p->fields));gh_room_state state;gh_room_record rec;CHECK(p&&expect);
 for(unsigned i=0;i<112;i++){
  CHECK(gh_room_record_at(i,&rec)==GH_ROOM_OK);const uint8_t *src=at(elf,(size_t)length,rec.original_address,20);
  CHECK((src[0]|(unsigned)src[1]<<8)==(uint16_t)rec.room&&(src[2]|(unsigned)src[3]<<8)==(uint16_t)rec.save_slot);
  CHECK(u32(src+4)==0&&u32(src+8)==0&&u32(src+12)==0&&u32(src+16)==rec.flags);
  for(unsigned preserve=0;preserve<2;preserve++){
   memset(p->fields,0xa5,sizeof(p->fields));memset(expect,0xa5,sizeof(p->fields));
   size_t offset=rec.save_slot<0?8:0x1660+(unsigned)rec.save_slot*0x1650;
   uint32_t handle=rec.save_slot<0?1:(uint32_t)rec.save_slot+2;
   if(rec.save_slot<0||(rec.save_slot==0&&!preserve))memset(expect+offset,0,0x1650);
   w32(expect,handle);w32(expect+0x1658,(uint16_t)rec.save_slot);
   CHECK(gh_room_state_select_002bebc8(p,rec.room,preserve?0x4000000:0,&state)==GH_ROOM_OK);
   CHECK(state.active==p->fields+offset&&state.size==0x1650&&state.native_handle==handle&&state.record->original_address==rec.original_address);
   CHECK(!memcmp(p->fields,expect,sizeof(p->fields)));
  }
 }
 CHECK(gh_room_record_at(112,&rec)==GH_ROOM_BOUNDS);
 CHECK(gh_room_state_select_002bebc8(p,0x1234,0,&state)==GH_ROOM_OK&&state.record->room==-1&&state.active==p->fields+8);
 gh_task_record32 records[16]={0};gh_task_manager manager={.records=records,.capacity=16,.count=16,.current_index=-1};
 gh_boot_work work={0};uint32_t frontier;work.room=0x102;work.flags=0x4000000;
 records[3].running=2;CHECK(gh_scenario_reset_002c32f0(p,&work,&manager,NULL,&state,&frontier)==GH_ROOM_UNSUPPORTED&&frontier==0x2d59f0);records[3].running=0;
 gh_task_threads *threads;CHECK(gh_task_threads_create(&manager,&threads)==GH_THREAD_OK);task_ctx context={threads,0};int32_t slot;
 for(unsigned i=0;i<16;i++){records[i].attributes=0xa5;records[i].thread_id=-1;}
 CHECK(gh_task_threads_start(threads,0,sleeping,&context,1,&slot)==GH_THREAD_OK&&slot==0);
 CHECK(gh_task_threads_start(threads,15,sleeping,&context,2,&slot)==GH_THREAD_OK&&slot==15);
 CHECK(gh_tasks_run_matching_002d52a0(&manager).status==GH_TASK_OK&&context.count==3);
 w32((uint8_t*)&work+0x5e4,0xffffffff);CHECK(gh_scenario_reset_002c32f0(p,&work,&manager,threads,&state,&frontier)==GH_ROOM_OK&&frontier==0);
 CHECK(u32((uint8_t*)&work+0x5e4)==0xbfffffff&&state.record->room==0x102&&state.record->save_slot==1);
 for(unsigned i=0;i<16;i++)CHECK(!records[i].running&&records[i].attributes==0xa4);
 CHECK(gh_tasks_run_matching_002d52a0(&manager).status==GH_TASK_OK&&context.count==3);
 CHECK(gh_task_threads_destroy(threads)==GH_THREAD_OK);free(p);free(expect);free(elf);
 puts("PASS actual112 room table records, persistent/transient bank bounds and real scenario task cancellation");return 0;
}
