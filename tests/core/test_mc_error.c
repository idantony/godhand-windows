#include "mc_error.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define C(x)do{if(!(x)){fprintf(stderr,"FAIL %d %s\n",__LINE__,#x);exit(1);}}while(0)
typedef struct fixture {gh_mc_task_state mc;gh_mc_task_bindings b;gh_mc_control_state control;gh_mc_error_state error;int done;} fixture;
static void put_input(gh_boot_work *w,size_t at,uint32_t value){memcpy((unsigned char*)w+at,&value,4);}
static void task(void *context,int32_t arg)
{
    fixture *f=context;gh_id_entry *entry,*target;unsigned i;
    C(arg==6 && gh_mc_task_prepare_002ae8a8(&f->mc,&f->b)==GH_MC_TASK_CONTROL_FRONTIER);
    C(gh_mc_control_advance(&f->control,&f->mc,&f->b)==GH_MC_BOOT_OK);
    C(gh_mc_control_advance(&f->control,&f->mc,&f->b)==GH_MC_BOOT_OK && f->mc.checks.phase==3 && f->mc.checks.error==1);
    memset(f->control.progress,0xa5,sizeof(f->control.progress));
    C(gh_mc_error_process_00000bd8(&f->error,&f->control,&f->mc,&f->b,0,0)==GH_MC_BOOT_OK);
    C(f->mc.checks.subphase==1 && f->mc.checks.mode==0x12 && !f->mc.checks.done);
    for(i=0;i<sizeof(f->control.progress);++i)C(f->control.progress[i]==0);
    C(gh_mc_error_set_display(&f->error,&f->control,&f->mc,&f->b)==GH_ID_OK);
    C(gh_id_base_get_work_002aa410(f->b.id_runtime,&f->mc.id_base,0x41,&entry,0)==GH_ID_OK && entry->message==0x201f);
    C(gh_id_base_get_work_002aa410(f->b.id_runtime,&f->mc.id_base,0x40,&entry,0)==GH_ID_OK && !(entry->flags&0x20000000));
    for(i=0;i<5;++i)C(gh_mc_error_process_00000bd8(&f->error,&f->control,&f->mc,&f->b,0,0)==GH_MC_BOOT_OK && !f->mc.checks.done);
    put_input(f->b.work,0x1a0,0x10000000);
    C(gh_mc_error_process_00000bd8(&f->error,&f->control,&f->mc,&f->b,0,0)==GH_MC_BOOT_UNSUPPORTED);
    C(f->error.first_unported==0x2cb8a0 && f->mc.checks.subphase==1 && !f->mc.checks.done);
    /* Structural injected confirmation-state cases: no fake sound callback
     * is installed merely to enter/complete a game branch. */
    f->mc.checks.subphase=2;f->error.choice=1;
    C(gh_mc_error_process_00000bd8(&f->error,&f->control,&f->mc,&f->b,0,0)==GH_MC_BOOT_UNSUPPORTED && !f->mc.checks.done);
    put_input(f->b.work,0x1a0,0);put_input(f->b.work,0x1b0,0x04000000);
    C(gh_mc_error_process_00000bd8(&f->error,&f->control,&f->mc,&f->b,0,0)==GH_MC_BOOT_UNSUPPORTED && f->error.choice==1);
    put_input(f->b.work,0x1b0,0);f->error.choice=0;
    C(gh_mc_error_set_display(&f->error,&f->control,&f->mc,&f->b)==GH_ID_OK);
    C(gh_id_base_get_work_002aa410(f->b.id_runtime,&f->mc.id_base,0x55,&entry,0)==GH_ID_OK && entry->message==0x2003);
    C(gh_id_base_get_work_002aa410(f->b.id_runtime,&f->mc.id_base,0x56,&entry,0)==GH_ID_OK && entry->message==0x2004);
    C(gh_id_base_get_work_002aa410(f->b.id_runtime,&f->mc.id_base,0x50,&entry,0)==GH_ID_OK);
    C(gh_id_base_get_work_002aa410(f->b.id_runtime,&f->mc.id_base,0x52,&target,0)==GH_ID_OK && entry->pos_x==target->pos_x && entry->pos_y==target->pos_y);
    f->done=1;
}
int main(int argc,char **argv)
{
    fixture f={0};gh_disc *disc;gh_heap *heap,*module_heap;gh_dvd_jobs dvd;gh_r207_allocation allocation;
    gh_disc_entry source;size_t index;gh_bundle language;gh_bundle_entry idd;gh_id_state *id;gh_id_runtime *runtime;
    gh_font_state font={0};gh_packed_message_state packed={0};gh_boot_work work={0};gh_memory_card *card;
    int32_t game=0,slot;unsigned ticks=0;gh_task_record32 records[3]={{0}};gh_task_manager manager={records,3,0,-1,3,0,0,0};
    C(argc==2 && gh_disc_open(argv[1],&disc)==GH_DISC_OK);C(gh_heap_create(8*1024*1024,&heap)==GH_MEM_OK && gh_heap_create(1024*1024,&module_heap)==GH_MEM_OK);
    gh_dvd_jobs_init(&dvd,disc);C(gh_dvd_read_alloc(&dvd,"etc/core_usa.bin",&allocation,heap,0,0,0,0));C(gh_dvd_pump(&dvd)==GH_DVD_OK);
    C(gh_disc_find(disc,"etc/core_usa.bin",&index)==GH_DISC_OK && gh_disc_entry_at(disc,index,&source)==GH_DISC_OK);
    C(gh_bundle_open(allocation.pointer,(size_t)source.size,&language)==GH_BUNDLE_OK);
    id=calloc(1,sizeof(*id));C(id && gh_id_find_language_data(&language,&idd)==GH_MENU_OK && gh_id_system_init_002acc70(id,&idd)==GH_MENU_OK);
    C(gh_id_runtime_create(id,1,&runtime)==GH_ID_OK && gh_font_initialize(&font,&language,1)==GH_FONT_OK);
    C(gh_memory_card_open("G:\\middle_thing\\godhand_windows\\build\\mc-error-tests\\absent-profile",&card)==GH_MC_OK);
    work.language=1;work.phase569=2;f.b=(gh_mc_task_bindings){&dvd,module_heap,heap,0,card,id,runtime,&font,&packed,&language,&work,&game};
    C(gh_task_threads_create(&manager,&f.b.threads)==GH_THREAD_OK && gh_task_threads_start(f.b.threads,-1,task,&f,6,&slot)==GH_THREAD_OK);
    while(!f.done && ticks<10){C(gh_tasks_run_matching_002d52a0(&manager).status==GH_TASK_OK);C(gh_dvd_pump(&dvd)==GH_DVD_OK);++ticks;}
    C(f.done && ticks==3 && !f.mc.checks.done);C(gh_mc_task_abort(&f.mc,&f.b)==GH_MC_TASK_OK);
    C(gh_task_threads_destroy(f.b.threads)==GH_THREAD_OK && gh_id_runtime_destroy(runtime)==GH_ID_OK);free(id);gh_memory_card_close(card);
    C(gh_heap_release(heap,allocation)==GH_MEM_OK && gh_heap_destroy(heap)==GH_MEM_OK && gh_heap_destroy(module_heap)==GH_MEM_OK);gh_disc_close(disc);
    puts("PASS realMC no-card ErrorProc/display/choice, and explicit SeCall frontier before input completion; no fake sound or boot success");return 0;
}
