#include "disc.h"
#include "game_enemy_2a2.h"
#include "game_enemy_factory.h"
#include "game_enemy_manager.h"
#include "game_enemy_near.h"
#include "game_enemy_request.h"
#include <fenv.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(x) do { if(!(x)){fprintf(stderr,"line %d: %s\n",__LINE__,#x);exit(1);} } while(0)

static uint16_t half(const uint8_t *p)
{
    uint16_t value;
    memcpy(&value,p,2);
    return value;
}

static void point(gh_game_title_player *actor,float x,float y,float z)
{
    const float position[]={x,y,z};
    memcpy(actor->view.alias_f0,position,sizeof(position));
}

typedef struct {
    const gh_game_title_player *first,*second;
    unsigned first_calls,second_calls;
    int fail_second;
} sound_capture;

static gh_actor_motion_status hit_sound(void *opaque,const gh_game_title_player *target,uint32_t *frontier)
{
    sound_capture *capture=opaque;
    *frontier=0;
    if(target==capture->first){
        capture->first_calls++;
        CHECK(half(target->view.bytes+0x54a)>0);
        return GH_ACTOR_MOTION_OK;
    }
    CHECK(target==capture->second);
    CHECK(half(target->view.bytes+0x54a)==1&&capture->first_calls==1);
    capture->second_calls++;
    if(capture->fail_second){
        capture->fail_second=0;
        *frontier=0x374cb8;
        return GH_ACTOR_MOTION_UNSUPPORTED;
    }
    return GH_ACTOR_MOTION_OK;
}

