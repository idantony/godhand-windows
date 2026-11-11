#include "mc_title_load.h"
#include "mc_exit.h"
#include "game_scene_aux.h"
#include "game_loaded_room.h"
#include "audio_startup.h"
#ifdef GH_MC_TITLE_SELECT_TEST
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "bgm_requests.h"
#endif
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
typedef struct title_call {
    gh_mc_task_state *state;
    gh_mc_task_bindings *bindings;
    gh_mc_task_status result;
    gh_screen_fade fade;
    gh_screen_fade_context fade_context;
    unsigned first_fade_frames;
    int done;
} title_call;

static void run_title_task(void *context,int32_t argument)
{
    title_call *call=context;
    C(argument==0);
    C(gh_mc_task_prepare_title_load_002ae838(call->state,call->bindings)==GH_MC_TASK_OK);
    C(call->state->load_mode==1&&call->state->load_phase==0&&call->state->checks.mode==1);
    C(call->state->flags==0x88400000u&&!call->state->ui_initialized);
    C(call->state->load_payload.pointer&&((uintptr_t)call->state->load_payload.pointer%128)==0);
    do {
        call->result=gh_mc_task_title_load_step_000012b0(
            call->state,call->bindings,&call->fade,&call->fade_context);
        C(call->result==GH_MC_TASK_LOAD_FRAME);
        if(call->state->load_phase==1){
            C(call->fade.color_from==0&&call->fade.color_to==0xff000000u);
            C(gh_mc_task_title_load_sleep_00008aa8(call->state,call->bindings)==GH_MC_TASK_OK);
        }
    } while(call->state->load_phase==1);
    call->done=1;
}

static void run_title(title_call *call)
{
    gh_task_record32 records[3]={{0}};
    gh_task_manager manager={records,3,0,-1,3,0,0,0};
    C(gh_task_threads_create(&manager,&call->bindings->threads)==GH_THREAD_OK);
    int32_t slot;
    C(gh_mc_task_start_title_load_002ae7f0(call->bindings,run_title_task,call,&slot)==GH_MC_TASK_OK&&slot==2);
    for(unsigned tick=0;!call->done&&tick<40;++tick){
        C(gh_tasks_run_matching_002d52a0(&manager).status==GH_TASK_OK);
        C(gh_dvd_pump(call->bindings->dvd)==GH_DVD_OK);
        if(call->state->load_phase==1&&!(call->fade.state&4)){
            gh_fade_draw draw;
            C(gh_screen_fade_step_00307388(&call->fade,&call->fade_context,&draw)==GH_FADE_OK);
            if(draw.visible)++call->first_fade_frames;
        }
    }
    C(call->done&&call->first_fade_frames==11);
    C(gh_task_threads_destroy(call->bindings->threads)==GH_THREAD_OK);
}


#ifdef GH_MC_TITLE_SELECT_TEST
static void save_fixture(const char *path,const void *bytes,size_t count)
{
    FILE *file=fopen(path,"wb");
    C(file&&fwrite(bytes,1,count,file)==count&&!fclose(file));
}
#endif

typedef struct {
    gh_audio_registry *registry;
    unsigned count;
    unsigned programs[12];
} title_sounds;

static int inspect_original_sound(void *context,const gh_mc_sound_request *request)
{
    title_sounds *sound=context;
    uint8_t descriptor[48];
    C(sound->count<12&&request->bank==0&&request->parameter3==-1&&request->parameter4==-1&&
      !request->parameter5&&!request->parameter6);
    C(gh_audio_get_program(sound->registry,0,request->sound_id,0,descriptor)==GH_AUDIO_OK);
    sound->programs[sound->count++]=(unsigned)request->sound_id;
    return 0;
}

