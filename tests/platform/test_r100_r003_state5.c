#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmsystem.h>
#include "startup_host.c"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(x) do{if(!(x)){fprintf(stderr,"fixture line %d: %s\n",__LINE__,#x);exit(1);}}while(0)

#include "startup_cpu_sink.h"

static void request_r003(gh_startup_host *host)
{
 gh_game_services_scene_view scene;
 CHECK(!gh_game_services_get_scene_view(host->services,&scene));
 CHECK(scene.room_state->active[8]<=64&&host->r100_exit.bindings.at==scene.scene_at);
 unsigned marked=0;
 for(unsigned i=0;i<scene.room_state->active[8];i++){
  uint8_t *row=scene.room_state->active+12+i*24;
  if(row[0x13]!=19&&row[0x13]!=20)continue;
  CHECK(row[0x14]==0);
  row[0x14]=1;
  ++marked;
 }
 CHECK(marked==2);
 gh_title_player_view armed,weapon;
 uint16_t stamp,kind;
 CHECK(!gh_game_services_actor_view(host->services,251,&armed)&&armed.size>=0x642);
 CHECK(!gh_game_services_actor_view(host->services,252,&weapon)&&weapon.size==0x790);
 memcpy(&stamp,armed.bytes+0x640,2);memcpy(&kind,weapon.bytes+0x2fe,2);
 CHECK(stamp==18&&kind==0x363&&weapon.original_vtable214==0x42aa98);
 /* Position the fixture on the authored A38 ray lane; original AT, action
  * and Door code still own the trigger, deferred reserve and transition. */
 const float matrix[]={1,0,0,0,0,1,0,0,0,0,1,0,2.65f,0,-80.2f,1};
 CHECK(scene.title_player->view.alias_f0==scene.title_player->view.bytes+0xb0);
 float previous[16];
 memcpy(previous,scene.title_player->view.bytes+0x80,sizeof(previous));
 memcpy(scene.title_player->view.bytes+0x80,matrix,sizeof(matrix));
 uint32_t frontier=0;
 CHECK(!scene_consumer(host,0x2c1618,0,&frontier)&&!frontier);
 gh_at_native_unit jump;
 CHECK(!gh_scene_at_get_jump_002c2ae0(scene.scene_at,0,&jump)&&jump.bytes);
 CHECK(scene.action_buttons->entries[0].callback_argument==jump.bytes);
 gh_native_input_snapshot confirm={.focused=1,.connected=1,.pressed=UINT64_C(8)<<32};
 CHECK(!gh_startup_host_set_input(host,&confirm));
 gh_action_button_frame action={0};
 CHECK(!gh_action_button_frame_begin(&action));
 CHECK(!gh_action_button_step_001f7360(&action,&host->action_bindings));
 CHECK(host->r100_exit.pending);
 CHECK(gh_tasks_run_matching_002d52a0(scene.scenario).status==GH_TASK_OK);
 if(host->r100_exit.status)fprintf(stderr,"Door request frontier %08x status %u\n",
  host->r100_exit.frontier,host->r100_exit.status);
 CHECK(!host->r100_exit.pending&&!host->r100_exit.status&&scene.door->target_room==3);
 CHECK(host->event_reserve.destination_room==0x105&&host->event_reserve.flags==11);
 memcpy(scene.title_player->view.bytes+0x80,previous,sizeof(previous));
 fprintf(stderr,"A38 completed: two RoomSave deaths, Door room0003, deferred room0105\n");
}

/* A38 has already published an ordinary Door request. Only the two
 * original State4 producers needed to finish the map and handoff run in
 * this fixture window. State5 and the target State3/4 stay unmodified. */
static int finish_door_fixture(void *context,gh_game_loop *loop)
{
 gh_startup_host *host=context;
 if(host->work.room!=0x100)return update_state4(context,loop);
 uint32_t frontier=0;
 static const uint32_t leaves[]={0x1601d8,0x1ffe10};
 for(unsigned i=0;i<2;i++){
  gh_scene_step_status status=scene_consumer(host,leaves[i],0,&frontier);
  if(status)return entry_frontier(host,frontier?frontier:leaves[i],0,status);
 }
 return 0;
}

