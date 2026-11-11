#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "mc_task.h"
#include "game_scene_aux.h"
#include "game_room_pools.h"
#include "audio_startup.h"
#include "mc_save_control.h"
#include "mc_exit.h"
#include "game_loaded_room.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define C(x) do{if(!(x)){fprintf(stderr,"FAIL %d %s\n",__LINE__,#x);exit(1);}}while(0)
typedef struct {
    gh_mc_save_task_state save;
    gh_mc_task_bindings *bindings;
    gh_heap *room_heap;
    gh_room_pools pools;
    gh_game_init_state init;
    gh_screen_fade fade;
    gh_screen_fade_context context;
    gh_mc_exit_state exit;
    gh_mc_control_state display;
    unsigned release_calls,release_sleeps,fade_frames[2],exit_fades,no_ui_sleeps;
    int done;
} save_call;

static uint32_t work_word(const gh_boot_work *work,unsigned offset)
{
    uint32_t value;
    memcpy(&value,(const uint8_t*)work+offset,4);
    return value;
}
static void put_work_word(gh_boot_work *work,unsigned offset,uint32_t value)
{memcpy((uint8_t*)work+offset,&value,4);}

static gh_mc_task_status release_empty_room(void *context,uint32_t *frontier)
{
    save_call *call=context;
    const gh_room_vector *registry;
    C(!call->init.enemy_top&&!memcmp(call->init.enemies+0x500,"\0\0\0\0",4));
    C(gh_room_registry_view(&call->pools,0x754c58,&registry)==GH_POOL_OK&&!registry->count);
    ++call->release_calls;
    *frontier=0;
    return GH_MC_TASK_OK;
}

static void run_save_task(void *context,int32_t argument)
{
    save_call *call=context;
    C(argument==0);
    C(gh_mc_task_prepare_save_002ae788(&call->save,call->bindings,call->room_heap,0,
       NULL,NULL)==GH_MC_TASK_UNSUPPORTED&&call->save.common.frontier_offset==0x2908c0);
    C(!call->save.common.checks.icon_work.pointer);
    C(gh_mc_task_prepare_save_002ae788(&call->save,call->bindings,call->room_heap,0,
       release_empty_room,call)==GH_MC_TASK_OK);
    C(call->save.common.flags==0x88800000u&&call->save.phase==0);
    for(;;){
        gh_mc_task_status result=gh_mc_task_save_step_00003c80(&call->save,call->bindings,
            call->room_heap,&call->fade,&call->context);
        if(result==GH_MC_TASK_CONTROL_FRONTIER)break;
        C(result==GH_MC_TASK_LOAD_FRAME);
        if(!call->save.common.ui_initialized)
            C(gh_mc_task_title_load_sleep_00008aa8(&call->save.common,call->bindings)==GH_MC_TASK_OK);
        else
            C(gh_task_threads_sleep(call->bindings->threads,1)==GH_THREAD_OK);
    }
    call->done=1;
}
static gh_scene_aux_status launch_save(void *context,int32_t mode)
{
    save_call *call=context;
    int32_t slot;
    C(mode==0);
    C(gh_mc_task_start_save_002ae740(call->bindings,run_save_task,call,&slot)==GH_MC_TASK_OK);
    C(slot==2);
    return GH_AUX_OK;
}


typedef struct {gh_audio_registry *registry;unsigned calls,last;} sound_fixture;
static int sound(void *context,const gh_mc_sound_request *request)
{
    sound_fixture *fixture=context;
    uint8_t row[48];
    C(request->bank==0&&request->parameter3==-1&&request->parameter4==-1&&
      !request->parameter5&&!request->parameter6);
    C(gh_audio_get_program(fixture->registry,0,request->sound_id,0,row)==GH_AUDIO_OK);
    ++fixture->calls;
    fixture->last=(unsigned)request->sound_id;
    return 0;
}
static void file_matches(const char *profile,const char *name,const void *expected,size_t bytes)
{
    char path[1550];
    C(snprintf(path,sizeof(path),"%s\\BASLUS-21503GODHD000\\%s",profile,name)>0);
    FILE *file=fopen(path,"rb");
    uint8_t *actual=malloc(bytes);
    C(file&&actual&&fread(actual,1,bytes,file)==bytes&&fgetc(file)==EOF&&!fclose(file));
    C(!memcmp(actual,expected,bytes));
    free(actual);
    C(DeleteFileA(path));
}

