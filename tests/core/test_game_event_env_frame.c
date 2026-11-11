#include "game_event_env_frame.h"
#include "game_event_play_env.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(x) do { if (!(x)) { fprintf(stderr,"line %d: %s\n",__LINE__,#x); exit(1); } } while (0)

static uint32_t word(const uint8_t *p){uint32_t value;memcpy(&value,p,4);return value;}
static float get_frame(const uint8_t *p){float value;memcpy(&value,p,4);return value;}
static void put(uint8_t *p,uint32_t value){memcpy(p,&value,4);}

int main(int argc,char **argv)
{
    CHECK(argc==2);
    gh_disc *disc;
    gh_heap *heap;
    CHECK(!gh_disc_open(argv[1],&disc) && !gh_heap_create(2*1024*1024,&heap));
    size_t elf_bytes=(size_t)gh_disc_boot_elf_size(disc);
    uint8_t *elf=malloc(elf_bytes);
    CHECK(elf && !gh_disc_read_boot_elf(disc,0,elf,elf_bytes));
    gh_game_event event={0};
    gh_game_event_config config={0};
    gh_boot_work work={0};
    CHECK(!gh_game_event_from_elf(&event,elf,elf_bytes) && !gh_game_event_config_from_elf(&config,elf,elf_bytes));
    CHECK(!gh_game_event_init_environment_002975f8(&event,&config,1,elf,elf_bytes));
    gh_dvd_jobs dvd;
    gh_dvd_jobs_init(&dvd,disc);
    gh_event_module module={0};
    gh_event_resource resource={0};
    int ready=0;
    CHECK(!gh_game_event_module_begin_00297378(&module,&event,&work,&dvd,heap));
    for(unsigned i=0;i<8 && !ready;++i)CHECK(!gh_dvd_pump(&dvd) && !gh_game_event_module_poll_00297428(&module,&ready));
    CHECK(ready && module.loaded);
    gh_event_components components={0};
    CHECK(!gh_event_components_bind(&components,&module,elf,elf_bytes));
    CHECK(!gh_event_components_construct_00297680(&components,1));
    CHECK(!gh_game_event_resource_begin_00297540(&resource,&event,&dvd,heap));
    ready=0;
    for(unsigned i=0;i<8 && !ready;++i)CHECK(!gh_dvd_pump(&dvd) && !gh_game_event_resource_poll_002975d0(&resource,&ready));
    CHECK(ready && resource.loaded);

    gh_event_data_prefix data={0};
    gh_event_timing timing={0};
    uint32_t frontier;
    CHECK(!gh_event_data_bind_prefix_002977e8(&data,&components,&resource,&frontier));
    CHECK(!gh_event_timing_prepare_000054c0(&timing,&components,heap,&ready) && ready && timing.count==10);
    gh_screen_fade fade={0};
    gh_display_state display={.initialized=1,.screen_width=512,.gs_y_origin=1824};
    CHECK(!gh_event_env_init_play_00005930(&components,&config,&work,&fade,&display,&frontier));
    gh_event_component_view env;
    CHECK(!gh_event_component_at(&components,0,&env));
    CHECK(word(env.bytes+0x10)==UINT32_MAX && get_frame(env.bytes+0x14)==-1);
    const gh_event_env_frame_bindings bindings={.timing=&timing,.components=&components,.data=&data,
        .config=&config,.work=&work,.fade=&fade,.display=&display};
    static const uint16_t lengths[]={120,106,43,94,142,137,71,127,150,219};
    const gh_event_cut_timing *rows=timing.allocation.pointer;
    uint32_t elapsed=0;
    int original_end,relative;
    for(unsigned cut=0;cut<10;++cut){
        uint16_t length;
        CHECK(!gh_event_camera_cut_length_00004e60(&data,cut,&length) && length==lengths[cut]);
        /* The last cut requests its fade 30 frames before the end. */
        uint32_t visits=cut==9?(uint32_t)length-30: length;
        for(uint32_t frame=0;frame<visits;++frame){
            CHECK(!gh_event_env_update_frame_000059e0(&timing,&components,&frontier));
            CHECK(word(env.bytes+0x10)==cut && get_frame(env.bytes+0x14)==(float)frame);
            CHECK(word(env.bytes+0x28)&1);
            CHECK(rows[cut].frames_seen==(float)(frame+1) && rows[cut].start_frame==(float)elapsed);
            if(cut<9)CHECK(rows[cut+1].start_frame==(float)(elapsed+frame+1));
            if(cut==0 && frame==0){
                /* Actual 59D0 freeze publishes the same current values. */
                put(env.bytes+0x28,0x101);
                put(env.bytes+0x18,1);
                CHECK(!gh_event_env_update_frame_000059e0(&timing,&components,&frontier));
                CHECK(word(env.bytes+0x18)==0 && !(word(env.bytes+0x28)&1));
                put(env.bytes+0x28,0x30000040);
                CHECK(!gh_event_env_calc_next_00005b78(&bindings,&original_end,&frontier,&relative));
                CHECK(!word(env.bytes+0x28) && get_frame(env.bytes+0x1c)==0);
            }
            CHECK(!gh_event_env_calc_next_00005b78(&bindings,&original_end,&frontier,&relative));
            CHECK(!original_end && !frontier && !relative);
            if(cut<9 && frame+1==length){
                CHECK(word(env.bytes+0x18)==cut+1 && get_frame(env.bytes+0x1c)==0);
            }else CHECK(word(env.bytes+0x18)==cut && get_frame(env.bytes+0x1c)==(float)(frame+1));
        }
        elapsed+=length;
    }
    CHECK((word(env.bytes+0x28)&6)==6 && fade.state==11 && fade.time==15 && fade.color_to==0xff000000);
    gh_screen_fade_context fade_context={512,1824,0,0};
    gh_fade_draw draw;
    for(unsigned i=0;i<16;++i)CHECK(!gh_screen_fade_step_00307388(&fade,&fade_context,&draw));
    CHECK(fade.state&4);
    CHECK(gh_event_env_calc_next_00005b78(&bindings,&original_end,&frontier,&relative)==GH_EVENT_UNSUPPORTED);
    CHECK(!original_end && frontier==0x2db6b0 && !relative && get_frame(env.bytes+0x1c)==189);
    CHECK(gh_event_env_calc_next_00005b78(&bindings,&original_end,&frontier,&relative)==GH_EVENT_UNSUPPORTED);
    CHECK(fade.state&4);

    uint8_t player_bytes[GH_TITLE_PLAYER_BYTES]={0},jacket_bytes[GH_PLAYER_CHILD_BYTES]={0};
    gh_game_title_player player={.ready=1};
    gh_game_player_child jacket={.complete=1};
    jacket.object.ready=1;
    CHECK(!gh_title_player_construct_00107e40(player_bytes,sizeof(player_bytes),&player.view));
    CHECK(!gh_game_actor_construct_0014e990(jacket_bytes,sizeof(jacket_bytes),&jacket.object.view));
    put(player_bytes+0x250,0xa5508003);
    put(jacket_bytes+0x250,0xa5508003);
    gh_event_weapon_slot weapon_slot={0};
    gh_game_player_child *work5f8=&jacket;
    gh_event_env_end_owners owners={.work5f4=&player,.weapon_6a8=&weapon_slot};
    gh_event_env_frame_bindings ending=bindings;
    ending.end_owners=&owners;
    CHECK(gh_event_env_calc_next_00005b78(&ending,&original_end,&frontier,&relative)==GH_EVENT_UNSUPPORTED);
    CHECK(frontier==0x2db6c0 && !relative && word(player_bytes+0x250)==0xa5508003);
    owners.work5f8=&work5f8;
    CHECK(gh_event_env_calc_next_00005b78(&ending,&original_end,&frontier,&relative)==GH_EVENT_UNSUPPORTED);
    CHECK(!original_end && frontier==0x656c && relative);
    CHECK(word(player_bytes+0x250)==0xa5500001 && word(jacket_bytes+0x250)==0xa5500001);
    weapon_slot.known=1; /* 107E40 establishes the empty player+6A8 slot. */
    CHECK(!gh_event_env_calc_next_00005b78(&ending,&original_end,&frontier,&relative));
    CHECK(original_end==1 && !frontier && !relative && fade.state&4);

    /* Exercise the conditional jacket and attached weapon branches without
     * changing the real EV01 fixture's empty-weapon provenance. */
    uint8_t weapon_bytes[0x600]={0};
    gh_title_player_view weapon_view;
    CHECK(!gh_game_actor_construct_0014e990(weapon_bytes,sizeof(weapon_bytes),&weapon_view));
    gh_event_weapon_attachment attachment={.weapon=&weapon_view,.parent=&player.view};
    weapon_slot.current=&attachment;
    put(player_bytes+0x250,0x10002);
    put(jacket_bytes+0x250,0x20002);
    put(weapon_bytes+0x250,0x38003);
    CHECK(!gh_event_env_finish_000064c8(&owners,&frontier,&relative));
    CHECK(word(player_bytes+0x250)==0x10000 && word(jacket_bytes+0x250)==0x20002);
    CHECK(word(weapon_bytes+0x250)==0x30001);
    work5f8=NULL;
    put(weapon_bytes+0x250,2);
    CHECK(!gh_event_env_finish_000064c8(&owners,&frontier,&relative) && word(weapon_bytes+0x250)==2);
    CHECK(!gh_event_timing_abort(&timing) && !gh_game_event_resource_abort(&resource) && !gh_game_event_module_abort(&module));
    CHECK(!gh_heap_used(heap) && !gh_heap_destroy(heap));
    free(elf);gh_disc_close(disc);
    puts("PASS actual EV01 ten-cut clock, final fade and canonical 64C8 cleanup");
    return 0;
}
