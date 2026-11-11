#include "game_subscreen_save.h"
#include "game_door.h"
#include "game_scene_exit.h"
#include "id_jobs.h"
#include "audio_startup.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define C(x) do{if(!(x)){fprintf(stderr,"FAIL %d %s\n",__LINE__,#x);exit(1);}}while(0)
typedef struct {gh_audio_registry *registry;unsigned count;uint16_t last;} sound_state;
typedef struct {
    gh_game_scene_exit exit;
    gh_game_scene_exit_bindings bindings;
    unsigned calls;
} scenario_state;
static gh_door_status end_empty_scenario(void *context,uint32_t entry,uint32_t *frontier)
{
    scenario_state *s=context;
    C(entry==0x2c3858);
    C(gh_game_scene_scenario_stop_002c3858(&s->exit,&s->bindings)==GH_SCENE_EXIT_OK);
    ++s->calls;
    *frontier=s->exit.frontier;
    return GH_DOOR_OK;
}
static gh_child_status sound(void *context,uint16_t program,uint32_t *frontier)
{
    sound_state *s=context;
    uint8_t row[48];
    C(gh_audio_get_program(s->registry,0,program,0,row)==GH_AUDIO_OK);
    ++s->count;s->last=program;*frontier=0;
    return GH_CHILD_OK;
}
typedef struct {
    const gh_save_map_bindings *map;
    gh_id_base *base;
    gh_render_draw_env *env;
    unsigned cover_count,trans_count;
    uint32_t failure;
} draw_fixture;

static gh_child_status cover(void *context,const gh_child_cover_quad *quad,
    int *emitted,uint32_t *frontier)
{
    draw_fixture *draw=context;
    C(quad);
    ++draw->cover_count;
    *emitted=1;
    *frontier=0;
    return GH_CHILD_OK;
}

