#include "game_om311.h"
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
 uint8_t *room_data=load(disc,"st1/r102.dat",&room_size);
 uint8_t *resource=load(disc,"om/om11.dat",&resource_size);
 gh_bundle room;
 gh_bundle_entry oms;
 CHECK(!gh_bundle_open(room_data,room_size,&room)&&!gh_bundle_find(&room,"OMS",0,&oms));
 gh_om_scene_placements records={0};
 int original;
 CHECK(gh_om_scene_bind_001c3cb8(&records,&oms,&original)==GH_PLACE_UNSUPPORTED);
 gh_game_prepare prepare={0};
 gh_room_state room_state;
 uint8_t start=0;
 CHECK(!gh_game_prepare_002a5d40(&prepare,(gh_core_save_view){0},0x102,&start));
 CHECK(!gh_room_state_select_002bebc8(&prepare,0x102,0,&room_state));
 gh_heap *heap;
 gh_room_pools pools={0};
 CHECK(!gh_heap_create(16*1024*1024,&heap)&&!gh_game_room_pools_initialize_002a5d80(&pools,&room,heap));
 gh_stage_slots global_slots={0},room_slots={0};
 gh_game_objects *global,*local;
 CHECK(!gh_game_objects_create(&global_slots,0,&global)&&!gh_game_objects_create(&room_slots,1,&local));
 CHECK(!gh_stage_slots_reset(&global_slots,NULL)&&!gh_stage_slots_reset(&room_slots,NULL));
 CHECK(!gh_game_objects_add_pinned_001fece0(local,0x311,resource,resource_size,&original));
 CHECK(!gh_game_objects_set_file_extent(local,0x311,resource,resource_size));
 gh_boot_work work={.room=0x102};
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


 gh_om_placement_record record;
 CHECK(!gh_om_scene_record_at(&records,18,&record)&&record.type==0x311);
 CHECK(!memcmp(record.bytes+0x30,"om046",6));
 gh_scene_placement_status status=gh_om_factory_create_record(&factory,&record,&original);
 if(status)fprintf(stderr,"status %d frontier %08x detail %u\n",status,factory.frontier,factory.detail);
 CHECK(!status&&original&&factory.count==1);
 gh_om_object *object=&factory.objects[0];
 gh_om311_state *state=(gh_om311_state*)object->initialization;
 const uint8_t *actor=object->view.bytes;
 CHECK(object->allocation.bytes==GH_OM311_BYTES&&object->registered&&object->stage==7);
 CHECK(object->view.original_vtable214==0x423f58&&word(actor+0x214)==0x423f58);
 CHECK(!memcmp(actor+0x490,record.bytes,12)&&!memcmp(actor+0x100,record.bytes+0x10,12));
 CHECK(word(actor+0xf88)==0x00010001&&word(actor+0xf8c)==0x00010001);
 gh_actor_model *model=NULL;
 CHECK(!gh_om_factory_model_at(&factory,object->handle,&model)&&model==&state->common.model);
 CHECK(model->ready&&model->part_count==GH_OM311_FRAGMENTS);
 for(unsigned i=0;i<20;++i)CHECK((word(model->parts[i].bytes+0x154)&0x18u)==0x18u);
 const uint8_t *node;
 gh_collision_pair_shape shape;
 CHECK(state->anchor_created&&state->anchor_box.group==1);
 CHECK((word(state->anchor_box.bytes+0x46)&0xffff)==0x53&&word(state->anchor_box.bytes+0x4c)==10);
 CHECK(!gh_hitarea_node_at(&state->anchor_box,0,&node,&shape)&&!(word(shape.record)&2));
 CHECK(word(shape.record+0x1f0)==0x40400000&&word(shape.record+0x1f4)==0x3f000000&&
  word(shape.record+0x1f8)==0x41000000);
 for(unsigned i=0;i<4;++i){
  CHECK(state->created[i]&&state->boxes[i].group==2&&(word(state->boxes[i].bytes)&1));
  CHECK(!gh_hitarea_node_at(&state->boxes[i],0,&node,&shape)&&word(shape.record+0xe0)==4);
  CHECK(word(shape.record+0x1f0)==0x3e99999a&&word(shape.record+0x1f4)==0x40000000&&
   word(shape.record+0x1f8)==0x3e99999a);
 }
 gh_om_frame_state frame={0};
 gh_object_move_request request={.handle=object->handle,.original_entry=0x190948,.actor=object->view};
 CHECK(!gh_om_frame_move(&frame,&factory,&request,&work,GH_STATIC_BINARY32_REFERENCE,&frontier));
 CHECK(frame.complete&&!frontier&&!actor[0x2f4]&&word(actor+0xf88)==0x00010001);
 CHECK(!gh_om_frame_next_call(&frame));
 object->view.bytes[0x2f4]=1;
 for(unsigned i=0;i<15;++i){
  CHECK(!gh_om_frame_move(&frame,&factory,&request,&work,GH_STATIC_BINARY32_REFERENCE,&frontier));
  CHECK((word(actor+0xf84)&0xffff)==14-i&&!frontier);
  CHECK(!gh_om_frame_next_call(&frame));
 }
 CHECK(!word(actor+0x2f4));
 CHECK(serial==1&&!gh_om_factory_release(&factory)&&gh_heap_used(heap)==baseline);
 CHECK(!pools.vectors[0].count&&!pools.vectors[3].count&&!pools.arenas[2].live_allocations);
 CHECK(!gh_game_object_pool_release_001fdfa8(hitareas,&frontier)&&!gh_collision_pairs_release(&pairs));
 gh_game_objects_destroy(local);
 gh_game_objects_destroy(global);
 CHECK(!gh_game_room_pools_release(&pools)&&!gh_heap_used(heap)&&!gh_heap_destroy(heap));
 free(hitareas);free(resource);free(room_data);free(elf);gh_disc_close(disc);
 puts("PASS actual R102 0311, 25 parts, five hit boxes, normal Move, 15-tick recovery and scoped release");
 return 0;
}
