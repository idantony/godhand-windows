#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "../../src/platform/memory_card.h"
#include "../../src/platform/task_threads.h"
#include "../../src/core/mc_boot.h"
#include "../../src/core/mc_file_load.h"
#include "../../src/core/bgm_requests.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do {if(!(x)){fprintf(stderr,"FAIL %d %s\n",__LINE__,#x);exit(1);}}while(0)
typedef struct flow {
    gh_mc_boot_checks *state;gh_memory_card *card;gh_task_threads *threads;
    uint32_t flags;int completed;
} flow;
static void check_task(void *context,int32_t unused)
{
    flow *f=context;gh_mc_boot_error e;
    CHECK(unused==91);
    while(f->state->phase<2) {
        e=gh_mc_boot_check_control(f->state,f->card,&f->flags);
        if(e==GH_MC_BOOT_NEEDS_SLEEP) {
            CHECK(f->state->phase==0 && f->state->delay_frames>0);
            CHECK(gh_task_threads_sleep(f->threads,(int32_t)f->state->delay_frames)==GH_THREAD_OK);
            CHECK(gh_mc_boot_delay_completed(f->state)==GH_MC_BOOT_OK);
        } else {
            CHECK(e==GH_MC_BOOT_OK);
        }
        if(f->state->phase<2)CHECK(gh_task_threads_sleep(f->threads,1)==GH_THREAD_OK);
    }
    CHECK(gh_mc_boot_check_control(f->state,f->card,&f->flags)==GH_MC_BOOT_UNSUPPORTED);
    f->completed=1; /* check slice finished, NOT original MC Main completed */
}

static void save_fixture(const char *path,const uint8_t *data,size_t bytes)
{
    FILE *f=fopen(path,"wb");
    CHECK(f&&fwrite(data,1,bytes,f)==bytes&&!fclose(f));
}
static gh_mc_load_status restore_output_mode(void *context,uint32_t mode,uint32_t *frontier)
{
    gh_bgm_requests *bgm=context;
    if(gh_bgm_requests_restore_idle_output_mode_00375700(bgm,mode)==GH_BGM_OK)
        return GH_MC_LOAD_RESTORED;
    *frontier=gh_bgm_requests_frontier(bgm);
    return GH_MC_LOAD_UNSUPPORTED;
}

