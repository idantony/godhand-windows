#include "game_om31a.h"
#include "game_om_frame.h"
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
 uint8_t *resource=load(disc,"om/om1a.dat",&resource_size);
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
 CHECK(!gh_game_objects_add_pinned_001fece0(local,0x31a,resource,resource_size,&original));
 CHECK(!gh_game_objects_set_file_extent(local,0x31a,resource,resource_size));
 gh_boot_work work={.room=3};
 gh_game_math_globals math={0};
 CHECK(!gh_game_math_globals_from_elf(&math,elf,(size_t)elf_size)&&!gh_game_math_static_initialize_003079c8(&math,&work,1,0xffff));
 gh_collision_pairs pairs={0};
 CHECK(!gh_collision_pairs_construct_002b8ae0(&pairs,heap)&&!gh_collision_pairs_initialize_002b8b50(&pairs));
 size_t baseline=gh_heap_used(heap);
 gh_game_init_state *hitareas=malloc(sizeof(*hitareas));
 CHECK(hitareas);
 gh_game_init_state_initialize(hitareas);
 uint32_t frontier;
 CHECK(!gh_hitarea_objects_construct_001fde00(hitareas,&frontier)&&!gh_game_object_pool_reset_001fdea8(hitareas));
 gh_hitarea_shape_bindings shapes={&pairs,&math,GH_STATIC_BINARY32_REFERENCE};
 uint8_t serial=0;
 uint32_t fog=0;
 gh_om_initializer_bindings init={.work=&work,.packet_heap=heap,.fog_rgb=&fog,
  .init=hitareas,.shapes=&shapes,.elf=elf,.elf_bytes=(size_t)elf_size};
 gh_om_factory_bindings bindings={.pools=&pools,.work=&work,.room_state=&room_state,
  .objects=global,.room_objects=local,.factory_serial=&serial,.initializer=&init};
 gh_om_factory factory={0};
 CHECK(!gh_om_factory_bind(&factory,&bindings,25));

 for(unsigned i=0;i<2;i++){
  gh_om_placement_record record;
  CHECK(!gh_om_scene_record_at(&records,18+i,&record)&&record.type==0x31a);
  CHECK(!memcmp(record.bytes+0x30,i?"om007":"om006",6));
  gh_scene_placement_status status=gh_om_factory_create_record(&factory,&record,&original);
  if(status)fprintf(stderr,"row %u status %u frontier %08x detail %u\n",18+i,status,factory.frontier,factory.detail);
  CHECK(!status&&original&&factory.count==i+1);
  gh_om_object *object=&factory.objects[i];
  gh_om303_state *state=(gh_om303_state*)object->initialization;
  const uint8_t *actor=object->view.bytes;
  CHECK(object->allocation.bytes==GH_OM31A_BYTES&&object->registered&&object->stage==7);
  CHECK(object->view.original_vtable214==0x4246a8&&word(actor+0x214)==0x4246a8);
  CHECK(!memcmp(actor+0x490,record.bytes,12)&&!memcmp(object->view.alias_f0,record.bytes,12));
  CHECK(!memcmp(actor+0x100,record.bytes+0x10,12));
  CHECK(state->segments==actor+0x620&&actor[0x60c]==4&&word(actor+0x5ec)==7);
  CHECK(word(actor+0x548)==0x00010001&&(word(actor+0x610)&0xffff)==30);
  gh_actor_model *model=NULL;
  CHECK(!gh_om_factory_model_at(&factory,object->handle,&model)&&model==&state->common.model);
  CHECK(model->part_count==4&&model->ready);
  for(unsigned part=1;part<4;part++)CHECK((word(model->parts[part].bytes+0x154)&0x18)==0x18);
  const uint8_t *node;
  gh_collision_pair_shape box;
  CHECK(state->common.hitarea_created&&state->common.hitarea.group==2);
  CHECK(!gh_hitarea_node_at(&state->common.hitarea,0,&node,&box)&&word(box.record+0xe0)==4);
  CHECK(word(box.record+0x1f0)==0x3f4ccccd&&word(box.record+0x1f4)==0x3fd9999a&&word(box.record+0x1f8)==0x3f4ccccd);
  gh_om_frame_state frame={0};
  gh_object_opacity_environment environment={.init=hitareas};
  CHECK(!gh_om_frame_bind_environment(&frame,&environment));
  gh_object_move_request request={.handle=object->handle,.original_entry=0x1933f8,.actor=object->view};
  CHECK(!gh_om_frame_move(&frame,&factory,&request,&work,GH_STATIC_BINARY32_REFERENCE,&frontier));
  CHECK(frame.complete&&!frontier&&(word(actor+0x610)&0xffff)==29&&!actor[0x2f4]);
  CHECK(!gh_om_frame_next_call(&frame));
  CHECK(!gh_om_frame_move(&frame,&factory,&request,&work,GH_STATIC_BINARY32_REFERENCE,&frontier));
  CHECK(frame.complete&&!frontier&&(word(actor+0x610)&0xffff)==28&&!actor[0x2f4]);
  CHECK(!gh_om_frame_next_call(&frame));
  const uint32_t paused=0x20000000;
  memcpy((uint8_t*)&work+0x5d8,&paused,4);
  CHECK(!gh_om_frame_move(&frame,&factory,&request,&work,GH_STATIC_BINARY32_REFERENCE,&frontier));
  CHECK(frame.complete&&!frontier&&(word(actor+0x610)&0xffff)==28);
  memset((uint8_t*)&work+0x5d8,0,4);
 }
 CHECK(serial==2&&!gh_om_factory_release(&factory)&&gh_heap_used(heap)==baseline);
 CHECK(!pools.vectors[0].count&&!pools.vectors[3].count&&!pools.arenas[2].live_allocations);
 CHECK(!gh_game_object_pool_release_001fdfa8(hitareas,&frontier)&&!gh_collision_pairs_release(&pairs));
 gh_game_objects_destroy(local);
 gh_game_objects_destroy(global);
 CHECK(!gh_game_room_pools_release(&pools)&&!gh_heap_used(heap)&&!gh_heap_destroy(heap));
 free(hitareas);free(resource);free(room_data);free(elf);gh_disc_close(disc);
 puts("PASS actual R003 031A placements, original fragment model and group2 box, ordinary Move countdown");
 return 0;
}
