#include "game_om382.h"
#include "game_actor_external_parent.h"
#include "game_player_factory.h"
#include "game_math_globals.h"
#include "disc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(x) do{if(!(x)){fprintf(stderr,"fixture line %d: %s\n",__LINE__,#x);exit(1);}}while(0)

static uint32_t word(const void *p)
{
    uint32_t value;
    memcpy(&value,p,4);
    return value;
}

static uint8_t *load(gh_disc *disc,const char *name,size_t *bytes)
{
    size_t index;
    gh_disc_entry file;
    CHECK(!gh_disc_find(disc,name,&index)&&!gh_disc_entry_at(disc,index,&file));
    *bytes=(size_t)file.size;
    uint8_t *data=malloc(*bytes);
    CHECK(data&&!gh_disc_read(disc,index,0,data,*bytes));
    return data;
}

int main(int argc,char **argv)
{
    gh_disc *disc;
    CHECK(argc==2&&!gh_disc_open(argv[1],&disc));
    size_t room_bytes, player_bytes, paper_bytes;
    uint8_t *room_data=load(disc,"st1/r105.dat",&room_bytes);
    uint8_t *player_data=load(disc,"pl/pl00.dat",&player_bytes);
    uint8_t *paper_data=load(disc,"om/om76.dat",&paper_bytes);
    CHECK(room_bytes==1120320&&paper_bytes==5600);
    const size_t elf_bytes=(size_t)gh_disc_boot_elf_size(disc);
    uint8_t *elf=malloc(elf_bytes);
    CHECK(elf&&!gh_disc_read_boot_elf(disc,0,elf,elf_bytes));

    gh_bundle room_pack,player_pack;
    gh_bundle_entry md,tm3;
    CHECK(!gh_bundle_open(room_data,room_bytes,&room_pack)&&
          !gh_bundle_open(player_data,player_bytes,&player_pack)&&
          !gh_bundle_find(&player_pack,"MD",0,&md)&&!gh_bundle_find(&player_pack,"TM3",0,&tm3));
    gh_heap *heap;
    gh_room_pools pools={0};
    CHECK(!gh_heap_create(32*1024*1024,&heap)&&!gh_game_room_pools_initialize_002a5d80(&pools,&room_pack,heap));

    gh_boot_work work={.room=0x105,.phase569=2,.canonical_rate574=1.0f};
    gh_game_math_globals math={0};
    CHECK(!gh_game_math_globals_from_elf(&math,elf,elf_bytes)&&!gh_game_math_static_initialize_003079c8(&math,&work,1,0xffff));
    gh_game_init_state *init=malloc(sizeof(*init));
    CHECK(init);
    gh_game_init_state_initialize(init);
    CHECK(!gh_hitarea_objects_construct_001fde00(init,NULL)&&!gh_game_object_pool_reset_001fdea8(init));
    gh_collision_pairs pairs={0};
    CHECK(!gh_collision_pairs_construct_002b8ae0(&pairs,heap)&&!gh_collision_pairs_initialize_002b8b50(&pairs));
    gh_hitarea_shape_bindings shapes={&pairs,&math,GH_STATIC_BINARY32_REFERENCE};
    gh_stage_slots global_slots={0},local_slots={0};
    gh_game_objects *global,*local;
    CHECK(!gh_game_objects_create(&global_slots,0,&global)&&!gh_game_objects_create(&local_slots,1,&local)&&
          !gh_stage_slots_reset(&global_slots,heap)&&!gh_stage_slots_reset(&local_slots,heap));
    int added;
    CHECK(!gh_game_objects_add_pinned_001fece0(global,0x100,player_data,player_bytes,&added)&&added&&
          !gh_game_objects_set_file_extent(global,0x100,player_data,player_bytes));
    CHECK(!gh_game_objects_add_pinned_001fece0(local,0x376,paper_data,paper_bytes,&added)&&added&&
          !gh_game_objects_set_file_extent(local,0x376,paper_data,paper_bytes));

    gh_game_title_player player={0};
    gh_game_player_factory player_factory={0};
    uint8_t save_bytes[GH_CORE_SAVE_BYTES];
    gh_core_save_view save={save_bytes,sizeof(save_bytes)};
    CHECK(!gh_core_save_system_init(save)&&!gh_core_save_game_init(save,&work));
    uint8_t serial=52;
    gh_player_factory_bindings player_bindings={&player,&pools,global,local,&work,disc,save,&serial};
    int original;
    CHECK(!gh_game_player_factory_create_0030fa98(&player_factory,&player_bindings,&original)&&original);
    uint32_t fog=0;
    gh_actor_model player_model={0};
    CHECK(!gh_actor_model_bind_packet_inputs(&player_model,heap,&fog)&&
          !gh_actor_model_initialize_00149738(&player_model,&player.view,&work,&pools,&md,&tm3,0,1,
          GH_STATIC_BINARY32_REFERENCE,&original)&&original);

    gh_game_scene_tables tables={0};
    CHECK(!gh_scene_actors_0012e670(&tables,&room_pack));
    gh_collision_scroll_mutation scroll={0};
    uint8_t room_page[0x1650]={0};
    gh_room_state room={.active=room_page,.size=sizeof(room_page)};
    gh_om_initializer_bindings init_om={.work=&work,.packet_heap=heap,.fog_rgb=&fog,.init=init,.shapes=&shapes};
    gh_om_factory_bindings om_bindings={.pools=&pools,.work=&work,.room_state=&room,
        .objects=global,.room_objects=local,.factory_serial=&serial,.initializer=&init_om};
    gh_om_factory factory={0};
    CHECK(!gh_om_factory_bind(&factory,&om_bindings,4));
    gh_om376_paper call={0};
    uint32_t frontier;
    const gh_scene_placement_status status=gh_om376_set_paper_0012c8d8(&call,&factory,&player_model,
        save,&tables,&scroll,&frontier);
    if(status)fprintf(stderr,"paper status%d frontier%08x stage%u\n",status,frontier,call.stage);
    CHECK(!status&&!frontier&&call.stage==6&&factory.count==1&&serial==54&&save_bytes[0x157]==1);
    const gh_om_object *paper=call.create.object;
    CHECK(paper&&paper->runtime_kind==0x376&&paper->registered&&paper->allocation.bytes==0x790);
    CHECK(paper->view.original_vtable214==0x42aa98&&word(player.view.bytes+0x6a8)==paper->handle);
    const uint8_t *p=paper->view.bytes;
    CHECK(word(p+0x2d0)==0x80000000u&&word(p+0x2f4)==0x100&&word(p+0x604)==2&&word(p+0x668)==100);
    CHECK(!word(p+0x610)&&word(p+0x614)==0x3d408312&&word(p+0x618)==0xbe0f5c29);
    CHECK(player_model.external_borrowers==1&&paper->initialization->model.ready);
    const uint8_t *parent;
    CHECK(!gh_actor_model_external_parent_at(&paper->initialization->model,0,&parent)&&parent==player_model.parts[2].bytes);
    CHECK(!gh_om376_set_paper_0012c8d8(&call,&factory,&player_model,save,&tables,&scroll,&frontier)&&serial==54&&factory.count==1);
    CHECK(!gh_om_factory_release(&factory)&&!player_model.external_borrowers);
    CHECK(!gh_collision_scroll_mutation_release(&scroll));
    CHECK(!gh_actor_model_release(&player_model)&&!gh_game_player_factory_release_prefix(&player_factory));
    CHECK(!gh_hitarea_shapes_abort(init,&frontier)&&!gh_game_object_pool_release_001fdfa8(init,&frontier));
    CHECK(!gh_collision_pairs_release(&pairs));
    gh_game_objects_destroy(local);
    gh_game_objects_destroy(global);
    CHECK(!gh_game_room_pools_release(&pools)&&!gh_heap_used(heap)&&!gh_heap_destroy(heap));
    free(elf);
    free(init);
    free(paper_data);
    free(player_data);
    free(room_data);
    gh_disc_close(disc);
    puts("PASS actual R105 PL00 and OM76 setPaper, original factory, attach part2, same-owner retry and scoped detach");
    return 0;
}
