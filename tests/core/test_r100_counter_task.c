#include "r100_counter_task.h"
#include "r100_live_tasks.h"
#include "game_scene_placements.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define C(x)do{if(!(x)){fprintf(stderr,"FAIL%d %s\n",__LINE__,#x);exit(1);}}while(0)
static uint32_t rd(const void*p){uint32_t x;memcpy(&x,p,4);return x;}
static void wr(void*p,uint32_t x){memcpy(p,&x,4);}
int main(int argc,char**argv){C(argc==2);gh_disc*d;C(!gh_disc_open(argv[1],&d));size_t index;gh_disc_entry entry;uint8_t header[16];C(!gh_disc_find(d,"dll/debugoptimize/r100.rel",&index)&&!gh_disc_entry_at(d,index,&entry)&&entry.size==25663&&!gh_disc_read(d,index,0,header,sizeof(header))&&!memcmp(header,"SNR2",4));size_t r100_index;gh_disc_entry r100_entry;
 C(!gh_disc_find(d,"st1/r100.dat",&r100_index)&&!gh_disc_entry_at(d,r100_index,&r100_entry));
 uint8_t *room_data=malloc((size_t)r100_entry.size);
 C(room_data&&!gh_disc_read(d,r100_index,0,room_data,(size_t)r100_entry.size));gh_disc_close(d);
 gh_heap*heap;C(!gh_heap_create(1024,&heap));gh_r100_init owner={0};C(!gh_heap_allocate(heap,12,16,&owner.work));owner.heap=heap;memset(owner.work.pointer,0xa5,12);((uint8_t*)owner.work.pointer)[4]=0;owner.known_work_bytes=1u<<4;uint8_t room_bytes[0x1650]={0},scenario_bytes[0x114]={0};wr(room_bytes+0x1610,7);gh_room_state room={.active=room_bytes,.size=sizeof(room_bytes)};gh_game_init_state init={0};gh_task_record32 records[16]={0};for(unsigned i=0;i<16;i++)records[i].thread_id=-1;gh_task_manager manager={.records=records,.count=16,.capacity=16,.current_index=-1};gh_task_threads*threads;C(!gh_task_threads_create(&manager,&threads));gh_scene_scenario_view scenario={scenario_bytes,sizeof(scenario_bytes),&manager};gh_r100_counter_bindings bindings={.room_init=&owner,.room=&room,.init=&init,.scenario=scenario,.threads=threads};gh_r100_counter_task task={0};int32_t slot;
 C(!gh_r100_counter_bind(&task,&bindings)&&!gh_r100_counter_register_000010f8(&task,-1,&slot)&&slot==2&&!task.entered);C(gh_tasks_run_matching_002d52a0(&manager).status==GH_TASK_OK&&task.entered&&task.stage==0&&task.sleep_calls==1&&rd(owner.work.pointer)==0xa5a5a5a5&&rd(room_bytes+0x1610)==7);
 C(gh_tasks_run_matching_002d52a0(&manager).status==GH_TASK_OK&&task.stage==2&&task.status==GH_R100_SCRIPT_OK&&rd(owner.work.pointer)==7&&rd(room_bytes+0x1610)==7&&((uint8_t*)owner.work.pointer)[5]==0&&owner.known_work_bytes==0x3f);for(unsigned i=6;i<12;i++)C(((uint8_t*)owner.work.pointer)[i]==0xa5);
 for(unsigned i=0;i<3;i++)C(gh_tasks_run_matching_002d52a0(&manager).status==GH_TASK_OK&&task.status==GH_R100_SCRIPT_OK&&rd(owner.work.pointer)==7&&rd(room_bytes+0x1610)==7&&records[slot].running==1);C(!gh_task_threads_cancel(threads,slot));
 /* Originalpositivehead cannotbe treatedaszero justbecause a nativeenemy
  * consumer isnotyetrecovered. No syntheticdeath/alivecountsuccess. */
 wr(init.enemies+0x500,1);gh_r100_counter_task blocked={0};C(!gh_r100_counter_bind(&blocked,&bindings)&&!gh_r100_counter_register_000010f8(&blocked,-1,&slot));C(gh_tasks_run_matching_002d52a0(&manager).status==GH_TASK_OK);C(gh_tasks_run_matching_002d52a0(&manager).status==GH_TASK_OK&&blocked.status==GH_R100_SCRIPT_UNSUPPORTED&&blocked.frontier==0x1028&&blocked.frontier_relative&&rd(room_bytes+0x1610)==7&&records[slot].running==1);C(!gh_task_threads_cancel(threads,slot));
 gh_bundle bundle;gh_bundle_entry ems;
 C(!gh_bundle_open(room_data,(size_t)r100_entry.size,&bundle)&&!gh_bundle_find(&bundle,"EMS",0,&ems));
 gh_game_prepare *prepare=calloc(1,sizeof(*prepare));C(prepare);
 uint8_t core[GH_CORE_SAVE_BYTES],game_state=0;
 gh_core_save_view save={core,sizeof(core)};
 C(!gh_core_save_system_init(save)&&!gh_game_prepare_002a5d40(prepare,save,0x100,&game_state));
 gh_room_state active;C(!gh_room_state_select_002bebc8(prepare,0x100,0,&active));
 gh_boot_work work={.room=0x100};gh_em_scene_placements em={0};int loaded;
 C(!gh_em_scene_load_native_00294b98(&em,&active,&ems,&work,0,GH_EM_NUMERIC_PCSX2_CHOPZERO_REFERENCE,&loaded)&&loaded);
 uint8_t *first=active.active+12;
 C(active.active[8]>1&&first[0x13]==0);
 _Alignas(16) uint8_t object_bytes[4][GH_TITLE_PLAYER_BYTES]={0};
 gh_game_title_player actors[4]={0};
 for(unsigned i=0;i<4;i++){
  C(!gh_title_player_construct_00107e40(object_bytes[i],sizeof(object_bytes[i]),&actors[i].view));
  init.enemy_actors[i]=&actors[i];init.enemy_next[i]=(uint8_t)(i+1);init.enemies[i*20+16]=1;
  uint16_t kind=i==2?0x2a2:0x202,stamp=i==1?0xff:i==3?0xfe:0;
  memcpy(object_bytes[i]+0x2fe,&kind,2);memcpy(object_bytes[i]+0x640,&stamp,2);
 }
 init.enemy_top=init.enemies;init.enemy_next[3]=UINT8_MAX;object_bytes[0][0x614]=1;
 uint32_t alive;int flagged;
 C(!gh_r100_count_alive_00001028(&init,&active,&alive,&flagged)&&alive==2&&flagged==1);
 bindings.room=&active;
 wr(active.active+0x1610,7);
 gh_r100_counter_task live={0};
 C(!gh_r100_counter_bind(&live,&bindings)&&!gh_r100_counter_register_000010f8(&live,-1,&slot));
 C(gh_tasks_run_matching_002d52a0(&manager).status==GH_TASK_OK&&live.stage==0);
 C(gh_tasks_run_matching_002d52a0(&manager).status==GH_TASK_OK&&live.stage==2&&!live.frontier);
 C(rd(owner.work.pointer)==9&&rd(active.active+0x1610)==7&&((uint8_t*)owner.work.pointer)[5]==1);
 first[0x14]=1;
 C(!gh_r100_count_alive_00001028(&init,&active,&alive,&flagged)&&alive==1&&!flagged);
 C(gh_tasks_run_matching_002d52a0(&manager).status==GH_TASK_OK&&!live.frontier);
 C(rd(owner.work.pointer)==9&&rd(active.active+0x1610)==8&&((uint8_t*)owner.work.pointer)[5]==0);
 C(!gh_task_threads_cancel(threads,slot));
 gh_heap *at_heap;C(!gh_heap_create(1<<20,&at_heap));
 gh_scene_at_manager at={0};C(!gh_scene_at_initialize_units_002c0f90(&at,&bundle,at_heap,0x100,1));
 gh_r100_live_tasks watchers={0};
 gh_r100_live_bindings watch_b={.scenario=scenario,.threads=threads,.room_init=&owner,.room=&active,
  .at=&at,.init=&init,.work=&work,.player=&actors[0].view};
 const uint32_t rels[]={0x2920,0x2650,0x2490,0x2288,0x2198,0x15b0,0x2d68,0x1bd0};
 uint32_t frontier;
 for(unsigned i=0;i<8;i++)
  C(gh_r100_live_entry(rels[i])&&!gh_r100_live_register(&watchers,&watch_b,rels[i],-1,&frontier)&&!frontier);
 for(unsigned frame=0;frame<3;frame++)C(gh_tasks_run_matching_002d52a0(&manager).status==GH_TASK_OK);
 for(unsigned i=0;i<8;i++)C(watchers.tasks[i].entered&&!watchers.tasks[i].frontier);
 C(watchers.tasks[5].finished&&watchers.tasks[6].finished);
 gh_at_native_unit unit;C(!gh_scene_at_first_unit_002c30c0(&at,&unit));
 while(unit.bytes){
  uint16_t id;memcpy(&id,unit.bytes+0x36,2);
  if(id==3)break;
  C(!gh_scene_at_next_unit_002c30c8(&at,unit,&unit));
 }
 C(unit.bytes);wr(unit.bytes+0x50,rd(unit.bytes+0x50)|0x80000000u);
 C(gh_tasks_run_matching_002d52a0(&manager).status==GH_TASK_OK);
 C(watchers.tasks[2].status==GH_R100_SCRIPT_UNSUPPORTED&&watchers.tasks[2].frontier==0x2952d0);
 C(rd(active.active+0x1610)==8);
 for(unsigned i=0;i<8;i++)C(!gh_task_threads_cancel(threads,watchers.tasks[i].slot));
 C(!gh_scene_at_units_release(&at)&&!gh_heap_destroy(at_heap));
 init.enemy_next[3]=0;
 C(gh_r100_count_alive_00001028(&init,&active,&alive,&flagged)==GH_R100_SCRIPT_UNSUPPORTED&&alive==1&&!flagged);
 free(prepare);free(room_data);
 C(!gh_task_threads_destroy(threads)&&!gh_heap_release(heap,owner.work)&&!gh_heap_destroy(heap));puts("PASS actualR100REL identity +nativeScenario10F8 firstSleep/emptycanonicalEM count +same12B roomwork/RoomSave1610 loop, preservesunknownbytes, real EMS live/dead/missing/FF/stamp and heavy exclusion; inconsistent typed head strict1028; all eight real first-wave watcher registrations and AT3 positive SetEm frontier");return 0;}
