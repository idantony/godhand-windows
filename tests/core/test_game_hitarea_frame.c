#include "game_hitarea_frame.h"
#include "game_hitarea_internal.h"
#include "game_player_init_tail.h"
#include "game_player_child_tail.h"
#include "game_world_light.h"
#include "game_actor_external_parent.h"
#include <fenv.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define C(x) do{if(!(x)){fprintf(stderr,"FAIL %d %s\n",__LINE__,#x);exit(1);}}while(0)
static void put(uint8_t*p,uint32_t v){memcpy(p,&v,4);}
static uint32_t w(const uint8_t*p){uint32_t v;memcpy(&v,p,4);return v;}
static uint8_t*load(gh_disc*d,const char*path,size_t*n){size_t i;gh_disc_entry e;C(!gh_disc_find(d,path,&i)&&!gh_disc_entry_at(d,i,&e));*n=(size_t)e.size;uint8_t*p=malloc(*n);C(p&&!gh_disc_read(d,i,0,p,*n));return p;}
typedef struct {gh_actor_model model;gh_game_player_factory*factory;gh_heap*heap;gh_game_world_light*light;gh_player_shared_vector vector;gh_player_child_tail child_tail;gh_game_player_child*publication;gh_core_save_view save;uint8_t*elf;size_t elf_size;} context;
static gh_player_init_status model_init(void*raw,gh_game_title_player*player,gh_boot_work*work,const gh_bundle_entry*md,const gh_bundle_entry*tm,uint32_t mode,int*original,uint32_t*frontier){context*c=raw;gh_actor_model_status e=gh_actor_model_initialize_00149738(&c->model,&player->view,work,c->factory->b.pools,md,tm,0,mode,GH_STATIC_BINARY32_REFERENCE,original);*frontier=c->model.frontier;return e?GH_PLAYER_INIT_UNSUPPORTED:GH_PLAYER_INIT_OK;}
static gh_title_player_status child_init(void*raw,gh_game_player_child*child,int*original,uint32_t*frontier){context*c=raw;if(!child->model.bound)C(!gh_actor_model_bind_packet_inputs(&child->model,c->heap,c->light->scalar+4));gh_title_player_status e=gh_game_player_child_initialize_00100378(child,original);if(e==GH_PLAYER_UNSUPPORTED&&child->frontier==0x100420)e=gh_game_player_child_finish_00100420(&c->child_tail,child,&c->vector,&c->publication,c->save,c->elf,c->elf_size,GH_STATIC_BINARY32_REFERENCE,original);*frontier=e?(c->child_tail.frontier?c->child_tail.frontier:child->frontier):0;return e;}
int main(int argc,char**argv){C(argc==3);context c={0};FILE*f=fopen(argv[2],"rb");C(f&&!fseek(f,0,SEEK_END));long size=ftell(f);C(size>0&&!fseek(f,0,SEEK_SET));c.elf_size=(size_t)size;c.elf=malloc(c.elf_size);C(c.elf&&fread(c.elf,1,c.elf_size,f)==c.elf_size);fclose(f);C(!gh_player_shared_vector_from_elf(&c.vector,c.elf,c.elf_size));gh_game_math_globals math={0};gh_boot_work work={.room=0x100};C(!gh_game_math_globals_from_elf(&math,c.elf,c.elf_size)&&!gh_game_math_static_initialize_003079c8(&math,&work,1,0xffff));gh_game_init_state*init=malloc(sizeof(*init));C(init);gh_game_init_state_initialize(init);C(!gh_hitarea_objects_construct_001fde00(init,NULL)&&!gh_game_object_pool_reset_001fdea8(init));uint8_t save_bytes[GH_CORE_SAVE_BYTES];c.save=(gh_core_save_view){save_bytes,sizeof(save_bytes)};C(!gh_core_save_system_init(c.save)&&!gh_core_save_game_init(c.save,&work));gh_disc*d;C(!gh_disc_open(argv[1],&d));size_t room_size,pl_size,child_size;uint8_t*room=load(d,"st1/r100.dat",&room_size),*pl=load(d,"pl/pl00.dat",&pl_size),*hair=load(d,"pl/pl01.dat",&child_size);gh_bundle bundle;C(!gh_bundle_open(room,room_size,&bundle));gh_room_pools pools={0};C(!gh_heap_create(32*1024*1024,&c.heap)&&!gh_game_room_pools_initialize_002a5d80(&pools,&bundle,c.heap));gh_game_world_light*light=calloc(1,sizeof(*light));C(light);c.light=light;C(!gh_game_world_light_construct_002d6d80(light)&&!gh_game_world_light_initialize_with_heap_002d6f40(light,&bundle,c.heap));gh_stage_slots gs={0},rs={0};gh_game_objects*global,*objects;C(!gh_game_objects_create(&gs,0,&global)&&!gh_game_objects_create(&rs,1,&objects)&&!gh_stage_slots_reset(&gs,NULL)&&!gh_stage_slots_reset(&rs,NULL));int added;C(!gh_game_objects_add_pinned_001fece0(global,0x100,pl,pl_size,&added)&&!gh_game_objects_set_file_extent(global,0x100,pl,pl_size)&&!gh_game_objects_add_pinned_001fece0(global,0x101,hair,child_size,&added)&&!gh_game_objects_set_file_extent(global,0x101,hair,child_size));uint8_t serial=33;gh_game_title_player player={0};gh_game_player_factory factory={0};c.factory=&factory;gh_player_factory_bindings fb={&player,&pools,global,objects,&work,d,c.save,&serial};int original;C(!gh_game_player_factory_create_0030fa98(&factory,&fb,&original)&&original&&serial==34);C(!gh_actor_model_bind_packet_inputs(&c.model,c.heap,light->scalar+4));gh_game_player_init prefix={0};gh_player_init_bindings pb={.factory=&factory,.work=&work,.current_player=&player,.model_initialize_00149738=model_init,.model_context=&c};C(gh_game_player_initialize_00107f58(&prefix,&pb)==GH_PLAYER_INIT_UNSUPPORTED&&prefix.frontier==0x30fa98&&prefix.stage==4&&prefix.model_call_complete&&c.model.ready&&c.model.parts_allocated==32);
 gh_collision_pairs pairs={0};C(!gh_collision_pairs_construct_002b8ae0(&pairs,c.heap)&&!gh_collision_pairs_initialize_002b8b50(&pairs));gh_game_collision_solids solids={0};gh_game_player_child child={0};gh_player_child_attachment attachment={0};gh_game_player_init_tail tail={0};gh_player_init_tail_bindings tb={.prefix=&prefix,.parent_model=&c.model,.child=&child,.attachment=&attachment,.solids=&solids,.default_heap=c.heap,.save=c.save,.initialize_child=child_init,.child_context=&c,.init=init,.pairs=&pairs,.math_globals=&math,.shape_policy=GH_STATIC_BINARY32_REFERENCE};gh_player_init_status status=gh_game_player_initialize_tail_00107f58(&tail,&tb);fprintf(stderr,"actualtail status%d stage%u frontier%08X take%u give%u serial%u\n",status,tail.stage,tail.frontier,tail.take_step,tail.give_step,serial);C(status==GH_PLAYER_INIT_UNSUPPORTED&&tail.frontier==0x14b730&&tail.stage==11&&tail.take_step==4&&tail.give_step==5&&tail.pending_lookup_name==0x41b828&&tail.active_child&&attachment.attached&&c.publication==&child&&c.child_tail.complete&&c.child_tail.springs.initialized&&serial==35&&tail.take_object.index==160&&tail.solid_unit&&tail.sphere&&gh_collision_pair_shapes_have_live(&pairs));for(unsigned i=0;i<4;i++){C(tail.take_sphere_indices[i]==(int)i&&w(player.view.bytes+0x67c+i*4)==i);const uint8_t*node;gh_collision_pair_shape shape;C(!gh_hitarea_node_at(&tail.take_object,i,&node,&shape)&&w(shape.record)==3&&w(shape.record+0x10)==2&&w(shape.record+0x1f0)==0x3e800000);}for(unsigned i=0;i<5;i++){C(tail.give_objects[i].index==i&&tail.give_shape_indices[i]==(int)i&&w(player.view.bytes+0x5b4+i*8)==i);const uint8_t*node;gh_collision_pair_shape shape;C(!gh_hitarea_node_at(&tail.give_objects[i],0,&node,&shape)&&w(shape.record)==1&&w(shape.record+0x10)==1&&w(shape.record+0xe0)==(i<4?2u:4u));}

 uint32_t frontier;gh_hitarea_frame frame={0};const uint8_t*node;gh_collision_pair_shape give,take;
 C(!gh_hitarea_node_at(&tail.give_objects[0],0,&node,&give));uint8_t*gn=(uint8_t*)node;
 C(!gh_hitarea_node_at(&tail.take_object,0,&node,&take));uint8_t*tn=(uint8_t*)node;
 put(tail.give_objects[0].bytes+0x34,0x12345678);gn[0x30]=1;tn[0x30]=2;put(tail.take_object.bytes+0x60,0xffffffff);
 put(player.view.bytes+0x54c,0x40000000);put(player.view.bytes+0x5a8,0x3f000000);put((uint8_t*)&work+0x5d8,0);put(init->object_pool+0xacd0,1);
 C(!gh_hitarea_frame_resume_001fdfd8(init,&work,1,GH_STATIC_BINARY32_REFERENCE,&frame,&frontier)&&!frame.active&&w(player.view.bytes+0x54c)==0x40000000&&w(tail.give_objects[0].bytes+0x34)==0x12345678);
 int modes[]={FE_TONEAREST,FE_DOWNWARD,FE_UPWARD,FE_TOWARDZERO};
 for(unsigned m=0;m<4;m++){put(player.view.bytes+0x54c,0x40000000);C(!fesetround(modes[m]));feclearexcept(FE_ALL_EXCEPT);feraiseexcept(FE_INVALID);int ex=fetestexcept(FE_ALL_EXCEPT);
 C(!gh_hitarea_frame_resume_001fdfd8(init,&work,1,GH_STATIC_BINARY32_REFERENCE,&frame,&frontier)&&!frame.active&&!frontier&&fegetround()==modes[m]&&fetestexcept(FE_ALL_EXCEPT)==ex&&w(player.view.bytes+0x54c)==0x3fc00000);}
 C(!w(tail.give_objects[0].bytes+0x34)&&w(tail.take_object.bytes+0x40)==0xffffffff&&!tail.take_object.bytes[0x44]&&w(tail.take_object.bytes+0x60)==0xfffffc40&&!(w(give.record)&2)&&!tn[0x30]&&!(w(take.record)&2));
 const uint8_t*position;C(!gh_collision_pair_shape_position(&give,&position)&&!memcmp(gn,position,12)&&!memcmp(gn,gn+0x10,12));
 put(give.record,w(give.record)|3);put(give.record+0xd8,1);gn[0x30]=2;put(player.view.bytes+0x54c,0x40000000);
 C(gh_hitarea_frame_resume_001fdfd8(init,&work,1,GH_STATIC_BINARY32_REFERENCE,&frame,&frontier)==GH_HITAREA_ARGUMENT&&frontier==0x1fc810&&frame.phase==2&&frame.slot==0&&frame.active);
 C(w(player.view.bytes+0x54c)==0x3fc00000&&gn[0x30]==2);
 gh_hitarea_frame copy=frame;C(gh_hitarea_frame_resume_001fdfd8(init,&work,1,GH_STATIC_BINARY32_REFERENCE,&copy,&frontier)==GH_HITAREA_ARGUMENT);
 for(unsigned k=0;k<3;k++)C(gh_hitarea_frame_resume_001fdfd8(init,&work,1,GH_STATIC_BINARY32_REFERENCE,&frame,&frontier)==GH_HITAREA_ARGUMENT&&w(player.view.bytes+0x54c)==0x3fc00000&&gn[0x30]==2);
 put(give.record+0xd8,0);C(!gh_hitarea_frame_resume_001fdfd8(init,&work,1,GH_STATIC_BINARY32_REFERENCE,&frame,&frontier)&&!frame.active&&gn[0x30]==1&&w(player.view.bytes+0x54c)==0x3fc00000);
 gh_game_damage_link*saved=init->object_links[0];init->object_links[0]=(gh_game_damage_link*)(uintptr_t)1;put(tail.give_objects[0].bytes+0x34,0xaabbccdd);
 C(gh_hitarea_frame_resume_001fdfd8(init,&work,1,GH_STATIC_BINARY32_REFERENCE,&frame,&frontier)==GH_HITAREA_ARGUMENT&&w(tail.give_objects[0].bytes+0x34)==0xaabbccdd);init->object_links[0]=saved;C(!gh_hitarea_frame_resume_001fdfd8(init,&work,1,GH_STATIC_BINARY32_REFERENCE,&frame,&frontier));
 C(gh_hitarea_frame_resume_001fdfd8(NULL,&work,1,GH_STATIC_BINARY32_REFERENCE,&frame,&frontier)==GH_HITAREA_ARGUMENT);
 frame=(gh_hitarea_frame){.active=1,.cursor_identity=&frame,.identity=init,.generation=init->hitarea_generation,.slot=416};C(gh_hitarea_frame_resume_001fdfd8(init,&work,1,GH_STATIC_BINARY32_REFERENCE,&frame,&frontier)==GH_HITAREA_ARGUMENT);frame=(gh_hitarea_frame){0};

 /* Unknown nonzero raw actor token is not typed NULL. */
 gh_title_player_view*saved_actor=init->hitarea_native->actors[160];init->hitarea_native->actors[160]=NULL;put(tail.take_object.bytes+0x30,0x12345678);put((uint8_t*)&work+0x5d8,0);
 C(gh_hitarea_frame_resume_001fdfd8(init,&work,1,GH_STATIC_BINARY32_REFERENCE,&frame,&frontier)==GH_HITAREA_UNSUPPORTED&&frontier==0x1fd448);init->hitarea_native->actors[160]=saved_actor;put(tail.take_object.bytes+0x30,0);C(!gh_hitarea_frame_resume_001fdfd8(init,&work,1,GH_STATIC_BINARY32_REFERENCE,&frame,&frontier));
 saved_actor=init->hitarea_native->actors[0];init->hitarea_native->actors[0]=NULL;put(tail.give_objects[0].bytes+0x30,0x12345678);
 C(gh_hitarea_frame_resume_001fdfd8(init,&work,1,GH_STATIC_BINARY32_REFERENCE,&frame,&frontier)==GH_HITAREA_UNSUPPORTED&&frontier==0x1fcf20);init->hitarea_native->actors[0]=saved_actor;put(tail.give_objects[0].bytes+0x30,0);C(!gh_hitarea_frame_resume_001fdfd8(init,&work,1,GH_STATIC_BINARY32_REFERENCE,&frame,&frontier));

 /* Real pair queries populate typed hit identities; same actor targets are
  * filtered only by original identity condition, never by distance zero. */
 gh_hitarea_object source,box1,box2;int nonnull,index;uint32_t zero[3]={0},size_xyz[3]={0x40000000,0x40000000,0x40000000};gh_hitarea_shape_bindings hb={&pairs,&math,GH_STATIC_BINARY32_REFERENCE};
 C(!gh_hitarea_object_create_001fe0e8(init,&player.view,0,&source,&nonnull,&frontier)&&nonnull);
 C(!gh_hitarea_add_sphere_001fdaf0(&source,&hb,player.view.bytes+0x80,zero,0x3f800000,&index,&frontier));
 C(!gh_hitarea_object_create_001fe218(init,&player.view,1,&box1,&nonnull,&frontier)&&!gh_hitarea_add_box_001fd9d8(&box1,&hb,player.view.bytes+0x80,zero,zero,size_xyz,&index,&frontier));
 C(!gh_hitarea_object_create_001fe218(init,&player.view,1,&box2,&nonnull,&frontier)&&!gh_hitarea_add_box_001fd9d8(&box2,&hb,player.view.bytes+0x80,zero,zero,size_xyz,&index,&frontier));
 C(!gh_collision_pairs_step_002b9d58(&pairs,GH_STATIC_BINARY32_REFERENCE,&frontier));
 gh_collision_pair_shape sphere;C(!gh_hitarea_node_at(&source,0,&node,&sphere)&&w(sphere.record+0xd8)>=2);
 gh_collision_pair_shape before0,before1,after;uint32_t distance;C(!gh_collision_pair_shape_hit_at(&sphere,0,&before0,&distance)&&!gh_collision_pair_shape_hit_at(&sphere,1,&before1,&distance));
 put(sphere.record+0x88,0x40800000);put(sphere.record+0x8c,0x3f800000);
 /* Disable remaining hits for this sort oracle only by placing their
  * distances later, retaining EVERY original identity and hit count. */
 for(unsigned i=2;i<w(sphere.record+0xd8);i++)put(sphere.record+0x88+4*i,0x41000000+i*0x10000);
 C(!gh_collision_pair_shape_sort_hits_001fc810(&sphere,&frontier)&&!gh_collision_pair_shape_hit_at(&sphere,0,&after,&distance)&&after.index==before1.index&&distance==0x3f800000);
 C(!gh_collision_pair_shape_hit_at(&sphere,1,&after,&distance)&&after.index==before0.index&&distance==0x40800000);
 C(!gh_hitarea_frame_resume_001fdfd8(init,&work,1,GH_STATIC_BINARY32_REFERENCE,&frame,&frontier)&&!frame.active);

 uint8_t other_bytes[GH_TITLE_PLAYER_BYTES];memcpy(other_bytes,player.view.bytes,sizeof(other_bytes));gh_title_player_view other_actor=player.view;other_actor.bytes=other_bytes;put(other_bytes+0x54c,0);put(other_bytes+0x250,0);
 init->hitarea_native->actors[box1.index]=&other_actor;
 C(gh_hitarea_frame_resume_001fdfd8(init,&work,1,GH_STATIC_BINARY32_REFERENCE,&frame,&frontier)==GH_HITAREA_UNSUPPORTED&&frontier==0x131e10&&frame.active);

 /* Explicit absent actor table is a ray-helper fixture, not R100 miss proof.
  * Real bound path validates original vtable descriptors and native aliases. */
 other_actor.alias_f0=other_bytes+(player.view.alias_f0-player.view.bytes);other_actor.alias_d0=other_bytes+(player.view.alias_d0-player.view.bytes);put(other_actor.alias_f0,0x3e800000);
 gh_game_scene_tables empty_tables={0};gh_hitarea_frame_bindings bound={.tables=&empty_tables,.boot_elf=c.elf,.boot_elf_size=c.elf_size};
 C(gh_hitarea_frame_resume_bound_001fdfd8(init,&work,1,GH_STATIC_BINARY32_REFERENCE,&frame,&bound,&frontier)==GH_HITAREA_UNSUPPORTED&&frontier==0x2db6b0);
 other_actor.original_vtable214^=4;C(gh_hitarea_frame_resume_bound_001fdfd8(init,&work,1,GH_STATIC_BINARY32_REFERENCE,&frame,&bound,&frontier)==GH_HITAREA_UNSUPPORTED&&frontier==0x2a9468);other_actor.original_vtable214^=4;
 uint8_t*alias=other_actor.alias_f0;other_actor.alias_f0=other_bytes+sizeof(other_bytes)-4;C(gh_hitarea_frame_resume_bound_001fdfd8(init,&work,1,GH_STATIC_BINARY32_REFERENCE,&frame,&bound,&frontier)==GH_HITAREA_BOUNDS&&frontier==0x2a9458);other_actor.alias_f0=alias;
 bound.current_player=&player.view;C(gh_hitarea_frame_resume_bound_001fdfd8(init,&work,1,GH_STATIC_BINARY32_REFERENCE,&frame,&bound,&frontier)==GH_HITAREA_UNSUPPORTED&&frontier==0x1fe348);
 uint32_t explicit_pause_fixture[2]={0,0};bound.player_pause_words=explicit_pause_fixture;
 C(!gh_hitarea_frame_resume_bound_001fdfd8(init,&work,1,GH_STATIC_BINARY32_REFERENCE,&frame,&bound,&frontier)&&!frame.active);
 const gh_title_player_view*last_actor;C(!gh_hitarea_object_last_hit_actor(&box1,&last_actor)&&last_actor&&last_actor->bytes==player.view.bytes&&(w(box1.bytes+0x60)&3)==3);


 init->hitarea_native->actors[box1.index]=&player.view;
 C(!gh_hitarea_frame_resume_001fdfd8(init,&work,1,GH_STATIC_BINARY32_REFERENCE,&frame,&frontier)&&!frame.active);
 put((uint8_t*)&work+0x5d8,0x80000000);put(player.view.bytes+0x54c,0x40000000);C(!gh_hitarea_frame_resume_001fdfd8(init,&work,1,GH_STATIC_BINARY32_REFERENCE,&frame,&frontier)&&w(player.view.bytes+0x54c)==0x40000000);

 int released;C(!gh_hitarea_object_clear_shapes_001fd580(&box1,&released,&frontier)&&released&&!gh_hitarea_object_release_empty_001fe278(&box1,&frontier));
 C(!gh_hitarea_object_last_hit_actor(&box1,&last_actor)&&!last_actor);
 C(!gh_hitarea_object_create_001fe218(init,&player.view,1,&box1,&nonnull,&frontier)&&nonnull&&!gh_hitarea_object_last_hit_actor(&box1,&last_actor)&&!last_actor);
 C(!gh_hitarea_shapes_abort(init,&frontier)&&!gh_game_object_pool_release_001fdfa8(init,&frontier)&&!gh_collision_solid_units_shutdown(&solids,&frontier));gh_game_player_child_attachment_clear(&attachment);c.publication=NULL;C(!gh_game_player_child_release_prefix(&child)&&!gh_actor_model_release(&c.model)&&!gh_game_player_factory_release_prefix(&factory)&&!gh_collision_pairs_release(&pairs)&&!gh_game_world_light_release_presets(light));free(light);gh_game_objects_destroy(objects);gh_game_objects_destroy(global);C(!gh_game_room_pools_release(&pools)&&!gh_heap_used(c.heap)&&!gh_heap_destroy(c.heap));free(hair);free(pl);free(room);free(init);free(c.elf);gh_disc_close(d);puts("PASS actualISO PL00/PL01 5Give+1Take: four-pass HitArea positions/reset/cooldown/countdown/FEnv/pause/strict positive query/late retry/foreign links/cleanup");return 0;}
