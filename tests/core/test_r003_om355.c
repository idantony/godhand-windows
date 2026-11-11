#include "game_om355.h"
#include "game_om_frame.h"
#include "game_scene_dynamic_geometry.h"
#include "game_dynamic_corners.h"
#include <math.h>
#include "disc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(condition) do {if(!(condition)){fprintf(stderr,"line %d: %s\n",__LINE__,#condition);exit(1);}}while(0)
static uint32_t word(const uint8_t *p){uint32_t value;memcpy(&value,p,4);return value;}

static uint8_t *load(gh_disc *disc,const char *path,size_t *size)
{
 size_t index;
 gh_disc_entry entry;
 CHECK(!gh_disc_find(disc,path,&index)&&!gh_disc_entry_at(disc,index,&entry));
 *size=(size_t)entry.size;
 uint8_t *p=malloc(*size);
 CHECK(p&&!gh_disc_read(disc,index,0,p,*size));
 return p;
}

int main(int argc,char **argv)
{
 CHECK(argc==3);
 FILE *file=fopen(argv[2],"rb");
 CHECK(file&&!fseek(file,0,SEEK_END));
 long elf_size=ftell(file);
 CHECK(elf_size>0&&!fseek(file,0,SEEK_SET));
 uint8_t *elf=malloc((size_t)elf_size);
 CHECK(elf&&fread(elf,1,(size_t)elf_size,file)==(size_t)elf_size&&!fclose(file));
 gh_disc *disc;
 CHECK(!gh_disc_open(argv[1],&disc));
 size_t room_size,resource_size;
 uint8_t *room_data=load(disc,"st0/r003.dat",&room_size);
 uint8_t *resource=load(disc,"om/om55.dat",&resource_size);
 gh_bundle room;
 gh_bundle_entry oms;
 CHECK(!gh_bundle_open(room_data,room_size,&room)&&!gh_bundle_find(&room,"OMS",0,&oms));
 gh_om_scene_placements records={0};
 int original;
 CHECK(gh_om_scene_bind_001c3cb8(&records,&oms,&original)==GH_PLACE_UNSUPPORTED);
 gh_game_prepare prepare={0};
 gh_room_state room_state;
 uint8_t start=0;
 CHECK(!gh_game_prepare_002a5d40(&prepare,(gh_core_save_view){0},3,&start));
 CHECK(!gh_room_state_select_002bebc8(&prepare,3,0,&room_state));
 gh_heap *heap;
 gh_room_pools pools={0};
 CHECK(!gh_heap_create(16*1024*1024,&heap)&&!gh_game_room_pools_initialize_002a5d80(&pools,&room,heap));
 gh_stage_slots global_slots={0},room_slots={0};
 gh_game_objects *global,*local;
 CHECK(!gh_game_objects_create(&global_slots,0,&global)&&!gh_game_objects_create(&room_slots,1,&local));
 CHECK(!gh_stage_slots_reset(&global_slots,NULL)&&!gh_stage_slots_reset(&room_slots,NULL));
 CHECK(!gh_game_objects_add_pinned_001fece0(local,0x355,resource,resource_size,&original));
 CHECK(!gh_game_objects_set_file_extent(local,0x355,resource,resource_size));
 gh_boot_work work={.room=3};
 gh_game_scene_tables scene={0};
 CHECK(!gh_scene_actors_0012e670(&scene,&room));
 gh_scene_dynamic_owner dynamic={0};
 CHECK(!gh_scene_dynamic_bind(&dynamic,&scene,heap));
 gh_dynamic_corner_tables corners={0};
 CHECK(!gh_dynamic_corners_from_elf(&corners,elf,(size_t)elf_size));
 CHECK(!gh_dynamic_corners_initialize_00133670(&corners,1,0xffff));
 const size_t baseline=gh_heap_used(heap);
 uint8_t serial=0;
 uint32_t fog=0;
 gh_om_initializer_bindings init={.work=&work,.packet_heap=heap,.fog_rgb=&fog,
    .dynamic=&dynamic,.elf=elf,.elf_bytes=(size_t)elf_size};
 gh_om_factory_bindings bindings={.pools=&pools,.work=&work,.room_state=&room_state,
    .objects=global,.room_objects=local,.factory_serial=&serial,.initializer=&init};
 gh_om_factory factory={0};
 CHECK(!gh_om_factory_bind(&factory,&bindings,25));

 _Alignas(16) uint8_t storage[GH_OM355_BYTES];
 memset(storage,0xa5,sizeof(storage));
 gh_title_player_view constructed;
 CHECK(gh_om355_construct_001a8348(storage,sizeof(storage)-1,&constructed)==GH_PLACE_BOUNDS&&storage[0]==0xa5);
 CHECK(!gh_om355_construct_001a8348(storage,sizeof(storage),&constructed));
 CHECK(word(storage+0x214)==0x4277b8&&!word(storage+0x600)&&word(storage+0x604)==0xa5a5a5a5);
 gh_om_placement_record record;
 CHECK(!gh_om_scene_record_at(&records,23,&record)&&record.type==0x355);
 CHECK(!memcmp(record.bytes+0x30,"om021",6)&&!record.bytes[0x2d]);
 gh_scene_placement_status status=gh_om_factory_create_record(&factory,&record,&original);
 if(status)fprintf(stderr,"row 23 status %u frontier %08x detail %u\n",status,factory.frontier,factory.detail);
 CHECK(!status&&original&&factory.count==1&&serial==1);
 gh_om_object *object=factory.objects;
 gh_om_init_state *state=object->initialization;
 uint8_t *root=object->view.bytes;
 CHECK(object->allocation.bytes==GH_OM355_BYTES&&object->registered&&object->stage==7);
 CHECK(word(root+0x214)==0x4277b8&&!word(root+0x600)&&state->stage==4&&!state->hitarea_created);
 CHECK(!memcmp(root+0x490,record.bytes,12)&&!memcmp(object->view.alias_f0,record.bytes,12));
 CHECK(!memcmp(root+0x100,record.bytes+0x10,12));
 gh_actor_model *model=NULL;
 CHECK(!gh_om_factory_model_at(&factory,object->handle,&model)&&model==&state->model&&model->ready);
 gh_scene_dynamic_box_view box;
 CHECK(state->geometry_state.owner==&dynamic&&state->geometry_state.object==object);
 CHECK(!gh_scene_dynamic_box_view_at(&dynamic,state->geometry_state.box,&box));
 CHECK(!box.world_ready&&box.size==0x190&&box.actor==&object->view&&box.matrix==root+0x80);
 CHECK(scene.pending_actor_count==1&&scene.pending_actor_head==state->geometry_state.box);
 CHECK(word(box.bytes+0x74)==0x3dcccccd&&word(box.bytes+0xa0)==0x3fc00000);
 CHECK(word(box.bytes+0xa4)==0x3e4ccccd&&word(box.bytes+0xa8)==0x3fc00000);
 const uint32_t flags[6]={0x20000,0x40000,0x10000,0x10000,0x10000,0x10000};
 for(unsigned i=0;i<6;i++){
    const uint8_t *attribute=box.bytes+0xb0+i*16;
    CHECK(word(attribute)==flags[i]&&word(attribute+4)==0x10&&word(attribute+8)==0xffff);
 }

 gh_game_init_state *game=calloc(1,sizeof(*game));
 CHECK(game);
 const gh_object_opacity_environment environment={.init=game};
 gh_om_frame_state frame={0};
 CHECK(!gh_om_frame_bind_environment(&frame,&environment));
 gh_object_move_request request={.handle=object->handle,.original_entry=0x1a85b8,.actor=object->view};
 uint32_t pc=0;
 CHECK(!gh_om_frame_move(&frame,&factory,&request,&work,GH_STATIC_BINARY32_REFERENCE,&pc));
 CHECK(!pc&&frame.complete&&!root[0x2f4]&&!(word(root+0x5b8)&0x10u));
 CHECK(!memcmp(root+0xb0,record.bytes,12));
 CHECK(!gh_scene_dynamic_step_0012e840(&dynamic,&corners,&work,GH_STATIC_BINARY32_REFERENCE,&pc));
 CHECK(!pc&&!gh_scene_dynamic_box_view_at(&dynamic,state->geometry_state.box,&box)&&box.world_ready);
 float minimum[3],maximum[3],placement[3];
 memcpy(minimum,box.bytes+0x10,12);
 memcpy(maximum,box.bytes+0x20,12);
 memcpy(placement,record.bytes,12);
 CHECK(fabsf(minimum[0]-(placement[0]-.75f))<0.00001f&&fabsf(maximum[0]-(placement[0]+.75f))<0.00001f);
 CHECK(fabsf(minimum[1]-placement[1])<0.00001f&&fabsf(maximum[1]-(placement[1]+.2f))<0.00001f);
 CHECK(fabsf(minimum[2]-(placement[2]-.75f))<0.00001f&&fabsf(maximum[2]-(placement[2]+.75f))<0.00001f);
 CHECK(!gh_om_frame_next_call(&frame));
 root[0x2f4]=1;
 CHECK(gh_om_frame_move(&frame,&factory,&request,&work,GH_STATIC_BINARY32_REFERENCE,&pc)==GH_OBJSTEP_UNSUPPORTED&&pc==0x1a8688);
 CHECK(!gh_om_factory_release(&factory)&&gh_heap_used(heap)==baseline);
 CHECK(!scene.pending_actor_count&&!scene.pending_actor_head&&!dynamic.allocation_count);
 CHECK(!gh_scene_dynamic_release(&dynamic)&&!gh_scene_actors_reset_0012e800(&scene));
 gh_game_objects_destroy(local);
 gh_game_objects_destroy(global);
 CHECK(!gh_game_room_pools_release(&pools)&&!gh_heap_used(heap)&&!gh_heap_destroy(heap));
 free(game);free(resource);free(room_data);free(elf);gh_disc_close(disc);
 puts("PASS actual R003 0355 model, original dynamic box/face attributes, first Move, world extents and release");
 return 0;
}