static void verify_control(gh_disc *disc,save_call *call,const char *profile)
{
    gh_mc_save_control control={0};
    gh_mc_load_menu_memory memory={-1,-1};
    gh_mc_boot_checks *checks=&call->save.common.checks;
    gh_boot_work *work=call->bindings->work;
    static uint8_t core[GH_CORE_SAVE_BYTES],payload[GH_CHECKPOINT_BYTES],player_raw[GH_TITLE_PLAYER_BYTES];
    static gh_game_prepare prepare;
    gh_title_player_view player={.bytes=player_raw,.size=sizeof(player_raw),.alias_f0=player_raw+0xb0,
        .initial_memory=GH_TITLE_MEMORY_PROVIDED_BYTES};
    work->room=0x100;
    player_raw[0x54a]=81;
    gh_game_door door={.target_room=0x102,.position_bits={0x3f800000,0x40000000,0x40400000},.yaw_bits=0x40800000};
    gh_game_checkpoint checkpoint={0};
    gh_checkpoint_bindings checkpoint_bindings={.work=work,.save={core,sizeof(core)},.prepare=&prepare,
        .init=&call->init,.player=&player,.destination=payload,.destination_size=sizeof(payload)};
    gh_mc_save_metadata meta={.output_mode=1,.clock_result=1,.clock={0,0x32,0x11,0x10,0,0x20,0x11,0x26}};
    sound_fixture sounds={0};
    C(gh_audio_registry_create(&sounds.registry)==GH_AUDIO_OK&&gh_audio_startup_load(sounds.registry,disc)==GH_AUDIO_OK);
    const gh_mc_save_providers providers={.checkpoint=&checkpoint,.checkpoint_bindings=&checkpoint_bindings,
        .door=&door,.metadata=&meta,.sound=sound,.sound_context=&sounds};
    uint32_t frontier=0;
#define INPUT(p,r) do{put_work_word(work,0x1a0,p);put_work_word(work,0x1b0,r);}while(0)
#define STEP(expected) do{gh_mc_save_status actual=gh_mc_control_save_00003f78(&control,&call->display,&call->save, \
        call->bindings,&memory,&providers,&frontier);if(actual!=(expected)||frontier) \
        fprintf(stderr,"save status%u frontier%06x phase%u/%u mode%d\\n",actual,frontier,checks->phase,checks->subphase,checks->mode); \
        C(actual==(expected)&&!frontier);}while(0)
    for(unsigned i=0;checks->phase<6&&i<30;++i)STEP(GH_MC_SAVE_PENDING);
    C(checks->phase==6&&!checks->present_mask&&control.scan.first_slot==0&&control.scan.selected_row==0);
    STEP(GH_MC_SAVE_PENDING);
    INPUT(0,0x01000000);STEP(GH_MC_SAVE_PENDING);
    C(control.scan.first_slot==17&&control.scan.selected_row==2&&sounds.last==0x161);
    INPUT(0,0x02000000);STEP(GH_MC_SAVE_PENDING);
    C(control.scan.first_slot==0&&control.scan.selected_row==0);
    INPUT(0x10000000,0);STEP(GH_MC_SAVE_PENDING);
    INPUT(0,0);STEP(GH_MC_SAVE_PENDING);
    C(checks->phase==7);
    STEP(GH_MC_SAVE_PENDING);
    C(!control.choice&&checks->mode==8);
    gh_id_entry *message;
    C(gh_id_base_get_work_002aa410(call->bindings->id_runtime,&call->save.common.id_base,0x43,&message,NULL)==GH_ID_OK);
    C(message->message==0x200e);
    INPUT(0x10000000,0);STEP(GH_MC_SAVE_PENDING);
    INPUT(0,0);STEP(GH_MC_SAVE_PENDING);
    C(checks->phase==6&&sounds.last==0x15e&&!checkpoint.captured);
    INPUT(0x10000000,0);STEP(GH_MC_SAVE_PENDING);
    INPUT(0,0);STEP(GH_MC_SAVE_PENDING);
    C(checks->phase==7);
    STEP(GH_MC_SAVE_PENDING);
    INPUT(0,0x04000000);STEP(GH_MC_SAVE_PENDING);
    C(control.choice&&sounds.last==0x161);
    INPUT(0x10000000,0);STEP(GH_MC_SAVE_PENDING);
    INPUT(0,0);STEP(GH_MC_SAVE_PENDING);
    C(checks->phase==10&&control.selected_slot==0&&!checkpoint.captured);
    unsigned visited=0;
    for(unsigned i=0;checks->phase<14&&i<35;++i){
        visited|=1u<<checks->phase;
        STEP(GH_MC_SAVE_PENDING);
    }
    C(checks->phase==14&&visited==((1u<<10)|(1u<<11)|(1u<<12)|(1u<<13)));
    C(checkpoint.captured&&!memcmp(call->save.current_record.pointer,call->save.previous_record.pointer,GH_MC_SAVE_RECORD_BYTES));
    const uint8_t *record=call->save.current_record.pointer;
    uint64_t checksum;
    memcpy(&checksum,record+0xb110,8);
    C(checksum==gh_mc_save_checksum_00008c08(record)&&record[0xa4]==81);
    STEP(GH_MC_SAVE_PENDING);
    C(checks->frame==89&&checks->mode==10&&(work->flags&4)&&!(work->flags&0x40));
    C(message->message==0x2019);
    INPUT(0x10000000,0);
    for(unsigned i=0;i<10;++i)STEP(GH_MC_SAVE_PENDING);
    STEP(GH_MC_SAVE_COMPLETE);
    C(checks->done&&control.complete);
    INPUT(0,0);

    gh_mc_selected_load loaded={0};
    C(gh_mc_selected_load_begin(&loaded,call->bindings->card,0)==GH_MC_LOAD_PENDING);
    C(gh_mc_selected_load_step(&loaded,&frontier)==GH_MC_LOAD_PENDING);
    C(gh_mc_selected_load_step(&loaded,&frontier)==GH_MC_LOAD_PENDING);
    C(gh_mc_selected_load_step(&loaded,&frontier)==GH_MC_LOAD_VERIFIED&&!frontier);
    C(!memcmp(loaded.record,record,GH_MC_SAVE_RECORD_BYTES));
    file_matches(profile,"BASLUS-21503GODHD000",record,GH_MC_SAVE_RECORD_BYTES);
    file_matches(profile,"godh0.ico",call->save.icon_model.bytes,0x8618);
    file_matches(profile,"icon.sys",call->save.icon_template.bytes,0x3c4);
    char folder[1550];
    C(snprintf(folder,sizeof(folder),"%s\\BASLUS-21503GODHD000",profile)>0&&RemoveDirectoryA(folder));
    gh_audio_registry_destroy(sounds.registry);