static void selected_fixture(const char *path,gh_memory_card *card,const char *iso)
{
    static gh_mc_selected_load load;
    static uint8_t file[GH_MC_SAVE_RECORD_BYTES], staged[GH_CHECKPOINT_BYTES];
    static uint8_t core[GH_CORE_SAVE_BYTES],raw[GH_TITLE_PLAYER_BYTES];
    static gh_game_prepare prepare;
    static gh_game_init_state init;
    gh_game_door door={.wait=7,.flags=0x1234};
    gh_game_checkpoint checkpoint={0};
    gh_boot_work work={0};
    gh_title_player_view player={.bytes=raw,.size=sizeof(raw),.alias_f0=raw+0xb0};
    gh_checkpoint_bindings bind={.work=&work,.save={core,sizeof(core)},.prepare=&prepare,
        .player=&player,.destination=staged,.destination_size=sizeof(staged)};
    uint32_t frontier, flags=0x1000;
    memcpy(work.pad588,&flags,4);
    /* Golden sparse original-format record: XOR computed independently.
     * The final two heat words are original payload but outside 8C08. */
    file[0]=0xcd;file[1]=0xcc;file[2]=0xcc;file[3]=0x3d;
    file[8]=2;file[0x30]=1;
    file[0x14bf]=0x9a;file[0x14c0]=0x7b;file[0xb0ef]=0x59;
    file[0xb0f2]=0x80;file[0xb0f3]=0x3f;file[0xb103]=0x40;
    file[0x2d]=1;file[0x2e]=0x21;file[0x175]=2;
    file[0xb101]=0x00;file[0xb102]=0x00;
    file[0xb109]=0x55;file[0xb10c]=0xaa;
    file[0x20]=0x12;file[0x24]=0x34;
    file[0xa0]=100;
    memset(file+0xa4,0xff,4);
    for(unsigned i=0;i<24;i++)file[0x178+i]=(uint8_t)(0x80+i);
    file[0x190]=1;file[0x194]=1;file[0x198]=2;
    const uint8_t checksum[]={0x45,0x8f,0x79,0xab,0x6d,0x9b,0xa6,0x38};
    memcpy(file+0xb110,checksum,8);
    CHECK(gh_mc_selected_load_begin(&load,card,4)==GH_MC_LOAD_PENDING);
    CHECK(gh_mc_selected_load_step(&load,&frontier)==GH_MC_LOAD_NOT_FOUND&&frontier==0x2e80);
    save_fixture(path,file,0x400);
    CHECK(gh_mc_selected_load_begin(&load,card,3)==GH_MC_LOAD_PENDING);
    CHECK(gh_mc_selected_load_step(&load,&frontier)==GH_MC_LOAD_PENDING&&!frontier);
    CHECK(gh_mc_selected_load_step(&load,&frontier)==GH_MC_LOAD_PENDING&&!frontier);
    CHECK(gh_mc_selected_load_step(&load,&frontier)==GH_MC_LOAD_SHORT&&frontier==0x2f50);
    file[0x500]^=1;
    save_fixture(path,file,sizeof(file));
    CHECK(gh_mc_selected_load_begin(&load,card,3)==GH_MC_LOAD_PENDING);
    CHECK(gh_mc_selected_load_step(&load,&frontier)==GH_MC_LOAD_PENDING);
    CHECK(gh_mc_selected_load_step(&load,&frontier)==GH_MC_LOAD_PENDING);
    CHECK(gh_mc_selected_load_step(&load,&frontier)==GH_MC_LOAD_CHECKSUM&&frontier==0x8c08);
    CHECK(gh_mc_selected_load_apply_00002c88(&load,&checkpoint,&bind,&frontier)==GH_MC_LOAD_ARGUMENT);
    CHECK(!work.flags&&!staged[0x149f]&&!core[0x149f]);
    file[0x500]^=1;file[0]^=1;file[0xb110]^=1;
    save_fixture(path,file,sizeof(file));
    CHECK(gh_mc_selected_load_begin(&load,card,3)==GH_MC_LOAD_PENDING);
    CHECK(gh_mc_selected_load_step(&load,&frontier)==GH_MC_LOAD_PENDING);
    CHECK(gh_mc_selected_load_step(&load,&frontier)==GH_MC_LOAD_PENDING);
    CHECK(gh_mc_selected_load_step(&load,&frontier)==GH_MC_LOAD_VERSION&&frontier==0x2918);
    file[0]^=1;file[0xb110]^=1;file[0xb108]=0;file[0xb109]=0;file[0xb10a]=7;file[0xb10b]=0x43;
    save_fixture(path,file,sizeof(file));
    CHECK(gh_mc_selected_load_begin(&load,card,3)==GH_MC_LOAD_PENDING);
    CHECK(gh_mc_selected_load_step(&load,&frontier)==GH_MC_LOAD_PENDING);
    CHECK(gh_mc_selected_load_step(&load,&frontier)==GH_MC_LOAD_PENDING);
    CHECK(gh_mc_selected_load_step(&load,&frontier)==GH_MC_LOAD_VERIFIED&&!frontier);
    CHECK(!work.flags&&!staged[0x149f]&&!core[0x149f]);
    CHECK(gh_mc_selected_load_apply_00002c88(&load,&checkpoint,&bind,&frontier)==GH_MC_LOAD_UNSUPPORTED&&
          frontier==0x2a9bf0);
    CHECK(!memcmp(staged,file+0x20,GH_CHECKPOINT_BYTES));
    CHECK(!memcmp(core,staged,GH_CORE_SAVE_BYTES));
    CHECK(!memcmp(prepare.fields+0x1660,staged+0x14a0,0x9c30));
    CHECK(!memcmp(&checkpoint.tail,staged+0xb0d0,0x20));
    CHECK(checkpoint.tail.heat==0x43070000&&checkpoint.tail.heat_max==0xaa);
    CHECK(!memcmp(raw+0xb0,checkpoint.tail.position,12));
    CHECK(!memcmp(raw+0x490,checkpoint.tail.position,12));
    CHECK(!memcmp(raw+0x100,checkpoint.tail.rotation,12));
    memcpy(&flags,work.pad588,4);
    CHECK(flags==0xc0001000&&work.flags==2);
    core[0x149f]=0;
    bind.init=&init;
    memset(init.heat,0x33,sizeof(init.heat));
    const float old_heat=500.0f;
    memcpy(init.heat+4,&old_heat,4);
    CHECK(gh_mc_selected_load_apply_00002c88(&load,&checkpoint,&bind,&frontier)==GH_MC_LOAD_UNSUPPORTED&&
          frontier==0x1f9f50&&!core[0x149f]);
    uint32_t heat_words[6];
    memcpy(heat_words,init.heat,sizeof(heat_words));
    CHECK(heat_words[0]==0x43340000&&heat_words[1]==0x43070000&&heat_words[5]==1);
    CHECK(heat_words[2]==0x33333333&&heat_words[3]==0x33333333&&heat_words[4]==0x33333333);
    CHECK(!memcmp((uint8_t*)&work+0x5c0,checkpoint.tail.position,12));
    CHECK(!memcmp((uint8_t*)&work+0x5d0,&checkpoint.tail.rotation[1],4));
    CHECK(!memcmp((uint8_t*)&work+0x5b4,core+0xc,4)&&work.room==0);
    init.heat[0]=0x77;
    CHECK(gh_mc_selected_load_apply_00002c88(&load,&checkpoint,&bind,&frontier)==GH_MC_LOAD_UNSUPPORTED&&
          frontier==0x1f9f50&&init.heat[0]==0x77);
    CHECK(gh_mc_selected_load_apply_with_door_00002c88(&load,&checkpoint,&bind,&door,&frontier)==
          GH_MC_LOAD_UNSUPPORTED&&frontier==0x375700);
    const uint8_t expected_door[24]={0x84,0x85,0x86,0x87,0x88,0x89,0x8a,0x8b,
        0x8c,0x8d,0x8e,0x8f,0x90,0x91,0x92,0x93,0x82,0x83,0x80,0x81,0x94,0x95,0x96,0x97};
    CHECK(!memcmp(door.backup,expected_door,24)&&door.wait==7&&door.flags==0x1234);
    CHECK(init.world_time==0x12&&prepare.global_0061a994==0x34&&init.world_active_005e85f8==1);
    CHECK(raw[0x548]==100&&!raw[0x549]&&raw[0x54a]==1&&!raw[0x54b]);
    door.backup[0]=0;init.world_time=0;
    CHECK(gh_mc_selected_load_apply_with_door_00002c88(&load,&checkpoint,&bind,&door,&frontier)==
          GH_MC_LOAD_UNSUPPORTED&&frontier==0x375700&&!door.backup[0]&&!init.world_time);
    gh_disc *disc;
    gh_audio_registry *registry;
    gh_sound_control sound={0};
    gh_bgm_requests *bgm;
    gh_heap *audio_heap;
    CHECK(iso&&gh_disc_open(iso,&disc)==GH_DISC_OK);
    CHECK(gh_audio_registry_create(&registry)==GH_AUDIO_OK);
    CHECK(gh_audio_startup_load(registry,disc)==GH_AUDIO_OK);
    CHECK(gh_sound_control_from_startup(&sound,disc,registry)==GH_VOICE_OK);
    CHECK(gh_heap_create(32*1024*1024,&audio_heap)==GH_MEM_OK);
    CHECK(gh_bgm_requests_create(registry,&sound,disc,audio_heap,&bgm)==GH_BGM_OK);
    CHECK(gh_bgm_requests_restore_idle_output_mode_00375700(bgm,0)==GH_BGM_UNSUPPORTED&&sound.output_mode==1);
    CHECK(gh_bgm_event_set_prefix_002d15a8(bgm,0,0,0)==GH_BGM_UNSUPPORTED);
    init.cheat_00568240=(gh_game_cheat_state){1,0x12345678,2,{3,4,5,6}};
    CHECK(gh_mc_selected_load_restore_00002c88(&load,&checkpoint,&bind,&door,restore_output_mode,bgm,&frontier)==
          GH_MC_LOAD_UNSUPPORTED&&frontier==0x32ba30&&!work.language&&init.cheat_00568240.flags00==1);
    CHECK(gh_bgm_requests_abort_pending(bgm)==GH_BGM_OK);
    CHECK(gh_mc_selected_load_restore_00002c88(&load,&checkpoint,&bind,&door,restore_output_mode,bgm,&frontier)==
          GH_MC_LOAD_RESTORED&&!frontier&&sound.output_mode==1&&work.language==2);
    const gh_game_cheat_state expected_cheat={.preserved04=0x12345678};
    CHECK(!memcmp(&init.cheat_00568240,&expected_cheat,sizeof(expected_cheat)));
    work.language=7;
    CHECK(gh_mc_selected_load_restore_00002c88(&load,&checkpoint,&bind,&door,restore_output_mode,bgm,&frontier)==
          GH_MC_LOAD_RESTORED&&!frontier&&work.language==7);
    gh_bgm_requests_destroy(bgm);
    CHECK(gh_heap_destroy(audio_heap)==GH_MEM_OK);
    gh_audio_registry_destroy(registry);
    gh_disc_close(disc);
    puts("PASS ISO-backed idle ADX mode and selected checkpoint tail");
}


