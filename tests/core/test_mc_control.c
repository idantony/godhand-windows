#include "mc_control.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define C(x)do{if(!(x)){fprintf(stderr,"FAIL %d %s\n",__LINE__,#x);exit(1);}}while(0)
typedef struct flow {
    gh_mc_task_state state;gh_mc_task_bindings b;gh_mc_control_state control;
    unsigned draws;uint32_t first_entry,first_consumer;uint8_t first_kind;int done;
} flow;
static gh_id_error stop_at_gpu(void *context,const gh_id_draw_job *job)
{
    flow *f=context;gh_id_texture_quad quad;unsigned char *pixels;
    ++f->draws;f->first_entry=job->entry_index;f->first_consumer=job->consumer_address;f->first_kind=job->entry->kind;
    if(job->entry->kind==0) {
        C(gh_id_texture_quad_prepare(f->b.id_manager,&f->state.id_base,job->entry,&quad)==GH_ID_OK);
        pixels=malloc((size_t)quad.texture.width*quad.texture.height*4);C(pixels);
        C(gh_id_texture_rgba(&quad.texture,pixels,(size_t)quad.texture.width*quad.texture.height*4)==GH_ID_OK);
        free(pixels);
    }
    /* Real geometry/texture consumer preparation above; GPU submission is
     * deliberately unimplemented in this headless test, never reported done. */
    return GH_ID_UNSUPPORTED;
}
static void task(void *context,int32_t arg)
{
    flow *f=context;gh_id_entry *e;
    C(arg==5 && gh_mc_task_prepare_002ae8a8(&f->state,&f->b)==GH_MC_TASK_CONTROL_FRONTIER);
    C(gh_mc_control_advance(&f->control,&f->state,&f->b)==GH_MC_BOOT_OK);
    C(f->state.checks.frame==1 && f->state.checks.phase==0 && f->state.checks.subphase==2);
    C(gh_id_base_get_work_002aa410(f->b.id_runtime,&f->state.id_base,0x40,&e,0)==GH_ID_OK && !(e->flags&0x20000000));
    C(gh_id_base_get_work_002aa410(f->b.id_runtime,&f->state.id_base,0x41,&e,0)==GH_ID_OK && e->message==0x2007);
    C(gh_id_base_get_work_002aa410(f->b.id_runtime,&f->state.id_base,0x55,&e,0)==GH_ID_OK && e->message==0x2005);
    C(gh_mc_control_sleep_frame(&f->control,&f->state,&f->b,0,0,stop_at_gpu,f)==GH_ID_UNSUPPORTED);
    C(f->draws==1 && f->control.first_unported==f->first_consumer && f->state.id_base.frame==1);
    f->done=1;
}
int main(int argc,char **argv)
{
    flow f={0};gh_disc *disc;gh_heap *heap,*module_heap;gh_dvd_jobs dvd;gh_r207_allocation language_data;
    gh_disc_entry source;size_t index;gh_bundle language;gh_bundle_entry idd;gh_id_state *id;
    gh_id_runtime *runtime;gh_font_state fonts={0};gh_packed_message_state packed={0};gh_boot_work work={0};
    gh_memory_card *card;int32_t game_suspend=0,slot;unsigned ticks=0;
    gh_task_record32 records[3]={{0}};gh_task_manager manager={records,3,0,-1,3,0,0,0};
    C((argc==2 || argc==3) && gh_disc_open(argv[1],&disc)==GH_DISC_OK);
    C(gh_heap_create(8*1024*1024,&heap)==GH_MEM_OK && gh_heap_create(1024*1024,&module_heap)==GH_MEM_OK);
    gh_dvd_jobs_init(&dvd,disc);C(gh_dvd_read_alloc(&dvd,"etc/core_usa.bin",&language_data,heap,0,0,0,0));
    C(gh_dvd_pump(&dvd)==GH_DVD_OK && gh_disc_find(disc,"etc/core_usa.bin",&index)==GH_DISC_OK);
    C(gh_disc_entry_at(disc,index,&source)==GH_DISC_OK && gh_bundle_open(language_data.pointer,(size_t)source.size,&language)==GH_BUNDLE_OK);
    id=calloc(1,sizeof(*id));C(id && gh_id_find_language_data(&language,&idd)==GH_MENU_OK);
    C(gh_id_system_init_002acc70(id,&idd)==GH_MENU_OK && gh_id_runtime_create(id,1,&runtime)==GH_ID_OK);
    C(gh_font_initialize(&fonts,&language,1)==GH_FONT_OK);
    C(gh_memory_card_open("G:\\middle_thing\\godhand_windows\\build\\mc-control-tests\\absent-profile",&card)==GH_MC_OK);
    work.language=1;work.phase569=2;
    f.b=(gh_mc_task_bindings){&dvd,module_heap,heap,0,card,id,runtime,&fonts,&packed,&language,&work,&game_suspend};
    C(gh_task_threads_create(&manager,&f.b.threads)==GH_THREAD_OK);
    C(gh_task_threads_start(f.b.threads,-1,task,&f,5,&slot)==GH_THREAD_OK);
    while(!f.done && ticks<10){C(gh_tasks_run_matching_002d52a0(&manager).status==GH_TASK_OK);C(gh_dvd_pump(&dvd)==GH_DVD_OK);++ticks;}
    C(f.done && ticks==3); /* did not silently sleep after unsupported draw */
    if(argc==3) {
        gh_id_draw_job jobs[640];gh_id_frontier frontier;size_t count,i;FILE *out=fopen(argv[2],"wbx");C(out);
        C(gh_id_jobs_collect_visible(&f.state.id_base,jobs,640,&count,&frontier)==GH_ID_OK);
        fputs("{\"visible\":[",out);
        for(i=0;i<count;++i) {
            gh_id_entry *e=jobs[i].entry;
            fprintf(out,"%s{\"index\":%u,\"id\":%d,\"kind\":%u,\"flags\":%u,\"layer\":%d,\"slot\":%u,\"texture_index\":%u,\"message\":%u,\"draw_x\":%.9g,\"draw_y\":%.9g,\"draw_sx\":%.9g,\"draw_sy\":%.9g,\"f68\":%.9g,\"f6c\":%.9g,\"consumer\":%u}",
                i?",":"",jobs[i].entry_index,e->id,e->kind,e->flags,e->layer,e->b8a,e->b8b,e->message,
                (double)e->draw_x,(double)e->draw_y,(double)e->draw_scale_x,(double)e->draw_scale_y,(double)e->f68,(double)e->f6c,jobs[i].consumer_address);
        }
        fputs("]}\n",out);C(!fclose(out));
    }
    C(gh_mc_control_advance(&f.control,&f.state,&f.b)==GH_MC_BOOT_OK && f.state.checks.phase==3 && f.state.checks.error==1);
    C(gh_mc_control_advance(&f.control,&f.state,&f.b)==GH_MC_BOOT_UNSUPPORTED && f.control.first_unported==0xbd8);
    C(gh_mc_task_abort(&f.state,&f.b)==GH_MC_TASK_OK);
    C(gh_task_threads_destroy(f.b.threads)==GH_THREAD_OK && gh_id_runtime_destroy(runtime)==GH_ID_OK);
    free(id);gh_memory_card_close(card);C(gh_heap_release(heap,language_data)==GH_MEM_OK);
    C(gh_heap_destroy(heap)==GH_MEM_OK && gh_heap_destroy(module_heap)==GH_MEM_OK);gh_disc_close(disc);
    printf("PASS originalMC Control fields/realcard/IDMove to explicit draw frontier: entry%u kind%u consumer%08x; no fake rendered frame\n",f.first_entry,f.first_kind,f.first_consumer);
    return 0;
}