#undef INPUT
#undef STEP
    puts("PASS original empty-slot Save choices, B400 checksum/readback, McFile model/icon and 063D8 notice");
}
static gh_id_error fail_if_visible(void *context,const gh_id_draw_job *job)
{(void)context;(void)job;return GH_ID_UNSUPPORTED;}

static void exit_save_task(void *context,int32_t argument)
{
    save_call *call=context;
    C(argument==0);
    call->save.common.id_base.hide=1;
    for(;;){
        gh_mc_task_status status=gh_mc_save_exit_step_00003e40(&call->exit,&call->save,
            call->bindings,&call->fade,&call->context);
        if(status==GH_MC_TASK_OK)break;
        C(status==GH_MC_TASK_LOAD_FRAME);
        if(call->save.common.ui_initialized){
            C(call->save.phase==3&&call->save.save_file.pointer);
            C(gh_mc_control_sleep_frame(&call->display,&call->save.common,call->bindings,
                NULL,NULL,fail_if_visible,NULL)==GH_ID_OK);
        }else{
            C(call->save.phase==4&&!call->save.save_file.pointer&&call->save.common.module_loaded);
            ++call->no_ui_sleeps;
            C(gh_mc_task_title_load_sleep_00008aa8(&call->save.common,call->bindings)==GH_MC_TASK_OK);
        }
    }
    call->done=1;
}