int main(int argc,char **argv)
{
    gh_disc *disc;
    CHECK(argc==2&&!gh_disc_open(argv[1],&disc));
    size_t elf_bytes=(size_t)gh_disc_boot_elf_size(disc),room_index;
    uint8_t *elf=malloc(elf_bytes);
    gh_disc_entry room_entry;
    CHECK(elf&&!gh_disc_read_boot_elf(disc,0,elf,elf_bytes)&&
        !gh_disc_find(disc,"st1/r100.dat",&room_index)&&!gh_disc_entry_at(disc,room_index,&room_entry));
    uint8_t *room_bytes=malloc((size_t)room_entry.size);
    CHECK(room_bytes&&!gh_disc_read(disc,room_index,0,room_bytes,(size_t)room_entry.size));
    gh_disc_close(disc);
    gh_bundle room_bundle;
    gh_bundle_entry ems;
    CHECK(!gh_bundle_open(room_bytes,(size_t)room_entry.size,&room_bundle)&&
        !gh_bundle_find(&room_bundle,"EMS",0,&ems));

    gh_boot_work work={.room=0x100};
    uint8_t save_bytes[GH_CORE_SAVE_BYTES],game_state=0,stop=1;
    gh_core_save_view save={save_bytes,sizeof(save_bytes)};
    gh_game_prepare prepare={0};
    gh_room_state room;
    gh_em_scene_placements placement={.stop_flag=&stop};
    int loaded;
    CHECK(!gh_core_save_system_init(save)&&!gh_game_prepare_002a5d40(&prepare,save,0x100,&game_state)&&
        !gh_room_state_select_002bebc8(&prepare,0x100,0,&room)&&
        !gh_em_scene_load_native_00294b98(&placement,&room,&ems,&work,0,
            GH_EM_NUMERIC_PCSX2_CHOPZERO_REFERENCE,&loaded)&&loaded);
    gh_enemy_request ordinary,heavy;
    int spawn;
    int16_t saved;
    uint32_t frontier;
    CHECK(!gh_enemy_request_from_room_00294ca0(&room,2,GH_EM_NUMERIC_PCSX2_CHOPZERO_REFERENCE,
        &ordinary,&spawn,&saved,&frontier)&&spawn&&ordinary.object_kind==0x202&&ordinary.entry_number==3);
    CHECK(!gh_enemy_request_from_room_00294ca0(&room,5,GH_EM_NUMERIC_PCSX2_CHOPZERO_REFERENCE,
        &heavy,&spawn,&saved,&frontier)&&spawn&&heavy.object_kind==0x2a2&&heavy.entry_number==6);
    CHECK(!gh_enemy_request_from_room_00294ca0(&room,10,GH_EM_NUMERIC_PCSX2_CHOPZERO_REFERENCE,
        &heavy,&spawn,&saved,&frontier)&&!spawn);

    gh_game_init_state *manager=calloc(1,sizeof(*manager));
    CHECK(manager);
    gh_game_init_state_initialize(manager);
    gh_game_mt_seed_0030f2b0(&manager->random,123);
    CHECK(!gh_game_enemy_reset_00290230(manager));
    gh_game_title_player source={0},other={0},first={0},second={0};
    _Alignas(16) uint8_t source_bytes[GH_EM202_BYTES]={0},other_bytes[GH_EM202_BYTES]={0};
    _Alignas(16) uint8_t first_bytes[GH_EM2A2_BYTES]={0},second_bytes[GH_EM2A2_BYTES]={0};
    CHECK(!gh_enemy202_construct_00201fa0(source_bytes,sizeof(source_bytes),&source.view)&&
        !gh_enemy202_construct_00201fa0(other_bytes,sizeof(other_bytes),&other.view)&&
        !gh_enemy2a2_construct_00284a28(first_bytes,sizeof(first_bytes),&first.view)&&
        !gh_enemy2a2_construct_00284a28(second_bytes,sizeof(second_bytes),&second.view));
    memcpy(source_bytes+0x2fe,&(uint16_t){0x202},2);
    memcpy(other_bytes+0x2fe,&(uint16_t){0x202},2);
    memcpy(first_bytes+0x2fe,&(uint16_t){0x2a2},2);
    memcpy(second_bytes+0x2fe,&(uint16_t){0x2a2},2);
    memcpy(first_bytes+0x54a,&(uint16_t){600},2);
    memcpy(source.view.alias_f0,ordinary.position_xyz,12);
    memcpy(first.view.alias_f0,heavy.position_xyz,12);
    CHECK(source.view.original_kind4ac==0x42c7f0&&first.view.original_kind4ac==0x448a00);
    const gh_game_title_player *found=NULL;
    CHECK(!gh_enemy_find_ema2_near_00292dc8(manager,&source.view,4,elf,elf_bytes,&found,&frontier)&&!found);
    CHECK(!gh_enemy_manager_insert_00290048(manager,11,&source,0x202,&frontier)&&
        !gh_enemy_manager_insert_00290048(manager,3,&first,0x2a2,&frontier));
    CHECK(!gh_enemy_find_ema2_near_00292dc8(manager,&source.view,4,elf,elf_bytes,&found,&frontier));
    CHECK(found==&first);
    CHECK(!gh_enemy_find_ema2_near_00292dc8(manager,&source.view,2,elf,elf_bytes,&found,&frontier)&&found==&first);
    float original_source[3],original_ema2[3];
    memcpy(original_source,source.view.alias_f0,12);memcpy(original_ema2,first.view.alias_f0,12);
    printf("Actual R100 entry3 (%.3f %.3f %.3f) to entry6 (%.3f %.3f %.3f): within both radius4 and radius2\n",
        original_source[0],original_source[1],original_source[2],original_ema2[0],original_ema2[1],original_ema2[2]);

    /* Boundary/retry fixture keeps the original class and list layout. */
    CHECK(!gh_enemy_manager_insert_00290048(manager,55,&second,0x2a2,&frontier)&&
        !gh_enemy_manager_insert_00290048(manager,0,&other,0x202,&frontier));
    point(&source,0,0,0);
    point(&first,2,1000,0);
    point(&second,0,-1000,0);
    point(&other,4,0,0);
    source_bytes[0x16d0]=2;
    other_bytes[0x16d0]=2;
    other_bytes[0x2f5]=2;
    first_bytes[0x2f4]=1;
    memcpy(first_bytes+0x54a,&(uint16_t){2},2);
    memcpy(second_bytes+0x54a,&(uint16_t){1},2);
    second_bytes[0x15b0]=2;
    CHECK(!fesetround(FE_UPWARD)&&!feclearexcept(FE_ALL_EXCEPT)&&!feraiseexcept(FE_DIVBYZERO));
    CHECK(!gh_enemy_find_ema2_near_00292dc8(manager,&source.view,2,elf,elf_bytes,&found,&frontier)&&found==&first);
    CHECK(!gh_enemy_find_ema2_near_00292dc8(manager,&source.view,1,elf,elf_bytes,&found,&frontier)&&found==&second);
    int near;
    CHECK(!gh_enemy_active_near_002919f8(manager,&source.view,4,&near,&frontier)&&!near);
    CHECK(!gh_enemy_active_near_002919f8(manager,&source.view,5,&near,&frontier)&&near);
    CHECK(!gh_enemy_other_near_00291ac8(manager,&source.view,&source.view,4,&near,&frontier)&&!near);
    CHECK(!gh_enemy_other_near_00291ac8(manager,&source.view,&source.view,5,&near,&frontier)&&near);
    CHECK(!gh_enemy_other_near_00291ac8(manager,&source.view,NULL,1,&near,&frontier)&&near);
    CHECK(fegetround()==FE_UPWARD&&fetestexcept(FE_ALL_EXCEPT)==FE_DIVBYZERO);

    gh_enemy_ema2_damage_call damage={0};
    gh_enemy_ema2_damage_bindings binding={.manager=manager,.origin=&source.view,
        .player=&source.view,.work=&work,.elf=elf,.elf_bytes=elf_bytes};
    CHECK(gh_enemy_damage_ema2_near_00292d18(&damage,&binding,&frontier)==GH_ACTOR_MOTION_UNSUPPORTED&&
        frontier==0x2cba48&&!damage.active&&half(first_bytes+0x54a)==2&&half(second_bytes+0x54a)==1);
    sound_capture sound={.first=&first,.second=&second,.fail_second=1};
    binding.sound=hit_sound;
    binding.sound_context=&sound;
    CHECK(gh_enemy_damage_ema2_near_00292d18(&damage,&binding,&frontier)==GH_ACTOR_MOTION_UNSUPPORTED&&
        frontier==0x374cb8&&damage.cursor==1&&half(first_bytes+0x54a)==1&&half(second_bytes+0x54a)==1);
    CHECK(!gh_enemy_damage_ema2_near_00292d18(&damage,&binding,&frontier)&&damage.complete&&
        half(second_bytes+0x54a)==0&&first_bytes[0x2f4]==1&&!first_bytes[0x2f5]&&
        second_bytes[0x2f4]==2&&second_bytes[0x2f5]==1&&!second_bytes[0x2f6]&&!second_bytes[0x2f7]);
    CHECK(!gh_enemy_damage_ema2_near_00292d18(&damage,&binding,&frontier)&&
        sound.first_calls==1&&sound.second_calls==2&&half(first_bytes+0x54a)==1);
    memcpy(second_bytes+0x54a,&(uint16_t){0xffff},2);
    CHECK(!gh_enemy_find_ema2_near_00292dc8(manager,&source.view,1,elf,elf_bytes,&found,&frontier)&&!found);

    work.room=0x501;
    memcpy(source_bytes+0x600,&(float){1.25f},4);
    memcpy(first_bytes+0x54a,&(uint16_t){20},2);
    first_bytes[0x3ac]=2;
    damage=(gh_enemy_ema2_damage_call){0};
    CHECK(!gh_enemy_damage_ema2_near_00292d18(&damage,&binding,&frontier)&&damage.complete&&
        sound.first_calls==2&&sound.second_calls==2&&half(first_bytes+0x54a)==8&&
        first_bytes[0x2f4]==1&&first_bytes[0x2f5]==1&&first_bytes[0x2f7]==1);
    CHECK(fegetround()==FE_UPWARD&&fetestexcept(FE_ALL_EXCEPT)==FE_DIVBYZERO);
    CHECK(!fesetround(FE_TONEAREST));
    free(manager);free(room_bytes);free(elf);
    puts("PASS actual R100 enemy manager proximity, original distance boundaries and ordered cEma2 damage");
    return 0;
}