#ifdef GH_MC_TITLE_SELECT_TEST
static gh_mc_load_status apply_idle_output(void *context,uint32_t mode,uint32_t *frontier)
{
    gh_bgm_requests *bgm=context;
    if(gh_bgm_requests_restore_idle_output_mode_00375700(bgm,mode)==GH_BGM_OK)
        return GH_MC_LOAD_RESTORED;
    *frontier=gh_bgm_requests_frontier(bgm);
    return GH_MC_LOAD_UNSUPPORTED;
}
#endif

typedef struct {
    gh_mc_exit_state exit;
    gh_mc_title_load *load;
    gh_mc_task_state *task;
    gh_mc_task_bindings *bindings;
    gh_mc_control_state *control;
    gh_screen_fade fade;
    gh_screen_fade_context context;
    unsigned fade_frames,no_ui_sleeps;
    int done;
} title_exit_fixture;

static gh_id_error unexpected_exit_draw(void *context,const gh_id_draw_job *job)
{
    (void)context;(void)job;
    return GH_ID_UNSUPPORTED;
}

static void exit_title_task(void *context,int32_t argument)
{
    title_exit_fixture *test=context;
    C(argument==0);
    for(;;){
        gh_mc_task_status status=gh_mc_title_exit_step_00001458(&test->exit,test->load,test->task,
            test->bindings,&test->fade,&test->context);
        if(status==GH_MC_TASK_OK)break;
        C(status==GH_MC_TASK_LOAD_FRAME);
        if(test->task->ui_initialized){
            C(test->task->load_phase==3&&test->task->load_payload.pointer);
            C(gh_mc_control_sleep_frame(test->control,test->task,test->bindings,NULL,NULL,
                unexpected_exit_draw,NULL)==GH_ID_OK);
        }else{
            C(test->task->load_phase==4&&!test->task->load_payload.pointer&&!test->task->checks.icon_work.pointer);
            C(test->task->module_loaded&&*test->bindings->game_suspend_word==0);
            ++test->no_ui_sleeps;
            C(gh_mc_task_title_load_sleep_00008aa8(test->task,test->bindings)==GH_MC_TASK_OK);
        }
    }
    C(test->exit.body_finished&&!test->exit.tail_entry&&test->exit.frontier_address==0x2d5a48);
    test->done=1;
}

static gh_scene_aux_status title_task_already_started(void *context)
{
    const gh_mc_task_state *task=context;
    C(task->load_mode&&task->load_phase==2&&task->checks.done);
    return 0;
}

