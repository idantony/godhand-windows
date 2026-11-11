#include "game_enemy_weapon.h"
#include "game_actor_external_parent.h"
#include <math.h>
#include "disc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(x) do {if(!(x)){fprintf(stderr,"line %d: %s\n",__LINE__,#x);return 1;}} while(0)

static uint32_t word(const uint8_t *bytes)
{
    uint32_t v;
    memcpy(&v,bytes,4);
    return v;
}

static uint8_t *load(gh_disc *disc,const char *name,size_t *extent)
{
    size_t index;
    gh_disc_entry entry;
    if(gh_disc_find(disc,name,&index)||gh_disc_entry_at(disc,index,&entry))return NULL;
    uint8_t *data=malloc((size_t)entry.size);
    if(!data||gh_disc_read(disc,index,0,data,(size_t)entry.size)){free(data);return NULL;}
    *extent=(size_t)entry.size;
    return data;
}

int main(int argc,char **argv)
{
    CHECK(argc==2);
    gh_disc *disc;
    CHECK(!gh_disc_open(argv[1],&disc));
    size_t room_extent,weapon_extent;
    char path[12];
    CHECK(!gh_game_object_path(0x363,path)&&!strcmp(path,"om/om63.dat"));
    uint8_t *room_bytes=load(disc,"st1/r100.dat",&room_extent);
    uint8_t *weapon_bytes=load(disc,path,&weapon_extent);
    CHECK(room_bytes&&weapon_bytes);
    gh_bundle room,model;
    gh_bundle_entry geometry;
    CHECK(!gh_bundle_open(room_bytes,room_extent,&room));
    CHECK(!gh_bundle_open(weapon_bytes,weapon_extent,&model)&&!gh_bundle_find(&model,"MD",0,&geometry));

    gh_heap *heap;
    gh_room_pools pools={0};
    gh_stage_slots gs={0},rs={0};
    gh_game_objects *global,*local;
    CHECK(!gh_heap_create(64*1024*1024,&heap)&&!gh_game_room_pools_initialize_002a5d80(&pools,&room,heap));
    CHECK(!gh_game_objects_create(&gs,0,&global)&&!gh_game_objects_create(&rs,1,&local));
    CHECK(!gh_stage_slots_reset(&gs,heap)&&!gh_stage_slots_reset(&rs,heap));
    int result;
    CHECK(!gh_game_objects_add_pinned_001fece0(global,0x363,weapon_bytes,weapon_extent,&result)&&result);
    CHECK(!gh_game_objects_set_file_extent(global,0x363,weapon_bytes,weapon_extent));
    gh_boot_work work={.room=0x100};
    gh_game_init_state *init=calloc(1,sizeof(*init));
    CHECK(init);
    gh_game_init_state_initialize(init);
    uint32_t pc;
    CHECK(!gh_hitarea_objects_construct_001fde00(init,&pc));
    size_t elf_bytes=(size_t)gh_disc_boot_elf_size(disc);
    void *elf=malloc(elf_bytes);
    gh_game_math_globals math={0};
    CHECK(elf&&!gh_disc_read_boot_elf(disc,0,elf,elf_bytes));
    CHECK(!gh_game_math_globals_from_elf(&math,elf,elf_bytes)&&!gh_game_math_static_initialize_003079c8(&math,&work,1,0xffff));
    free(elf);
    gh_collision_pairs pairs={0};
    CHECK(!gh_collision_pairs_construct_002b8ae0(&pairs,heap)&&!gh_collision_pairs_initialize_002b8b50(&pairs));
    const gh_hitarea_shape_bindings shapes={&pairs,&math,GH_STATIC_BINARY32_REFERENCE};
    const uint32_t fog=0;
    const size_t baseline=gh_heap_used(heap);
    uint8_t serial=255;
    const gh_enemy_entry_bindings bindings={.pools=&pools,.room_objects=local,.global_objects=global,
        .work=&work,.factory_serial=&serial,.init=init,.packet_heap=heap,.fog_rgb=&fog,
        .policy=GH_STATIC_BINARY32_REFERENCE};
    gh_enemy_weapon weapon={0};
    uint32_t original_flags=0x10000;
    memcpy((uint8_t *)&work+0x594,&original_flags,4);
    CHECK(!gh_enemy_weapon_create_0030fa98(&weapon,&bindings,&result)&&!result&&serial==255);
    CHECK(!weapon.handle&&!pools.arenas[2].live_allocations&&!gh_enemy_weapon_release_factory(&weapon));

    original_flags=0;
    memcpy((uint8_t *)&work+0x594,&original_flags,4);
    CHECK(!gh_enemy_weapon_create_0030fa98(&weapon,&bindings,&result)&&result&&serial==0);
    CHECK(weapon.frontier==0x1c9268&&weapon.resource_owner==global&&weapon.resource.bytes==weapon_bytes);
    CHECK(weapon.resource.file_extent==weapon_extent&&weapon.resource.use_count==-100);
    const uint8_t *bytes=weapon.object.view.bytes;
    CHECK(bytes[0x2fc]==255&&word(bytes+0x214)==0x42aa98&&word(bytes+0x2d0)==0x20000000);
    CHECK(bytes[0x2fe]==0x63&&bytes[0x2ff]==3&&bytes[0x300]==0x63&&bytes[0x301]==3);
    CHECK(word(bytes+0x250)&0x02000000);
    for(unsigned i=0;i<14;i++)CHECK(word(bytes+0x6b0+i*16+12)==0x3f800000);
    CHECK(pools.arenas[2].live_allocations==1);
    void *object=NULL;
    CHECK(!gh_room_object_lookup(&pools,weapon.handle,GH_ROOM_OBJECT_ENEMY_WEAPON,&weapon,&object)&&object==&weapon.object);
    const gh_room_vector *all,*named;
    CHECK(!gh_room_registry_view(&pools,0x754c30,&all)&&all->count==1);
    CHECK(!gh_room_registry_view(&pools,0x754c58,&named)&&named->count==1);
    const uint32_t handle=weapon.handle;
    CHECK(!gh_enemy_weapon_create_0030fa98(&weapon,&bindings,&result)&&result&&serial==0&&weapon.handle==handle);
    gh_enemy_weapon_init state={0};
    gh_actor_model_status init_status=gh_enemy_weapon_initialize_001c9268(&state,&weapon,&shapes,&result);
    if(init_status)fprintf(stderr,"virtual status %d frontier %08x stage %u\n",init_status,state.frontier,state.stage);
    CHECK(!init_status&&result==1&&state.model.ready&&state.model.part_count>0);
    CHECK(word(bytes+0x668)==60&&word(bytes+0x2f4)==0x00020000);
    CHECK(state.hit[0].index!=state.hit[1].index&&state.hit_owned==3);
    CHECK(state.hit[0].bytes[0x48]==1&&state.hit[1].bytes[0x48]==0);
    CHECK(word(state.hit[1].bytes+0x4c)==50&&state.hit[1].bytes[0x46]==0x2f);
    CHECK(word(state.hit[1].bytes+0x40)==UINT32_MAX&&!memcmp(state.hit[1].bytes+0x10,math.zero_vector,12));
    for(unsigned i=0;i<2;i++){
        const uint8_t *record;
        gh_collision_pair_shape sphere;
        CHECK(!gh_hitarea_node_at(&state.hit[i],0,&record,&sphere));
        CHECK(word(sphere.record+0x1f0)==(i?0x3ecccccd:0x3e99999a));
        CHECK(!(word(sphere.record)&2));
    }
    size_t initialized_bytes=gh_heap_used(heap);
    CHECK(!gh_enemy_weapon_initialize_001c9268(&state,&weapon,&shapes,&result)&&result==1&&gh_heap_used(heap)==initialized_bytes);
    size_t parent_extent;
    uint8_t *parent_resource=load(disc,"em/em02.dat",&parent_extent);
    CHECK(parent_resource);
    gh_game_title_player parent={0};
    CHECK(!gh_enemy202_allocate_003130e0(&parent,heap,&work,&result)&&result);
    memcpy(parent.view.bytes+0x2fe,&(uint16_t){0x202},2);
    memcpy(parent.view.bytes+0x564,&(uint32_t){0x202},4);
    gh_object_view source={.bytes=parent_resource,.kind=0x202,.size=parent_extent,
        .file_extent=parent_extent,.file_extent_known=1};
    gh_enemy_model parent_state={0};
    CHECK(!gh_enemy_model_initialize_00208268(&parent_state,&parent.view,&source,&work,&pools,
        heap,&fog,GH_STATIC_BINARY32_REFERENCE,&result)&&result);
    const float actor_position[3]={1,-2,3};
    memcpy(parent.view.alias_f0,actor_position,12);
    CHECK(!gh_actor_model_update_hierarchy_00149eb8(&parent_state.model,GH_STATIC_BINARY32_REFERENCE));
    gh_game_scene_tables actors={0};
    gh_collision_scroll_mutation scroll={0};
    CHECK(!gh_scene_actors_0012e670(&actors,&room));
    init_status=gh_enemy_weapon_attach_0026ab20(&state,&parent.view,&parent_state.model,&actors,&scroll);
    if(init_status)fprintf(stderr,"attach status %d frontier %08x stage %u\n",init_status,state.frontier,state.attach_stage);
    CHECK(!init_status&&state.attach_stage==5&&parent_state.model.external_borrowers==1);
    const uint8_t *linked;
    CHECK(!gh_actor_model_external_parent_at(&state.model,0,&linked)&&linked==parent_state.model.parts[16].bytes);
    CHECK(!memcmp(bytes+0x490,actor_position,12)&&word(bytes+0x604)==16&&word(bytes+0x2f4)==0x100);
    const uint32_t expected_offset[]={0x3e99999a,0xbd8f5c29,0x3f19999a};
    CHECK(!memcmp(bytes+0x610,expected_offset,12)&&word(bytes+0x624)==0x3eb2b8c2);
    float matrix[16],offset[3],position[4];
    memcpy(matrix,linked+0x80,64);
    memcpy(offset,expected_offset,12);
    memcpy(position,weapon.object.view.alias_f0,16);
    for(unsigned i=0;i<3;i++){
        double expected=(double)matrix[i]*offset[0]+(double)matrix[4+i]*offset[1]+
            (double)matrix[8+i]*offset[2]+matrix[12+i];
        CHECK(fabs(position[i]-expected)<1e-5);
    }
    CHECK(!gh_enemy_weapon_attach_0026ab20(&state,&parent.view,&parent_state.model,&actors,&scroll));
    CHECK(parent_state.model.external_borrowers==1&&gh_enemy_model_release_prefix(&parent_state)==GH_ACTOR_MODEL_BUSY);
    CHECK(!gh_enemy_weapon_release_init(&state)&&!parent_state.model.external_borrowers);
    CHECK(!gh_enemy_model_release_prefix(&parent_state)&&!gh_game_title_player_release(&parent));
    CHECK(!gh_collision_scroll_mutation_release(&scroll));
    free(parent_resource);
    CHECK(gh_heap_used(heap)==baseline);
    CHECK(!gh_collision_pair_shapes_have_live(&pairs));
    CHECK(!gh_enemy_weapon_release_factory(&weapon)&&!weapon.handle&&!all->count&&!named->count);
    CHECK(!pools.arenas[2].live_allocations);
    gh_object_view view;
    CHECK(!gh_game_objects_find(global,0x363,&view)&&view.use_count==-100&&view.bytes==weapon_bytes);

    CHECK(!gh_game_objects_add_pinned_001fece0(local,0x363,weapon_bytes,weapon_extent,&result)&&result);
    CHECK(!gh_game_objects_set_file_extent(local,0x363,weapon_bytes,weapon_extent));
    CHECK(!gh_enemy_weapon_create_0030fa98(&weapon,&bindings,&result)&&result&&serial==1);
    CHECK(weapon.resource_owner==local&&!(word(weapon.object.view.bytes+0x250)&0x02000000));
    CHECK(!gh_enemy_weapon_release_factory(&weapon)&&!all->count&&!named->count);
    CHECK(!gh_game_objects_find(local,0x363,&view)&&view.use_count==-100&&view.bytes==weapon_bytes);

    CHECK(!gh_game_object_pool_release_001fdfa8(init,&pc)&&!gh_collision_pairs_release(&pairs));
    free(init);
    gh_game_objects_destroy(local);
    gh_game_objects_destroy(global);
    CHECK(!gh_game_room_pools_release(&pools)&&!gh_heap_used(heap)&&!gh_heap_destroy(heap));
    gh_disc_close(disc);
    free(weapon_bytes);free(room_bytes);
    puts("PASS actual R100 arena and startup weapon 363, factory gates, original stamps, dual registry, virtual Init, HitAreas, typed hand attachment and scoped release");
    return 0;
}
