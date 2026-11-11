#include "game_hitarea_frame.h"
#include "game_player_attack_hitarea.h"
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

 uint32_t frontier;
 uint8_t *actor = player.view.bytes;
 const size_t sequence = w(pl + 0xe8);
 C(sequence == 0x96520 && sequence + 0x60 <= pl_size &&
   !memcmp(pl + sequence, "SEQ", 4) && pl[sequence + 0x1b] == 1);
 const unsigned sequence_rows = pl[sequence + 0x12];
 const uint8_t *rows = pl + sequence + pl[sequence + 0x10];
 C(sequence_rows == 4 && rows == pl + sequence + 0x40 &&
   w(rows) == 0x00080006 && (w(rows + 4) & 0xffff) == 2);
 const uint32_t record = w(c.elf + 0x3bc7d0 - 0xff000);
 C(record == 3);
 put(actor + 0x1610, 0x3f800000);
 put(actor + 0x1664, 0);
 put(actor + 0x166c, record);
 put(actor + 0x1674, 7);
 put(actor + 0x167c, 0);
 actor[0x1682] = 100;
 memset(actor + 0x100, 0, 12);
 C(!gh_player_attack_hitarea_0010a438(&player.view, NULL, &frontier) && !frontier);

 const uint8_t *node;
 gh_collision_pair_shape box;
 C(!gh_hitarea_node_at(&tail.give_objects[4], 0, &node, &box));
 C(w(box.record + 0x1f0) == 0x3f800000 && w(box.record + 0x1f4) == 0x3f000000 &&
   w(box.record + 0x1f8) == 0x3f800000);
 for (unsigned frame = 5; frame <= 9; ++frame) {
     for (unsigned i = 0; i < 5; ++i) {
         C(!gh_hitarea_object_set_active_001fd668(&tail.give_objects[i], 0, &frontier));
         put(tail.give_objects[i].bytes + 0x4c, 0);
         memset(tail.give_objects[i].bytes + 0x46, 0, 2);
     }
     unsigned flags = 0;
     for (unsigned i = 0; i < sequence_rows; ++i) {
         const uint8_t *row = rows + i * 8;
         if ((w(row) & 0xffff) <= frame && frame <= w(row) >> 16)
             flags |= w(row + 4) & 0xffff;
     }
     C((flags & 3) == (frame >= 6 && frame <= 8 ? 2u : 0u));
     actor[0x3ac] = (uint8_t)flags;
     actor[0x3ad] = (uint8_t)(flags >> 8);
     C(!gh_player_attack_hitarea_0010a438(&player.view, tail.give_objects, &frontier) && !frontier);
     for (unsigned i = 0; i < 5; ++i) {
         gh_collision_pair_shape shape;
         C(!gh_hitarea_node_at(&tail.give_objects[i], 0, &node, &shape));
         const unsigned active = (actor[0x3ac] & 3) && i == 4;
         C(!!(w(shape.record) & 2) == active);
         C(tail.give_objects[i].bytes[0x46] == (active ? 7 : 0));
         C(w(tail.give_objects[i].bytes + 0x4c) == (active ? 3u : 0u));
         if (actor[0x3ac] & 3) C(tail.give_objects[i].bytes[0x44] == 100 && !w(tail.give_objects[i].bytes + 0x40));
     }
     if (actor[0x3ac] & 3) {
         C(w(box.record + 0x130) == 0 && w(box.record + 0x134) == 0x3f666666 &&
           w(box.record + 0x138) == 0x3f333333);
         /* Original 10AF24 replaces the constructor box through its SetSize virtual. */
         C(w(box.record + 0x1f0) == 0x3f333333 && w(box.record + 0x1f4) == 0x3f800000 &&
           w(box.record + 0x1f8) == 0x3f99999a && w(box.record + 0x1c4) == 0x3f5b19c8);
         C(w(tail.give_objects[4].bytes + 0x18) == 0x3f800000);
         C(!gh_collision_pairs_step_002b9d58(&pairs, GH_STATIC_BINARY32_REFERENCE, &frontier));
         const uint8_t *position;
         C(!gh_collision_pair_shape_position(&box, &position) &&
           position == box.record + 0x190);
     }
 }
 actor[0x3ac] = 3;
 put(actor + 0x1660, 1);
 C(!gh_player_attack_hitarea_0010a438(&player.view, tail.give_objects, &frontier) && !frontier);
 C(w(box.record + 0x134) == 0x3f000000 && w(box.record + 0x1f0) == 0x3fe66666 &&
   w(box.record + 0x1f4) == 0x3fc00000 && w(box.record + 0x1f8) == 0x3fe66666);
 C(!gh_hitarea_object_set_active_001fd668(&tail.give_objects[4], 0, &frontier));
 put(actor + 0x1660, 2);
 C(gh_player_attack_hitarea_0010a438(&player.view, tail.give_objects, &frontier) == GH_HITAREA_UNSUPPORTED &&
   frontier == 0x10a608 && !(w(box.record) & 2));

 /* Controlled contact at the actual Give box center. Feed the pair solver's
  * hit to the production ledger with a distinct typed Take owner. */
 uint8_t other_bytes[GH_TITLE_PLAYER_BYTES];
 memcpy(other_bytes, actor, sizeof(other_bytes));
 gh_title_player_view other = player.view;
 other.bytes = other_bytes;
 other.alias_f0 = other_bytes + (player.view.alias_f0 - actor);
 other.alias_d0 = other_bytes + (player.view.alias_d0 - actor);
 put(other_bytes + 0x54c, 0);
 put(other_bytes + 0x250, 0);
 put(actor + 0x250, 0);
 memcpy(actor + 0x80, math.identity, 64);
 uint32_t placement[16], offset[3] = {0};
 memcpy(placement, math.identity, sizeof(placement));
 placement[13] = 0x3f933333;
 placement[14] = 0x3f333333;
 gh_hitarea_object target;
 int nonnull, index;
 gh_hitarea_shape_bindings hit_bind = {&pairs, &math, GH_STATIC_BINARY32_REFERENCE};
 C(!gh_hitarea_object_create_001fe218(init, &other, 1, &target, &nonnull, &frontier) && nonnull);
 C(!gh_hitarea_add_sphere_001fdaf0(&target, &hit_bind, (const uint8_t *)placement, offset,
   0x3e800000, &index, &frontier) && index >= 0);
 actor[0x3ac] = 2;
 C(!gh_player_attack_hitarea_0010a438(&player.view, tail.give_objects, &frontier));
 C(!gh_collision_pairs_step_002b9d58(&pairs, GH_STATIC_BINARY32_REFERENCE, &frontier) && w(box.record + 0xd8));
 gh_hitarea_frame ledger = {0};
 uint32_t pause_words[2] = {0};
 gh_game_scene_tables tables = {0};
 put(other.alias_f0, 0x3e800000);
 gh_hitarea_frame_bindings ledger_bind = {.tables = &tables, .boot_elf = c.elf, .boot_elf_size = c.elf_size,
     .current_player = &player.view, .player_pause_words = pause_words};
 gh_hitarea_status contact = gh_hitarea_frame_resume_bound_001fdfd8(
     init, &work, 1.0f, GH_STATIC_BINARY32_REFERENCE, &ledger, &ledger_bind, &frontier);
 fprintf(stderr, "actual contact status=%u frontier=%08X box hits=%u\n", contact, frontier, w(box.record + 0xd8));
 C(!contact && !frontier && !ledger.active);
 const gh_title_player_view *last;
 C(!gh_hitarea_object_last_hit_actor(&target, &last) && last == &player.view &&
   !gh_hitarea_object_last_hit_actor(&tail.give_objects[4], &last) && last == &other &&
   (w(target.bytes + 0x60) & 3) == 3 && target.bytes[0x46] == 7 &&
   target.bytes[0x44] == 100 && w(target.bytes + 0x4c) == 3);
 C(!gh_hitarea_object_set_active_001fd668(&tail.give_objects[4], 0, &frontier));
 actor[0x3ac] = 0;
