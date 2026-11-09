#include "id_base.h"
#include "mc_boot_id.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define C(x) do{if(!(x)){fprintf(stderr,"FAIL %d %s\n",__LINE__,#x);exit(1);}}while(0)
static uint32_t w(const unsigned char *p){return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static void put(unsigned char *p,uint32_t x){p[0]=(unsigned char)x;p[1]=(unsigned char)(x>>8);p[2]=(unsigned char)(x>>16);p[3]=(unsigned char)(x>>24);}
static uint32_t bits(float f){uint32_t n;memcpy(&n,&f,4);return n;}
typedef struct {gh_mc_id_resource r;gh_dvd_jobs *dvd;gh_heap *heap;gh_task_threads *threads;gh_id_state *id;int done;} loader;
static void task(void *context,int32_t arg)
{loader *l=context;C(arg==1);C(gh_mc_boot_load_id_resource(&l->r,l->dvd,l->heap,l->threads,l->id,-1,1)==GH_MC_BOOT_OK);l->done=1;}
static uint32_t random_word(void *context){uint32_t *n=context;return ++*n;}
int main(int argc,char **argv)
{
    gh_disc *disc;gh_heap *heap;gh_dvd_jobs dvd;gh_r207_allocation language_allocation;
    gh_bundle language;gh_bundle_entry idd;gh_disc_entry source;size_t index,i;
    gh_id_state *manager;gh_id_runtime *runtime;gh_id_base object={0};gh_id_view view;
    gh_id_job_view job;int ret,freed;uint32_t jobid;loader l;
    gh_task_record32 tasks[3]={{0}};gh_task_manager tm={tasks,3,0,-1,3,0,0,0};int32_t slot;
    C(argc==2 && gh_disc_open(argv[1],&disc)==GH_DISC_OK);C(gh_heap_create(8*1024*1024,&heap)==GH_MEM_OK);
    gh_dvd_jobs_init(&dvd,disc);jobid=gh_dvd_read_alloc(&dvd,"etc/core_usa.bin",&language_allocation,heap,0,0,0,0);
    C(jobid && gh_dvd_pump(&dvd)==GH_DVD_OK);C(gh_disc_find(disc,"etc/core_usa.bin",&index)==GH_DISC_OK);
    C(gh_disc_entry_at(disc,index,&source)==GH_DISC_OK);C(gh_bundle_open(language_allocation.pointer,(size_t)source.size,&language)==GH_BUNDLE_OK);
    manager=calloc(1,sizeof(*manager));C(manager);C(gh_id_find_language_data(&language,&idd)==GH_MENU_OK);
    C(gh_id_system_init_002acc70(manager,&idd)==GH_MENU_OK);
    memset(&l,0,sizeof(l));l.dvd=&dvd;l.heap=heap;l.id=manager;
    C(gh_task_threads_create(&tm,&l.threads)==GH_THREAD_OK);C(gh_task_threads_start(l.threads,-1,task,&l,1,&slot)==GH_THREAD_OK);
    for(i=0;i<5 && !l.done;++i){C(gh_tasks_run_matching_002d52a0(&tm).status==GH_TASK_OK);C(gh_dvd_pump(&dvd)==GH_DVD_OK);}
    C(l.done && gh_task_threads_destroy(l.threads)==GH_THREAD_OK);
    C(gh_id_lookup_002acd78(manager,11,0,&view)==GH_ID_NOT_FOUND);
    C(gh_id_lookup_002acd78(manager,33,1,&view)==GH_ID_NOT_FOUND);
    C(gh_id_lookup_002acd78(manager,32,1,&view)==GH_ID_BOUNDS);
    C(gh_id_lookup_002acd78(manager,11,1,&view)==GH_ID_OK && w(view.bytes)==0x504449 && w(view.bytes+4)==67);
    C(gh_id_lookup_002acd78(manager,11,3,&view)==GH_ID_NOT_FOUND);
    C(gh_id_lookup_002acd78(manager,11,1,&view)==GH_ID_OK);
    memset(manager->pool,0xa5,sizeof(manager->pool));C(gh_id_runtime_create(manager,1,&runtime)==GH_ID_OK);
    C(gh_id_base_initialize_002a9ea0(runtime,&object,11,0,0,0,&ret)==GH_ID_OK && ret==1);
    C(object.initialized && object.entry_count==67 && object.mode==6 && object.id==0 && object.resource_number==11);
    C(bits(object.vector_a[3])==0x3f800000 && bits(object.vector_a[0])==0 && bits(object.f40)==0x3c23d70a && object.h44==128);
    C(manager->job_count==1 && manager->pool_used==67);
    C(gh_id_runtime_job(runtime,0,&job)==GH_ID_OK && job.tag==11 && job.arg==0 && job.start==0 && job.wire_count==67 && job.active);
    C(w(manager->jobs+8)==0); /* full host pointer lives in runtime, never truncated */
    for(i=0;i<sizeof(manager->pool);++i)C(manager->pool[i]==(i<67*0xac?0:0xa5));
    for(i=0;i<67;++i) {
        gh_id_entry *e=object.entries+i;const unsigned char *s=view.bytes+16+i*0x2e0;
        uint32_t flags=w(s+4)&~0x4000u;
        C(e->source==s && e->source_index==i && e->parts[0]==s+0x130 && e->parts[4]==s+0x27c);
        C(e->seed==12345678 && e->kind==s[0x66] && bits(e->pos_x)==w(s+8) && bits(e->pos_y)==w(s+12));
        C(bits(e->scale_x)==w(s+0x28) && bits(e->scale_y)==w(s+0x2c));
        C(e->b28==s[1] && e->id==(int8_t)s[3] && e->message==0 && e->texture.bytes==0);
        C(!(w(s+4)&0x40) && s[0x6e]==2); /* actual MC does not require unverified RNG/sqrt */
        if(!s[0x5d])flags|=0x2000;
        if(!s[0x5e]){if((s[0x1b0]|(unsigned)s[0x1b1]<<8)<2)flags|=0x1000;else flags&=~0x1000u;}
        if(!s[0x5f]){if((s[0x214]|(unsigned)s[0x215]<<8)<2)flags|=0x800;else flags&=~0x800u;}
        if(!s[0x64]){if((s[0x278]|(unsigned)s[0x279]<<8)<2)flags|=0x400;else flags&=~0x400u;}
        C(e->flags==flags && e->path_length==0);
        for(unsigned j=0;j<5;++j)C(e->jitter[j]==0);
    }
    {
        gh_id_entry *selected,*dummy,*same_dummy;int fallback;uint32_t own_flags;
        C(gh_id_base_get_work_002aa410(runtime,&object,0x40,&selected,&fallback)==GH_ID_OK && !fallback);
        C(selected->pos_y==210.0f);own_flags=selected->flags;
        C(gh_id_base_set_family_display(runtime,&object,0x40,0)==GH_ID_OK);
        C(selected->flags==own_flags); /* original family method excludes self */
        C(gh_id_base_get_work_002aa410(runtime,&object,999,&dummy,&fallback)==GH_ID_OK && fallback);
        C(gh_id_base_get_work_002aa410(runtime,&object,1000,&same_dummy,&fallback)==GH_ID_OK && fallback && dummy==same_dummy);
        C(gh_id_base_restart_anim(&object,0,0)==GH_ID_OK && object.playing==1);
    }
    C(gh_id_runtime_destroy(runtime)==GH_ID_BUSY);
    C(gh_id_base_release_002aa078(runtime,&object,&ret,&freed)==GH_ID_OK && ret==1 && freed);
    C(!object.entries && !object.initialized && object.entry_count==67 && !manager->pool_used && !manager->job_count);
    /* Synthetic structural edge cases exercise source filtering, last-parent
     * matching, original self-reference early return, and explicit frontiers. */
    {
        unsigned char data[8+16+4*0x2e0]={0};gh_bundle_entry save=manager->pair[2].data;
        unsigned char *s=data+24;uint32_t rng_calls=0;
        put(data,1);put(data+4,8);put(data+8,0x504449);put(data+12,4);
        manager->pair[2].data.bytes=data;manager->pair[2].data.size=sizeof(data);
        s[1]=1;s[2]=9;s[0x2e0+1]=9;s[0x2e0+2]=255;s[2*0x2e0+1]=9;s[2*0x2e0+2]=255;s[3*0x2e0+2]=255;
        C(gh_id_base_initialize_002a9ea0(runtime,&object,2,0,0,0,&ret)==GH_ID_OK);
        C(object.entries[0].parent==object.entries+2); /* last duplicate wins */
        C(gh_id_base_release_002aa078(runtime,&object,&ret,&freed)==GH_ID_OK && freed);
        s[2]=1;s[0x2e0+2]=9;
        C(gh_id_base_initialize_002a9ea0(runtime,&object,2,0,0,0,&ret)==GH_ID_OK);
        C(!object.entries[0].parent && !object.entries[1].parent); /* early return */
        C(gh_id_base_release_002aa078(runtime,&object,&ret,&freed)==GH_ID_OK && freed);
        put(s+4,0x200);C(gh_id_base_initialize_002a9ea0(runtime,&object,2,0,0,0,&ret)==GH_ID_OK);
        C(object.entry_count==3 && object.entries[0].source_index==1);
        C(gh_id_base_release_002aa078(runtime,&object,&ret,&freed)==GH_ID_OK && freed);
        put(s+4,0x40);C(gh_id_base_initialize_002a9ea0(runtime,&object,2,0,0,0,&ret)==GH_ID_UNSUPPORTED);
        C(object.first_unported==0x31cfe0 && ret==-1 && !object.initialized);
        C(gh_id_base_release_002aa078(runtime,&object,&ret,&freed)==GH_ID_OK && freed);
        C(gh_id_base_initialize_002a9ea0(runtime,&object,2,0,random_word,&rng_calls,&ret)==GH_ID_OK && rng_calls==5);
        C(gh_id_base_release_002aa078(runtime,&object,&ret,&freed)==GH_ID_OK && freed);
        put(s+4,0);s[0x6e]=4;
        C(gh_id_base_initialize_002a9ea0(runtime,&object,2,0,0,0,&ret)==GH_ID_UNSUPPORTED && object.first_unported==0x2abc30);
        C(gh_id_base_release_002aa078(runtime,&object,&ret,&freed)==GH_ID_OK && freed);
        manager->pair[2].data=save;
    }
    {
        gh_id_pool_run runs[100],large;int found;
        for(i=0;i<100;++i)C(gh_id_pool_allocate_002ace48(runtime,1,7,9,&runs[i])==GH_ID_OK);
        C(gh_id_pool_allocate_002ace48(runtime,1,7,9,&large)==GH_ID_CAPACITY);
        C(gh_id_pool_release_002acf30(runtime,runs[0],7,&found)==GH_ID_OK && !found);
        for(i=100;i>0;--i)C(gh_id_pool_release_002acf30(runtime,runs[i-1],7,&found)==GH_ID_OK && found);
        C(gh_id_pool_allocate_002ace48(runtime,640,1,0,&large)==GH_ID_OK);
        C(gh_id_runtime_job(runtime,0,&job)==GH_ID_OK && job.wire_count==-128);
        C(gh_id_pool_allocate_002ace48(runtime,1,1,0,&runs[0])==GH_ID_CAPACITY);
        C(gh_id_pool_release_002acf30(runtime,large,1,&found)==GH_ID_OK && !found); /* original signed-byte quirk */
    }
    C(gh_id_runtime_destroy(runtime)==GH_ID_OK);
    C(gh_mc_boot_release_id_resource(&l.r,heap,manager)==GH_MC_BOOT_OK);
    free(manager);C(gh_heap_release(heap,language_allocation)==GH_MEM_OK);C(gh_heap_destroy(heap)==GH_MEM_OK);gh_disc_close(disc);
    puts("PASS actual67 MC IDP entries: pool/build/parents/reset/release; generic RNG and sqrt frontiers explicit");return 0;
}