int main(int argc,char **argv)
{
 CHECK(argc==5);
 char *end;
 unsigned long expected=strtoul(argv[4],&end,16);
 CHECK(!*end&&expected<=UINT32_MAX);
 gh_startup_host *host;
 CHECK(!gh_startup_host_create_with_profile(argv[1],argv[3],1,0,64*1024*1024,&host));
 fixture_sink sink={0};
 CHECK(configure_cpu_sink(host,&sink,argv[2]));

 size_t elf_size=(size_t)gh_disc_boot_elf_size(host->disc);
 uint8_t *elf=malloc(elf_size);
 gh_native_input_map mapping;
 CHECK(elf&&!gh_disc_read_boot_elf(host->disc,0,elf,elf_size)&&
  gh_native_input_map_from_elf(elf,elf_size,1,&mapping));
 gh_native_input qa={0};
 gh_native_input_config config;
 gh_native_input_defaults(&config);
 CHECK(gh_native_input_init(&qa,&config,mapping));
 gh_native_input_focus(&qa,1);
 unsigned boot_choice=0,previous_room=0,previous_state=255;
 int opening_seen=0,requested=0,map_bgm_verified=0,state5_seen=0;
 unsigned previous_map=255;
 uint8_t incoming_serial=0;
 int saw_r003_setup=0;
 unsigned r003_world_start=0,r003_actor_start=0;
 gh_startup_snapshot snapshot={0};
 CHECK(!gh_startup_host_snapshot(host,&snapshot));
 while(snapshot.ticks<5000&&snapshot.phase==GH_HOST_RUNNING){
  unsigned key=0;
  if(snapshot.mc_phase==3&&snapshot.mc_subphase==1)key='Z';
  else if(snapshot.mc_phase==3&&snapshot.mc_subphase==2){
   if(boot_choice==0){key=39;boot_choice++;}
   else if(boot_choice==1){key='Z';boot_choice++;}
  }else boot_choice=0;
  if(snapshot.title_live){
   if(snapshot.title_mode==1&&snapshot.ticks>=180&&snapshot.ticks%30==0)key='Z';
   else if(snapshot.title_mode==3&&snapshot.title_phase>=1&&snapshot.title_phase<=3&&
    snapshot.title_subphase==1&&(snapshot.title_phase==1||snapshot.title_timer>=5)&&snapshot.ticks%6==0)key='Z';
  }
  gh_native_input_key(&qa,'Z',key=='Z');
  gh_native_input_key(&qa,39,key==39);
  gh_native_gamepad_sample pad={0};
  gh_native_input_snapshot input={0};
  CHECK(gh_native_input_tick_device(&qa,snapshot.work,&pad,&input));
  if(requested&&host->game_children){
   const uint8_t *manager;
   size_t bytes;
   CHECK(!gh_game_children_manager_view(host->game_children,&manager,&bytes)&&bytes>0xee8);
   uint16_t map_phase;
   memcpy(&map_phase,manager+0x50,2);
   if(map_phase!=previous_map){
    fprintf(stderr,"Save map phase %u tick %llu\n",map_phase,(unsigned long long)snapshot.ticks);
    previous_map=map_phase;
   }
   gh_save_map_view map;
   CHECK(!gh_game_children_save_map_view(host->game_children,&map));
   if(map.active&&map_phase==5){
    if(map.selected==0)input.repeat_menu=UINT64_C(0x2000002000000);
    else if(map.selected==1)input.pressed|=UINT64_C(0x10000000);
   }
  }
  CHECK(!gh_startup_host_set_input(host,&input));
  (void)gh_startup_host_tick(host);
  CHECK(!gh_startup_host_snapshot(host,&snapshot));
  if(snapshot.current_room!=previous_room||snapshot.game_state!=previous_state){
   previous_room=snapshot.current_room;previous_state=snapshot.game_state;
   if(previous_room==3&&previous_state==3&&!saw_r003_setup){
    incoming_serial=host->factory_serial;
    saw_r003_setup=1;
   }
   if(previous_room==3&&previous_state==4){
    r003_world_start=sink.world;
    r003_actor_start=sink.actor;
   }
   fprintf(stderr,"room %04x state %u tick %llu\n",previous_room,previous_state,(unsigned long long)snapshot.ticks);
  }
  if(sink.movie&&!host->movie_video.complete)Sleep(15);
  if(requested&&snapshot.current_room==0x100&&snapshot.game_state==5)state5_seen=1;
  if(requested&&!map_bgm_verified&&host->game_children){
   gh_save_map_view map;
   CHECK(!gh_game_children_save_map_view(host->game_children,&map));
   if(map.active){
    int32_t event;
    CHECK(gh_bgm_requests_scene_event(host->bgm,&event)&&event==2);
    map_bgm_verified=1;
   }
  }
  if(snapshot.current_room==0x100&&snapshot.event_flags&1)opening_seen=1;
  if(!requested&&opening_seen&&snapshot.current_room==0x100&&snapshot.game_state==4&&
     !(snapshot.event_flags&1)&&snapshot.scene_frame_ready){
   request_r003(host);
   requested=1;
   host->game_platform.states[4]=finish_door_fixture;
  }
  if(snapshot.current_room==3&&snapshot.game_state==4&&snapshot.scene_frame_ready)break;
 }
 fprintf(stderr,"terminal tick %llu room %04x next %04x state %u stage %u entry %08x detail %d world %u actor %u movie %u\n",
  (unsigned long long)snapshot.ticks,snapshot.current_room,snapshot.next_room,snapshot.game_state,
  snapshot.scene_stage,snapshot.unavailable_entry,snapshot.subsystem_error,sink.world,sink.actor,sink.movie);
 fprintf(stderr,"MC %u/%u title %u/%u/%u\n",snapshot.mc_phase,snapshot.mc_subphase,snapshot.title_mode,snapshot.title_phase,snapshot.title_subphase);
 CHECK(requested&&state5_seen&&map_bgm_verified&&sink.movie==90);
 fprintf(stderr,"last Move %u entry %08x enemy stamp %u dep %04x phase %u variant %u\n",
  snapshot.last_move_handle,snapshot.last_move_entry,snapshot.current_enemy_stamp,
  snapshot.current_enemy_dependency,snapshot.current_enemy_phase,snapshot.current_enemy_variant);
 fprintf(stderr,"State5 cursor phase=%u operation=%u frontier=%08x\n",
  host->scene_exit.phase,host->scene_exit.operation,host->scene_exit.frontier);
 gh_game_services_scene_view final_scene;
 CHECK(!gh_game_services_get_scene_view(host->services,&final_scene));
 fprintf(stderr,"Door step%u verify%u save%u frontier%08x fade%u flags5D8=%08x flags5E4=%08x\n",
 final_scene.door->step_stage,final_scene.door->verify_phase39,final_scene.door->save_prompt_stage,final_scene.door->frontier,
 host->fade.state,*(uint32_t*)((uint8_t*)&host->work+0x5d8),*(uint32_t*)((uint8_t*)&host->work+0x5e4));
 gh_game_services_snapshot native;
 gh_game_services_inspect(host->services,&native);
 if(snapshot.current_room==3&&snapshot.game_state==3){
  gh_om_placement_record row={0};
  (void)gh_om_scene_record_at(final_scene.om_placements,final_scene.om_placements->cursor,&row);
  fprintf(stderr,"R003 setup service frontier%08x detail%d OMS cursor%u count%u kind%04x\n",
   native.frontier,native.detail,final_scene.om_placements->cursor,final_scene.om_placements->count,row.type);
 }
 if(expected){
  CHECK(snapshot.phase==GH_HOST_FRONTIER&&snapshot.unavailable_entry==expected);
  if(expected==0x1f83e8){
   CHECK(saw_r003_setup&&snapshot.current_room==3&&snapshot.game_state==4&&snapshot.scene_stage==11);
   CHECK(final_scene.meshes->count==146&&final_scene.om_placements->created==25);
   CHECK(final_scene.meshes->factory_serial==&host->factory_serial&&
    host->factory_serial==(uint8_t)(incoming_serial+146+2+25));
   CHECK(final_scene.title_player->ready&&final_scene.pools->initialized&&
    host->event_reserve.destination_room==0x105);
   fprintf(stderr,"R003 setup complete: 146 scroll, 25 OMS, canonical PL00/PL01 and serial %u -> %u\n",
    incoming_serial,host->factory_serial);
  }
  if(expected==0x2cb340){
   CHECK(snapshot.current_room==0x100&&snapshot.next_room==3&&snapshot.game_state==5&&
    host->scene_exit.phase==5&&host->scene_exit.operation==3);
   const gh_stage_slots *global=&host->game_stage.global_slots;
   CHECK(global->count==80&&global->records);
   for(unsigned i=0;i<global->count;i++){
    const uint8_t *row=global->records+i*0x5c;
    uint32_t state,count,token;
    memcpy(&state,row,4);memcpy(&count,row+4,4);memcpy(&token,row+0x10,4);
    if(!(count&0x80000000))CHECK(state==8&&!token);
   }
   fprintf(stderr,"all nonpinned global source allocations released before room sound-slot boundary\n");
  }
  if(expected==0x2a9680){
   CHECK(snapshot.current_room==0x100&&snapshot.next_room==3&&snapshot.game_state==5&&
    host->scene_exit.phase==5&&host->scene_exit.operation==2);
   CHECK(!host->game_init.enemy_top&&!host->game_init.enemy_last);
   CHECK(!final_scene.pools->vectors[0].count&&!final_scene.pools->vectors[3].count);
   fprintf(stderr,"all physical State5 actor retirements completed; both registries and enemy manager empty\n");
  }
  if(expected==0x2ad108){
   CHECK(snapshot.current_room==0x100&&snapshot.next_room==3&&snapshot.game_state==5&&
    host->scene_exit.phase==1&&host->scene_exit.operation==18);
   for(uint32_t handle=251;handle<=252;handle++){
    gh_title_player_view marked={0};
    uint32_t flags;
    CHECK(!gh_game_services_actor_view(host->services,handle,&marked)&&marked.bytes);
    memcpy(&flags,marked.bytes+0x250,4);
    CHECK((flags&0x80000002)==0x80000002);
   }
   /* 2AD108 is independent ID compaction. Invoke just the later ordered
    * 30F8B0 visits in this focused check while Main remains stopped. */
   uint32_t frontier=0,blocked=0;
   for(unsigned i=0;i<3&&final_scene.pools->vectors[0].count;i++){
    gh_game_services_status status=gh_game_services_scene_retire_objects_0030f8b0(
     host->services,&frontier,&blocked);
    if(status)fprintf(stderr,"later retirement visit %u status %u handle %u frontier %08x\n",i,status,blocked,frontier);
    CHECK(!status);
   }
   CHECK(!final_scene.pools->vectors[0].count&&!final_scene.pools->vectors[3].count);
   for(uint32_t handle=251;handle<=252;handle++){
    gh_title_player_view gone={0};
    CHECK(gh_game_services_actor_view(host->services,handle,&gone)!=GH_SERVICES_OK||!gone.bytes);
   }
   fprintf(stderr,"retired canonical stamp18 and held0363, both registries empty\n");
  }
  if(expected==0x1b79b0){
   CHECK(snapshot.current_room==0x100&&snapshot.next_room==3&&snapshot.game_state==5&&
    host->scene_exit.phase==1&&host->scene_exit.operation==10);
   gh_title_player_view armed;
   uint16_t stamp,kind;
   CHECK(!gh_game_services_actor_view(host->services,251,&armed)&&armed.size>=0x642);
   memcpy(&stamp,armed.bytes+0x640,2);memcpy(&kind,armed.bytes+0x2fe,2);
   CHECK(stamp==18&&kind==0x202);
   fprintf(stderr,"retirement owner handle251 kind%04x stamp%u\n",kind,stamp);
  }
 }else{
  CHECK(saw_r003_setup&&snapshot.current_room==3&&snapshot.game_state==4&&snapshot.scene_frame_ready&&
   snapshot.phase==GH_HOST_RUNNING&&!snapshot.unavailable_entry);
  CHECK(final_scene.meshes->count==146&&final_scene.om_placements->created==25&&
   final_scene.meshes->factory_serial==&host->factory_serial);
  CHECK(sink.world-r003_world_start==258&&sink.actor-r003_actor_start==75);
  gh_bgm_request_view voice;
  CHECK(!gh_bgm_requests_inspect(host->bgm,0,&voice)&&voice.linked&&voice.event==2&&voice.mode==1);
  uint32_t target,fade;
  memcpy(&target,voice.original_scalars+0x18,4);
  memcpy(&fade,voice.original_scalars+0x1c,4);
  CHECK(!target&&fade==0x41f00000);
  puts("PASS complete R003 first scene commands and original 30-frame BGM suppression");
 }
 free(elf);
 puts("PASS actual R100 opening, A38 and Door map Continue, scoped State5 frontier");
 return 0;
}