static gh_child_status trans(void *context,gh_id_base *base,uint16_t chapter_message,
    uint32_t *frontier)
{
    draw_fixture *draw=context;
    C(draw->cover_count==draw->trans_count+1&&base==draw->base&&chapter_message==0x2024);
    C(draw->env->scalar.bank==7&&draw->env->scalar.depth==1);
    ++draw->trans_count;
    if(draw->failure){*frontier=draw->failure;return GH_CHILD_UNSUPPORTED;}
    const gh_save_map_bindings *map=draw->map;
    int original;
    C(gh_packed_message_bind(base,map->manager,map->font,map->packed,map->language,1,7,1,&original)==GH_ID_OK&&original==1);
    gh_id_draw_job jobs[128];
    size_t count;
    gh_id_frontier job_frontier;
    C(gh_id_jobs_collect_visible(base,jobs,128,&count,&job_frontier)==GH_ID_OK&&count>0);
    unsigned chapter_jobs=0;
    for(size_t i=0;i<count;++i)
        if(jobs[i].entry->message==0x2022){
            C(jobs[i].consumer_address==0x2ab0b8);
            ++chapter_jobs;
        }
    C(chapter_jobs>0);
    *frontier=0;
    return GH_CHILD_OK;
}
static uint32_t word(const void *p){uint32_t v;memcpy(&v,p,4);return v;}
static void set(gh_boot_work *w,unsigned at,uint32_t value){memcpy((uint8_t*)w+at,&value,4);}
int main(int argc,char **argv)
{
    gh_disc *disc;gh_heap *heap;
    gh_game_children *children;
    gh_id_state *manager=calloc(1,sizeof(*manager));
    gh_id_runtime *runtime;
    gh_dvd_jobs dvd;
    gh_r207_allocation allocation;
    gh_bundle language;
    gh_bundle_entry idd;
    gh_disc_entry entry;
    size_t file;
    gh_packed_message_state packed={0};
    gh_font_state font={0};
    gh_screen_fade fade={0};
    gh_game_scene_aux cursor={0};
    gh_boot_work work={.room=0x100,.language=1};
    uint8_t player_bytes[GH_TITLE_PLAYER_BYTES]={0},save_bytes[GH_CORE_SAVE_BYTES];
    gh_game_title_player player={.ready=1};
    C(argc==2&&manager&&gh_disc_open(argv[1],&disc)==GH_DISC_OK);
    C(gh_heap_create(12*1024*1024,&heap)==GH_MEM_OK);
    gh_dvd_jobs_init(&dvd,disc);
    C(gh_dvd_read_alloc(&dvd,"etc/core_usa.bin",&allocation,heap,0,0,0,0));
    C(gh_dvd_pump(&dvd)==GH_DVD_OK&&gh_disc_find(disc,"etc/core_usa.bin",&file)==GH_DISC_OK);
    C(gh_disc_entry_at(disc,file,&entry)==GH_DISC_OK&&gh_bundle_open(allocation.pointer,(size_t)entry.size,&language)==GH_BUNDLE_OK);
    C(gh_id_find_language_data(&language,&idd)==GH_MENU_OK&&gh_id_system_init_002acc70(manager,&idd)==GH_MENU_OK);
    C(gh_id_runtime_create(manager,1,&runtime)==GH_ID_OK&&gh_font_initialize(&font,&language,1)==GH_FONT_OK);
    C(gh_game_children_create(NULL,&children)==GH_CHILD_OK&&gh_game_children_initialize_0015fd98(children)==GH_CHILD_OK);
    C(gh_title_player_construct_00107e40(player_bytes,sizeof(player_bytes),&player.view)==GH_PLAYER_OK);
    gh_core_save_view save={save_bytes,sizeof(save_bytes)};
    C(gh_core_save_system_init(save)==GH_INIT_OK);
    sound_state audio={0};
    C(gh_audio_registry_create(&audio.registry)==GH_AUDIO_OK&&gh_audio_startup_load(audio.registry,disc)==GH_AUDIO_OK);
    gh_save_map_bindings b={.work=&work,.player=&player,.cursor=&cursor,.save=save,.dvd=&dvd,
       .subscreen_heap=heap,.manager=manager,.id_runtime=runtime,.font=&font,.packed=&packed,.language=&language,
       .fade=&fade,.fade_context={512,1824,0,0},.map_room=0x100,.target_room=0x101,.sound=sound,.sound_context=&audio};
    uint32_t frontier=0;
    int started=-1;
    set(&work,0x5e4,0x20000000);
    C(gh_game_children_save_map_open_00160020(children,&b,&started,&frontier)==GH_CHILD_OK&&!started);
    gh_game_door door={0};
    C(gh_game_door_from_disc(&door,disc)==GH_DOOR_OK);
    door.target_room=0x101;
    door.position_bits[0]=0x3f800000;
    gh_sound_control control={0};
    C(gh_sound_control_from_startup(&control,disc,audio.registry)==GH_VOICE_OK);
    control.bgm_state_13c=2;
    uint8_t scenario_bytes[0x114]={0};
    gh_task_record32 records[4]={{0}};
    gh_task_manager tasks={records,4,0,-1,4,0,0,0};
    gh_task_threads *threads;
    C(gh_task_threads_create(&tasks,&threads)==GH_THREAD_OK);
    scenario_state scenario={.bindings={.scenario={.bytes=scenario_bytes,.size=sizeof(scenario_bytes),.manager=&tasks},
        .scenario_threads=threads}};
    const gh_door_save_prompt prompt={children,&b,&control,end_empty_scenario,&scenario};
    const gh_game_door_bindings door_bindings={.work=&work,.player=&player,.fade=&fade,.save_prompt=&prompt};
    set(&work,0x5e4,0x02000000);
    set(&work,0x5d8,0x00012000);
    set(&work,0x5e0,0x00100000);
    C(gh_game_door_step_001ffe10(&door,&door_bindings)==GH_DOOR_OK&&!door.frontier);
    C(door.verify_phase39==1&&scenario.calls==1&&control.bgm_state_13c==0);
    C(!memcmp(door.backup,(const uint8_t*)&door+4,sizeof(door.backup)));
    C(gh_game_door_step_001ffe10(&door,&door_bindings)==GH_DOOR_OK&&scenario.calls==1);
    C(word((uint8_t*)&work+0x5e4)==0x0a000000&&word(player_bytes+0x54c)==0x40a00000);
    const uint8_t *state;
    size_t bytes;
    C(gh_game_children_manager_view(children,&state,&bytes)==GH_CHILD_OK&&bytes>=0xf18);
    gh_save_map_view view;
#define TICK() do{gh_child_status e=gh_game_children_save_map_move_001601d8(children,&b,&frontier); \
 if(e)fprintf(stderr,"status%u frontier%06x state%u\n",e,frontier,state[0x50]);C(e==GH_CHILD_OK&&!frontier); \
 C(gh_dvd_pump(&dvd)==GH_DVD_OK);if(fade.state&&!(fade.state&4)){gh_fade_draw draw; \
 C(gh_screen_fade_step_00307388(&fade,&b.fade_context,&draw)==GH_FADE_OK);} }while(0)
    const size_t baseline=gh_heap_used(heap);
    for(unsigned frame=0;frame<30&&state[0x50]!=5;++frame)TICK();
    C(state[0x50]==5&&manager->pair[7].bound&&packed.initialized);
    C(gh_game_children_save_map_view(children,&view)==GH_CHILD_OK&&view.visible&&view.variant==2);
    C(view.chapter_message==0x2024&&view.selected==0&&view.base->resource_number==7);
    const size_t elf_size=(size_t)gh_disc_boot_elf_size(disc);
    uint8_t *elf=malloc(elf_size);
    gh_render_draw_env env;
    C(elf&&gh_disc_read_boot_elf(disc,0,elf,elf_size)==GH_DISC_OK);
    C(gh_render_draw_env_from_elf(&env,elf,elf_size)==GH_RENDER_STEP_OK);
    free(elf);
    draw_fixture draw={.map=&b,.base=view.base,.env=&env};
    work.system_flags|=0x80000000;
    C(gh_game_children_draw_with_save_map_00160b90(children,&work,&env,cover,trans,&draw,&frontier)==GH_CHILD_OK&&!frontier);
    work.system_flags&=~0x80000000u;
    set(&work,0x5dc,0x100000);
    C(gh_game_children_draw_with_save_map_00160b90(children,&work,&env,cover,trans,&draw,&frontier)==GH_CHILD_OK&&!frontier);
    set(&work,0x5dc,0);
    C(!draw.cover_count&&!draw.trans_count);
    C(gh_game_children_draw_with_save_map_00160b90(children,&work,&env,cover,trans,&draw,&frontier)==GH_CHILD_OK&&!frontier);
    C(draw.cover_count==1&&draw.trans_count==1);
    draw.failure=0x2ab0b8;
    C(gh_game_children_draw_with_save_map_00160b90(children,&work,&env,cover,trans,&draw,&frontier)==GH_CHILD_UNSUPPORTED&&frontier==0x2ab0b8);
    C(draw.cover_count==2&&draw.trans_count==2);
    C(gh_game_children_draw_00160b90(children,&work,&env,cover,&draw,&frontier)==GH_CHILD_UNSUPPORTED&&frontier==0x1640e8);
    C(draw.cover_count==3&&draw.trans_count==2);
    gh_id_entry *pointer;
    C(gh_id_base_get_work_002aa410(runtime,view.base,18,&pointer,NULL)==GH_ID_OK&&pointer->pos_y==22);
    set(&work,0x1b0,0x01000000);TICK();
    C(gh_game_children_save_map_view(children,&view)==GH_CHILD_OK&&view.selected==3&&pointer->pos_y==156&&audio.last==0x15f);
    set(&work,0x1b0,0x02000000);TICK();
    C(gh_game_children_save_map_view(children,&view)==GH_CHILD_OK&&view.selected==0&&pointer->pos_y==22);
    set(&work,0x1b0,0);
    set(&work,0x1a0,0x10000000);TICK();
    C(state[0x50]==10&&audio.last==0x15e&&audio.count==3);
    set(&work,0x1a0,0);
    for(unsigned frame=0;frame<25&&state[0x50]!=12;++frame)TICK();
    C(state[0x50]==12&&!manager->pair[7].bound&&gh_heap_used(heap)==baseline);
    TICK();
    C(state[0x50]==12&&state[0x52]==1&&cursor.save_load[0]==3&&cursor.save_mode==0);
    C((word((uint8_t*)&work+0x5e8)&0x40000000)&&word((uint8_t*)&work+0x5e4)==0x0a000000);
    TICK();
    C(state[0x50]==12&&state[0x52]==1&&audio.count==3);
#undef TICK
    C(gh_task_threads_destroy(threads)==GH_THREAD_OK);
    gh_game_children_destroy(children);
    gh_audio_registry_destroy(audio.registry);
    C(gh_id_runtime_destroy(runtime)==GH_ID_OK&&gh_heap_release(heap,allocation)==GH_MEM_OK&&gh_heap_destroy(heap)==GH_MEM_OK);
    gh_disc_close(disc);free(manager);
    puts("PASS actual USA Door map outer cover gates, typed 1640E8 Trans, selection, fade and OpenSave handoff");
    return 0;
}
