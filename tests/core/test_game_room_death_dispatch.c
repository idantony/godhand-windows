#include "game_object_live_trans.h"
#include "game_object_sound.h"
#include "game_scene_exit_leaves.h"
#include "game_room_death_dispatch.h"
#include "game_om303.h"
#include "game_om307.h"
#include "game_om310.h"
#include "game_om322.h"
#include "game_player_init_tail.h"
#include "game_player_node_cache.h"
#include "game_player_post_state.h"
#include "game_player_child_tail.h"
#include "game_world_light.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define C(x) do{if(!(x)){fprintf(stderr,"FAIL %d %s\n",__LINE__,#x);exit(1);}}while(0)
static uint32_t w(const uint8_t*p){uint32_t v;memcpy(&v,p,4);return v;}
static uint8_t*load(gh_disc*d,const char*path,size_t*n){size_t i;gh_disc_entry e;C(!gh_disc_find(d,path,&i)&&!gh_disc_entry_at(d,i,&e));*n=(size_t)e.size;uint8_t*p=malloc(*n);C(p&&!gh_disc_read(d,i,0,p,*n));return p;}
typedef struct {gh_actor_model model;gh_game_player_factory*factory;gh_heap*heap;gh_game_world_light*light;gh_player_shared_vector vector;gh_player_child_tail child_tail;gh_game_player_child*publication;gh_core_save_view save;uint8_t*elf;size_t elf_size;} context;
static gh_player_init_status model_init(void*raw,gh_game_title_player*player,gh_boot_work*work,const gh_bundle_entry*md,const gh_bundle_entry*tm,uint32_t mode,int*original,uint32_t*frontier){context*c=raw;gh_actor_model_status e=gh_actor_model_initialize_00149738(&c->model,&player->view,work,c->factory->b.pools,md,tm,0,mode,GH_STATIC_BINARY32_REFERENCE,original);*frontier=c->model.frontier;return e?GH_PLAYER_INIT_UNSUPPORTED:GH_PLAYER_INIT_OK;}
static gh_title_player_status child_init(void*raw,gh_game_player_child*child,int*original,uint32_t*frontier){context*c=raw;if(!child->model.bound)C(!gh_actor_model_bind_packet_inputs(&child->model,c->heap,c->light->scalar+4));gh_title_player_status e=gh_game_player_child_initialize_00100378(child,original);if(e==GH_PLAYER_UNSUPPORTED&&child->frontier==0x100420)e=gh_game_player_child_finish_00100420(&c->child_tail,child,&c->vector,&c->publication,c->save,c->elf,c->elf_size,GH_STATIC_BINARY32_REFERENCE,original);*frontier=e?(c->child_tail.frontier?c->child_tail.frontier:child->frontier):0;return e;}
typedef struct {
 gh_om300_retire_bindings bindings;
 gh_om300_full_retirement state;
 gh_room_pools *pools;
 uint32_t selected;
 unsigned invoked;
} live_retire;
static gh_object_step_status live_resolve(void *opaque,uint32_t handle,
 gh_title_player_view *out,uint32_t *frontier)
{
 live_retire *fixture=opaque;
 *frontier=0;
 return gh_om_factory_object_view(fixture->bindings.factory,handle,out)?GH_OBJSTEP_CALLEE:GH_OBJSTEP_OK;
}
static gh_object_step_status live_destroy(void *opaque,const gh_object_trans_request *request,
 const gh_boot_work *work,uint32_t *frontier)
{
 (void)work;
 live_retire *fixture=opaque;
 if(request->handle!=fixture->selected||request->kind!=GH_OBJECT_TRANS_DESTROY||
    request->original_entry!=0x198ce0||request->original_vtable!=0x425000||
    request->this_delta||request->argument!=3)return GH_OBJSTEP_ARGUMENT;
 ++fixture->invoked;
 gh_room_pool_status status=gh_om324_destroy_detached_00198ce0(&fixture->state,
   &fixture->bindings,request->handle,&request->actor);
 *frontier=fixture->state.frontier;
 return status?GH_OBJSTEP_CALLEE:GH_OBJSTEP_OK;
}

typedef struct {
 gh_om_factory *factory;
 gh_om_runtime_create request;
 gh_om_object *object;
 gh_scene_placement_status status;
 int finished;
} tutorial_load;

static void load_tutorial(void *context,int32_t arg)
{
 (void)arg;
 tutorial_load *load=context;
 load->status=gh_om_factory_create_0030fa98(load->factory,&load->request,0x3eb,0xffff,&load->object);
 load->finished=1;
}

