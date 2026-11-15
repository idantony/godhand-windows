#include "game_om382.h"
#include "../../src/rooms/r102_room.h"
#include "game_om_frame.h"
#include "game_actor_model_nodes.h"
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

typedef struct {
 gh_r102_room *room;
 gh_room_module *module;
 gh_heap *heap;
 gh_om_factory *objects;
 uint32_t frontier;
 gh_scene_status status;
 int done;
} room_init;

static void enter(void *opaque,int32_t argument)
{
 room_init *test=opaque;
 (void)argument;
 CHECK(gh_room_module_load_002be708(test->module,0x102)==GH_ROOM_MODULE_OK);
 test->status=gh_r102_room_init_prefix_000001a8(test->room,test->module,test->heap,test->objects,&test->frontier);
 test->done=1;
}

static void preload_item(void *opaque,int32_t argument)
{
 room_init *test=opaque;
 (void)argument;
 test->status=gh_r102_room_preload_00000520(test->room,&test->frontier);
 test->done=1;
}

static gh_object_status no_sound(void *opaque,int32_t slot,uint32_t *frontier)
{
 (void)opaque;
 CHECK(slot==-1&&frontier);
 *frontier=0;
 return GH_OBJECT_OK;
}

typedef struct {
 gh_om_factory *factory;
 unsigned calls;
} om046_probe;

static gh_r100_init_status om_view(void *opaque,uint32_t handle,gh_r100_om_view *out,uint32_t *frontier)
{
 om046_probe *probe=opaque;
 gh_title_player_view actor;
 if(gh_om_factory_object_view(probe->factory,handle,&actor)){
  *frontier=0x14fd98;
  return GH_R100_INIT_CALLEE;
 }
 *out=(gh_r100_om_view){actor.bytes,actor.size,actor.bytes};
 *frontier=0;
 return GH_R100_INIT_OK;
}

