#include "r105_room.h"
#include "game_om_frame.h"
#include "game_scene_dynamic_geometry.h"
#include "disc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(x) do{if(!(x)){fprintf(stderr,"line %d: %s\n",__LINE__,#x);exit(1);}}while(0)

static uint32_t word(const void *p)
{
    uint32_t value;
    memcpy(&value,p,4);
    return value;
}

static uint8_t *load(gh_disc *disc,const char *path,size_t *size)
{
    gh_disc_entry entry;
    size_t index;
    CHECK(!gh_disc_find(disc,path,&index)&&!gh_disc_entry_at(disc,index,&entry));
    *size=(size_t)entry.size;
    uint8_t *data=malloc(*size);
    CHECK(data&&!gh_disc_read(disc,index,0,data,*size));
    return data;
}

typedef struct {
    gh_om_factory *factory;
    gh_game_scene_tables *tables;
    gh_collision_scroll_mutation collision;
    unsigned visits;
    uint32_t disabled;
} gate_fixture;

static gh_r100_init_status object_by_handle(void *opaque,uint32_t handle,gh_r100_om_view *out,uint32_t *frontier)
{
    gate_fixture *fixture=opaque;
    gh_title_player_view view;
    if(gh_om_factory_object_view(fixture->factory,handle,&view)){
        *frontier=0x14fd98;
        return GH_R100_INIT_BOUNDS;
    }
    *out=(gh_r100_om_view){view.bytes,view.size,view.bytes};
    return GH_R100_INIT_OK;
}

static gh_r100_init_status disable_geometry(void *opaque,const char *name,int enabled,uint32_t *frontier)
{
    gate_fixture *fixture=opaque;
    CHECK(!strcmp(name,"aaa178")&&!enabled);
    if(++fixture->visits==1){
        *frontier=0x1331b8;
        return GH_R100_INIT_CALLEE;
    }
    CHECK(!gh_collision_scroll_set_enabled_001332d0(fixture->tables,&fixture->collision,name,enabled,&fixture->disabled));
    *frontier=0;
    return GH_R100_INIT_OK;
}

