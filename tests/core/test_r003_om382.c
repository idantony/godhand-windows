#include "game_om382.h"
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
 uint8_t *resource=load(disc,"om/om82.dat",&resource_size);
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
 CHECK(!gh_game_objects_add_pinned_001fece0(local,0x382,resource,resource_size,&original));
 CHECK(!gh_game_objects_set_file_extent(local,0x382,resource,resource_size));
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

 _Alignas(16) uint8_t storage[0x790];
 memset(storage,0xa5,sizeof(storage));
 gh_title_player_view constructed;
 CHECK(gh_om382_construct_001c9140(storage,sizeof(storage)-1,&constructed)==GH_PLACE_BOUNDS&&storage[0]==0xa5);
 CHECK(!gh_om382_construct_001c9140(storage,sizeof(storage),&constructed));
 CHECK(word(storage+0x214)==0x42aa98&&word(storage+0x600)==0xa5a5a5a5);
 CHECK(word(storage+0x61c)==0x3f800000&&word(storage+0x78c)==0x3f800000);
 gh_om_placement_record record;
 CHECK(!gh_om_scene_record_at(&records,24,&record)&&record.type==0x382);
 CHECK(!memcmp(record.bytes+0x30,"om050",6));
 gh_scene_placement_status status=gh_om_factory_create_record(&factory,&record,&original);
 if(status)fprintf(stderr,"row 24 status %u frontier %08x detail %u\n",status,factory.frontier,factory.detail);
 CHECK(!status&&original&&factory.count==1&&serial==1);
 gh_om_object *object=factory.objects;
 gh_om382_state *state=(gh_om382_state*)object->initialization;
 uint8_t *actor=object->view.bytes;
 CHECK(object->allocation.bytes==0x790&&object->registered&&object->stage==7);
 CHECK(word(actor+0x214)==0x42aa98&&word(actor+0x668)==100&&word(actor+0x2f4)==0x1700);
 CHECK(!memcmp(actor+0x490,record.bytes,12)&&!memcmp(object->view.alias_f0,record.bytes,12));
 CHECK(!memcmp(actor+0x100,record.bytes+0x10,12)&&state->hit_owned==3);
 gh_actor_model *model=NULL;
 CHECK(!gh_om_factory_model_at(&factory,object->handle,&model)&&model==&state->common.model&&model->ready);
 for(unsigned i=0;i<2;i++){
  gh_title_player_view *actual_actor=NULL;
  CHECK(!gh_hitarea_object_actor(&state->hit[i],&actual_actor)&&actual_actor==&object->view);
  const uint8_t *node;
  gh_collision_pair_shape shape;
  CHECK(!gh_hitarea_node_at(&state->hit[i],0,&node,&shape));
  CHECK(word(shape.record+0x1f0)==(i?0x3ecccccd:0x3e99999a)&&!(word(shape.record)&2));
  CHECK(state->hit[i].bytes[0x48]==1-i);
 }
 CHECK(word(state->hit[1].bytes+0x4c)==50);

 uint8_t player_data[GH_TITLE_PLAYER_BYTES]={0};
 gh_title_player_view player;
 CHECK(!gh_title_player_construct_00107e40(player_data,sizeof(player_data),&player));
 float player_pos[3];
 memcpy(player_pos,object->view.alias_f0,12);
 player_pos[0]+=3;
 player_pos[1]+=100;
 memcpy(player.alias_f0,player_pos,12);
 gh_game_collision_solids solids={0};
 const float rate=1;
 memcpy(hitareas->enemies+0x548,&rate,4);
 gh_om_frame_state frame={0};
 const gh_om_frame_runtime runtime={.init=hitareas,.canonical_rate=&rate,.solids=&solids};
 const gh_object_opacity_environment environment={.init=hitareas,.current_player=&player,.player_binding_known=1};
 CHECK(!gh_om_frame_bind_runtime(&frame,&runtime));
 gh_object_move_request request={.handle=object->handle,.original_entry=0x1c9860,.actor=object->view};
 gh_object_step_status result=gh_om_frame_move(&frame,&factory,&request,&work,GH_STATIC_BINARY32_REFERENCE,&frontier);
 if(result&&frontier!=0x2db6b0)fprintf(stderr,"first Move status %u pc %08x phase %u/%u\n",result,frontier,frame.phase,frame.special_phase);
 CHECK(result==GH_OBJSTEP_UNSUPPORTED&&frontier==0x2db6b0&&frame.special_phase==3);
 const uint32_t first_sample=word(actor+0x430);
 CHECK(first_sample!=0&&state->motion.complete&&state->clip[0].offset==15040&&actor[0x2f6]==1);
 CHECK(!gh_om_frame_bind_environment(&frame,&environment));
 CHECK(!gh_om_frame_move(&frame,&factory,&request,&work,GH_STATIC_BINARY32_REFERENCE,&frontier));
 CHECK(!frontier&&frame.complete&&word(actor+0x430)==first_sample&&word(actor+0x674)==0x43160000);
 CHECK(!word(state->hit[1].bytes+0x4c)&&!state->hit[1].bytes[0x46]);

 const uint8_t follow_marker=actor[0x531];
 player_pos[0]-=2;
 memcpy(player.alias_f0,player_pos,12);
 for(unsigned tick=0;tick<270&&actor[0x2f4]==0;tick++){
  CHECK(!gh_om_frame_next_call(&frame));
  result=gh_om_frame_move(&frame,&factory,&request,&work,GH_STATIC_BINARY32_REFERENCE,&frontier);
  if(result)fprintf(stderr,"tick %u Move status %u pc %08x phase %u/%u selector %u\n",tick,result,frontier,frame.phase,frame.special_phase,actor[0x2f6]);
  CHECK(!result&&!frontier&&frame.complete);
  if(!tick)CHECK((word(actor+0x568)&0xffff)==1&&word(actor+0x674)==0x43150000);
 }
 CHECK(actor[0x2f4]==1&&!actor[0x2f5]&&!actor[0x2f6]&&state->clip[1].offset==15488);
 CHECK(!word(actor+0x24c)&&actor[0x531]==follow_marker&&(word(actor+0x250)&0x10u));
 CHECK(!gh_om_frame_next_call(&frame));
 CHECK(gh_om_frame_move(&frame,&factory,&request,&work,GH_STATIC_BINARY32_REFERENCE,&frontier)==GH_OBJSTEP_UNSUPPORTED&&frontier==0x2cbe18&&actor[0x531]==0xff);
 CHECK(!gh_om_factory_release(&factory)&&gh_heap_used(heap)==baseline);
 CHECK(!pools.vectors[0].count&&!pools.vectors[3].count&&!pools.arenas[2].live_allocations);
 CHECK(!gh_game_object_pool_release_001fdfa8(hitareas,&frontier)&&!gh_collision_pairs_release(&pairs));
 gh_game_objects_destroy(local);
 gh_game_objects_destroy(global);
 CHECK(!gh_game_room_pools_release(&pools)&&!gh_heap_used(heap)&&!gh_heap_destroy(heap));
 free(hitareas);free(resource);free(room_data);free(elf);gh_disc_close(disc);
 puts("PASS actual R003 0382 world weapon, both sphere volumes and MOT clips, proximity and fade");
 return 0;
}