static void create_scan_slot(const char *profile,unsigned slot,unsigned icon_size,int version,unsigned year)
{
    char folder[1500],path[1600];
    CHECK(snprintf(folder,sizeof(folder),"%s\\BASLUS-21503GODHD0%02u",profile,slot)>0);
    CHECK(CreateDirectoryA(folder,NULL));
    CHECK(snprintf(path,sizeof(path),"%s\\icon.sys",folder)>0);
    uint8_t icon[0x3c4]={0};
    save_fixture(path,icon,icon_size);
    if(version<0)return;
    uint8_t header[GH_MC_SAVE_HEADER_BYTES]={0xcd,0xcc,0xcc,0x3d};
    if(!version)header[0]=0;
    header[0x19]=(uint8_t)year;
    CHECK(snprintf(path,sizeof(path),"%s\\BASLUS-21503GODHD0%02u",folder,slot)>0);
    save_fixture(path,header,sizeof(header));
}

static void remove_scan_slot(const char *profile,unsigned slot,int have_save)
{
    char folder[1500],path[1600];
    CHECK(snprintf(folder,sizeof(folder),"%s\\BASLUS-21503GODHD0%02u",profile,slot)>0);
    CHECK(snprintf(path,sizeof(path),"%s\\icon.sys",folder)>0);
    CHECK(DeleteFileA(path));
    if(have_save){
        CHECK(snprintf(path,sizeof(path),"%s\\BASLUS-21503GODHD0%02u",folder,slot)>0);
        CHECK(DeleteFileA(path));
    }
    CHECK(RemoveDirectoryA(folder));
}

