#include "game_enemy_entry_tail.h"
#include "game_enemy_factory.h"
#include "disc.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fenv.h>
#define C(x) do{if(!(x)){fprintf(stderr,"line%d %s\n",__LINE__,#x);exit(1);}}while(0)
static uint8_t*load(gh_disc*d,const char*path,size_t*n){size_t i;gh_disc_entry e;C(!gh_disc_find(d,path,&i)&&!gh_disc_entry_at(d,i,&e));*n=(size_t)e.size;uint8_t*p=malloc(*n);C(p&&!gh_disc_read(d,i,0,p,*n));return p;}
int main(int argc,char**argv){C(argc==2);gh_disc*d;C(!gh_disc_open(argv[1],&d));size_t n,m;uint8_t*room=load(d,"st1/r100.dat",&n),*em=load(d,"em/em02.dat",&m);gh_bundle bundle;C(!gh_bundle_open(room,n,&bundle));
 gh_heap*heap;C(!gh_heap_create(32*1024*1024,&heap));gh_room_pools pools={0};C(!gh_game_room_pools_initialize_002a5d80(&pools,&bundle,heap));
 size_t core_n;uint8_t*core=load(d,"etc/core_usa.bin",&core_n);gh_bundle cb;gh_bundle_entry idd;C(!gh_bundle_open(core,core_n,&cb)&&!gh_id_find_language_data(&cb,&idd));gh_id_state*ids=calloc(1,sizeof(*ids));C(ids&&!gh_id_system_init_002acc70(ids,&idd));gh_id_runtime*runtime;C(!gh_id_runtime_create(ids,1,&runtime)&&!gh_id_runtime_set_path_policy(runtime,GH_ID_PATH_REFERENCE_NEAREST));
 gh_stage_slots global_slots={0},room_slots={0};gh_game_objects*global,*objects;C(!gh_game_objects_create(&global_slots,0,&global)&&!gh_game_objects_create(&room_slots,1,&objects));C(!gh_stage_slots_reset(&global_slots,heap)&&!gh_stage_slots_reset(&room_slots,heap));int added;C(!gh_game_objects_add_pinned_001fece0(objects,0x202,em,m,&added)&&added==1);C(!gh_game_objects_set_file_extent(objects,0x202,em,m));
 gh_game_init_state*init=calloc(1,sizeof(*init));C(init);gh_game_init_state_initialize(init);gh_game_mt_seed_0030f2b0(&init->random,7);C(!gh_game_enemy_reset_00290230(init));int foreign;uint32_t frontier;C(!gh_enemy_manager_insert_00290048(init,9,&foreign,-1,&frontier));
 gh_boot_work work={0};work.room=0x100;uint32_t disabledSE=0x20000000;memcpy((uint8_t*)&work+0x5e4,&disabledSE,4);uint8_t saved[GH_CORE_SAVE_BYTES]={0},scenario=0,serial=255;uint32_t fog=0;
 gh_enemy_entry_bindings b={.init=init,.pools=&pools,.global_objects=global,.room_objects=objects,.work=&work,.room_heap=heap,.packet_heap=heap,.fog_rgb=&fog,.save={saved,sizeof(saved)},.scenario14=&scenario,.factory_serial=&serial,.policy=GH_STATIC_BINARY32_REFERENCE};
 gh_enemy_request request={.object_kind=0x202,.position_xyz={0x3f800000,0x40000000,0x40400000},.dependency_kind=0x20a,.entry_number=3};gh_enemy_entry entry={0};int original=9;
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
 uint32_t parts=pools.blocks[0].used_count;C(gh_enemy_entry_002905d0(&entry,&b,&request,0,0,&original)==GH_ACTOR_MODEL_UNSUPPORTED&&entry.frontier==0x28ebf0&&serial==0&&pools.blocks[0].used_count==parts);
 gh_enemy_request changed=request;changed.entry_number++;
 C(gh_enemy_entry_002905d0(&entry,&b,&changed,0,0,&original)==GH_ACTOR_MODEL_BUSY&&serial==0);
 init->enemy_actors[0]=&foreign;C(gh_enemy_entry_abort_prefix(&entry)==GH_ACTOR_MODEL_BUSY&&entry.initialize.model.model.ready&&pools.blocks[0].used_count==parts);init->enemy_actors[0]=&entry.object;
 C(!gh_enemy_entry_abort_prefix(&entry)&&!pools.blocks[0].used_count&&!pools.blocks[1].used_count&&vector->count==0&&init->enemy_actors[9]==&foreign&&init->enemy_top==init->enemies+180&&init->enemy_last==init->enemy_top&&init->enemy_next[9]==255);
 C(!gh_game_cockpit_destroy(c));gh_id_runtime_destroy(runtime);free(ids);free(core);
 gh_game_objects_destroy(objects);gh_game_objects_destroy(global);free(init);
 C(!fesetround(FE_TONEAREST));C(!gh_game_room_pools_release(&pools)&&!gh_heap_used(heap)&&!gh_heap_destroy(heap));free(em);free(room);gh_disc_close(d);puts("PASS actualEntryEm202 realresource/serialwrap/registry/manager/model31/scopedforeign-preservingabort frontier202B88/counterwrap/realCockpit/resetno-repeat");return 0;
}
