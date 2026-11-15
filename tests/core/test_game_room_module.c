#include "game_room_module.h"
#include "scene_at_units.h"
#include "game_enemy_row2.h"
#include "game_enemy_request.h"
#include "game_om30e.h"
#include <stdio.h>
#include <stdlib.h>

#define CHECK(x) do{if(!(x)){fprintf(stderr,"line%d %s\n",__LINE__,#x);exit(1);}}while(0)

typedef struct {
 gh_room_module *module;
 uint16_t room;
 int returned;
 gh_room_module_status result;
} context;

static void load(void *opaque,int32_t argument)
{
 context *c=opaque;
 (void)argument;
 c->result=gh_room_module_load_002be708(c->module,c->room);
 c->returned=1;
}

int main(int argc,char **argv)
{
 static const struct {uint16_t room;size_t bytes;uint32_t init,move,end;} profiles[]={
  {0x20,24129,0x1a0,0x220,0x248},{0x100,25663,0x208,0x740,0},
  {3,5005,0x1a0,0x228,0},{0x101,14284,0x1a0,0x520,0},
  {0x102,23217,0x1a8,0x670,0},{0x103,11708,0x1a0,0x250,0},{0x105,12628,0x1a8,0x398,0},
 };
 gh_disc *disc;
 gh_heap *heap;
 gh_dvd_jobs dvd;
 gh_task_threads *threads;
 gh_task_record32 records[4]={0};
 gh_task_manager manager={.records=records,.capacity=4,.count=4,.current_index=-1};
 CHECK(argc==2&&gh_disc_open(argv[1],&disc)==GH_DISC_OK&&gh_heap_create(1024*1024,&heap)==GH_MEM_OK);
 gh_dvd_jobs_init(&dvd,disc);
 CHECK(gh_task_threads_create(&manager,&threads)==GH_THREAD_OK);
 gh_room_module module={.input={.dvd=&dvd,.heap=heap,.threads=threads}};
 CHECK(gh_room_module_load_002be708(&module,0x207)==GH_ROOM_MODULE_UNSUPPORTED&&!module.bound&&!gh_heap_used(heap));

 for(size_t i=0;i<sizeof(profiles)/sizeof(profiles[0]);++i){
  for(int corrupt=1;corrupt>=0;--corrupt){
   context c={.module=&module,.room=profiles[i].room};
   int32_t slot;
   CHECK(gh_task_threads_start(threads,1,load,&c,0,&slot)==GH_THREAD_OK);
   unsigned visits=0;
   while(!c.returned&&visits<8){
    CHECK(gh_tasks_run_matching_002d52a0(&manager).status==GH_TASK_OK);
    if(!visits){
     CHECK(module.input.allocation.pointer&&module.input.job&&!module.bound);
     CHECK(gh_room_module_release(&module)==GH_ROOM_MODULE_BUSY);
    }
    CHECK(gh_dvd_pump(&dvd)==GH_DVD_OK);
    /* Mutate only this owned native buffer after the disc read. */
    if(!visits&&corrupt)((uint8_t*)module.input.allocation.pointer)[0x100]^=1;
    ++visits;
   }
   CHECK(c.returned&&visits==3&&module.input.file_bytes==profiles[i].bytes);
   if(corrupt){
    CHECK(c.result==GH_ROOM_MODULE_VERSION&&!module.bound&&module.frontier==0x394dd8);
   }else{
    CHECK(c.result==GH_ROOM_MODULE_OK&&module.bound&&module.room==profiles[i].room);
    CHECK(module.hook8.kind==GH_SCENE_HOOK_UNRESOLVED&&module.hook12.kind==GH_SCENE_HOOK_UNRESOLVED);
    CHECK(module.init_offset==profiles[i].init&&module.move_offset==profiles[i].move&&module.end_offset==profiles[i].end);
    if(module.room==0x102||module.room==0x105){
     const uint8_t *bytes=module.input.allocation.pointer;
     static const uint8_t original_preinit[]={0x08,0x00,0xe0,0x03,0,0,0,0};
     for(size_t j=0;j<sizeof(original_preinit);j++)CHECK(bytes[0x1a0+j]==original_preinit[j]);
     uint32_t frontier=0xffffffffu;
     CHECK(module.hook4.kind==GH_SCENE_HOOK_NATIVE&&module.hook4.call&&module.hook4.context==&module);
     CHECK(module.hook4.call(module.hook4.context,&frontier)==GH_SCENE_OK&&!frontier);
    }else{
     CHECK(module.hook4.kind==(module.room==0x100?GH_SCENE_HOOK_UNRESOLVED:GH_SCENE_HOOK_ABSENT)&&!module.hook4.call);
    }
    if(module.room==0x103){
     const uint8_t *bytes=module.input.allocation.pointer;
     static const uint8_t original_move[]={0x08,0x00,0xe0,0x03,0,0,0,0};
     for(size_t j=0;j<sizeof(original_move);j++)CHECK(bytes[0x250+j]==original_move[j]);
    }
    CHECK(gh_room_module_load_002be708(&module,profiles[i].room)==GH_ROOM_MODULE_BUSY);
   }
   CHECK(gh_room_module_release(&module)==GH_ROOM_MODULE_OK&&!module.bound&&!gh_heap_used(heap));
  }
 }
 /* The original R103 room pack supplies the return jump disabled by 0670. */
 gh_disc_entry room_entry;
 size_t room_index;
 CHECK(gh_disc_find(disc,"st1/r103.dat",&room_index)==GH_DISC_OK);
 CHECK(gh_disc_entry_at(disc,room_index,&room_entry)==GH_DISC_OK&&room_entry.size==439744);
 uint8_t *room=malloc((size_t)room_entry.size);
 gh_bundle bundle;
 CHECK(room&&gh_disc_read(disc,room_index,0,room,(size_t)room_entry.size)==GH_DISC_OK);
 CHECK(gh_bundle_open(room,(size_t)room_entry.size,&bundle)==GH_BUNDLE_OK);
 gh_bundle_entry original_jumps;
 CHECK(gh_bundle_find(&bundle,"JMP",0,&original_jumps)==GH_BUNDLE_OK&&original_jumps.size>=44);
 CHECK(original_jumps.bytes[0]==2&&original_jumps.bytes[32]==2&&
       original_jumps.bytes[33]==1&&original_jumps.bytes[34]==3);
 /* Original EMS admits exactly the eight live cEm00 204 entries here. */
 gh_bundle_entry placements;
 CHECK(gh_bundle_find(&bundle,"EMS",0,&placements)==GH_BUNDLE_OK);
 uint8_t page[0x1620]={0};
 gh_room_state placed={.active=page,.size=sizeof(page)};
 gh_em_scene_placements em={0};
 gh_boot_work boot={.room=0x103};
 int loaded;
 CHECK(gh_em_scene_load_native_00294b98(&em,&placed,&placements,&boot,0,
       GH_EM_NUMERIC_PCSX2_CHOPZERO_REFERENCE,&loaded)==GH_PLACE_OK&&loaded&&page[8]==10);
 const unsigned original_models[]={3,4,0,0,2,2,1,1};
 const uint32_t original_motions[]={0x413a80,0x413a80,0x1b5400,0x1b5400,
     0x31f380,0x31f380,0x2b9d80,0x2b9d80};
 gh_disc_entry enemy_entry;
 size_t enemy_index;
 CHECK(gh_disc_find(disc,"em/em04.dat",&enemy_index)==GH_DISC_OK);
 CHECK(gh_disc_entry_at(disc,enemy_index,&enemy_entry)==GH_DISC_OK&&enemy_entry.size==5274560);
 uint8_t *enemy=malloc((size_t)enemy_entry.size);
 CHECK(enemy&&gh_disc_read(disc,enemy_index,0,enemy,(size_t)enemy_entry.size)==GH_DISC_OK);
 const gh_object_view enemy_data={.bytes=enemy,.size=(size_t)enemy_entry.size,.kind=0x204,
      .file_extent=(size_t)enemy_entry.size,.file_extent_known=1};
 for(unsigned i=0;i<10;++i){
  gh_enemy_request request;
  int spawn;
  int16_t saved;
  uint32_t frontier;
  CHECK(gh_enemy_request_from_room_00294ca0(&placed,i,GH_EM_NUMERIC_PCSX2_CHOPZERO_REFERENCE,
        &request,&spawn,&saved,&frontier)==GH_PLACE_OK&&spawn==(i<8));
  if(i>=8)continue;
  CHECK(gh_enemy204_is_r103_entry(&request,0x103)&&!gh_enemy204_is_r103_entry(&request,0x102));
  unsigned model;
  CHECK(gh_enemy_model_occurrence_002ddea0(request.object_kind,request.dependency_kind,&model)==GH_ACTOR_MODEL_OK&&model==original_models[i]);
  gh_enemy_initial_motion_selection animation;
  CHECK(gh_enemy204_motion_select_002036b0(&request,&enemy_data,&animation)==GH_ACTOR_MODEL_OK&&
        animation.motion_offset==original_motions[i]&&animation.substate==(i<2?0x7d:1));
  request.dependency_flags=1;
  CHECK(!gh_enemy204_is_r103_entry(&request,0x103));
 }
 free(enemy);
 gh_bundle_entry objects;
 CHECK(gh_bundle_find(&bundle,"OMS",0,&objects)==GH_BUNDLE_OK);
 gh_om_scene_placements props={0};
 int original_placement;
 CHECK(gh_om_scene_bind_001c3cb8(&props,&objects,&original_placement)==GH_PLACE_UNSUPPORTED);
 for(unsigned i=8;i<=9;++i){
  gh_om_placement_record prop;
  CHECK(gh_om_scene_record_at(&props,i,&prop)==GH_PLACE_OK&&prop.type==0x326);
 }
 uint8_t object[0x610]={0};
 gh_title_player_view view;
 CHECK(gh_om326_construct_00199dc0(object,sizeof(object)-1,&view)==GH_PLACE_BOUNDS);
 CHECK(gh_om326_construct_00199dc0(object,sizeof(object),&view)==GH_PLACE_OK&&view.original_vtable214==0x4251c0);
 gh_scene_at_manager at={0};
 CHECK(gh_scene_at_initialize_units_002c0f90(&at,&bundle,heap,0x103,1)==GH_AT_OK);
 gh_at_native_unit jump;
 CHECK(gh_scene_at_get_jump_002c2ae0(&at,1,&jump)==GH_AT_OK&&jump.bytes&&(jump.bytes[0x34]&1));
 int original;
 CHECK(gh_scene_at_set_enabled(&at,jump,0,&original)==GH_AT_OK&&original&&!(jump.bytes[0x34]&1));
 CHECK(gh_scene_at_units_release(&at)==GH_AT_OK&&!gh_heap_used(heap));
 free(room);
 CHECK(gh_task_threads_destroy(threads)==GH_THREAD_OK&&gh_heap_destroy(heap)==GH_MEM_OK);
 gh_disc_close(disc);
 puts("PASS actual R020/R100/R003/R101/R102/R103/R105 module identity and release, corruption rejection, original R102/R105 PreInit; R103 Move and return jump from disc");
 return 0;
}