static void title_scan_fixture(const char *profile,gh_heap *heap,gh_memory_card *card,gh_memory_card *absent)
{
    gh_mc_boot_checks checks;
    gh_mc_load_menu_memory memory={3,1};
    gh_mc_load_scan scan={0};
    uint32_t frontier;
    CHECK(gh_mc_boot_checks_create(&checks,heap)==GH_MC_BOOT_OK);
    checks.mode=1;
    CHECK(!gh_mc_title_load_scan_00001590(&scan,&checks,absent,&memory,&frontier)&&checks.phase==0);
    CHECK(!gh_mc_title_load_scan_00001590(&scan,&checks,absent,&memory,&frontier)&&checks.phase==7&&checks.error==1);
    CHECK(memory.first_slot==3&&memory.selected_row==1);
    CHECK(gh_mc_boot_checks_release(&checks,heap)==GH_MC_BOOT_OK);
    CHECK(gh_mc_boot_checks_create(&checks,heap)==GH_MC_BOOT_OK);
    checks.mode=1;
    CHECK(!gh_mc_title_load_scan_00001590(&scan,&checks,card,&memory,&frontier)&&checks.phase==0);
    CHECK(!gh_mc_title_load_scan_00001590(&scan,&checks,card,&memory,&frontier)&&checks.phase==1);
    /* The existing slot3 has a short icon and is invisible to cMcLoad. */
    CHECK(!gh_mc_title_load_scan_00001590(&scan,&checks,card,&memory,&frontier)&&
          checks.phase==1&&checks.file_index==3&&checks.subphase==2);
    CHECK(!gh_mc_title_load_scan_00001590(&scan,&checks,card,&memory,&frontier)&&checks.phase==7&&checks.error==8);
    CHECK(memory.first_slot==-1&&memory.selected_row==-1);
    CHECK(gh_mc_boot_checks_release(&checks,heap)==GH_MC_BOOT_OK);

    create_scan_slot(profile,1,0x3c4,1,0x26);
    create_scan_slot(profile,4,0x3c4,-1,0);
    create_scan_slot(profile,6,0x3c4,0,0x29);
    create_scan_slot(profile,19,0x3c4,1,0x27);
    CHECK(gh_mc_boot_checks_create(&checks,heap)==GH_MC_BOOT_OK);
    checks.mode=1;
    unsigned scans=0;
    while(checks.phase<3&&scans<25){
        CHECK(!gh_mc_title_load_scan_00001590(&scan,&checks,card,&memory,&frontier)&&!frontier);
        ++scans;
    }
    CHECK(scans==19&&checks.phase==3&&scan.menu_ready);
    CHECK(checks.present_mask==((1u<<1)|(1u<<19))&&checks.load_index==20&&checks.file_index==20);
    CHECK(scan.first_slot==17&&scan.selected_row==2);
    const uint8_t *old=(const uint8_t*)checks.icon_work.pointer+6*GH_MC_SAVE_HEADER_BYTES;
    CHECK(old[0]==0&&old[4]==0xff&&old[0x19]==0);
    CHECK(gh_mc_title_load_scan_00001590(&scan,&checks,card,&memory,&frontier)==GH_MC_BOOT_UNSUPPORTED&&
          frontier==0x20b0&&checks.phase==3);
    CHECK(gh_mc_boot_checks_release(&checks,heap)==GH_MC_BOOT_OK);
    remove_scan_slot(profile,1,1);
    remove_scan_slot(profile,4,0);
    remove_scan_slot(profile,6,1);
    remove_scan_slot(profile,19,1);
}