static void run_title_exit(gh_mc_title_load *load,gh_mc_task_state *task,gh_mc_task_bindings *bindings,
    gh_mc_control_state *control)
{
    title_exit_fixture fixture={.load=load,.task=task,.bindings=bindings,.control=control,
        .context={512,1908,0,0}};
    gh_game_scene_aux cursor={0};
    C(gh_scene_cursor_open_load_002bf268(&cursor,bindings->work)==GH_AUX_OK);
    C(gh_scene_cursor_step_002bf2d0(&cursor,bindings->work,title_task_already_started,task)==GH_AUX_OK);
    C(cursor.save_load[0]==2&&cursor.save_load[1]==1&&cursor.load_flags_saved);

    /* Isolate task lifetime with the real ID hide branch. A positive draw
     * reaching the test callback fails, rather than acknowledging it. */
    task->id_base.hide=1;
    gh_task_record32 records[3]={{0}};
    gh_task_manager manager={records,3,0,-1,3,0,0,0};
    C(gh_task_threads_create(&manager,&bindings->threads)==GH_THREAD_OK);
    int32_t slot;
    C(gh_task_threads_start(bindings->threads,2,exit_title_task,&fixture,0,&slot)==GH_THREAD_OK&&slot==2);
    for(unsigned tick=0;!fixture.done&&tick<20;++tick){
        C(gh_tasks_run_matching_002d52a0(&manager).status==GH_TASK_OK);
        if(task->load_phase==3){
            gh_fade_draw draw;
            C(gh_screen_fade_step_00307388(&fixture.fade,&fixture.context,&draw)==GH_FADE_OK&&draw.visible);
            ++fixture.fade_frames;
        }
    }
    C(fixture.done&&fixture.fade_frames==11&&fixture.no_ui_sleeps==1&&!records[2].running);
    C(gh_task_threads_destroy(bindings->threads)==GH_THREAD_OK);
    C(!load->selected.borrowed_record&&!task->module_loaded&&task->stage==5&&task->load_phase==5);
    C(gh_memory_card_request_info(bindings->card)==GH_MC_ENDED);

    if(!(bindings->work->flags&0x40)){
        C(gh_scene_cursor_step_002bf2d0(&cursor,bindings->work,NULL,NULL)==GH_AUX_OK&&
          cursor.save_load[0]==5&&cursor.save_load[0xc]==0);
        gh_scene_loaded_room transition={0};
        const gh_game_door_bindings door={.work=bindings->work,.fade=&fixture.fade};
        const gh_scene_loaded_room_bindings handoff={.door_bindings=&door};
        C(gh_scene_cursor_loaded_room_002bf568(&cursor,&transition,&handoff)==GH_AUX_OK);
        C(!cursor.save_load[0]&&!cursor.load_flags_saved&&!transition.stage);
        puts("PASS phase7 cancellation fades, frees task, and returns through title cursor without Door");
        return;
    }
    C(gh_scene_cursor_step_002bf2d0(&cursor,bindings->work,NULL,NULL)==GH_AUX_OK&&
      cursor.save_load[0]==4&&cursor.save_load[0xc]==1);
    cursor.save_load[1]=1;
    C(gh_scene_cursor_step_002bf2d0(&cursor,bindings->work,NULL,NULL)==GH_AUX_OK&&cursor.save_load[0]==4);
    cursor.save_load[1]=0;
    C(gh_scene_cursor_step_002bf2d0(&cursor,bindings->work,NULL,NULL)==GH_AUX_OK&&
      cursor.save_load[0]==5&&cursor.save_load[0xc]==1);
    C(gh_scene_cursor_step_002bf2d0(&cursor,bindings->work,NULL,NULL)==GH_AUX_UNSUPPORTED&&
      cursor.frontier==0x2bf568);
    puts("PASS original title fade, End and current task return through cursor 2BF540");
}

