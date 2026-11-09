#include "mc_boot_id.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do {if(!(x)){fprintf(stderr,"FAIL %d %s\n",__LINE__,#x);exit(1);}}while(0)
typedef struct load_context {
    gh_mc_id_resource resource;gh_dvd_jobs *dvd;gh_heap *heap;
    gh_task_threads *threads;gh_id_state *id;int8_t language;int done;
} load_context;
static void load_id_task(void *context,int32_t arg)
{
    load_context *c=context;volatile int preserve=arg;
    CHECK(gh_mc_boot_load_id_resource(&c->resource,c->dvd,c->heap,c->threads,c->id,-1,c->language)==GH_MC_BOOT_OK);
    CHECK(preserve==75);c->done=1;
}
static void run(load_context *c)
{
    gh_task_record32 records[3]={{0}};gh_task_manager manager={records,3,0,-1,3,0,0,0};
    int32_t slot;unsigned ticks=0;
    c->done=0;CHECK(gh_task_threads_create(&manager,&c->threads)==GH_THREAD_OK);
    CHECK(gh_task_threads_start(c->threads,-1,load_id_task,c,75,&slot)==GH_THREAD_OK);
    while(!c->done && ticks<10) {
        CHECK(gh_tasks_run_matching_002d52a0(&manager).status==GH_TASK_OK);
        CHECK(gh_dvd_pump(c->dvd)==GH_DVD_OK);++ticks;
    }
    CHECK(c->done && ticks==2); /* no CheckWaitFrame extra final sleep */
    CHECK(gh_task_threads_destroy(c->threads)==GH_THREAD_OK);
}
int main(int argc,char **argv)
{
    gh_disc *disc;gh_heap *heap;gh_dvd_jobs dvd;gh_r207_allocation language_alloc;
    gh_disc_entry source;size_t index;gh_bundle language;gh_bundle_entry idd;
    gh_id_state *id;load_context c;const unsigned char *original_pair1;
    void *expected;uint32_t job;
    CHECK(argc==2 && gh_disc_open(argv[1],&disc)==GH_DISC_OK);
    CHECK(gh_heap_create(8*1024*1024,&heap)==GH_MEM_OK);gh_dvd_jobs_init(&dvd,disc);
    job=gh_dvd_read_alloc(&dvd,"etc/core_usa.bin",&language_alloc,heap,0,0,0,0);
    CHECK(job && gh_dvd_pump(&dvd)==GH_DVD_OK);
    CHECK(gh_disc_find(disc,"etc/core_usa.bin",&index)==GH_DISC_OK);
    CHECK(gh_disc_entry_at(disc,index,&source)==GH_DISC_OK);
    CHECK(gh_bundle_open(language_alloc.pointer,(size_t)source.size,&language)==GH_BUNDLE_OK);
    CHECK(gh_id_find_language_data(&language,&idd)==GH_MENU_OK);
    id=calloc(1,sizeof(*id));CHECK(id && gh_id_system_init_002acc70(id,&idd)==GH_MENU_OK);
    original_pair1=id->pair[1].data.bytes;
    memset(&c,0,sizeof(c));c.dvd=&dvd;c.heap=heap;c.id=id;c.language=1;
    run(&c);
    CHECK(c.resource.preferred_path_found && !strcmp(c.resource.path,"id/local/US/idsavecore_US.idd"));
    CHECK(c.resource.image.size==455952 && c.resource.pair_bound && id->pair[11].bound);
    CHECK(id->pair[1].data.bytes==original_pair1);
    CHECK(gh_disc_find(disc,c.resource.path,&index)==GH_DISC_OK);
    expected=malloc(c.resource.image.size);CHECK(expected);
    CHECK(gh_disc_read(disc,index,0,expected,c.resource.image.size)==GH_DISC_OK);
    CHECK(!memcmp(expected,c.resource.allocation.pointer,c.resource.image.size));free(expected);
    CHECK(gh_mc_boot_release_id_resource(&c.resource,heap,id)==GH_MC_BOOT_OK);
    CHECK(!id->pair[11].bound && !id->pair[11].data.bytes && id->pair[1].data.bytes==original_pair1);
    c.language=2;run(&c); /* no EN resource on this actual USA disc -> original fallback */
    CHECK(!c.resource.preferred_path_found && !strcmp(c.resource.path,"id/idsavecore.idd"));
    CHECK(c.resource.image.size==520752 && id->pair[11].bound);
    CHECK(gh_mc_boot_release_id_resource(&c.resource,heap,id)==GH_MC_BOOT_OK);
    free(id);CHECK(gh_heap_release(heap,language_alloc)==GH_MEM_OK);
    CHECK(gh_heap_used(heap)==0 && gh_heap_destroy(heap)==GH_MEM_OK);gh_disc_close(disc);
    puts("PASS real localized/fallback idsavecore DVD loads and slot11 TBL binding; IDBase UI frontier remains");
    return 0;
}