int main(int argc,char **argv)
{
    char canonical[1200],profile[1400],folder[1500],icon[1600],save_path[1600],absent[1500];
    static const char suffix[]="\\memory-card-tests\\scratch";
    size_t canonical_length;
    gh_memory_card *card,*missing;gh_mc_result result;int done;FILE *file;
    gh_heap *heap;gh_mc_boot_checks state;uint32_t flags=4;
    gh_task_record32 records[3]={{0}};gh_task_manager manager={records,3,0,-1,3,0,0,0};
    flow f={&state,0,0,4,0};int32_t slot;unsigned ticks=0,i;int32_t sentinel;
    CHECK(argc==3);
    CHECK(GetFullPathNameA(argv[1],sizeof(canonical),canonical,0));
    canonical_length=strlen(canonical);
    while(canonical_length>3 && canonical[canonical_length-1]=='\\')canonical[--canonical_length]=0;
    CHECK(canonical_length>=sizeof(suffix)-1);
    CHECK(!_stricmp(canonical+canonical_length-(sizeof(suffix)-1),suffix));
    /* Accept the build's external scratch root, with canonical absolute path
     * and a task-specific directory suffix; only a newly created PID child is
     * mutated/deleted, never recursive cleanup or preexisting profile data. */
    CHECK(snprintf(profile,sizeof(profile),"%s\\profile-%lu",canonical,(unsigned long)GetCurrentProcessId())>0);
    CHECK(CreateDirectoryA(profile,0));
    CHECK(snprintf(folder,sizeof(folder),"%s\\BASLUS-21503GODHD003",profile)>0);
    CHECK(CreateDirectoryA(folder,0));
    CHECK(snprintf(icon,sizeof(icon),"%s\\icon.sys",folder)>0);
    CHECK(snprintf(save_path,sizeof(save_path),"%s\\BASLUS-21503GODHD003",folder)>0);
    file=fopen(icon,"wb");CHECK(file);CHECK(fwrite("test-icon",1,9,file)==9);CHECK(!fclose(file));
    CHECK(snprintf(absent,sizeof(absent),"%s\\not-inserted",profile)>0);
    CHECK(gh_memory_card_open(profile,&card)==GH_MC_OK);
    CHECK(gh_memory_card_request_info(card)==GH_MC_OK);
    CHECK(gh_memory_card_request_icon(card,0)==GH_MC_BUSY);
    CHECK(gh_memory_card_poll(card,&done,&result)==GH_MC_OK && done);
    CHECK(result.result==0 && result.card_type==2 && result.formatted==1 && result.free_clusters>=150);
    CHECK(gh_memory_card_poll(card,&done,&result)==GH_MC_OK && !done);
    CHECK(gh_memory_card_request_icon(card,3)==GH_MC_OK);
    CHECK(gh_memory_card_poll(card,&done,&result)==GH_MC_OK && done && result.result==1);
    CHECK(gh_memory_card_request_icon(card,0)==GH_MC_OK);
    CHECK(gh_memory_card_poll(card,&done,&result)==GH_MC_OK && done && result.result==0);
    CHECK(gh_memory_card_request_icon(card,20)==GH_MC_ARGUMENT);
    CHECK(gh_heap_create(65536,&heap)==GH_MEM_OK);
    CHECK(gh_mc_boot_checks_create(&state,heap)==GH_MC_BOOT_OK);
    CHECK(gh_heap_used(heap)==0x5000 && (uintptr_t)state.icon_work.pointer%64==0);
    for(i=0;i<20;++i) {
        memcpy(&sentinel,(unsigned char*)state.icon_work.pointer+i*0x400+4,4);
        CHECK(sentinel==-1);
    }
    f.card=card;
    CHECK(gh_task_threads_create(&manager,&f.threads)==GH_THREAD_OK);
    CHECK(gh_task_threads_start(f.threads,-1,check_task,&f,91,&slot)==GH_THREAD_OK);
    while(!f.completed && ticks<100) {CHECK(gh_tasks_run_matching_002d52a0(&manager).status==GH_TASK_OK);++ticks;}
    CHECK(f.completed && ticks==32 && state.phase==2 && !state.done);
    CHECK(state.present_mask==(1u<<3) && state.file_index==20 && !(f.flags&4));
    CHECK(gh_task_threads_destroy(f.threads)==GH_THREAD_OK);
    gh_boot_work work={0};
    uint32_t frontier;
    CHECK(!gh_mc_file_load_00000730(&state,card,&work,0,&frontier)&&!frontier&&state.subphase==1);
    CHECK(gh_memory_card_end(card)==GH_MC_BUSY);
    CHECK(!gh_mc_file_load_00000730(&state,card,&work,0,&frontier)&&!frontier&&state.phase==3&&state.error==2);
    CHECK(gh_mc_boot_checks_release(&state,heap)==GH_MC_BOOT_OK);
    uint8_t header[GH_MC_SAVE_HEADER_BYTES]={0xcd,0xcc,0xcc,0x3d};
    header[8]=2;header[0x30]=1;
    file=fopen(save_path,"wb");CHECK(file);
    CHECK(fwrite(header,1,sizeof(header),file)==sizeof(header)&&fputc(0x5a,file)==0x5a&&!fclose(file));
    CHECK(!gh_mc_boot_checks_create(&state,heap));
    state.phase=2;state.present_mask=1u<<3;
    for(unsigned stage=1;stage<=3;stage++){
        CHECK(!gh_mc_file_load_00000730(&state,card,&work,0,&frontier)&&!frontier&&
              state.phase==2&&state.subphase==stage&&state.load_index==3);
    }
    CHECK(!gh_mc_file_load_00000730(&state,card,&work,0,&frontier)&&!frontier&&
          state.phase==3&&state.error==10&&state.present_mask==(1u<<3)&&state.load_index==20);
    CHECK(!memcmp((uint8_t *)state.icon_work.pointer+3*GH_MC_SAVE_HEADER_BYTES,header,sizeof(header)));
    memcpy(&flags,work.pad588,4);CHECK(flags==0xc0000000u&&(work.flags&4));
    CHECK(gh_mc_boot_checks_release(&state,heap)==GH_MC_BOOT_OK);
    /* Original version check rejects an older 0x400-byte boot record. */
    header[0]=0;
    file=fopen(save_path,"wb");CHECK(file);CHECK(fwrite(header,1,sizeof(header),file)==sizeof(header)&&!fclose(file));
    CHECK(!gh_mc_boot_checks_create(&state,heap));
    state.phase=2;state.present_mask=1u<<3;work.flags=0;
    for(unsigned stage=0;stage<4;stage++)CHECK(!gh_mc_file_load_00000730(&state,card,&work,0,&frontier)&&!frontier);
    CHECK(state.phase==3&&state.error==2&&!state.present_mask&&!(work.flags&4));
    CHECK(gh_mc_boot_checks_release(&state,heap)==GH_MC_BOOT_OK);
    /* A genuinely missing native profile enters the original card-error path;
     * it is not silently treated as an empty inserted/formatted card. */
    CHECK(gh_memory_card_open(absent,&missing)==GH_MC_OK);
    CHECK(gh_mc_boot_checks_create(&state,heap)==GH_MC_BOOT_OK);
    CHECK(gh_mc_boot_check_control(&state,missing,&flags)==GH_MC_BOOT_OK);
    CHECK(gh_mc_boot_check_control(&state,missing,&flags)==GH_MC_BOOT_OK);
    CHECK(state.phase==3 && state.error==1 && !state.done);
    CHECK(gh_mc_boot_check_control(&state,missing,&flags)==GH_MC_BOOT_UNSUPPORTED);
    CHECK(gh_mc_boot_checks_release(&state,heap)==GH_MC_BOOT_OK);
    selected_fixture(save_path,card,argv[2]);
    title_scan_fixture(profile,heap,card,missing);
    gh_memory_card_close(missing);gh_memory_card_close(card);
    CHECK(gh_heap_used(heap)==0 && gh_heap_destroy(heap)==GH_MEM_OK);
    CHECK(DeleteFileA(save_path));CHECK(DeleteFileA(icon));CHECK(RemoveDirectoryA(folder));CHECK(RemoveDirectoryA(profile));
    puts("PASS native card queries, 32 fiber ticks, and boot header checks, selected full-record restore, and title load card scan");
    return 0;
}