#ifdef GH_MC_TITLE_SELECT_TEST
static void selected_fixture(gh_disc *disc,const char *scratch_arg,gh_mc_task_state *task,
                             const gh_mc_task_bindings *original_bindings)
{
    char scratch[1200],profile[1400],folder[1500],icon_path[1600],file_path[1600];
    const char suffix[]="\\memory-card-tests\\scratch";
    C(GetFullPathNameA(scratch_arg,sizeof(scratch),scratch,NULL));
    size_t length=strlen(scratch);
    C(length>=sizeof(suffix)-1&&!_stricmp(scratch+length-(sizeof(suffix)-1),suffix));
    C(snprintf(profile,sizeof(profile),"%s\\title-select-%lu",scratch,(unsigned long)GetCurrentProcessId())>0);
    C(CreateDirectoryA(profile,NULL));
    C(snprintf(folder,sizeof(folder),"%s\\BASLUS-21503GODHD003",profile)>0);
    C(CreateDirectoryA(folder,NULL));
    C(snprintf(icon_path,sizeof(icon_path),"%s\\icon.sys",folder)>0);
    C(snprintf(file_path,sizeof(file_path),"%s\\BASLUS-21503GODHD003",folder)>0);
    static uint8_t record[GH_MC_SAVE_RECORD_BYTES],icon[0x3c4];
    record[0]=0xcd;record[1]=0xcc;record[2]=0xcc;record[3]=0x3d;
    record[0x19]=0x27;record[0x2d]=1;record[0xa0]=100;record[0xa4]=60;
    record[0x190]=1;record[0x198]=1;
    const uint64_t checksum=UINT64_C(0x9be6fa3b72afda1d)^UINT64_C(0x3dcccccd)^
        (UINT64_C(0x27)<<8)^(UINT64_C(1)<<40)^UINT64_C(0x3c00000064);
    memcpy(record+0xb110,&checksum,8);

    gh_memory_card *card;
    C(gh_memory_card_open(profile,&card)==GH_MC_OK);
    gh_mc_task_bindings bindings=*original_bindings;
    bindings.card=card;
    gh_boot_work *work=bindings.work;
    gh_mc_load_scan scan={0};
    gh_mc_control_state control={0};
    gh_mc_load_menu_memory menu={-1,-1};
    uint32_t frontier;
    unsigned ticks=0;
    while(task->checks.phase<3&&ticks++<15)
        C(gh_mc_control_scan_title_00001590(&control,&scan,task,&bindings,&menu,&frontier)==GH_MC_BOOT_OK&&!frontier);
    C(task->checks.phase==7&&task->checks.error==8&&!scan.menu_ready);

    static gh_game_init_state init;
    static gh_game_prepare prepare;
    static uint8_t core[GH_CORE_SAVE_BYTES],payload[GH_CHECKPOINT_BYTES],raw[GH_TITLE_PLAYER_BYTES];
    gh_game_checkpoint checkpoint={0};
    gh_game_door door={0};
    gh_title_player_view player={.bytes=raw,.size=sizeof(raw),.alias_f0=raw+0xb0};
    const gh_checkpoint_bindings checkpoint_bindings={.work=work,.save={core,sizeof(core)},
        .prepare=&prepare,.init=&init,.player=&player,.destination=payload,.destination_size=sizeof(payload)};
    gh_audio_registry *registry;
    gh_bgm_requests *bgm;
    gh_heap *audio_heap;
    gh_sound_control sound={0};
    C(gh_audio_registry_create(&registry)==GH_AUDIO_OK&&gh_audio_startup_load(registry,disc)==GH_AUDIO_OK);
    C(gh_sound_control_from_startup(&sound,disc,registry)==GH_VOICE_OK);
    C(gh_heap_create(32*1024*1024,&audio_heap)==GH_MEM_OK);
    C(gh_bgm_requests_create(registry,&sound,disc,audio_heap,&bgm)==GH_BGM_OK);
    title_sounds sounds={.registry=registry};
    gh_mc_title_load_providers providers={.checkpoint=&checkpoint,.checkpoint_bindings=&checkpoint_bindings,
        .door=&door,.output_mode=apply_idle_output,.audio_context=bgm,
        .sound=inspect_original_sound,.sound_context=&sounds};
    static gh_mc_title_load load;
    uint8_t *work_bytes=(uint8_t*)work;
    uint32_t pressed=0,repeat=0;
#define VISIT(expected) C(gh_mc_control_title_select_00001590(&load,&control,&scan,task,&bindings,&menu,&providers,&frontier)==(expected)&&!frontier)
#define INPUT(p,r) do{pressed=(p);repeat=(r);memcpy(work_bytes+0x1a0,&pressed,4);memcpy(work_bytes+0x1b0,&repeat,4);}while(0)
    VISIT(GH_MC_LOAD_PENDING);
    C(task->checks.subphase==1&&task->checks.mode==0x18);
    gh_id_entry *entry;
    C(gh_id_base_get_work_002aa410(bindings.id_runtime,&task->id_base,0x41,&entry,NULL)==GH_ID_OK);
    C(entry->message==0x2008);
    INPUT(0x20000000,0);VISIT(GH_MC_LOAD_PENDING);
    C(task->checks.subphase==2&&!sounds.count);
    INPUT(0,0);VISIT(GH_MC_LOAD_PENDING);
    C(task->checks.mode==0x16);
    C(gh_id_base_get_work_002aa410(bindings.id_runtime,&task->id_base,0x43,&entry,NULL)==GH_ID_OK);
    C(entry->message==0x201d);
    INPUT(0x10000000,0);VISIT(GH_MC_LOAD_PENDING);
    C(task->checks.subphase==3&&sounds.count==1&&sounds.programs[0]==0x15e);
    INPUT(0,0);VISIT(GH_MC_LOAD_PENDING);
    C(task->checks.phase==0&&!task->checks.subphase);
    save_fixture(icon_path,icon,sizeof(icon));
    save_fixture(file_path,record,sizeof(record));
    ticks=0;
    while(task->checks.phase<3&&ticks++<15)
        C(gh_mc_control_scan_title_00001590(&control,&scan,task,&bindings,&menu,&frontier)==GH_MC_BOOT_OK&&!frontier);
    C(task->checks.phase==3&&scan.menu_ready&&scan.first_slot==2&&scan.selected_row==1);
    INPUT(0x20000000,0);VISIT(GH_MC_LOAD_PENDING);
    C(task->checks.phase==7&&task->checks.error==-1&&!task->checks.subphase);
    C(sounds.count==2&&sounds.programs[1]==0x15e);
    INPUT(0,0);VISIT(GH_MC_LOAD_PENDING);
    C(task->checks.subphase==2);
    VISIT(GH_MC_LOAD_PENDING);
    C(task->checks.mode==0x16&&!load.choice);
    INPUT(0x10000000,0);VISIT(GH_MC_LOAD_PENDING);
    C(task->checks.subphase==3&&sounds.count==3&&sounds.programs[2]==0x15e);
    INPUT(0,0);VISIT(GH_MC_LOAD_PENDING);
    C(task->checks.phase==0&&!task->checks.subphase);
    ticks=0;
    while(task->checks.phase<3&&ticks++<15)
        C(gh_mc_control_scan_title_00001590(&control,&scan,task,&bindings,&menu,&frontier)==GH_MC_BOOT_OK&&!frontier);
    C(task->checks.phase==3&&scan.menu_ready&&scan.first_slot==2&&scan.selected_row==1);
    sounds.count=0;
    puts("PASS actual no-slot notice and list cancel/retry through real card profile");
    INPUT(0,0x02000000);VISIT(GH_MC_LOAD_PENDING);
    C(scan.selected_row==2&&task->checks.phase==3);
    INPUT(0x10000000,0);VISIT(GH_MC_LOAD_PENDING);
    C(task->checks.phase==3&&task->checks.subphase==1&&sounds.programs[1]==0x122);
    INPUT(0,0x01000000);VISIT(GH_MC_LOAD_PENDING);
    C(scan.selected_row==1);
    INPUT(0x10000000,0);VISIT(GH_MC_LOAD_PENDING);
    C(task->checks.phase==3&&task->checks.subphase==4);
    INPUT(0,0);VISIT(GH_MC_LOAD_PENDING);
    C(task->checks.phase==4&&load.selected_slot==3);
    VISIT(GH_MC_LOAD_PENDING);
    C(load.choice==1&&task->checks.mode==9);
    INPUT(0,0x04000000);VISIT(GH_MC_LOAD_PENDING);C(!load.choice);
    VISIT(GH_MC_LOAD_PENDING);C(load.choice);
    INPUT(0x10000000,0);VISIT(GH_MC_LOAD_PENDING);
    INPUT(0,0);VISIT(GH_MC_LOAD_PENDING);
    C(task->checks.phase==5);
    for(unsigned i=1;i<=20;++i){
        VISIT(GH_MC_LOAD_PENDING);
        C(i==20?(task->checks.phase==6&&!task->checks.frame):(task->checks.phase==5&&task->checks.frame==i));
    }
    C(!memcmp(task->load_payload.pointer,record,sizeof(record))&&load.selected.borrowed_record==task->load_payload.pointer);
    C(!core[0x80]&&!raw[0x54a]);
    INPUT(0x10000000,0);
    for(unsigned i=0;i<11;++i)VISIT(GH_MC_LOAD_PENDING);
    C(task->checks.frame==79&&task->checks.mode==0xb&&menu.first_slot==2&&menu.selected_row==1);
    C(!raw[0x54a]&&!(work->flags&0x40));
    VISIT(GH_MC_LOAD_RESTORED);
    C(task->checks.done&&raw[0x548]==100&&raw[0x54a]==60&&(work->flags&0x46)==0x46);
    C(!memcmp(payload,record+0x20,GH_CHECKPOINT_BYTES));
    C(sounds.count==7&&sounds.programs[0]==0x161&&sounds.programs[2]==0x161&&
      sounds.programs[3]==0x160&&sounds.programs[4]==0x161&&sounds.programs[5]==0x161&&sounds.programs[6]==0x160);
    INPUT(0,0);
    static gh_mc_title_load canceled;
    C(gh_mc_selected_load_begin_using(&canceled.selected,card,3,task->load_payload.pointer,sizeof(record))==GH_MC_LOAD_PENDING);
    C(gh_mc_selected_load_step(&canceled.selected,&frontier)==GH_MC_LOAD_PENDING);
    C(gh_mc_control_title_abort_io(&canceled,card)==GH_MC_OK&&!canceled.selected.borrowed_record);
    C(gh_mc_selected_load_begin(&canceled.selected,card,3)==GH_MC_LOAD_PENDING);
    C(gh_mc_control_title_abort_io(&canceled,card)==GH_MC_OK);
    run_title_exit(&load,task,&bindings,&control);
#undef VISIT
#undef INPUT
    gh_bgm_requests_destroy(bgm);
    C(gh_heap_destroy(audio_heap)==GH_MEM_OK);
    gh_audio_registry_destroy(registry);
    gh_memory_card_close(card);
    C(DeleteFileA(file_path)&&DeleteFileA(icon_path)&&RemoveDirectoryA(folder)&&RemoveDirectoryA(profile));
    puts("PASS selected native save through title menu, 20-visit read gate and actual checkpoint restore");
}
#endif