static gh_r100_init_status collision(void *opaque,const char *name,int enabled,uint32_t *frontier)
{
 om046_probe *probe=opaque;
 CHECK(!strcmp(name,"aaa209")&&enabled==(probe->calls!=0));
 ++probe->calls;
 *frontier=0;
 return GH_R100_INIT_OK;
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
 uint8_t *resource=load(disc,"om/om63.dat",&resource_size);
 gh_bundle room;
 CHECK(gh_bundle_open(room_data,room_size,&room)==GH_BUNDLE_OK);
 int original;
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
 CHECK(!gh_game_objects_add_pinned_001fece0(global,0x363,resource,resource_size,&original));
 CHECK(!gh_game_objects_set_file_extent(global,0x363,resource,resource_size));
 gh_boot_work work={.room=0x102};
 gh_game_scene_tables scene={0};
 gh_scene_dynamic_owner dynamic={0};
 CHECK(!gh_scene_actors_0012e670(&scene,&room)&&!gh_scene_dynamic_bind(&dynamic,&scene,heap));
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
  .init=hitareas,.shapes=&shapes,.dynamic=&dynamic,.elf=elf,.elf_bytes=(size_t)elf_size};
 gh_dvd_jobs dvd;
 gh_dvd_jobs_init(&dvd,disc);
 gh_task_record32 records[4]={0};
 gh_task_manager tasks={.records=records,.capacity=4,.count=4,.current_index=-1};
 gh_task_threads *threads;
 CHECK(!gh_task_threads_create(&tasks,&threads));
 const gh_object_resource_bindings resources={.dvd=&dvd,.heap=heap,.threads=threads};
 gh_om_factory_bindings bindings={.pools=&pools,.work=&work,.room_state=&room_state,
  .objects=global,.room_objects=local,.factory_serial=&serial,.resources=&resources,.initializer=&init};
 gh_om_factory factory={0};
 CHECK(!gh_om_factory_bind(&factory,&bindings,25));

 gh_room_module module={.input={.dvd=&dvd,.heap=heap,.threads=threads}};
 gh_r102_room prefix={0};
 room_init entry={&prefix,&module,heap,&factory,0,GH_SCENE_ARGUMENT,0};
 int32_t slot;
 CHECK(gh_task_threads_start(threads,1,enter,&entry,0,&slot)==GH_THREAD_OK);
 for(unsigned i=0;!entry.done&&i<12;i++){
  CHECK(gh_tasks_run_matching_002d52a0(&tasks).status==GH_TASK_OK);
  CHECK(gh_dvd_pump(&dvd)==GH_DVD_OK);
 }
 if(entry.status)fprintf(stderr,"prefix status=%u frontier=%08x\n",entry.status,entry.frontier);
 CHECK(entry.done&&!entry.status&&!entry.frontier&&prefix.prefix_complete&&prefix.work.pointer&&prefix.work.owner==heap);
 CHECK(factory.count==1&&serial==1&&prefix.weapon.object==factory.objects);
 gh_om_object *weapon=prefix.weapon.object;
 const gh_om382_state *state=(const gh_om382_state *)weapon->initialization;
 const uint8_t *actor=weapon->view.bytes;
 CHECK(weapon->runtime_created&&weapon->runtime_kind==0x363&&weapon->registered&&weapon->allocation.bytes==0x790);
 CHECK(weapon->resource_manager==global&&weapon->resource.bytes==resource&&word(actor+0x214)==0x42aa98);
 CHECK(word(actor+0x668)==60&&word(actor+0x2f4)==0x00020000u&&state->common.stage==6&&state->hit_owned==3);
 CHECK(word(weapon->view.alias_f0)==0x41c67ae1&&!word(weapon->view.alias_f0+4)&&
       word(weapon->view.alias_f0+8)==0xbfc147ae&&word(actor+0x104)==0x3ff5be0b);
 gh_actor_model *model;
 CHECK(!gh_om_factory_model_at(&factory,weapon->handle,&model)&&model==&state->common.model&&model->ready);
 CHECK(!gh_r102_room_init_prefix_000001a8(&prefix,&module,heap,&factory,&frontier)&&!frontier&&factory.count==1&&serial==1);
 size_t control_size;
 uint8_t *control_data=load(disc,"om/om11.dat",&control_size);
 CHECK(!gh_game_objects_add_pinned_001fece0(local,0x311,control_data,control_size,&original));
 CHECK(!gh_game_objects_set_file_extent(local,0x311,control_data,control_size));
 gh_bundle_entry oms;
 gh_om_scene_placements placements={0};
 gh_om_placement_record record;
 CHECK(!gh_bundle_find(&room,"OMS",0,&oms));
 CHECK(gh_om_scene_bind_001c3cb8(&placements,&oms,&original)==GH_PLACE_UNSUPPORTED);
 CHECK(!gh_om_scene_record_at(&placements,18,&record)&&record.type==0x311);
 CHECK(!gh_om_factory_create_record(&factory,&record,&original)&&original&&factory.count==2);
 const uint8_t *control=factory.objects[1].view.bytes;
 CHECK(!word(control+0x5ec)&&!word(control+0x5f0));
 om046_probe probe={.factory=&factory};
 CHECK(gh_r102_room_prepare_om046_000002b8(&prefix,om_view,collision,&probe,&frontier)==GH_SCENE_UNSUPPORTED);
 CHECK(frontier==0x2f8&&probe.calls==1&&word(control+0x5ec)==1&&word(control+0x5f0)==1);
 CHECK(gh_r102_room_prepare_om046_000002b8(&prefix,om_view,collision,&probe,&frontier)==GH_SCENE_UNSUPPORTED);
 CHECK(frontier==0x2f8&&probe.calls==1&&prefix.weapon.object==weapon);
 uint8_t scenario_bytes[0x114]={0};
 const gh_r102_om046_bindings task={
  .scenario={scenario_bytes,sizeof(scenario_bytes),&tasks},.threads=threads,.room=&room_state,
  .object_view=om_view,.set_collision=collision,.context=&probe
 };
 CHECK(!gh_r102_room_register_om046_00000898(&prefix,&task,&frontier)&&!frontier);
 CHECK(prefix.task_registered&&prefix.task_slot>=0&&!prefix.task_finished&&probe.calls==1);
 CHECK(!gh_r102_room_register_om046_00000898(&prefix,&task,&frontier)&&!frontier);
 CHECK(gh_r102_room_prepare_om046_000002b8(&prefix,om_view,collision,&probe,&frontier)==GH_SCENE_UNSUPPORTED&&frontier==0x348);
 CHECK(gh_tasks_run_matching_002d52a0(&tasks).status==GH_TASK_OK);
 CHECK(!prefix.task_finished&&probe.calls==1);
 CHECK(gh_tasks_run_matching_002d52a0(&tasks).status==GH_TASK_OK);
 CHECK(!prefix.task_finished&&probe.calls==1);
 const uint32_t broken=2u;
 memcpy(factory.objects[1].view.bytes+0x5b0,&broken,sizeof(broken));
 CHECK(gh_tasks_run_matching_002d52a0(&tasks).status==GH_TASK_OK);
 CHECK(prefix.task_finished&&!prefix.task_frontier&&probe.calls==2&&
  !tasks.records[prefix.task_slot].running);
 size_t layer_size;
 uint8_t *layer_data=load(disc,"om/om55.dat",&layer_size);
 CHECK(!gh_game_objects_add_pinned_001fece0(local,0x355,layer_data,layer_size,&original));
 CHECK(!gh_game_objects_set_file_extent(local,0x355,layer_data,layer_size));
 CHECK(!gh_om_scene_record_at(&placements,42,&record)&&record.type==0x355);
 CHECK(!gh_om_factory_create_record(&factory,&record,&original)&&original&&factory.count==3);
 gh_actor_model *layer_model=NULL;
 CHECK(!gh_om_factory_model_at(&factory,factory.objects[2].handle,&layer_model));
 CHECK(layer_model->script.mesh_count<=255);
 uint32_t previous[255],affected=0;
 for(unsigned i=0;i<layer_model->script.mesh_count;++i){
  const gh_actor_node_packets *packet;
  CHECK(!gh_actor_model_packet_at(layer_model,i,&packet));
  previous[i]=word(packet->node+0x380);
  if(packet->node[0x40c]==2){
   ++affected;
   previous[i]&=~1u;
   memcpy(packet->node+0x380,&previous[i],4);
  }
 }
 CHECK(affected);
 CHECK(!gh_r102_room_hide_om095_00000348(&prefix,om_view,&probe,&frontier)&&!frontier);
 CHECK(!gh_r102_room_hide_om095_00000348(&prefix,om_view,&probe,&frontier)&&!frontier);
 for(unsigned i=0;i<layer_model->script.mesh_count;++i){
  const gh_actor_node_packets *packet;
  CHECK(!gh_actor_model_packet_at(layer_model,i,&packet));
  CHECK(word(packet->node+0x380)==(previous[i]|(packet->node[0x40c]==2)));
 }
 CHECK(gh_r102_room_prepare_om046_000002b8(&prefix,om_view,collision,&probe,&frontier)==GH_SCENE_UNSUPPORTED&&frontier==0x370);
 gh_scene_at_manager areas={0};
 CHECK(!gh_scene_at_initialize_units_002c0f90(&areas,&room,heap,0x102,1));
 gh_at_native_unit area11;
 CHECK(!gh_scene_at_find_id_002c3118(&areas,0x0b,&area11)&&area11.bytes&&area11.size>=0x54);
 CHECK(!gh_game_enemy_reset_00290230(hitareas));
 const gh_r102_area11_bindings event={
  .scenario={scenario_bytes,sizeof(scenario_bytes),&tasks},.threads=threads,.room=&room_state,
  .init=hitareas,.areas=&areas
 };
 CHECK(!gh_r102_room_register_area11_00002508(&prefix,&event,&frontier)&&!frontier);
 CHECK(prefix.area11_registered&&prefix.area11_slot>=0);
 CHECK(!gh_r102_room_register_area11_00002508(&prefix,&event,&frontier)&&!frontier);
 CHECK(gh_tasks_run_matching_002d52a0(&tasks).status==GH_TASK_OK);
 CHECK(!prefix.task_frontier&&!(word(room_state.active+4)&0x01000000u)&&
  tasks.records[prefix.area11_slot].running);
 const uint32_t occupied=word(area11.bytes+0x50)|0x80000000u;
 memcpy(area11.bytes+0x50,&occupied,sizeof(occupied));
 CHECK(gh_tasks_run_matching_002d52a0(&tasks).status==GH_TASK_OK);
 CHECK(prefix.task_frontier==0x25f8&&(word(room_state.active+4)&0x01000000u)&&
  !tasks.records[prefix.area11_slot].running);
 CHECK(!gh_scene_at_units_release(&areas));
 size_t mount_size;
 uint8_t *mount_data=load(disc,"om/om53.dat",&mount_size);
 CHECK(!gh_game_objects_add_pinned_001fece0(local,0x353,mount_data,mount_size,&original));
 CHECK(!gh_game_objects_set_file_extent(local,0x353,mount_data,mount_size));
 prefix.om094_complete=1;
 const size_t before_mount=factory.count;
 const uint8_t serial_before_mount=serial;
 CHECK(!gh_r102_room_create_mount_00000488(&prefix,&frontier)&&!frontier);
 CHECK(!gh_r102_room_create_mount_00000488(&prefix,&frontier)&&!frontier);
 gh_om_object *mount=prefix.mount.object;
 CHECK(prefix.mount_complete&&mount&&mount->runtime_kind==0x353&&mount->allocation.bytes==0x630);
 CHECK(factory.count==before_mount+1&&serial==(uint8_t)(serial_before_mount+1));
 const uint8_t *mount_actor=mount->view.bytes;
 CHECK(word(mount_actor+0x214)==0x4275c8&&(word(mount_actor+0x5a0)&1u)&&(word(mount_actor+0x5b8)&8u));
 CHECK(!word(mount_actor+0x620)&&word(mount_actor+0x628)==450);
 for(unsigned i=0;i<16;++i)CHECK(mount_actor[0x600+i]==0xff);
 CHECK(word(mount->view.alias_f0)==0x40870a3d&&word(mount->view.alias_f0+4)==0x41a00000&&
       word(mount->view.alias_f0+8)==0xc299570a);
 CHECK(!word(mount_actor+0x100)&&!word(mount_actor+0x104)&&!word(mount_actor+0x108));
 gh_object_move_request move={.handle=mount->handle,.original_entry=0x1a7880,.actor=mount->view};
 gh_om_frame_state frame={0};
 CHECK(!gh_om_frame_move(&frame,&factory,&move,&work,GH_STATIC_BINARY32_REFERENCE,&frontier)&&!frontier);
 CHECK(frame.complete&&!word(mount_actor+0x2f4));
 CHECK(!gh_om_frame_next_call(&frame));
 mount->view.bytes[0x615]=1;
 CHECK(!gh_om_frame_move(&frame,&factory,&move,&work,GH_STATIC_BINARY32_REFERENCE,&frontier)&&!frontier);
 CHECK(frame.complete&&word(mount_actor+0x2f4)==0x100);
 entry.done=0;
 CHECK(gh_task_threads_start(threads,1,preload_item,&entry,0,&slot)==GH_THREAD_OK);
 for(unsigned i=0;!entry.done&&i<12;++i){
  CHECK(gh_tasks_run_matching_002d52a0(&tasks).status==GH_TASK_OK);
  CHECK(gh_dvd_pump(&dvd)==GH_DVD_OK);
 }
 CHECK(entry.done&&!entry.status&&!entry.frontier&&prefix.item3bc_complete);
 gh_object_view item;
 CHECK(!gh_game_objects_find(global,0x3bc,&item)&&item.use_count==1&&item.file_extent_known);
 size_t item_size;
 uint8_t *item_data=load(disc,"om/ombc.dat",&item_size);
 CHECK(item.file_extent==item_size&&!memcmp(item.bytes,item_data,item_size));
 CHECK(!gh_r102_room_preload_00000520(&prefix,&frontier)&&!frontier);
 CHECK(!gh_game_objects_find(global,0x3bc,&item)&&item.use_count==1);
 free(item_data);
 CHECK(!gh_game_objects_release_nonpinned_001fef30(global,no_sound,NULL,&frontier)&&!frontier);
 CHECK(!gh_r102_room_release_prefix(&prefix)&&!prefix.work.pointer&&weapon->registered);
 CHECK(!gh_room_module_release(&module)&&!gh_task_threads_destroy(threads));
 CHECK(!gh_om_factory_release(&factory)&&gh_heap_used(heap)==baseline);
 CHECK(!pools.vectors[0].count&&!pools.vectors[3].count&&!pools.arenas[2].live_allocations);
 CHECK(!gh_game_object_pool_release_001fdfa8(hitareas,&frontier)&&!gh_collision_pairs_release(&pairs));
 CHECK(!gh_scene_dynamic_release(&dynamic)&&!gh_scene_actors_reset_0012e800(&scene));
 gh_game_objects_destroy(local);
 gh_game_objects_destroy(global);
 CHECK(!gh_game_room_pools_release(&pools)&&!gh_heap_used(heap)&&!gh_heap_destroy(heap));
 free(hitareas);free(resource);free(mount_data);free(control_data);free(layer_data);free(room_data);free(elf);gh_disc_close(disc);
 puts("PASS actual R102 original 0363 and om046 flags, 0898 collision, original AT11, om095 layer, 0353 idle and actual 0520 3BC load");
 return 0;
}