C(!gh_collision_pairs_step_002b9d58(&pairs,GH_STATIC_BINARY32_REFERENCE,&frontier)&&!frontier);for(unsigned i=0;i<9;i++)C(!w((uint8_t*)pairs.records.pointer+i*0x270+0xd8));C(!gh_hitarea_shapes_abort(init,&frontier)&&!gh_game_object_pool_release_001fdfa8(init,&frontier)&&!init->hitarea_native&&init->hitarea_cold_constructed&&!gh_collision_solid_units_shutdown(&solids,&frontier));gh_game_player_child_attachment_clear(&attachment);c.publication=NULL;C(!gh_game_player_child_release_prefix(&child)&&!gh_actor_model_release(&c.model)&&!gh_game_player_factory_release_prefix(&factory)&&!gh_collision_pairs_release(&pairs)&&!gh_game_world_light_release_presets(light));free(light);gh_game_objects_destroy(objects);gh_game_objects_destroy(global);C(!gh_game_room_pools_release(&pools)&&!gh_heap_used(c.heap)&&!gh_heap_destroy(c.heap));free(hair);free(pl);free(room);free(init);free(c.elf);gh_disc_close(d);puts("PASS actual PL00 canonical give box, first Square class 0 frame gates, original payload/duration/offset/effect/power, original virtual size updates and production ledger contact");return 0;}