int main(int argc,char **argv)
{
    gh_disc *disc;gh_heap *heap,*module_heap;gh_dvd_jobs dvd;gh_r207_allocation allocation;
    gh_disc_entry entry;gh_bundle language;gh_bundle_entry idd;size_t index,baseline;
    gh_id_state *id;gh_id_runtime *runtime;gh_font_state font={0};gh_packed_message_state packed;
    gh_boot_work work={0};gh_mc_task_state state={0};gh_mc_task_bindings bindings={0};
    gh_memory_card *card;int32_t game_suspend=0;gh_id_entry *e;uint32_t busy;call c;unsigned ticks;
#ifdef GH_MC_TITLE_SELECT_TEST
    C(argc==3);
#else
    C(argc==2);
#endif
    C(gh_disc_open(argv[1],&disc)==GH_DISC_OK);
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
    memset(&state,0,sizeof(state));
    title_call title={.state=&state,.bindings=&bindings,.fade_context={512,1908,0,0}};
    run_title(&title);
    C(state.ui_initialized&&state.load_phase==2&&state.flags==0xc8400000u);
    C(gh_heap_used(heap)>baseline+0x5000+0xb400);
    C(title.fade.color_from==0xff000000u&&!title.fade.color_to&&title.fade.time==10);
    const unsigned prompt_ids[]={2,3,4,9};
    for(unsigned i=0;i<4;++i){
        C(gh_id_base_get_work_002aa410(runtime,&state.id_base,prompt_ids[i],&e,0)==GH_ID_OK);
        C(e->message==0x20a4+(i&1)&&!(e->flags&0x20000000u));
    }
    C(gh_id_base_get_work_002aa410(runtime,&state.id_base,0x40,&e,0)==GH_ID_OK&&e->pos_y==210);
    C(gh_mc_task_title_load_step_000012b0(&state,&bindings,&title.fade,&title.fade_context)==GH_MC_TASK_LOAD_FRAME);
    /* Unit verification of the second fade readiness gate. */
    for(unsigned i=0;i<11;++i){
        gh_fade_draw draw;
        C(gh_screen_fade_step_00307388(&title.fade,&title.fade_context,&draw)==GH_FADE_OK&&draw.visible);
    }
    C(gh_mc_task_title_load_step_000012b0(&state,&bindings,&title.fade,&title.fade_context)==
      GH_MC_TASK_CONTROL_FRONTIER&&state.frontier_offset==0x1590);
#ifdef GH_MC_TITLE_SELECT_TEST
    selected_fixture(disc,argv[2],&state,&bindings);
#else
    gh_mc_control_state control={0};
    gh_mc_load_scan scan={0};
    gh_mc_load_menu_memory menu={-1,-1};
    uint32_t scan_frontier;
    C(gh_mc_control_scan_title_00001590(&control,&scan,&state,&bindings,&menu,&scan_frontier)==GH_MC_BOOT_OK);
    C(gh_id_base_get_work_002aa410(runtime,&state.id_base,0x41,&e,0)==GH_ID_OK&&e->message==0x2007);
    C(gh_mc_control_scan_title_00001590(&control,&scan,&state,&bindings,&menu,&scan_frontier)==GH_MC_BOOT_OK);
    C(state.checks.phase==7&&state.checks.error==1&&!scan.menu_ready);
    gh_mc_title_load load={0};
    gh_audio_registry *registry;
    C(gh_audio_registry_create(&registry)==GH_AUDIO_OK&&gh_audio_startup_load(registry,disc)==GH_AUDIO_OK);
    title_sounds sounds={.registry=registry};
    const gh_mc_title_load_providers providers={.sound=inspect_original_sound,.sound_context=&sounds};
    uint8_t *raw=(uint8_t*)&work;
#define VISIT(expected) C(gh_mc_control_title_select_00001590(&load,&control,&scan,&state,&bindings,&menu,&providers,&scan_frontier)==(expected)&&!scan_frontier)
#define INPUT(p,r) do{uint32_t a=(p),b=(r);memcpy(raw+0x1a0,&a,4);memcpy(raw+0x1b0,&b,4);}while(0)
    VISIT(GH_MC_LOAD_PENDING);
    C(state.checks.mode==1&&state.checks.subphase==2&&!state.checks.done);
    VISIT(GH_MC_LOAD_PENDING);
    C(state.checks.mode==0x11&&!load.choice&&sounds.count==0);
    C(gh_id_base_get_work_002aa410(runtime,&state.id_base,0x41,&e,0)==GH_ID_OK&&e->message==0x200c);
    INPUT(0x20000000,0);VISIT(GH_MC_LOAD_PENDING); /* action2 ignored in subphase2 */
    C(!state.checks.done&&state.checks.subphase==2&&!sounds.count);
    INPUT(0x10000000,0);VISIT(GH_MC_LOAD_PENDING);
    C(state.checks.subphase==3&&sounds.count==1&&sounds.programs[0]==0x15e);
    INPUT(0,0);VISIT(GH_MC_LOAD_PENDING);
    C(state.checks.phase==0&&!state.checks.subphase&&state.checks.mode==0x11);
    C(gh_mc_control_scan_title_00001590(&control,&scan,&state,&bindings,&menu,&scan_frontier)==GH_MC_BOOT_OK);
    C(state.checks.mode==1&&state.checks.subphase==2);
    C(gh_mc_control_scan_title_00001590(&control,&scan,&state,&bindings,&menu,&scan_frontier)==GH_MC_BOOT_OK);
    C(state.checks.phase==7&&state.checks.error==1);

    const gh_mc_boot_checks absent_checks=state.checks;
    const gh_mc_load_scan absent_scan=scan;
    /* Use the same real ID family to verify initial 7F30 row presentation. */
    gh_mc_boot_reset_slot_records(&state.checks);
    uint8_t *row=(uint8_t*)state.checks.icon_work.pointer+19*GH_MC_SAVE_HEADER_BYTES;
    memset(row+4,0,4);
    row[0xc]=3;row[0xd]=2;row[0xe]=7;row[0x10]=2;
    uint32_t seconds=(3600+2*60+3)*30;
    memcpy(row+0x20,&seconds,4);
    scan=(gh_mc_load_scan){17,2,1};
    state.checks.phase=3;state.checks.subphase=0;
    C(gh_mc_control_init_title_list_00007f30(&control,&scan,&state,&bindings)==GH_ID_OK);
    C(state.checks.mode==0&&state.checks.subphase==1);
    C(gh_id_base_get_work_002aa410(runtime,&state.id_base,0x14,&e,0)==GH_ID_OK&&e->seed==18&&e->color_base==0x801e1c1e);
    C(gh_id_base_get_work_002aa410(runtime,&state.id_base,0x16,&e,0)==GH_ID_OK&&e->seed==20&&e->color_base==0x80808080);
    C(gh_id_base_get_work_002aa410(runtime,&state.id_base,0x1a,&e,0)==GH_ID_OK&&e->message==0x2036);
    C(gh_id_base_get_work_002aa410(runtime,&state.id_base,0x26,&e,0)==GH_ID_OK&&e->seed==1);
    C(gh_id_base_get_work_002aa410(runtime,&state.id_base,0x2a,&e,0)==GH_ID_OK&&e->seed==2);
    C(gh_id_base_get_work_002aa410(runtime,&state.id_base,0x2e,&e,0)==GH_ID_OK&&e->seed==3);
    C(gh_id_base_get_work_002aa410(runtime,&state.id_base,0x32,&e,0)==GH_ID_OK&&e->message==0x2002&&e->scale_x==1.1f);
    C(gh_id_base_get_work_002aa410(runtime,&state.id_base,0x36,&e,0)==GH_ID_OK&&e->seed==7);
    C(gh_id_base_get_work_002aa410(runtime,&state.id_base,0x78,&e,0)==GH_ID_OK&&e->message==0x20a6);
    puts("PASS title load scan display and three-row save list on actual ISO UI");

    state.checks=absent_checks;
    scan=absent_scan;
    INPUT(0,0);VISIT(GH_MC_LOAD_PENDING);
    C(state.checks.subphase==2&&!load.choice);
    VISIT(GH_MC_LOAD_PENDING);
    C(state.checks.mode==0x11);
    INPUT(0x10000000,0x04000000);VISIT(GH_MC_LOAD_PENDING);
    C(load.choice&&!state.checks.done&&sounds.count==2&&sounds.programs[1]==0x161);
    INPUT(0x10000000,0);VISIT(GH_MC_LOAD_CANCELED);
    C(state.checks.done&&!load.selected.applied&&!(work.flags&0x40));
    C(sounds.count==3&&sounds.programs[2]==0x160);
    VISIT(GH_MC_LOAD_CANCELED);
    C(sounds.count==3);
    INPUT(0,0);
    run_title_exit(&load,&state,&bindings,&control);
    gh_audio_registry_destroy(registry);
#undef VISIT
#undef INPUT
#endif
    C(gh_heap_used(heap)==baseline&&gh_heap_used(module_heap)==0);
    C(!state.load_payload.pointer&&!game_suspend&&!(work.flags&0x4000));
    puts("PASS real title load module, task slot 2, fade gates, and DispInit mode 1");
    gh_memory_card_close(card);C(gh_id_runtime_destroy(runtime)==GH_ID_OK);free(id);
    C(gh_heap_release(heap,allocation)==GH_MEM_OK && gh_heap_destroy(heap)==GH_MEM_OK && gh_heap_destroy(module_heap)==GH_MEM_OK);
    gh_disc_close(disc);puts("PASS realMC body through module/version/5000work/IDBase/packed/DispInit; exactControl frontier, not task completion");return 0;
}