int main(int argc,char **argv)
{
    CHECK(argc==3);
    FILE *file=fopen(argv[2],"rb");
    CHECK(file&&!fseek(file,0,SEEK_END));
    const long elf_size=ftell(file);
    CHECK(elf_size>0&&!fseek(file,0,SEEK_SET));
    uint8_t *elf=malloc((size_t)elf_size);
    CHECK(elf&&fread(elf,1,(size_t)elf_size,file)==(size_t)elf_size&&!fclose(file));
    gh_disc *disc;
    CHECK(!gh_disc_open(argv[1],&disc));
    size_t room_size,resource_size;
    uint8_t *room_data=load(disc,"st1/r105.dat",&room_size);
    uint8_t *resource=load(disc,"om/om55.dat",&resource_size);
    gh_bundle room;
    gh_bundle_entry oms;
    CHECK(!gh_bundle_open(room_data,room_size,&room)&&!gh_bundle_find(&room,"OMS",0,&oms));
    gh_om_scene_placements records={0};
    int original;
    CHECK(gh_om_scene_bind_001c3cb8(&records,&oms,&original)==GH_PLACE_UNSUPPORTED);
    gh_om_placement_record record;
    CHECK(!gh_om_scene_record_at(&records,16,&record)&&record.type==0x355);
    CHECK(!memcmp(record.bytes+0x30,"om021",6)&&!record.bytes[0x2d]);

    gh_heap *heap;
    gh_room_pools pools={0};
    CHECK(!gh_heap_create(16*1024*1024,&heap)&&!gh_game_room_pools_initialize_002a5d80(&pools,&room,heap));
    gh_game_scene_tables scene={0};
    gh_scene_dynamic_owner dynamic={0};
    CHECK(!gh_scene_actors_0012e670(&scene,&room)&&!gh_scene_dynamic_bind(&dynamic,&scene,heap));
    gh_scene_at_manager at={0};
    CHECK(!gh_scene_at_initialize_units_002c0f90(&at,&room,heap,0x105,1));
    gh_stage_slots global_slots={0},room_slots={0};
    gh_game_objects *global,*local;
    CHECK(!gh_game_objects_create(&global_slots,0,&global)&&!gh_game_objects_create(&room_slots,1,&local));
    CHECK(!gh_stage_slots_reset(&global_slots,NULL)&&!gh_stage_slots_reset(&room_slots,NULL));
    CHECK(!gh_game_objects_add_pinned_001fece0(local,0x355,resource,resource_size,&original));
    CHECK(!gh_game_objects_set_file_extent(local,0x355,resource,resource_size));
    gh_game_prepare prepare={0};
    gh_room_state room_state;
    uint8_t start=0;
    CHECK(!gh_game_prepare_002a5d40(&prepare,(gh_core_save_view){0},0x105,&start));
    CHECK(!gh_room_state_select_002bebc8(&prepare,0x105,0,&room_state));
    const size_t baseline=gh_heap_used(heap);
    gh_boot_work work={.room=0x105};
    uint8_t serial=0;
    uint32_t fog=0;
    gh_om_initializer_bindings initializer={.work=&work,.packet_heap=heap,.fog_rgb=&fog,.dynamic=&dynamic,
        .elf=elf,.elf_bytes=(size_t)elf_size};
    gh_om_factory_bindings bindings={.pools=&pools,.work=&work,.room_state=&room_state,
        .objects=global,.room_objects=local,.factory_serial=&serial,.initializer=&initializer};
    gh_om_factory factory={0};
    CHECK(!gh_om_factory_bind(&factory,&bindings,1));
    gh_r105_room state={.bindings={.work=&work,.room=&room_state,.at=&at}};
    CHECK(!gh_heap_allocate(heap,0x170,16,&state.work));
    uint8_t *raw=state.work.pointer;
    memset(raw,0,0x170);
    for(unsigned offset=0x50;offset<=0x120;offset+=16){
        const uint32_t w=0x3f800000;
        memcpy(raw+offset+12,&w,4);
    }
    gate_fixture fixture={.factory=&factory,.tables=&scene};
    uint32_t frontier;
    CHECK(!gh_r105_gate_000005f8(&state,&factory,object_by_handle,disable_geometry,&fixture,&frontier));
    CHECK(!frontier&&!state.gate&&!state.gate_stage&&!fixture.visits);
    const gh_scene_placement_status status=gh_om_factory_create_record(&factory,&record,&original);
    if(status)fprintf(stderr,"create=%u frontier=%08x detail=%u\n",status,factory.frontier,factory.detail);
    CHECK(!status&&original&&serial==1);
    const gh_om_object *gate=factory.objects;
    const gh_actor_model *model=&gate->initialization->model;
    uint8_t *actor=gate->view.bytes;
    const uint32_t original_y=word(gate->view.alias_f0+4);
    CHECK(original_y==0xbf69e9c3&&original_y==word(record.bytes+4));
    gh_at_native_unit jump;
    CHECK(!gh_scene_at_get_jump_002c2ae0(&at,1,&jump));
    CHECK(jump.bytes&&jump.handle==2&&!word(jump.bytes+0x3c)&&(jump.bytes[0x34]&1));
    CHECK(gh_r105_gate_000005f8(&state,&factory,object_by_handle,disable_geometry,&fixture,&frontier)==GH_SCENE_CALLEE);
    CHECK(frontier==0x1331b8&&state.gate==gate&&state.gate_stage==3&&fixture.visits==1);
    CHECK(raw[0]==0&&raw[1]==1&&raw[2]==0&&raw[3]==9&&word(raw+0x30)==0x42700000);
    CHECK(word(raw+0x74)==original_y&&word(raw+0x94)==0x40c00000&&word(raw+0xb4)==original_y);
    CHECK(word(raw+0x134)==0x3e4ccccd&&word(raw+0x13c)==0x3e4ccccd&&word(raw+0x140)==0x41000000);
    CHECK(word(raw+0x144)==0x41000000&&word(raw+0x148)==0x40000000&&word(raw+0x14c)==0x3f000000&&word(raw+0x150)==0x40000000);
    const uint32_t raised_y=word(gate->view.alias_f0+4);
    CHECK(raised_y==0x40a2c2c8&&word(raw+0xd4)==raised_y&&word(model->root.bytes+0xb4)==raised_y);
    CHECK((word(actor+0x5a0)&1)&&!memcmp(actor+0x100,record.bytes+0x10,12));
    CHECK(!gh_r105_gate_000005f8(&state,&factory,object_by_handle,disable_geometry,&fixture,&frontier));
    CHECK(!frontier&&state.gate_stage==5&&fixture.visits==2&&word(gate->view.alias_f0+4)==raised_y);
    CHECK(word(jump.bytes+0x3c)==0x768&&jump.bytes[0x44]==0xff&&!(jump.bytes[0x34]&1));
    CHECK(fixture.disabled==9&&scene.actor_links[4].view.bytes==fixture.collision.owned);
    CHECK(!gh_r105_gate_000005f8(&state,&factory,object_by_handle,disable_geometry,&fixture,&frontier));
    CHECK(!frontier&&fixture.visits==2&&word(gate->view.alias_f0+4)==raised_y);
    printf("PASS actual R105 om021 REL05F8 y=%08x->%08x aaa178=%u jump1=%u\n",
        original_y,raised_y,fixture.disabled,jump.handle);
    CHECK(!gh_heap_release(heap,state.work));
    CHECK(!gh_collision_scroll_mutation_release(&fixture.collision));
    CHECK(!gh_om_factory_release(&factory)&&gh_heap_used(heap)==baseline);
    CHECK(!gh_scene_at_units_release(&at)&&!gh_scene_dynamic_release(&dynamic)&&!gh_scene_actors_reset_0012e800(&scene));
    gh_game_objects_destroy(local);
    gh_game_objects_destroy(global);
    CHECK(!gh_game_room_pools_release(&pools)&&!gh_heap_used(heap)&&!gh_heap_destroy(heap));
    free(elf);free(resource);free(room_data);gh_disc_close(disc);
    return 0;
}
