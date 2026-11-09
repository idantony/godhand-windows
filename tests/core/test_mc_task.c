#include "mc_task.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define C(x)do{if(!(x)){fprintf(stderr,"FAIL %d %s\n",__LINE__,#x);exit(1);}}while(0)
typedef struct call {gh_mc_task_state *state;gh_mc_task_bindings *bindings;gh_mc_task_status result;int done;} call;
static void run_task(void *context,int32_t arg)
{call *c=context;C(arg==4);c->result=gh_mc_task_prepare_002ae8a8(c->state,c->bindings);c->done=1;}
static unsigned run(call *c,int corrupt_module)
{
    gh_task_record32 records[3]={{0}};gh_task_manager manager={records,3,0,-1,3,0,0,0};
    int32_t slot;unsigned ticks=0;
    C(gh_task_threads_create(&manager,&c->bindings->threads)==GH_THREAD_OK);
    C(gh_task_threads_start(c->bindings->threads,-1,run_task,c,4,&slot)==GH_THREAD_OK);
    while(!c->done && ticks<10) {
        C(gh_tasks_run_matching_002d52a0(&manager).status==GH_TASK_OK);
        C(gh_dvd_pump(c->bindings->dvd)==GH_DVD_OK);
        if(corrupt_module && ticks==0)((unsigned char*)c->state->module.pointer)[100]^=1;
        ++ticks;
    }
    C(c->done && gh_task_threads_destroy(c->bindings->threads)==GH_THREAD_OK);
    return ticks;
}
int main(int argc,char **argv)
{
    gh_disc *disc;gh_heap *heap,*module_heap;gh_dvd_jobs dvd;gh_r207_allocation allocation;
    gh_disc_entry entry;gh_bundle language;gh_bundle_entry idd;size_t index,baseline;
    gh_id_state *id;gh_id_runtime *runtime;gh_font_state font={0};gh_packed_message_state packed;
    gh_boot_work work={0};gh_mc_task_state state={0};gh_mc_task_bindings bindings={0};
    gh_memory_card *card;int32_t game_suspend=0;gh_id_entry *e;uint32_t busy;call c;unsigned ticks;
    C(argc==2 && gh_disc_open(argv[1],&disc)==GH_DISC_OK);
    C(gh_heap_create(8*1024*1024,&heap)==GH_MEM_OK && gh_heap_create(1024*1024,&module_heap)==GH_MEM_OK);
    gh_dvd_jobs_init(&dvd,disc);C(gh_dvd_read_alloc(&dvd,"etc/core_usa.bin",&allocation,heap,0,0,0,0));
    C(gh_dvd_pump(&dvd)==GH_DVD_OK);C(gh_disc_find(disc,"etc/core_usa.bin",&index)==GH_DISC_OK);
    C(gh_disc_entry_at(disc,index,&entry)==GH_DISC_OK && gh_bundle_open(allocation.pointer,(size_t)entry.size,&language)==GH_BUNDLE_OK);
    id=calloc(1,sizeof(*id));C(id && gh_id_find_language_data(&language,&idd)==GH_MENU_OK);
    C(gh_id_system_init_002acc70(id,&idd)==GH_MENU_OK && gh_id_runtime_create(id,1,&runtime)==GH_ID_OK);
    C(gh_font_initialize(&font,&language,1)==GH_FONT_OK);
    /* Service opening does not fake an inserted card or create profile data.
     * This prefix ends before first GetInfo/card check, as the original order. */
    C(gh_memory_card_open("G:\\middle_thing\\godhand_windows\\build\\mc-task-tests\\absent-profile",&card)==GH_MC_OK);
    work.language=1;work.phase569=2;work.flags=0x80000;
    memset(&packed,0xa5,sizeof(packed));
    bindings.dvd=&dvd;bindings.module_heap=module_heap;bindings.subscreen_heap=heap;bindings.card=card;
    bindings.id_manager=id;bindings.id_runtime=runtime;bindings.fonts=&font;bindings.packed=&packed;
    bindings.language=&language;bindings.work=&work;bindings.game_suspend_word=&game_suspend;
    baseline=gh_heap_used(heap);c=(call){&state,&bindings,GH_MC_TASK_ARGUMENT,0};ticks=run(&c,0);
    C(ticks==3 && c.result==GH_MC_TASK_CONTROL_FRONTIER && state.frontier_offset==0x2e8);
    C(state.stage==4 && state.module_loaded && state.mc_sys_initialized && state.ui_initialized);
    C(state.flags==0xc9000000u && game_suspend==1 && (work.flags&0x4000));
    memcpy(&busy,(const unsigned char*)&work+0x5e8,4);C(busy&0x80000000u);
    C(state.system_phase_dc==2 && state.checks.icon_work.pointer && state.checks.phase==0 && state.checks.frame==0);
    C(state.id_resource.pair_bound && state.id_base.initialized && state.id_base.entry_count==67 && state.id_base.playing);
    C(packed.initialized && state.id_base.packed.bytes && packed.cursor.bytes);
    C(gh_id_base_get_work_002aa410(runtime,&state.id_base,0x40,&e,0)==GH_ID_OK && e->pos_y==310 && e->scale_x==1 && e->scale_y==1.4f);
    C(gh_id_base_get_work_002aa410(runtime,&state.id_base,0x41,&e,0)==GH_ID_OK && e->pos_y==213);
    C(gh_id_base_get_work_002aa410(runtime,&state.id_base,0x51,&e,0)==GH_ID_OK && e->pos_y==388);
    C(gh_id_base_get_work_002aa410(runtime,&state.id_base,0x55,&e,0)==GH_ID_OK && e->pos_y==365);
    for(int n=0;n<3;++n)C(gh_id_base_get_work_002aa410(runtime,&state.id_base,0x14+n,&e,0)==GH_ID_OK && e->seed==(uint32_t)n);
    C(gh_mc_task_abort(&state,&bindings)==GH_MC_TASK_OK);
    C(!state.id_base.entries && !id->pool_used && !id->job_count && !id->pair[11].bound);
    C(!game_suspend && !(work.flags&0x4000) && gh_heap_used(module_heap)==0 && gh_heap_used(heap)==baseline);
    memcpy(&busy,(const unsigned char*)&work+0x5e8,4);C(!(busy&0x80000000u));
    /* Different bytes may never execute the pinned native source version. */
    memset(&state,0,sizeof(state));c=(call){&state,&bindings,GH_MC_TASK_ARGUMENT,0};ticks=run(&c,1);
    C(ticks==2 && c.result==GH_MC_TASK_MODULE_VERSION && !state.mc_sys_initialized && !state.checks.icon_work.pointer);
    C(gh_mc_task_abort(&state,&bindings)==GH_MC_TASK_OK && gh_heap_used(module_heap)==0 && gh_heap_used(heap)==baseline);
    gh_memory_card_close(card);C(gh_id_runtime_destroy(runtime)==GH_ID_OK);free(id);
    C(gh_heap_release(heap,allocation)==GH_MEM_OK && gh_heap_destroy(heap)==GH_MEM_OK && gh_heap_destroy(module_heap)==GH_MEM_OK);
    gh_disc_close(disc);puts("PASS realMC body through module/version/5000work/IDBase/packed/DispInit; exactControl frontier, not task completion");return 0;
}
