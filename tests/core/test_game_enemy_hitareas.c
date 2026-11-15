#include "game_enemy_hitareas.h"
#include "game_camera_envelope.h"
#include "game_ray_static.h"
#include "game_actor_motion_ik.h"
#include <math.h>
#include <fenv.h>
#include "game_enemy_factory.h"
#include "disc.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fenv.h>
#define C(x) do{if(!(x)){fprintf(stderr,"line%d %s\n",__LINE__,#x);exit(1);}}while(0)
static unsigned shape_count(const gh_collision_pairs*p){unsigned count=0;for(unsigned i=0;i<p->count;i++){uint32_t flags;memcpy(&flags,(const uint8_t*)p->records.pointer+(size_t)i*GH_COLLISION_RECORD_BYTES,4);if(flags&1)count++;}return count;}
static uint8_t*load(gh_disc*d,const char*path,size_t*n){size_t i;gh_disc_entry e;C(!gh_disc_find(d,path,&i)&&!gh_disc_entry_at(d,i,&e));*n=(size_t)e.size;uint8_t*p=malloc(*n);C(p&&!gh_disc_read(d,i,0,p,*n));return p;}
int main(int argc,char**argv){C(argc==3);gh_disc*d;C(!gh_disc_open(argv[1],&d));size_t n,m;uint8_t*room=load(d,"st1/r100.dat",&n),*em=load(d,"em/em02.dat",&m);gh_bundle bundle;C(!gh_bundle_open(room,n,&bundle));
 gh_heap*heap;C(!gh_heap_create(32*1024*1024,&heap));gh_room_pools pools={0};C(!gh_game_room_pools_initialize_002a5d80(&pools,&bundle,heap));
 size_t core_n;uint8_t*core=load(d,"etc/core_usa.bin",&core_n);gh_bundle cb;gh_bundle_entry idd;C(!gh_bundle_open(core,core_n,&cb)&&!gh_id_find_language_data(&cb,&idd));gh_id_state*ids=calloc(1,sizeof(*ids));C(ids&&!gh_id_system_init_002acc70(ids,&idd));gh_id_runtime*runtime;C(!gh_id_runtime_create(ids,1,&runtime)&&!gh_id_runtime_set_path_policy(runtime,GH_ID_PATH_REFERENCE_NEAREST));
 gh_stage_slots global_slots={0},room_slots={0};gh_game_objects*global,*objects;C(!gh_game_objects_create(&global_slots,0,&global)&&!gh_game_objects_create(&room_slots,1,&objects));C(!gh_stage_slots_reset(&global_slots,heap)&&!gh_stage_slots_reset(&room_slots,heap));int added;C(!gh_game_objects_add_pinned_001fece0(objects,0x202,em,m,&added)&&added==1);C(!gh_game_objects_set_file_extent(objects,0x202,em,m));
 gh_game_init_state*init=calloc(1,sizeof(*init));C(init);gh_game_init_state_initialize(init);gh_game_mt_seed_0030f2b0(&init->random,7);C(!gh_game_enemy_reset_00290230(init));int foreign;uint32_t frontier;C(!gh_enemy_manager_insert_00290048(init,9,&foreign,-1,&frontier));
 gh_boot_work work={0};work.room=0x100;uint32_t disabledSE=0x20000000;memcpy((uint8_t*)&work+0x5e4,&disabledSE,4);uint8_t saved[GH_CORE_SAVE_BYTES]={0},scenario=0,serial=255;uint32_t fog=0;
 gh_enemy_entry_bindings b={.init=init,.pools=&pools,.global_objects=global,.room_objects=objects,.work=&work,.room_heap=heap,.packet_heap=heap,.fog_rgb=&fog,.save={saved,sizeof(saved)},.scenario14=&scenario,.factory_serial=&serial,.policy=GH_STATIC_BINARY32_REFERENCE};
 gh_enemy_request request={.object_kind=0x202,.position_xyz={0x3f800000,0x40000000,0x40400000},.dependency_kind=0x20a,.flags=0x04000000,.appearance=3,.entry_number=3};gh_enemy_entry entry={0};int original=9;
 uint32_t inhibit=0x100000;memcpy((uint8_t*)&work+0x594,&inhibit,4);
 C(!gh_enemy_entry_002905d0(&entry,&b,&request,0,0,&original)&&!original&&!entry.object.allocation.pointer&&serial==255&&!pools.blocks[0].used_count);
 C(!gh_enemy_entry_abort_prefix(&entry));inhibit=0;memcpy((uint8_t*)&work+0x594,&inhibit,4);

 C(gh_enemy_entry_002905d0(&entry,&b,&request,0,0,&original)==GH_ACTOR_MODEL_UNSUPPORTED&&entry.frontier==0x28ebf0&&!original&&entry.published&&entry.stage==8);
 C(serial==0&&entry.object.view.bytes[0x2fc]==255&&entry.object.view.bytes[0x640]==3&&entry.initialize.model.model.ready&&entry.initialize.model.model.part_count==31);
 C(entry.slot==0&&init->enemy_actors[0]==&entry.object&&init->enemy_top==init->enemies+180&&init->enemy_last==init->enemies&&init->enemy_next[9]==0&&init->enemy_prev[0]==9);
 const gh_room_vector*vector;C(!gh_room_registry_view(&pools,0x754c58,&vector)&&vector->count==1);
 gh_game_cockpit*c;C(!gh_game_cockpit_create(runtime,ids,&init->random,&c));C(!gh_core_save_system_init(b.save)&&!gh_game_cockpit_initialize_00140c88(c,b.save));gh_id_base*ui;C(!gh_game_cockpit_base(c,0xe00,&ui));
 uint32_t counter=UINT32_MAX;memcpy(init->enemies+0x55c,&counter,4);gh_enemy_entry_tail tail={0};C(gh_enemy_entry_continue_002022d0(&tail,&entry,NULL)==GH_ACTOR_MODEL_UNSUPPORTED&&tail.frontier==0x2abbe8);
 C(gh_enemy_entry_continue_002022d0(&tail,&entry,c)==GH_ACTOR_MODEL_UNSUPPORTED&&tail.frontier==0x202b88&&tail.stage==3&&tail.original_counter==UINT32_MAX&&ui->playing&&tail.base.cockpit_base==ui);
 memcpy(&counter,init->enemies+0x55c,4);C(counter==0&&entry.object.view.bytes[0x140]==4);uint32_t flags27;memcpy(&flags27,entry.initialize.model.model.parts[27].bytes+0x154,4);C(!(flags27&0x1100000));
 uint32_t mt_index=init->random.index;C(gh_enemy_entry_continue_002022d0(&tail,&entry,c)==GH_ACTOR_MODEL_UNSUPPORTED&&tail.frontier==0x202b88&&init->random.index==mt_index);memcpy(&counter,init->enemies+0x55c,4);C(!counter);
 gh_enemy_hp hp={0};uint8_t*raw=entry.object.view.bytes;uint16_t hpword;
 const int modes[]={FE_TONEAREST,FE_DOWNWARD,FE_UPWARD,FE_TOWARDZERO};for(unsigned mode=0;mode<4;mode++){C(!fesetround(modes[mode]));
 for(unsigned level=1;level<=8;level++)for(unsigned difficulty=0;difficulty<3;difficulty++){
  static const unsigned expected[]={148,192,236,296,340,399,444,488};raw[0x186b]=(uint8_t)level;saved[0x1f]=(uint8_t)difficulty;hp=(gh_enemy_hp){0};unsigned expect=expected[level-1];
  if(difficulty==0){C(gh_enemy_hp_initialize_00202b88(&hp,&tail,GH_EM_NUMERIC_EXACT)==GH_ACTOR_MODEL_UNSUPPORTED&&hp.frontier==0x20367c&&!hp.stage);float factor;uint32_t factorbits=0x3f4ccccd;memcpy(&factor,&factorbits,4);expect=(unsigned)((double)expect*(double)factor);}else if(difficulty==2)expect+=20;
  C(gh_enemy_hp_initialize_00202b88(&hp,&tail,GH_EM_NUMERIC_PCSX2_CHOPZERO_REFERENCE)==GH_ACTOR_MODEL_UNSUPPORTED&&hp.frontier==0x2036b0&&hp.hp==expect);memcpy(&hpword,raw+0x548,2);C(hpword==expect);memcpy(&hpword,raw+0x54a,2);C(hpword==expect);
 }
 C(fegetround()==modes[mode]);}C(!fesetround(FE_TONEAREST));
 raw[0x186b]=1;saved[0x1f]=0;uint32_t newgame=0x4000000;memcpy(saved+0x14,&newgame,4);hp=(gh_enemy_hp){0};C(gh_enemy_hp_initialize_00202b88(&hp,&tail,GH_EM_NUMERIC_EXACT)==GH_ACTOR_MODEL_UNSUPPORTED&&hp.hp==148);newgame=0;memcpy(saved+0x14,&newgame,4);
 FILE*ef=fopen(argv[2],"rb");C(ef&&!fseek(ef,0,SEEK_END));long en=ftell(ef);C(en>0);rewind(ef);uint8_t*elf=malloc((size_t)en);C(elf&&fread(elf,1,(size_t)en,ef)==(size_t)en);fclose(ef);gh_motion_ik_globals ik={0};C(!gh_motion_ik_globals_from_elf(&ik,elf,(size_t)en));
 gh_enemy_motion motion={0};gh_actor_motion_status motionstatus=gh_enemy_motion_initialize_002036b0(&motion,&hp,&ik,NULL,NULL);printf("enemy motion status%d frontier%x stage%u beginstage%u\n",motionstatus,motion.frontier,motion.stage,motion.motion.stage);
 C(motionstatus==GH_ACTOR_MOTION_UNSUPPORTED&&motion.frontier==0x157c18&&motion.stage==2&&motion.motion.complete&&motion.motion.sequence_valid&&motion.motion_offset==3803136&&motion.sequence_offset==3816512&&raw[0x2f5]==0x67);
 C(gh_enemy_motion_initialize_002036b0(&motion,&hp,&ik,NULL,NULL)==GH_ACTOR_MOTION_UNSUPPORTED&&motion.frontier==0x157c18);
 int ended=9;gh_actor_motion_status sampled=gh_enemy_motion_initial_step_00204abc(&motion,1.0f,GH_STATIC_BINARY32_REFERENCE,&ended);printf("enemy sampled%d frontier%x stage%u\n",sampled,motion.frontier,motion.stage); C(sampled==GH_ACTOR_MOTION_UNSUPPORTED&&motion.frontier==0x152a20&&motion.stage==4);
 gh_game_scene_tables actors={0};C(!gh_scene_actors_0012e670(&actors,&bundle));gh_camera_envelope envelope={0};uint32_t posefrontier=0;C(!gh_game_camera_envelope_from_elf(&envelope,elf,(size_t)en,&posefrontier));
 gh_motion_solve_bindings solvebinding={.model=&entry.initialize.model.model,.work=&work,.actors=&actors,.ik_globals=&ik,.atan_coeff=envelope.atan_coeff};gh_enemy_pose pose={0};C(gh_enemy_pose_initialize_00204af8(&pose,&motion,NULL,GH_STATIC_BINARY32_REFERENCE)==GH_ACTOR_MOTION_UNSUPPORTED&&pose.frontier==0x152a20);
 gh_actor_motion_status posestatus=gh_enemy_pose_initialize_00204af8(&pose,&motion,&solvebinding,GH_STATIC_BINARY32_REFERENCE);printf("enemy pose%d frontier%x stage%u solveindex%u\n",posestatus,pose.frontier,pose.stage,pose.solve.index);
 C(posestatus==GH_ACTOR_MOTION_UNSUPPORTED&&pose.frontier==0x14ef50&&pose.stage==3&&pose.solve.index==0);
 C(actors.ray.native_queries==2);uint64_t queries=actors.ray.native_queries;C(gh_enemy_pose_initialize_00204af8(&pose,&motion,&solvebinding,GH_STATIC_BINARY32_REFERENCE)==GH_ACTOR_MOTION_UNSUPPORTED&&pose.frontier==0x14ef50&&actors.ray.native_queries==queries);
 uint8_t before_shadow[GH_EM202_BYTES];memcpy(before_shadow,raw,sizeof(before_shadow));gh_enemy_shadow shadow={0};C(gh_enemy_shadow_initialize_00204b10(&shadow,&pose)==GH_ACTOR_MOTION_UNSUPPORTED&&shadow.frontier==0x204b4c&&shadow.stage==1&&shadow.shadow.initialized&&shadow.shadow.packets[0]==raw+0x780&&shadow.shadow.packets[1]==raw+0xed0&&shadow.shadow.pattern_original==0x3c3f68);
 C(!memcmp(raw,before_shadow,0x780)&&!memcmp(raw+0x1620,before_shadow+0x1620,GH_EM202_BYTES-0x1620));uint64_t packetword;memcpy(&packetword,raw+0x780,8);C(packetword==0x10000074);memcpy(&packetword,raw+0x780+0x740,8);C(packetword==0x3a0000f8);C(!memcmp(raw+0x780,raw+0xed0,0x750));
 gh_player_shadow invalid={0};gh_title_player_view shortview=entry.object.view;shortview.size=0x161f;uint8_t after_shadow[GH_EM202_BYTES];memcpy(after_shadow,raw,sizeof(after_shadow));C(gh_game_actor_shadow_initialize_0014ef50(&invalid,&shortview,0x780,0x3c3f68)==GH_PLAYER_BOUNDS&&!memcmp(raw,after_shadow,sizeof(after_shadow))&&!invalid.initialized);
 C(gh_enemy_shadow_initialize_00204b10(&shadow,&pose)==GH_ACTOR_MOTION_UNSUPPORTED&&!memcmp(raw,after_shadow,sizeof(after_shadow)));
 gh_game_collision_solids solids={0};gh_game_collision_solids_reset_00134690(&solids);uint8_t foreignbytes[0x5b0]={0};gh_title_player_view foreignview;C(!gh_game_object_construct_002a7338(foreignbytes,sizeof(foreignbytes),&foreignview));gh_collision_solid_unit*foreignunit;C(!gh_collision_solid_create_unit_001346f8(&solids,heap,&foreignview,1,0x3e99999a,&foreignunit,&frontier));
 gh_enemy_solids enemySolids={0};C(gh_enemy_solids_initialize_00204b4c(&enemySolids,&shadow,NULL,heap)==GH_ACTOR_MOTION_UNSUPPORTED&&enemySolids.frontier==0x1346f8&&!enemySolids.stage);
 C(gh_enemy_solids_initialize_00204b4c(&enemySolids,&shadow,&solids,heap)==GH_ACTOR_MOTION_UNSUPPORTED&&enemySolids.frontier==0x14b730&&enemySolids.stage==4&&solids.unit_count==2&&solids.global_00460d18==1);
 C(enemySolids.unit->type==5&&enemySolids.unit->bound_radius_bits==0x3e4ccccd&&enemySolids.sphere->radius_bits==0x3f333333&&enemySolids.sphere->matrix==raw+0x80&&raw[0x1868]==1);
 uint32_t rngflags;memcpy(&rngflags,raw+0x16d0,4);C(!(rngflags&0x200));
 uint32_t finalMT=init->random.index;C(gh_enemy_solids_initialize_00204b4c(&enemySolids,&shadow,&solids,heap)==GH_ACTOR_MOTION_UNSUPPORTED&&init->random.index==finalMT&&solids.unit_count==2);
 gh_enemy_node_names names={0};C(!gh_enemy_node_names_from_elf(&names,elf,(size_t)en));gh_enemy_node_cache nodes={0};C(gh_enemy_node_cache_initialize_002055b8(&nodes,&enemySolids,&names)==GH_PLAYER_NODES_UNSUPPORTED&&nodes.frontier==0x205ae4&&nodes.stage==2&&nodes.cursor==11);
 unsigned foundnames=0;for(unsigned ni=0;ni<11;ni++)if(nodes.nodes[ni].bytes){foundnames++;C(nodes.nodes[ni].owner==&entry.initialize.model.model);gh_model_mesh mesh;C(!gh_model_mesh_at(&entry.initialize.model.model.script,nodes.nodes[ni].mesh_index,&mesh)&&!memcmp(mesh.name,names.names[ni],8));if(ni>=8){uint32_t flags;memcpy(&flags,nodes.nodes[ni].bytes+0x380,4);C(flags&1);}}printf("actualEM202nodecache%uof11\n",foundnames);C(foundnames>0);
 C(gh_enemy_node_cache_initialize_002055b8(&nodes,&enemySolids,&names)==GH_PLAYER_NODES_UNSUPPORTED&&nodes.frontier==0x205ae4);
 uint32_t untouched=0xdeadbeef,cleared=0xfeedface;memcpy(raw+0x73c,&untouched,4);memcpy(raw+0x740,&cleared,4);gh_game_mt expectedMT=init->random;uint32_t expectedWords[3];for(unsigned ti=0;ti<3;ti++)C(!gh_game_mt_next_0030f348(&expectedMT,&expectedWords[ti]));
 gh_enemy_timers timers={0};C(gh_enemy_timers_initialize_00205ae4(&timers,&nodes,GH_EM_NUMERIC_EXACT)==GH_ACTOR_MOTION_UNSUPPORTED&&timers.frontier==0x30f530&&timers.pending&&timers.cursor==0&&timers.pending_word==expectedWords[0]);
 C(gh_enemy_timers_initialize_00205ae4(&timers,&nodes,GH_EM_NUMERIC_PCSX2_CHOPZERO_REFERENCE)==GH_ACTOR_MOTION_UNSUPPORTED&&timers.frontier==0x2064fc&&timers.stage==2&&timers.cursor==3&&!timers.pending&&!memcmp(&expectedMT,&init->random,sizeof(expectedMT))&&!memcmp(timers.words,expectedWords,sizeof(expectedWords)));
 memcpy(&untouched,raw+0x73c,4);memcpy(&cleared,raw+0x740,4);C(untouched==0xdeadbeef&&!cleared&&!raw[0x1869]);uint32_t timerIndex=init->random.index;C(gh_enemy_timers_initialize_00205ae4(&timers,&nodes,GH_EM_NUMERIC_PCSX2_CHOPZERO_REFERENCE)==GH_ACTOR_MOTION_UNSUPPORTED&&init->random.index==timerIndex);
 static const uint32_t speeds[]={0x3f800000,0x3f99999a,0x3fb33333,0x3fcccccd,0x3ff33333,0x40066666,0x40133333,0x40200000};for(unsigned level=1;level<=8;level++){
 raw[0x186b]=(uint8_t)level;memset(raw+0x17b8,0xaa,6);gh_enemy_behavior behavior={0};C(gh_enemy_behavior_initialize_002064fc(&behavior,&timers)==GH_ACTOR_MOTION_UNSUPPORTED&&behavior.frontier==0x1fe218&&behavior.stage==2);uint32_t speed;memcpy(&speed,raw+0x76c,4);C(speed==speeds[level-1]&&raw[0x17b9]==0&&raw[0x17b8]==1&&raw[0x17ba]==0&&raw[0x17bd]==1&&raw[0x17bb]==1&&raw[0x17bc]==0);}
 gh_enemy_behavior special={0};work.room=0x803;C(gh_enemy_behavior_initialize_002064fc(&special,&timers)==GH_ACTOR_MOTION_UNSUPPORTED&&special.frontier==0x274140&&special.stage==1);work.room=0x801;C(gh_enemy_behavior_initialize_002064fc(&special,&timers)==GH_ACTOR_MOTION_UNSUPPORTED&&special.frontier==0x1fe218);uint32_t specialspeed;memcpy(&specialspeed,raw+0x76c,4);C(specialspeed==0x3f800000);work.room=0x100;
 raw[0x186b]=1;gh_enemy_behavior hitBehavior={0};C(gh_enemy_behavior_initialize_002064fc(&hitBehavior,&timers)==GH_ACTOR_MOTION_UNSUPPORTED&&hitBehavior.stage==2);
 gh_game_math_globals shapeMath={0};C(!gh_game_math_globals_from_elf(&shapeMath,elf,(size_t)en)&&!gh_game_math_static_initialize_003079c8(&shapeMath,&work,1,0xffff));gh_collision_pairs pairs={0};C(!gh_collision_pairs_construct_002b8ae0(&pairs,heap)&&!gh_collision_pairs_initialize_002b8b50(&pairs));gh_hitarea_shape_bindings shapeBindings={&pairs,&shapeMath,GH_STATIC_BINARY32_REFERENCE};gh_enemy_hitareas hitareas={0};
 C(gh_enemy_hitareas_initialize_00207018(&hitareas,&hitBehavior,&shapeBindings)==GH_ACTOR_MOTION_UNSUPPORTED&&hitareas.frontier==0x1fde00&&!hitareas.stage);
 C(!gh_hitarea_objects_construct_001fde00(init,&frontier)&&!gh_game_object_pool_reset_001fdea8(init));gh_hitarea_object foreignArea;int foreignPresent;C(!gh_hitarea_object_create_001fe0e8(init,&foreignview,1,&foreignArea,&foreignPresent,&frontier)&&foreignPresent);const uint32_t zeroXYZ[]={0,0,0};int foreignIndex;C(!gh_hitarea_add_sphere_001fdaf0(&foreignArea,&shapeBindings,foreignbytes+0x80,zeroXYZ,0x3e800000,&foreignIndex,&frontier));
 gh_actor_motion_status hitStatus=gh_enemy_hitareas_initialize_00207018(&hitareas,&hitBehavior,&shapeBindings);printf("enemy HitArea%d frontier%x stage%u pairs%u\n",hitStatus,hitareas.frontier,hitareas.stage,shape_count(&pairs));C(hitStatus==GH_ACTOR_MOTION_UNSUPPORTED&&hitareas.frontier==0x26b9e8&&hitareas.stage==4&&shape_count(&pairs)==10);
 const unsigned expectedParts[]={1,3,19,23};for(unsigned bi=0;bi<4;bi++){const uint8_t*record,*matrix;gh_collision_pair_shape shape;C(!gh_hitarea_node_at(&hitareas.units[0],bi,&record,&shape)&&!gh_collision_pair_shape_source_matrix(&shape,&matrix)&&matrix==entry.initialize.model.model.parts[expectedParts[bi]].bytes+0x80);uint32_t radius,flags;memcpy(&radius,shape.record+0x1f0,4);memcpy(&flags,shape.record,4);C(radius==(bi?0x3e99999a:0x3ecccccd)&&(flags&2));}
 for(unsigned ui=1;ui<6;ui++){const uint8_t*record,*matrix;gh_collision_pair_shape shape;C(!gh_hitarea_node_at(&hitareas.units[ui],0,&record,&shape)&&!gh_collision_pair_shape_source_matrix(&shape,&matrix)&&matrix==raw+0x80);uint32_t flags;memcpy(&flags,shape.record,4);C(!(flags&2)&&hitareas.units[ui].bytes[0x48]==(ui==5?0:1));}
 {const uint8_t*record;gh_collision_pair_shape shape;uint32_t a,b,c;C(!gh_hitarea_node_at(&hitareas.units[2],0,&record,&shape));memcpy(&a,shape.record+0x1f0,4);memcpy(&b,shape.record+0x1f4,4);memcpy(&c,shape.record+0x124,4);C(a==0x40000000&&b==0x3f000000&&c==0x3fa66666);memcpy(&a,shape.record+0x128,4);C(a==0x3f400000);
 for(unsigned ui=4;ui<6;ui++){C(!gh_hitarea_node_at(&hitareas.units[ui],0,&record,&shape));memcpy(&a,shape.record+0x1f0,4);memcpy(&b,shape.record+0x1f4,4);memcpy(&c,shape.record+0x1f8,4);C(a==0x3f800000&&b==0x3f000000&&c==0x3f800000);memcpy(&a,shape.record+0x124,4);C(a==(ui==4?0x3fa66666u:0u));}}
 uint32_t priorPairs=shape_count(&pairs);C(gh_enemy_hitareas_initialize_00207018(&hitareas,&hitBehavior,&shapeBindings)==GH_ACTOR_MOTION_UNSUPPORTED&&shape_count(&pairs)==priorPairs);
 C(!gh_enemy_hitareas_abort_prefix(&hitareas)&&shape_count(&pairs)==1&&init->object_links[foreignArea.index]);C(!gh_hitarea_shapes_abort_object(&foreignArea,&frontier)&&!gh_hitarea_object_release_empty_001fe278(&foreignArea,&frontier)&&!shape_count(&pairs));C(!gh_game_object_pool_release_001fdfa8(init,&frontier)&&!gh_collision_pairs_release(&pairs));
 C(!gh_enemy_solids_abort_prefix(&enemySolids)&&solids.unit_count==1&&!solids.global_00460d18&&solids.native_units==foreignunit&&foreignunit->object==&foreignview);C(!gh_collision_solid_release_unit_001347e8(&solids,&foreignview,&frontier)&&gh_collision_solid_units_empty(&solids));
 gh_scene_ray_triangle_state_release(&actors);
 uint32_t parts=pools.blocks[0].used_count;C(gh_enemy_entry_002905d0(&entry,&b,&request,0,0,&original)==GH_ACTOR_MODEL_UNSUPPORTED&&entry.frontier==0x28ebf0&&serial==0&&pools.blocks[0].used_count==parts);
 gh_enemy_request changed=request;changed.entry_number++;
 C(gh_enemy_entry_002905d0(&entry,&b,&changed,0,0,&original)==GH_ACTOR_MODEL_BUSY&&serial==0);
 init->enemy_actors[0]=&foreign;C(gh_enemy_entry_abort_prefix(&entry)==GH_ACTOR_MODEL_BUSY&&entry.initialize.model.model.ready&&pools.blocks[0].used_count==parts);init->enemy_actors[0]=&entry.object;
 C(!gh_enemy_entry_abort_prefix(&entry)&&!pools.blocks[0].used_count&&!pools.blocks[1].used_count&&vector->count==0&&init->enemy_actors[9]==&foreign&&init->enemy_top==init->enemies+180&&init->enemy_last==init->enemy_top&&init->enemy_next[9]==255);
 free(elf);C(!gh_game_cockpit_destroy(c));gh_id_runtime_destroy(runtime);free(ids);free(core);
 gh_game_objects_destroy(objects);gh_game_objects_destroy(global);free(init);
 C(!fesetround(FE_TONEAREST));C(!gh_game_room_pools_release(&pools)&&!gh_heap_used(heap)&&!gh_heap_destroy(heap));free(em);free(room);gh_disc_close(d);puts("PASS actualEntryEm202 realresource/serialwrap/registry/manager/model31/scopedforeign-preservingabort actualEMSappearance3/realMTB3+SEQ/IK+sample/realR100rays/IK31/finishmodel/same6HitAreaUnits/9shapes/scopedabort/foreignkept/frontier26B9E8");return 0;
}