static void save_entry_fixture(gh_disc *disc,gh_mc_task_bindings *bindings,const char *profile)
{
    save_call *call=calloc(1,sizeof(*call));
    C(call);
    call->bindings=bindings;
    call->context=(gh_screen_fade_context){512,1908,0,0};
    C(gh_heap_create(16*1024*1024,&call->room_heap)==GH_MEM_OK);
    size_t index;
    gh_disc_entry entry;
    C(gh_disc_find(disc,"st1/r100.dat",&index)==GH_DISC_OK&&gh_disc_entry_at(disc,index,&entry)==GH_DISC_OK);
    uint8_t *bytes=malloc((size_t)entry.size);
    gh_bundle room;
    C(bytes&&gh_disc_read(disc,index,0,bytes,(size_t)entry.size)==GH_DISC_OK);
    C(gh_bundle_open(bytes,(size_t)entry.size,&room)==GH_BUNDLE_OK);
    gh_game_init_state_initialize(&call->init);
    C(gh_game_room_pools_initialize_002a5d80(&call->pools,&room,call->room_heap)==GH_POOL_OK);
    const size_t initial=gh_heap_used(call->room_heap);

    gh_game_scene_aux cursor={0};
    put_work_word(bindings->work,0x598,UINT32_C(0x80000000));
    C(gh_scene_cursor_open_save_002bf1a0(&cursor,bindings->work,0)==GH_AUX_OK);
    C(!cursor.save_load[0]&&!(work_word(bindings->work,0x5e8)&0x40000000u));
    put_work_word(bindings->work,0x598,0);
    put_work_word(bindings->work,0x594,0x800);
    C(gh_scene_cursor_open_save_002bf1a0(&cursor,bindings->work,0)==GH_AUX_OK);
    C(!cursor.save_load[0]&&!(work_word(bindings->work,0x5e8)&0x40000000u));
    put_work_word(bindings->work,0x594,0);
    put_work_word(bindings->work,0x5d8,0x01234567);
    put_work_word(bindings->work,0x5e0,0x76543210);
    C(gh_scene_cursor_open_save_002bf1a0(&cursor,bindings->work,0)==GH_AUX_OK);
    C(cursor.save_load[0]==3&&cursor.save_load[1]==1&&cursor.save_mode==0);

    gh_task_record32 records[3]={{0}};
    gh_task_manager manager={records,3,0,-1,3,0,0,0};
    C(gh_task_threads_create(&manager,&bindings->threads)==GH_THREAD_OK);
    C(gh_scene_cursor_save_step_002bf478(&cursor,bindings->work,launch_save,call)==GH_AUX_OK);
    C(cursor.save_load[1]==2&&cursor.load_flags_saved&&work_word(bindings->work,0x5e0)==0xfff43210u);
    for(unsigned tick=0;!call->done&&tick<45;++tick){
        C(gh_tasks_run_matching_002d52a0(&manager).status==GH_TASK_OK);
        C(gh_dvd_pump(bindings->dvd)==GH_DVD_OK);
        if(call->release_calls&&!call->save.common.checks.icon_work.pointer)++call->release_sleeps;
        if(call->save.phase&&!(call->fade.state&4)){
            gh_fade_draw draw;
            C(gh_screen_fade_step_00307388(&call->fade,&call->context,&draw)==GH_FADE_OK&&draw.visible);
            ++call->fade_frames[call->save.phase-1];
        }
    }
    C(call->done&&call->release_calls==1&&call->release_sleeps==5);
    C(call->fade_frames[0]==11&&call->fade_frames[1]==11);
    C(call->save.common.frontier_offset==0x3f78&&call->save.common.ui_initialized);
    C(call->save.current_record.pointer&&call->save.previous_record.pointer);
    C(call->save.current_record.pointer!=call->save.previous_record.pointer);
    C((uintptr_t)call->save.current_record.pointer%128==0&&(uintptr_t)call->save.previous_record.pointer%128==0);
    C(call->save.icon_model.offset==0x20&&call->save.icon_model.size==0x18ec0);
    C(call->save.icon_template.offset==0x18ee0&&call->save.icon_template.size==0x400);
    gh_id_entry *heading;
    C(gh_id_base_get_work_002aa410(bindings->id_runtime,&call->save.common.id_base,0xa,&heading,NULL)==GH_ID_OK);
    C(!(heading->flags&0x20000000u));
    C(gh_id_base_get_work_002aa410(bindings->id_runtime,&call->save.common.id_base,0xb,&heading,NULL)==GH_ID_OK);
    C(heading->flags&0x20000000u);
    C(gh_id_base_get_work_002aa410(bindings->id_runtime,&call->save.common.id_base,2,&heading,NULL)==GH_ID_OK);
    C(heading->message==0x20a4);
    verify_control(disc,call,profile);
    int32_t exit_slot;
    call->done=0;
    C(gh_task_threads_start(bindings->threads,2,exit_save_task,call,0,&exit_slot)==GH_THREAD_OK&&exit_slot==2);
    for(unsigned tick=0;!call->done&&tick<20;++tick){
        C(gh_tasks_run_matching_002d52a0(&manager).status==GH_TASK_OK);
        if(call->save.phase==3){
            gh_fade_draw draw;
            C(gh_screen_fade_step_00307388(&call->fade,&call->context,&draw)==GH_FADE_OK&&draw.visible);
            ++call->exit_fades;
        }
    }
    C(call->done&&call->exit_fades==11&&call->no_ui_sleeps==1&&call->exit.body_finished);
    C(call->save.phase==5&&!call->save.common.module_loaded&&!call->save.card_begun);
    C(gh_heap_used(call->room_heap)==initial&&gh_memory_card_request_info(bindings->card)==GH_MC_ENDED);
    C(gh_scene_cursor_save_step_002bf478(&cursor,bindings->work,NULL,NULL)==GH_AUX_OK);
    C(cursor.save_load[0]==5&&cursor.save_load[0xc]==0);
    gh_scene_loaded_room no_door={0};
    const gh_game_door_bindings door={.work=bindings->work,.fade=&call->fade};
    const gh_scene_loaded_room_bindings handoff={.door_bindings=&door};
    C(gh_scene_cursor_loaded_room_002bf568(&cursor,&no_door,&handoff)==GH_AUX_OK&&!no_door.stage);
    C(!cursor.save_load[0]&&work_word(bindings->work,0x5d8)==0x01234567);
    C(work_word(bindings->work,0x5e0)==0x76543210&&!(work_word(bindings->work,0x5e8)&0xc0000000));
    C(gh_task_threads_destroy(bindings->threads)==GH_THREAD_OK);
    C(gh_game_room_pools_release(&call->pools)==GH_POOL_OK);
    C(gh_heap_destroy(call->room_heap)==GH_MEM_OK);
    free(bytes);
    free(call);
    puts("PASS actual in-game cMcSave cursor/task, 5 heap-release sleeps, McFile.dat/DispInit0, completion fade, DeInit and no-Door cursor return");
}
int main(int argc,char **argv)
{
    char scratch[1200],profile[1450];
    static const char suffix[]="\\memory-card-tests\\scratch";
    C(argc==3&&GetFullPathNameA(argv[1],sizeof(scratch),scratch,NULL));
    const size_t n=strlen(scratch);
    C(n>=sizeof(suffix)-1&&!_stricmp(scratch+n-(sizeof(suffix)-1),suffix));
    C(snprintf(profile,sizeof(profile),"%s\\native-save-%lu",scratch,(unsigned long)GetCurrentProcessId())>0);
    C(CreateDirectoryA(profile,NULL));
    gh_disc *disc;
    gh_heap *heap,*module_heap;
    gh_dvd_jobs dvd;
    gh_r207_allocation allocation;
    gh_bundle language;
    gh_bundle_entry idd;
    gh_disc_entry entry;
    size_t index;
    gh_id_state *id=calloc(1,sizeof(*id));
    gh_id_runtime *runtime;
    gh_font_state font={0};
    gh_packed_message_state packed={0};
    gh_boot_work work={.language=1,.phase569=2};
    gh_memory_card *card;
    int32_t suspend=0;
    C(id&&gh_disc_open(argv[2],&disc)==GH_DISC_OK);
    C(gh_heap_create(8*1024*1024,&heap)==GH_MEM_OK&&gh_heap_create(1024*1024,&module_heap)==GH_MEM_OK);
    C(gh_memory_card_open(profile,&card)==GH_MC_OK);
    gh_dvd_jobs_init(&dvd,disc);
    C(gh_dvd_read_alloc(&dvd,"etc/core_usa.bin",&allocation,heap,0,0,0,0));
    C(gh_dvd_pump(&dvd)==GH_DVD_OK&&gh_disc_find(disc,"etc/core_usa.bin",&index)==GH_DISC_OK);
    C(gh_disc_entry_at(disc,index,&entry)==GH_DISC_OK&&gh_bundle_open(allocation.pointer,(size_t)entry.size,&language)==GH_BUNDLE_OK);
    C(gh_id_find_language_data(&language,&idd)==GH_MENU_OK&&gh_id_system_init_002acc70(id,&idd)==GH_MENU_OK);
    C(gh_id_runtime_create(id,1,&runtime)==GH_ID_OK&&gh_font_initialize(&font,&language,1)==GH_FONT_OK);
    gh_mc_task_bindings bindings={.dvd=&dvd,.module_heap=module_heap,.subscreen_heap=heap,.card=card,
        .id_manager=id,.id_runtime=runtime,.fonts=&font,.packed=&packed,.language=&language,.work=&work,
        .game_suspend_word=&suspend};
    const size_t baseline=gh_heap_used(heap);
    save_entry_fixture(disc,&bindings,profile);
    C(!suspend&&gh_heap_used(heap)==baseline&&!gh_heap_used(module_heap));
    C(gh_id_runtime_destroy(runtime)==GH_ID_OK&&gh_heap_release(heap,allocation)==GH_MEM_OK);
    C(gh_heap_destroy(heap)==GH_MEM_OK&&gh_heap_destroy(module_heap)==GH_MEM_OK);
    gh_memory_card_close(card);
    gh_disc_close(disc);
    free(id);
    C(RemoveDirectoryA(profile));
    return 0;
}
