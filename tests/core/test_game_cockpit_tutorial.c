#include "game_cockpit_tutorial.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define C(x) do{if(!(x)){fprintf(stderr,"FAIL %d %s\n",__LINE__,#x);exit(1);}}while(0)
static uint8_t*read_file(gh_disc*d,const char*path,size_t*size){size_t i;gh_disc_entry e;C(!gh_disc_find(d,path,&i)&&!gh_disc_entry_at(d,i,&e)&&e.size<=SIZE_MAX);*size=(size_t)e.size;uint8_t*p=malloc(*size);C(p&&!gh_disc_read(d,i,0,p,*size));return p;}
int main(int argc,char**argv){C(argc==2);gh_disc*d;C(!gh_disc_open(argv[1],&d));size_t n;uint8_t*language=read_file(d,"etc/core_usa.bin",&n);gh_bundle bundle;gh_bundle_entry idd;C(!gh_bundle_open(language,n,&bundle)&&!gh_id_find_language_data(&bundle,&idd));gh_id_state*id=calloc(1,sizeof(*id));C(id&&!gh_id_system_init_002acc70(id,&idd));gh_id_runtime*runtime;C(!gh_id_runtime_create(id,1,&runtime)&&!gh_id_runtime_set_path_policy(runtime,GH_ID_PATH_REFERENCE_NEAREST));gh_game_mt mt={.index=625};gh_game_cockpit*c;uint8_t savebytes[GH_CORE_SAVE_BYTES]={0};gh_core_save_view save={savebytes,sizeof(savebytes)};C(!gh_core_save_system_init(save)&&!gh_game_cockpit_create(runtime,id,&mt,&c)&&!gh_game_cockpit_initialize_00140c88(c,save));C(id->pool_used==405&&id->job_count==39);gh_heap*heap;C(!gh_heap_create(2*1024*1024,&heap));gh_dvd_jobs dvd;gh_dvd_jobs_init(&dvd,d);gh_font_state fonts={0};gh_packed_message_state packed={0};gh_boot_work work={0};work.language=1;gh_tutorial_bindings b={c,&dvd,heap,id,&fonts,&packed,&bundle,&work};gh_cockpit_tutorial t={0};
 gh_heap*tiny;C(!gh_heap_create(16,&tiny));gh_tutorial_bindings small=b;small.room_heap=tiny;gh_cockpit_tutorial oom={0};C(gh_cockpit_tutorial_room_init_00147028(&oom,&small)==GH_TUTORIAL_MEMORY&&oom.frontier==0x200cb0&&!oom.resource.pointer&&!id->pair[0x1d].bound&&id->pool_used==405);C(!gh_cockpit_tutorial_release(&oom)&&!gh_heap_destroy(tiny));
 for(unsigned round=0;round<2;round++){
  C(!gh_cockpit_tutorial_room_init_00147028(&t,&b));gh_cockpit_tutorial_view v;C(!gh_game_cockpit_get_tutorial(c,&v));C(t.complete&&v.data_present&&v.read_id==t.job&&v.base->initialized&&v.base->resource_number==0x1d&&v.base->id==0&&v.base->playing&&t.packed_return==1&&packed.initialized&&v.current_message==0xffff&&!v.state_b8);C(t.image.size==(round?47104:29712));gh_id_entry*refs[7];for(unsigned i=0;i<7;i++){C(!gh_game_cockpit_entry_at(c,0x1a20+0x90+i*4,&refs[i]));C(refs[i]>=v.base->entries&&refs[i]<v.base->entries+v.base->entry_count);}C(!refs[0]->scale_x&&!refs[0]->scale_y&&(refs[1]->flags&0x8000000)&&(refs[2]->flags&0x8000000)&&refs[3]->message==0x200c&&refs[4]->message==0x200c);
  uint32_t pool=id->pool_used,jobs=id->job_count,job=t.job;void*source=t.resource.pointer;gh_id_entry*entries=v.base->entries;
  gh_cockpit_tutorial foreign={0};C(gh_cockpit_tutorial_room_init_00147028(&foreign,&b)==GH_TUTORIAL_BUSY&&!foreign.resource.pointer);C(!gh_cockpit_tutorial_release(&foreign)&&v.base->entries==entries&&v.base->owns_run&&id->pool_used==pool&&id->pair[0x1d].bound);
  refs[0]->scale_x=4;refs[1]->flags&=~0x8000000u;refs[3]->message=0;v.base->frame=37;C(!gh_cockpit_tutorial_next_call(&t)&&!gh_cockpit_tutorial_room_init_00147028(&t,&b));C(t.completed_calls==2&&t.job==job&&t.resource.pointer==source&&id->pool_used==pool&&id->job_count==jobs&&v.base->entries==entries&&v.base->frame==0&&!refs[0]->scale_x&&(refs[1]->flags&0x8000000)&&refs[3]->message==0x200c);printf("actual tutorial path=%s bytes=%zu entries=%u pool=%u jobs=%u repeat-source-preserved\n",t.path,t.image.size,v.base->entry_count,pool,jobs);
  /* Actual scene exit invokes 147188 before original 2AD108. The
   * source allocation survives both, until the room heap cleanup. */
  int compacted=99;
  C(gh_id_pool_compact_002ad108(runtime,&compacted)==GH_ID_BUSY&&!compacted);
  C(id->pool_used==pool&&id->job_count==jobs&&v.base->entries==entries);
  gh_id_job_view tutorial_job;
  C(!gh_id_runtime_job(runtime,jobs-1,&tutorial_job)&&tutorial_job.tag==0x1d&&tutorial_job.start==405);
  uint8_t heat[0x18]={0};uint32_t max=0x42f00000;memcpy(heat,&max,4);
  C(!gh_game_cockpit_room_quit_00141628(c,save,heat));
  C(!gh_game_cockpit_get_tutorial(c,&v)&&!v.data_present&&!v.base->owns_run&&!v.base->entries);
  C(id->pool_used==pool&&id->job_count==jobs&&t.resource.pointer==source&&id->pair[0x1d].bound);
  C(!gh_id_runtime_job(runtime,jobs-1,&tutorial_job)&&tutorial_job.tag==0x1d&&tutorial_job.active);
  gh_id_entry*unused;
  C(gh_game_cockpit_entry_at(c,0x1a20+0x90,&unused)==GH_ID_NOT_FOUND);
  C(!gh_id_pool_compact_002ad108(runtime,&compacted)&&compacted&&id->pool_used==405&&id->job_count==39);
  C(gh_id_runtime_job(runtime,39,&tutorial_job)==GH_ID_BOUNDS&&t.resource.pointer==source);
  C(!gh_cockpit_tutorial_release(&t)&&!gh_heap_used(heap)&&!id->pair[0x1d].bound);
  C(id->pool_used==405&&id->job_count==39);
  work.language=-1;
 }
 C(!gh_game_cockpit_destroy(c)&&!id->pool_used&&!id->job_count&&!gh_id_runtime_destroy(runtime)&&!gh_heap_destroy(heap));free(id);free(language);gh_disc_close(d);puts("PASS actual USA/fallback tutorial IDD through DVD201160, sameCockpit ownedrun/7refs/packed/font state, explicit repeat-call restart, roomQuit preserves counts, original 2AD108 compacts, then safe resource lifecycle");return 0;}