int main(int argc,char**argv){C(argc==3);context c={0};FILE*f=fopen(argv[2],"rb");C(f&&!fseek(f,0,SEEK_END));long size=ftell(f);C(size>0&&!fseek(f,0,SEEK_SET));c.elf_size=(size_t)size;c.elf=malloc(c.elf_size);C(c.elf&&fread(c.elf,1,c.elf_size,f)==c.elf_size);fclose(f);C(!gh_player_shared_vector_from_elf(&c.vector,c.elf,c.elf_size));gh_player_node_names names={0};C(!gh_player_node_names_from_elf(&names,c.elf,c.elf_size)&&names.initialized);gh_game_math_globals math={0};gh_boot_work work={.room=0x100};C(!gh_game_math_globals_from_elf(&math,c.elf,c.elf_size)&&!gh_game_math_static_initialize_003079c8(&math,&work,1,0xffff));gh_game_init_state*init=malloc(sizeof(*init));C(init);gh_game_init_state_initialize(init);C(!gh_hitarea_objects_construct_001fde00(init,NULL)&&!gh_game_object_pool_reset_001fdea8(init));uint8_t save_bytes[GH_CORE_SAVE_BYTES];c.save=(gh_core_save_view){save_bytes,sizeof(save_bytes)};C(!gh_core_save_system_init(c.save)&&!gh_core_save_game_init(c.save,&work));gh_disc*d;C(!gh_disc_open(argv[1],&d));size_t room_size,pl_size,child_size;uint8_t*room=load(d,"st1/r100.dat",&room_size),*pl=load(d,"pl/pl00.dat",&pl_size),*hair=load(d,"pl/pl01.dat",&child_size);gh_bundle bundle;C(!gh_bundle_open(room,room_size,&bundle));gh_room_pools pools={0};C(!gh_heap_create(32*1024*1024,&c.heap)&&!gh_game_room_pools_initialize_002a5d80(&pools,&bundle,c.heap));gh_game_world_light*light=calloc(1,sizeof(*light));C(light);c.light=light;C(!gh_game_world_light_construct_002d6d80(light)&&!gh_game_world_light_initialize_with_heap_002d6f40(light,&bundle,c.heap));gh_stage_slots gs={0},rs={0};gh_game_objects*global,*objects;C(!gh_game_objects_create(&gs,0,&global)&&!gh_game_objects_create(&rs,1,&objects)&&!gh_stage_slots_reset(&gs,NULL)&&!gh_stage_slots_reset(&rs,NULL));int added;C(!gh_game_objects_add_pinned_001fece0(global,0x100,pl,pl_size,&added)&&!gh_game_objects_set_file_extent(global,0x100,pl,pl_size)&&!gh_game_objects_add_pinned_001fece0(global,0x101,hair,child_size,&added)&&!gh_game_objects_set_file_extent(global,0x101,hair,child_size));uint8_t serial=33;
 gh_game_scene_tables tables={0};C(!gh_scene_actors_0012e670(&tables,&bundle));
 gh_scene_meshes meshes={0};gh_stage_textures *textures;
 C(!gh_stage_textures_create(&textures));
 gh_scene_mesh_construction_bindings construction={textures,light,c.elf,c.elf_size};
 meshes.factory_serial=&serial;
 C(!gh_scene_meshes_bind_construction(&meshes,&construction)&&
   !gh_scene_meshes_construct_0031d1b0(&meshes,&tables,&work,&bundle,&pools,c.heap)&&
   meshes.constructed_count==156&&serial==189);
gh_game_title_player player={0};gh_game_player_factory factory={0};c.factory=&factory;gh_player_factory_bindings fb={&player,&pools,global,objects,&work,d,c.save,&serial};int original;C(!gh_game_player_factory_create_0030fa98(&factory,&fb,&original)&&original&&serial==190);C(!gh_actor_model_bind_packet_inputs(&c.model,c.heap,light->scalar+4));gh_game_player_init prefix={0};gh_player_init_bindings pb={.factory=&factory,.work=&work,.current_player=&player,.model_initialize_00149738=model_init,.model_context=&c};C(gh_game_player_initialize_00107f58(&prefix,&pb)==GH_PLAYER_INIT_UNSUPPORTED&&prefix.frontier==0x30fa98&&prefix.stage==4&&prefix.model_call_complete&&c.model.ready&&c.model.parts_allocated==32);
 gh_collision_pairs pairs={0};C(!gh_collision_pairs_construct_002b8ae0(&pairs,c.heap)&&!gh_collision_pairs_initialize_002b8b50(&pairs));gh_game_collision_solids solids={0};gh_game_player_child child={0};gh_player_child_attachment attachment={0};gh_game_player_init_tail tail={0};gh_player_init_tail_bindings tb={.prefix=&prefix,.parent_model=&c.model,.child=&child,.attachment=&attachment,.solids=&solids,.default_heap=c.heap,.save=c.save,.initialize_child=child_init,.child_context=&c,.init=init,.pairs=&pairs,.math_globals=&math,.shape_policy=GH_STATIC_BINARY32_REFERENCE,.node_names=&names};gh_player_init_status status=gh_game_player_initialize_tail_00107f58(&tail,&tb);fprintf(stderr,"actualtail status%d stage%u frontier%08X take%u give%u serial%u\n",status,tail.stage,tail.frontier,tail.take_step,tail.give_step,serial);C(status==GH_PLAYER_INIT_UNSUPPORTED&&tail.frontier==0x2a8578&&tail.stage==13&&tail.node_cache.ready&&tail.motion_entry.bytes==pl+w(pl+0x28)&&tail.motion_entry.size>0&&tail.take_step==4&&tail.give_step==5&&!tail.pending_lookup_name&&tail.active_child&&attachment.attached&&c.publication==&child&&c.child_tail.complete&&c.child_tail.springs.initialized&&serial==191&&tail.take_object.index==160&&tail.solid_unit&&tail.sphere&&gh_collision_pair_shapes_have_live(&pairs));for(unsigned i=0;i<4;i++){C(tail.take_sphere_indices[i]==(int)i&&w(player.view.bytes+0x67c+i*4)==i);const uint8_t*node;gh_collision_pair_shape shape;C(!gh_hitarea_node_at(&tail.take_object,i,&node,&shape)&&w(shape.record)==3&&w(shape.record+0x10)==2&&w(shape.record+0x1f0)==0x3e800000);}for(unsigned i=0;i<5;i++){C(tail.give_objects[i].index==i&&tail.give_shape_indices[i]==(int)i&&w(player.view.bytes+0x5b4+i*8)==i);const uint8_t*node;gh_collision_pair_shape shape;C(!gh_hitarea_node_at(&tail.give_objects[i],0,&node,&shape)&&w(shape.record)==1&&w(shape.record+0x10)==1&&w(shape.record+0xe0)==(i<4?2u:4u));}

 size_t om_size,om303_size,om304_size,om305_size,om307_size,om30a_size,om30e_size,om310_size,om320_size,om322_size,om323_size,om324_size,om33f_size,om346_size,om354_size,om35d_size,om309_size,omeb_size;
 uint8_t *om=load(d,"om/om00.dat",&om_size),*om303=load(d,"om/om03.dat",&om303_size),
  *om304=load(d,"om/om04.dat",&om304_size),*om305=load(d,"om/om05.dat",&om305_size),
  *om307=load(d,"om/om07.dat",&om307_size),*om30a=load(d,"om/om0a.dat",&om30a_size),
  *om30e=load(d,"om/om0e.dat",&om30e_size),*om310=load(d,"om/om10.dat",&om310_size),
  *om320=load(d,"om/om20.dat",&om320_size),*om322=load(d,"om/om22.dat",&om322_size),
  *om323=load(d,"om/om23.dat",&om323_size),*om324=load(d,"om/om24.dat",&om324_size),
  *om33f=load(d,"om/om3f.dat",&om33f_size),
  *om346=load(d,"om/om46.dat",&om346_size),
  *om354=load(d,"om/om54.dat",&om354_size),
  *om35d=load(d,"om/om5d.dat",&om35d_size),
  *om309=load(d,"om/om09.dat",&om309_size),*omeb=load(d,"om/omeb.dat",&omeb_size);
 C(!gh_game_objects_add_pinned_001fece0(global,0x300,om,om_size,&added)&&
   !gh_game_objects_set_file_extent(global,0x300,om,om_size)&&
   !gh_game_objects_add_pinned_001fece0(global,0x303,om303,om303_size,&added)&&
   !gh_game_objects_set_file_extent(global,0x303,om303,om303_size)&&
   !gh_game_objects_add_pinned_001fece0(global,0x304,om304,om304_size,&added)&&
   !gh_game_objects_set_file_extent(global,0x304,om304,om304_size)&&
   !gh_game_objects_add_pinned_001fece0(global,0x305,om305,om305_size,&added)&&
   !gh_game_objects_set_file_extent(global,0x305,om305,om305_size)&&
   !gh_game_objects_add_pinned_001fece0(global,0x307,om307,om307_size,&added)&&
   !gh_game_objects_set_file_extent(global,0x307,om307,om307_size)&&
   !gh_game_objects_add_pinned_001fece0(global,0x30a,om30a,om30a_size,&added)&&
   !gh_game_objects_set_file_extent(global,0x30a,om30a,om30a_size)&&
   !gh_game_objects_add_pinned_001fece0(global,0x30e,om30e,om30e_size,&added)&&
   !gh_game_objects_set_file_extent(global,0x30e,om30e,om30e_size)&&
   !gh_game_objects_add_pinned_001fece0(global,0x310,om310,om310_size,&added)&&
   !gh_game_objects_set_file_extent(global,0x310,om310,om310_size)&&
   !gh_game_objects_add_pinned_001fece0(global,0x320,om320,om320_size,&added)&&
   !gh_game_objects_set_file_extent(global,0x320,om320,om320_size)&&
   !gh_game_objects_add_pinned_001fece0(global,0x322,om322,om322_size,&added)&&
   !gh_game_objects_set_file_extent(global,0x322,om322,om322_size)&&
   !gh_game_objects_add_pinned_001fece0(global,0x323,om323,om323_size,&added)&&
   !gh_game_objects_set_file_extent(global,0x323,om323,om323_size)&&
   !gh_game_objects_add_pinned_001fece0(global,0x324,om324,om324_size,&added)&&
   !gh_game_objects_set_file_extent(global,0x324,om324,om324_size)&&
   !gh_game_objects_add_pinned_001fece0(global,0x33f,om33f,om33f_size,&added)&&
   !gh_game_objects_set_file_extent(global,0x33f,om33f,om33f_size)&&
   !gh_game_objects_add_pinned_001fece0(global,0x346,om346,om346_size,&added)&&
   !gh_game_objects_set_file_extent(global,0x346,om346,om346_size)&&
   !gh_game_objects_add_pinned_001fece0(global,0x354,om354,om354_size,&added)&&
   !gh_game_objects_set_file_extent(global,0x354,om354,om354_size)&&
   !gh_game_objects_add_pinned_001fece0(global,0x35d,om35d,om35d_size,&added)&&
   !gh_game_objects_set_file_extent(global,0x35d,om35d,om35d_size)&&
   !gh_game_objects_add_pinned_001fece0(global,0x309,om309,om309_size,&added)&&
   !gh_game_objects_set_file_extent(global,0x309,om309,om309_size));
 gh_bundle_entry oms;
 gh_om_scene_placements placement={0};
 C(!gh_bundle_find(&bundle,"OMS",0,&oms)&&gh_om_scene_bind_001c3cb8(&placement,&oms,&original)==GH_PLACE_UNSUPPORTED);
 gh_game_prepare prepare={0};
 uint8_t main_stage=0;
 gh_room_state room_state;
 C(!gh_game_prepare_002a5d40(&prepare,(gh_core_save_view){0},0x100,&main_stage)&&
   !gh_room_state_select_002bebc8(&prepare,0x100,0,&room_state));
 gh_scene_dynamic_owner dynamic={0};
 gh_om_door_shared shared={0};
 C(!gh_scene_dynamic_bind(&dynamic,&tables,c.heap)&&!gh_om_door_shared_from_elf(&shared,c.elf,c.elf_size));
 gh_hitarea_shape_bindings shapes={&pairs,&math,GH_STATIC_BINARY32_REFERENCE};
 gh_om_initializer_bindings om_init={.work=&work,.packet_heap=c.heap,.fog_rgb=light->scalar+4,
   .init=init,.shapes=&shapes,.dynamic=&dynamic,.door_shared=&shared,.solids=&solids,.player=&player.view};
 gh_dvd_jobs dvd;
 gh_dvd_jobs_init(&dvd,d);
 gh_task_record32 records[4]={0};
 gh_task_manager manager={.records=records,.capacity=4,.count=4,.current_index=-1};
 gh_task_threads *threads;
 C(!gh_task_threads_create(&manager,&threads));
 gh_object_resource_bindings resources={.dvd=&dvd,.heap=c.heap,.threads=threads};
 gh_om_factory doors={0};
 gh_om_factory_bindings om_bindings={.pools=&pools,.work=&work,.room_state=&room_state,.objects=global,
   .room_objects=objects,.factory_serial=&serial,.resources=&resources,.initializer=&om_init};
 C(!gh_om_factory_bind(&doors,&om_bindings,73));
 for(unsigned i=0;i<3;++i){
  gh_om_placement_record record;
  C(!gh_om_scene_record_at(&placement,i,&record)&&record.type==0x300&&
    !gh_om_factory_create_record(&doors,&record,&original)&&original);
 }
 C(pools.vectors[0].count==161&&pools.vectors[1].count==156&&pools.vectors[3].count==5);
 C(w(pools.vectors[0].allocation.pointer)==meshes.first.handle&&
   w((uint8_t*)pools.vectors[0].allocation.pointer+156*4)==factory.handle&&
   w((uint8_t*)pools.vectors[0].allocation.pointer+157*4)==child.handle&&
   w((uint8_t*)pools.vectors[0].allocation.pointer+158*4)==doors.objects[0].handle);
 gh_audio_registry *registry;
 gh_se_voice_engine *voices;
 gh_se_mix mix;
 max_align_t output_identity={0};
 C(!gh_audio_registry_create(&registry)&&!gh_se_mix_boot_defaults_002c9e78(&mix,0,GH_SE_DSP_NATIVE_LINEAR_REFERENCE)&&
   !gh_se_voice_create(registry,(gh_audio_output *)&output_identity,&mix,&voices));
 gh_collision_scroll_mutation scroll={0};
 gh_game_effects *effects=calloc(1,sizeof(*effects));
 C(effects);
 gh_sound_control control={.initialized=1,.recent_initialized=1};
 control.recent[10].bank=7;
 gh_object_sound_binding sound={.control=&control,.voices=voices};
 gh_room_death_bindings bindings={.scroll_meshes=&meshes,
   .player={&factory,&c.model,&solids,&attachment,&resources},.child=&child,.child_tail=&c.child_tail,
   .door={.factory=&doors,.tables=&tables,.scroll=&scroll,.solids=&solids,
     .release_sound_slot=gh_sound_object_slot_release_002cb340,.sound_context=&sound},
   .hitarea=init,.pairs=&pairs,.dynamic=&dynamic,.effects=effects,.voices=voices};
 gh_room_death_dispatch dispatch={0};
 const uint8_t *box_record[5],*sphere_record[5];
 gh_collision_pair_shape box[5],sphere[5];
 for(unsigned i=0;i<5;++i){
  gh_om_placement_record fragment;
  C(!gh_om_scene_record_at(&placement,3+i,&fragment)&&fragment.type==0x303&&
    !gh_om_factory_create_record(&doors,&fragment,&original)&&original);
  gh_om303_state *state=(gh_om303_state *)doors.objects[3+i].initialization;
  C(state->sphere_created&&state->common.hitarea_created&&
    !gh_hitarea_node_at(&state->common.hitarea,0,&box_record[i],&box[i])&&
    !gh_hitarea_node_at(&state->sphere_unit,0,&sphere_record[i],&sphere[i]));
 }
 gh_om_placement_record next;
 unsigned next_row=8;
 for(;next_row<69;++next_row){
  C(!gh_om_scene_record_at(&placement,next_row,&next));
  if(!gh_om_retire_entry(next.type))break;
  C(!gh_om_factory_create_record(&doors,&next,&original)&&original);
 }
 C(next_row==69&&doors.objects[59].source.type==0x33f);
 tutorial_load first={.factory=&doors};
 int32_t slot;
 C(!gh_task_threads_start(threads,2,load_tutorial,&first,0,&slot));
 for(unsigned tick=0;tick<16&&!first.finished;++tick)
  C(!gh_tasks_run_matching_002d52a0(&manager).status&&!gh_dvd_pump(&dvd));
 C(first.finished&&!first.status&&first.object&&
   first.object->resource.file_extent==omeb_size&&!memcmp(first.object->resource.bytes,omeb,omeb_size));
 for(unsigned i=0;i<4;++i){
  gh_om_runtime_create request={0};
  gh_om_object *object=first.object;
  if(i)C(!gh_om_factory_create_0030fa98(&doors,&request,0x3eb,0xffff,&object));
  C(object==&doors.objects[69+i]&&object->runtime_created&&!object->source.bytes);
  C(!gh_om_factory_initialize_runtime_3eb(&doors,object,&original)&&original);
  object->view.bytes[0x600]=(uint8_t)i;
 }
 gh_object_view tutorial;
 C(!gh_game_objects_find(global,0x3eb,&tutorial)&&tutorial.use_count==4);
 const uint32_t known=158+doors.count;
 C(!gh_room_death_dispatch_0030f8b0(&dispatch,&bindings)&&!dispatch.scan.cursor&&
   pools.vectors[0].count==known&&(w(player.view.bytes+0x250)&0xe0000002u)==0xc0000002u);
 /* Original Main5 op11 deletes still-live R100 door collision boxes after
  * the first death pass and before later mode3 actor destruction. */
 gh_game_player_spawn spawn={.begun=1,.complete=1,.room=0x100};
 C(gh_game_player_reset_room_cursors(&prefix,&tail,&spawn,&player,&child)==GH_PLAYER_INIT_ARGUMENT);
 C(prefix.stage==4&&tail.stage==13&&spawn.complete);
 C(dynamic.allocation_count>=3&&tables.pending_actor_head&&tables.pending_actor_count>=3);
 for(unsigned i=0;i<3;++i)C(doors.objects[i].initialization->geometry_state.box);
 C(gh_scene_actors_reset_0012e800(&tables)==GH_TABLE_OK&&dynamic.bound&&!dynamic.allocation_count);
 C(!tables.pending_actor_head&&!tables.pending_actor_count);
 for(unsigned i=0;i<3;++i)C(!doors.objects[i].initialization->geometry_state.box);
 puts("PASS actual R100 12E800/12ECF0 door boxes and native borrow retirement before object death");
 for(unsigned i=3;i<next_row;++i)C((w(doors.objects[i].view.bytes+0x250)&0xe0000002u)==0xc0000002u);
 C(!gh_room_death_dispatch_0030f8b0(&dispatch,&bindings)&&!dispatch.scan.cursor&&
   pools.vectors[0].count==known&&(w(child.object.view.bytes+0x250)&0xe0000002u)==0xe0000002u);
 for(unsigned i=3;i<next_row;++i)C((w(doors.objects[i].view.bytes+0x250)&0xe0000002u)==0xe0000002u);
 for(unsigned i=0;i<5;++i)C(box_record[i][0x2e]&&sphere_record[i][0x2e]);
 gh_om303_state *type304=(gh_om303_state *)doors.objects[8].initialization;
 const uint8_t *box304_record,*sphere304_record;
 gh_collision_pair_shape box304,sphere304;
 C(type304->sphere_created&&type304->common.hitarea_created&&
   !gh_hitarea_node_at(&type304->common.hitarea,0,&box304_record,&box304)&&
   !gh_hitarea_node_at(&type304->sphere_unit,0,&sphere304_record,&sphere304));
 gh_om303_state *type305=(gh_om303_state *)doors.objects[18].initialization;
 const uint8_t *box305_record,*a307_record,*b307_record,*b2307_record;
 gh_collision_pair_shape box305,a307,b307,b2307;
 C(type305->common.hitarea_created&&!type305->sphere_created&&
   !gh_hitarea_node_at(&type305->common.hitarea,0,&box305_record,&box305));
 unsigned first307=18;
 while(first307<next_row&&doors.objects[first307].source.type!=0x307)++first307;
 C(first307<next_row);
 gh_om307_state *type307=(gh_om307_state *)doors.objects[first307].initialization;
 C(type307->common.hitarea_created&&type307->b2_created&&type307->a_created&&
   !gh_hitarea_node_at(&type307->common.hitarea,0,&b307_record,&b307)&&
   !gh_hitarea_node_at(&type307->box_b2,0,&b2307_record,&b2307)&&
   !gh_hitarea_node_at(&type307->box_a,0,&a307_record,&a307));
 unsigned first310=26;
 while(first310<next_row&&doors.objects[first310].source.type!=0x310)++first310;
 C(first310<next_row&&doors.objects[26].source.type==0x30a);
 gh_om303_state *type30a=(gh_om303_state *)doors.objects[26].initialization;
 gh_om310_state *type310=(gh_om310_state *)doors.objects[first310].initialization;
 const uint8_t *b30a_record,*a310_record,*b310_record;
 gh_collision_pair_shape b30a,a310,b310;
 C(type30a->common.hitarea_created&&type310->base.common.hitarea_created&&type310->base.sphere_created&&
   !gh_hitarea_node_at(&type30a->common.hitarea,0,&b30a_record,&b30a)&&
   !gh_hitarea_node_at(&type310->base.common.hitarea,0,&b310_record,&b310)&&
   !gh_hitarea_node_at(&type310->base.sphere_unit,0,&a310_record,&a310));
 unsigned first322=45;
 while(first322<next_row&&doors.objects[first322].source.type!=0x322)++first322;
 C(first322<next_row&&doors.objects[45].source.type==0x320);
 gh_om322_state *type322=(gh_om322_state *)doors.objects[first322].initialization;
 const uint8_t *a322_record,*b322_record;
 gh_collision_pair_shape a322,b322;
 C(type322->common.hitarea_created&&type322->a_created&&
   !gh_hitarea_node_at(&type322->common.hitarea,0,&b322_record,&b322)&&
   !gh_hitarea_node_at(&type322->box_a,0,&a322_record,&a322));
 unsigned index324=0;
 while(index324<69&&doors.objects[index324].source.type!=0x324)++index324;
 C(index324<69);
 gh_om_object *broken=doors.objects+index324;
 gh_hitarea_object saved_box=broken->initialization->hitarea;
 const uint8_t *dead_record;
 gh_collision_pair_shape dead_shape;
 C(!gh_hitarea_node_at(&saved_box,0,&dead_record,&dead_shape));
 gh_game_object_step step={0};
 C(!gh_game_object_step_construct_002c53a8(&step,init));
 live_retire live={.bindings=bindings.door,.pools=&pools,.selected=broken->handle};
 gh_object_move_bindings mb={.resolve=live_resolve,.context=&live,.elf=c.elf,.elf_bytes=c.elf_size};
 /* Only the Trans provider is being exercised, so do not invent a Move callback. */
 step.live_objects=mb;
 step.live_bound=1;
 const gh_object_trans_bindings trans={.invoke=live_destroy,.context=&live};
 C(!gh_game_object_step_bind_trans(&step,&trans));
 const uint32_t count_before=pools.vectors[0].count, secondary_before=pools.vectors[3].count;
 uint32_t live_frontier=0;
 int erased=0;
 C(!gh_object_live_trans_one_0030f650(&step,&pools,&work,live.selected,&erased,&live_frontier)&&
    erased&&!live_frontier&&live.invoked==1&&live.state.stage==5&&
    pools.vectors[0].count==count_before-1&&pools.vectors[3].count==secondary_before-1&&
    !broken->view.bytes&&!dead_record[0x2e]&&(w(dead_shape.record)&0x200u));
 void *retired=NULL;
 C(gh_room_object_lookup(&pools,live.selected,GH_ROOM_OBJECT_OM,&doors,&retired)==GH_POOL_BOUNDS);
 puts("PASS real OM324 ordinary 30F650 descriptor198CE0, same canonical handle and both registry compactions");
 const gh_om_object *retiring_objects=doors.objects;
 C(gh_om_factory_release_retired(&doors)==GH_PLACE_BUSY&&doors.objects==retiring_objects&&doors.bound);
 gh_room_pool_status final=gh_room_death_dispatch_0030f8b0(&dispatch,&bindings);
 if(final)fprintf(stderr,"retire status%d frontier %08x handle %u kind %x stage %u prefix %u\n",final,dispatch.frontier,dispatch.blocked_handle,dispatch.retiring_kind,dispatch.door.stage,dispatch.door.prefix.stage);
 C(!final&&
   !dispatch.frontier&&!dispatch.blocked_handle&&!dispatch.scan.cursor&&
   !pools.vectors[0].count&&!pools.vectors[1].count&&!pools.vectors[3].count&&
   !pools.blocks[0].used_count&&!pools.blocks[1].used_count&&
   !player.view.bytes&&!child.object.view.bytes&&!c.publication&&!attachment.attached);
 C(prefix.stage==4&&tail.stage==13&&spawn.complete);
 C(!gh_game_player_reset_room_cursors(&prefix,&tail,&spawn,&player,&child));
 C(!prefix.stage&&!prefix.factory&&!tail.stage&&!tail.player&&!tail.motion_entry.bytes&&
   !spawn.begun&&!spawn.complete&&!spawn.room);
 for(unsigned i=0;i<73;++i)C(!doors.objects[i].view.bytes);
 C(!gh_game_objects_find(global,0x3eb,&tutorial)&&tutorial.use_count==0&&tutorial.state==5);
 C(control.recent[10].bank==7);
 uint32_t object_frontier=0;
 for(unsigned phase=6;phase<=7;++phase){
  C(!gh_game_objects_update_001fe748(global,&object_frontier)&&control.recent[10].bank==7);
  C(!gh_game_objects_find(global,0x3eb,&tutorial)&&tutorial.state==phase);
 }
 C(!gh_game_objects_update_001fe748(global,&object_frontier)&&control.recent[10].bank==3);
 int unloaded=0;
 C(!gh_game_objects_is_unloaded_001fefd0(global,0x3eb,&unloaded)&&unloaded);
 for(unsigned i=0;i<5;++i)C(!box_record[i][0x2e]&&!sphere_record[i][0x2e]&&
   (w(box[i].record)&0x200)&&(w(sphere[i].record)&0x200));
 C(!box304_record[0x2e]&&!sphere304_record[0x2e]&&(w(box304.record)&0x200)&&(w(sphere304.record)&0x200));
 C(!box305_record[0x2e]&&(w(box305.record)&0x200));
 C(!a307_record[0x2e]&&!b307_record[0x2e]&&!b2307_record[0x2e]&&
   (w(a307.record)&0x200)&&(w(b307.record)&0x200)&&(w(b2307.record)&0x200));
 C(!b30a_record[0x2e]&&!a310_record[0x2e]&&!b310_record[0x2e]&&
   (w(b30a.record)&0x200)&&(w(a310.record)&0x200)&&(w(b310.record)&0x200));
 C(!a322_record[0x2e]&&!b322_record[0x2e]&&(w(a322.record)&0x200)&&(w(b322.record)&0x200));
 uint32_t frontier;
 /* The same stable wrappers receive a fresh actor and model on the next
  * original constructor call. The authored resource still comes from PL00. */
 C(!gh_game_player_factory_create_0030fa98(&factory,&fb,&original)&&original);
 C(!gh_actor_model_bind_packet_inputs(&c.model,c.heap,light->scalar+4));
 C(gh_game_player_initialize_00107f58(&prefix,&pb)==GH_PLAYER_INIT_UNSUPPORTED&&
   prefix.frontier==0x30fa98&&prefix.model_call_complete&&c.model.ready&&
   c.model.parts_allocated==32&&c.model.root.bytes==player.view.bytes);
 C(!gh_actor_model_release(&c.model)&&!gh_game_player_factory_release_prefix(&factory));
 puts("PASS retired PL00/PL01 cursors accept a fresh actual PL00 constructor");
 C(!gh_om_factory_release_retired(&doors)&&!doors.bound&&!doors.objects&&!doors.capacity&&
   !doors.count&&!doors.runtime_pending);
 C(!gh_om_factory_bind(&doors,&om_bindings,6)&&doors.bound&&doors.capacity==6);
 C(!gh_om_factory_release_retired(&doors)&&!gh_scene_meshes_release_prefix(&meshes)&&
   !gh_game_object_pool_release_001fdfa8(init,&frontier)&&
   !gh_collision_scroll_mutation_release(&scroll)&&!gh_scene_dynamic_release(&dynamic)&&
   !gh_scene_actors_reset_0012e800(&tables)&&!gh_collision_pairs_clear_002a71f0(&pairs,&frontier)&&
   !gh_collision_pairs_release(&pairs)&&!gh_game_world_light_release_presets(light));
 gh_stage_textures_destroy(textures);
 gh_se_voice_destroy(voices);gh_audio_registry_destroy(registry);
 gh_game_objects_destroy(objects);gh_game_objects_destroy(global);
 C(!gh_task_threads_destroy(threads));
 C(!gh_game_room_pools_release(&pools)&&!gh_heap_used(c.heap)&&!gh_heap_destroy(c.heap));
 free(om);free(om303);free(om304);free(om305);free(om307);free(om30a);free(om30e);free(om310);free(om320);free(om322);free(om323);free(om324);free(om33f);free(om346);free(om354);free(om35d);free(om309);free(omeb);free(effects);free(light);free(hair);free(pl);free(room);free(init);free(c.elf);
 gh_disc_close(d);
 puts("PASS combined actual ISO R100 retirement dispatcher: 156 static, PL00, PL01, all 69 regular OMS placements and four runtime tutorial 3EB actors");
 return 0;
}
