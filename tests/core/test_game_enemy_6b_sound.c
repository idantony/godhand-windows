#include "game_actor_attached_sound.h"
#include "game_enemy_factory.h"
#include "game_enemy_model.h"
#include "enemy_se_banks.h"
#include "game_scene_placements.h"
#include "game_enemy_ems.h"
#include "game_scene_tables.h"
#include "game_ray_query.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fenv.h>
#define C(x) do{if(!(x)){fprintf(stderr,"line%d %s\n",__LINE__,#x);exit(1);}}while(0)
static uint8_t *read_file(gh_disc*d,const char*p,size_t*n){size_t i;gh_disc_entry e;C(!gh_disc_find(d,p,&i)&&!gh_disc_entry_at(d,i,&e));*n=(size_t)e.size;uint8_t*b=malloc(*n);C(b&&!gh_disc_read(d,i,0,b,*n));return b;}
typedef struct {gh_sound_scene*s;gh_bundle*b;gh_boot_work*w;gh_dvd_jobs*d;gh_heap*h;gh_task_threads*t;int done;} loader;
static void load(void*p,int32_t unused){loader*l=p;(void)unused;C(!gh_sound_scene_initialize_002cabe8(l->s,l->b,l->w,l->d,l->h,l->t));l->done=1;}
static void store(void*p,float f){memcpy(p,&f,4);}
/* GNU link wrapping records calls from unchanged production se_voice.
 * No implementation here initializes a device or claims samples played. */
typedef struct {float left,right;size_t frames;int live;} recorded_voice;
static recorded_voice submitted[32];
static unsigned submissions,gain_updates,stops,pitch_updates;
static char command_order[256];
static unsigned command_count;
static uint16_t latest_target,latest_submitted;
static max_align_t transport;
gh_voice_status __wrap_gh_audio_output_play(gh_audio_output *output,const gh_adpcm_pcm *pcm,
 uint32_t rate,float left,float right,uint32_t *handle)
{
 C(output==(gh_audio_output *)&transport&&pcm&&pcm->samples&&pcm->frames&&rate==48000);
 C(submissions<32&&left>=0&&right>=0);
 size_t nonzero=0;
 for(size_t i=0;i<pcm->frames;++i)nonzero+=pcm->samples[i]!=0;
 C(nonzero>0);
 submitted[submissions]=(recorded_voice){left,right,pcm->frames,1};
 *handle=++submissions;
 return GH_VOICE_OK;
}
gh_voice_status __wrap_gh_audio_output_active(gh_audio_output *output,uint32_t handle,
 int *active,uint64_t *played)
{
 C(output==(gh_audio_output *)&transport&&handle&&handle<=submissions);
 *active=submitted[handle-1].live;*played=0;
 return GH_VOICE_OK;
}
gh_voice_status __wrap_gh_audio_output_stop(gh_audio_output *output,uint32_t handle)
{
 C(output==(gh_audio_output *)&transport&&handle&&handle<=submissions&&submitted[handle-1].live);
 submitted[handle-1].live=0;++stops;
 return GH_VOICE_OK;
}
gh_voice_status __wrap_gh_audio_output_voice_gain(gh_audio_output *output,uint32_t handle,
 float left,float right)
{
 C(output==(gh_audio_output *)&transport&&handle&&handle<=submissions&&submitted[handle-1].live);
 submitted[handle-1].left=left;submitted[handle-1].right=right;++gain_updates; C(command_count<sizeof(command_order)); command_order[command_count++]='G';
 return GH_VOICE_OK;
}

gh_voice_status __wrap_gh_audio_output_voice_pitch_ratio(gh_audio_output *output,uint32_t handle,
 uint16_t target,uint16_t submitted_pitch)
{
 C(output==(gh_audio_output *)&transport&&handle&&handle<=submissions&&submitted[handle-1].live);
 C(target&&submitted_pitch&&command_count<sizeof(command_order));
 command_order[command_count++]='P';
 ++pitch_updates;
 latest_target=target;
 latest_submitted=submitted_pitch;
 return GH_VOICE_OK;
}

static void check_program(gh_audio_registry *registry,int16_t bank,int16_t program,
 unsigned slot,unsigned rate,size_t frames)
{
 uint8_t row[48];
 gh_sound_span descriptor,tail;
 gh_adpcm_pcm pcm={0};
 C(!gh_audio_get_program(registry,bank,program,0,row)&&row[4]==slot&&!row[5]);
 C(!gh_audio_sample_at(registry,bank,slot,&descriptor,&tail)&&descriptor.size==16);
 uint32_t actual_rate;
 memcpy(&actual_rate,descriptor.data+4,4);
 C(actual_rate==rate&&descriptor.data[8]==0xff&&descriptor.data[9]==0x80&&
   descriptor.data[10]==0xcb&&descriptor.data[11]==0x5f);
 C(!gh_audio_decode_ps_adpcm(tail.data,tail.size,&pcm)&&pcm.frames==frames);
 for(size_t block=0;block<pcm.source_bytes;block+=16)
  C(tail.data[block+1]==(block+16==pcm.source_bytes?1:0));
 gh_audio_pcm_release(&pcm);
}

int main(int argc,char **argv)
{
 C(argc==2);
 gh_disc *disc;
 gh_audio_registry *registry;
 C(!gh_disc_open(argv[1],&disc)&&!gh_audio_registry_create(&registry)&&!gh_audio_startup_load(registry,disc));
 gh_sound_control control={0};
 gh_sound_scene scene={0};
 C(!gh_sound_control_from_startup(&control,disc,registry));
 C(!gh_sound_scene_from_startup(&scene,&control,registry));
 gh_heap *heap;
 C(!gh_heap_create(32*1024*1024,&heap));
 gh_dvd_jobs dvd;
 gh_dvd_jobs_init(&dvd,disc);
 gh_se_mix mix;
 gh_se_voice_engine *voices;
 C(!gh_se_mix_boot_defaults_002c9e78(&mix,0,GH_SE_DSP_NATIVE_LINEAR_REFERENCE));
 C(!gh_se_voice_create(registry,(gh_audio_output *)&transport,&mix,&voices));
 C(!gh_se_voice_bind_pitch_reference(voices,disc));
 gh_boot_work work={.room=0x100};
 gh_enemy_se_banks *banks;
 gh_enemy_se_bindings bank_bind={.control=&control,.registry=registry,.disc=disc,.dvd=&dvd,
     .default_heap=heap,.room_heap=heap,.voices=voices};
 C(!gh_enemy_se_banks_from_startup(&bank_bind,&banks));
 size_t room_bytes,em_bytes;
 uint8_t *room=read_file(disc,"st1/r100.dat",&room_bytes);
 uint8_t *resource_bytes=read_file(disc,"em/em02.dat",&em_bytes);
 gh_bundle room_bundle;
 C(!gh_bundle_open(room,room_bytes,&room_bundle));
 gh_game_scene_tables tables={0};
 C(!gh_scene_actors_0012e670(&tables,&room_bundle));
 gh_room_pools pools={0};
 C(!gh_game_room_pools_initialize_002a5d80(&pools,&room_bundle,heap));
 gh_task_record32 records[4]={0};
 gh_task_manager manager={.records=records,.capacity=4,.count=4,.current_index=-1};
 gh_task_threads *threads;
 C(!gh_task_threads_create(&manager,&threads));
 loader scene_load={&scene,&room_bundle,&work,&dvd,heap,threads,0};
 int32_t slot;
 C(!gh_task_threads_start(threads,2,load,&scene_load,0,&slot));
 for(unsigned i=0;!scene_load.done&&i<16;++i)
  C(!gh_tasks_run_matching_002d52a0(&manager).status&&!gh_dvd_pump(&dvd));
 C(scene_load.done&&scene.ready&&!scene.seh.bytes&&!scene.seh.size);

 gh_bundle_entry ems;
 C(!gh_bundle_find(&room_bundle,"EMS",0,&ems));
 uint8_t save_bytes[GH_CORE_SAVE_BYTES]={0},game_state=0,stop=0;
 gh_core_save_view save={save_bytes,sizeof(save_bytes)};
 gh_game_prepare prepare={0};
 gh_room_state selected_room;
 gh_em_scene_placements placement={.stop_flag=&stop};
 int loaded;
 C(!gh_core_save_system_init(save)&&!gh_game_prepare_002a5d40(&prepare,save,0x100,&game_state)&&
   !gh_room_state_select_002bebc8(&prepare,0x100,0,&selected_room)&&
   !gh_em_scene_load_native_00294b98(&placement,&selected_room,&ems,&work,0,
       GH_EM_NUMERIC_PCSX2_CHOPZERO_REFERENCE,&loaded)&&loaded);

 gh_enemy_request req;
 unsigned physical=0;
 for(;physical<64;++physical){
  int spawn;
  int16_t saved;
  uint32_t frontier;
  C(!gh_enemy_request_from_room_00294ca0(&selected_room,physical,GH_EM_NUMERIC_PCSX2_CHOPZERO_REFERENCE,
     &req,&spawn,&saved,&frontier));
  if(spawn&&req.entry_number==20)break;
 }
 C(physical<64&&req.object_kind==0x202&&req.dependency_kind==0x200);
 fprintf(stdout,"EMS physical row %u, stamp %u, appearance %u, dependency %03X\n",
     physical,req.entry_number,req.appearance,req.dependency_kind);
 int32_t bank,resolved;
 C(!gh_enemy_se_request_002cb278(banks,&work,req.dependency_kind,&bank)&&bank>=1);
 int ready=0;
 for(unsigned n=0;n<16&&!ready;++n){
  C(!gh_dvd_pump(&dvd)&&!gh_enemy_se_banks_update_002cf440(banks));
  C(!gh_enemy_se_ready_002cb778(banks,bank,&ready));
 }
 C(ready&&!gh_enemy_se_resolve_002cb418(banks,req.dependency_kind,&resolved)&&resolved==bank);
 check_program(registry,(int16_t)bank,0x0a,9,11025,12628);
 check_program(registry,(int16_t)bank,0x0b,10,11025,9772);

 gh_game_title_player enemy={0};
 int original;
 C(!gh_enemy202_allocate_003130e0(&enemy,heap,&work,&original)&&original);
 memcpy(enemy.view.bytes+0x2fe,&(uint16_t){0x202},2);
 uint32_t dep=req.dependency_kind;
 memcpy(enemy.view.bytes+0x564,&dep,4);
 gh_object_view resource={.bytes=resource_bytes,.kind=0x202,.size=em_bytes,
      .file_extent=em_bytes,.file_extent_known=1};
 gh_enemy_model model={0};
 uint32_t fog=0;
 C(!gh_enemy_model_initialize_00208268(&model,&enemy.view,&resource,&work,&pools,heap,&fog,
    GH_STATIC_BINARY32_REFERENCE,&original)&&original);
 store(enemy.view.bytes+0x5a8,1);
 gh_game_mt random;
 gh_game_mt_seed_0030f2b0(&random,1);
 gh_game_mt expected=random;
 gh_actor_attached_sound source={0};
 gh_actor_attached_sound_bindings binding={voices,&control,&scene,&work,&random};
 C(!gh_actor_attached_sound_bind(&source,&binding));
 C(!gh_actor_attached_sound_bind_scene_tables(&source,&tables));
 float anchor[4];
 memcpy(anchor,enemy.view.alias_f0,sizeof(anchor));
 float listener[4]={anchor[0]-3,anchor[1]+1.3f,anchor[2],1};
 float matrix[16]={1,0,0,0,0,1,0,0,0,0,1,0,-listener[0],-listener[1],-listener[2],1};
 memcpy(control.listener_position_c0,listener,sizeof(listener));
 memcpy(control.listener_matrix_d0,matrix,sizeof(matrix));
 control.listener_mode110=1;
 for(unsigned i=0;i<2;++i){
  gh_motion_sound_request request={.model=&model.model,.actor=&enemy.view,.bank=1,.program=(int16_t)(0x0a+i)};
  uint32_t handle=0,frontier=0,duplicate=99;
  C(!gh_actor_attached_sound_motion(&source,&request,&handle,&frontier)&&!frontier&&handle);
  C(!memcmp(&random,&expected,sizeof(random))); /* No MT variation in either row. */
  C(!gh_actor_attached_sound_motion(&source,&request,&duplicate,&frontier)&&!frontier&&!duplicate);
  C(!gh_se_voice_attached_reservations(voices)&&model.model.sound_borrowers==i+1);
  gh_voice_status status=gh_se_voice_update(voices);
  if(status)fprintf(stderr,"voice update %u frontier %08x\n",status,gh_se_voice_frontier(voices));
  C(!status);
  gh_se_voice_info info;
  C(!gh_se_voice_query(voices,handle,&info)&&!info.pending&&info.native_voice&&
    info.position_mode==2&&info.distance==3&&info.pitch_q12&&info.sample_slot==9+i&&
    info.channel>=45&&info.channel<=47&&info.adsr2==0x5fcb);
  C(submissions==i+1&&submitted[i].frames>(i?9772u:12628u));
  C(info.left_q14||info.right_q14);
  fprintf(stdout,"EM00 cue %02X slot %u pitch %u channel %u PCM %zu\n",request.program,
      info.sample_slot,info.pitch_q12,info.channel,submitted[i].frames);
 }
 C(!gh_actor_attached_sound_abort(&source)&&submissions==2&&stops==2);

 const size_t attack_frames[]={4312,3388,5684,9044};
 for(int16_t program=4;program<=6;program+=2){
  for(unsigned variant=0;variant<2;++variant){
   const int16_t actual=(int16_t)(program+variant);
   check_program(registry,(int16_t)bank,actual,actual-1,11025,attack_frames[actual-4]);
   gh_game_mt_seed_0030f2b0(&random,variant?1:2);
   expected=random;
   uint32_t roll;
   C(!gh_game_mt_next_0030f348(&expected,&roll)&&(roll&1u)==variant);
   C(!gh_actor_attached_sound_bind(&source,&binding));
   C(!gh_actor_attached_sound_bind_scene_tables(&source,&tables));
   gh_motion_sound_request attack={.model=&model.model,.actor=&enemy.view,.bank=1,.program=program};
   uint32_t handle=0,frontier=0;
   C(!gh_actor_attached_sound_motion(&source,&attack,&handle,&frontier)&&handle&&!frontier);
   C(!memcmp(&random,&expected,sizeof(random)));
   const unsigned before=submissions;
   if(actual==6){
    control.listener_mode110=2;
    C(gh_se_voice_update(voices)==GH_VOICE_UNSUPPORTED&&gh_se_voice_frontier(voices)==0x2cf058);
    gh_se_attached_failure failure;
    gh_se_voice_last_attached_failure(voices,&failure);
    C(failure.during_update&&failure.frontier==0x2cf058&&failure.actor_kind==0x202&&
      failure.actor_dependency==0x200&&failure.bank==bank&&failure.program==actual&&
      failure.position_mode==2&&failure.status==GH_VOICE_UNSUPPORTED&&submissions==before);
    control.listener_mode110=1;
   }
   C(!gh_se_voice_update(voices));
   gh_se_voice_info info;
   C(!gh_se_voice_query(voices,handle,&info)&&!info.pending&&info.native_voice&&
     info.bank==bank&&info.program==actual&&info.sample_slot==(uint32_t)(actual-1)&&info.position_mode==2&&
     info.rate==11025&&info.adsr2==0x5fcb&&info.channel>=42&&info.channel<=44);
   C(submissions==before+1&&submitted[before].frames);
   C(!gh_actor_attached_sound_abort(&source)&&stops==submissions);
  }
 }

 const int16_t core_attack[]={0x83,0x84,0x85,0x87};
 for(unsigned i=0;i<sizeof(core_attack)/sizeof(core_attack[0]);++i){
  const int16_t program=core_attack[i];
  const unsigned before=submissions;
  expected=random;
  C(!gh_actor_attached_sound_bind(&source,&binding));
  C(!gh_actor_attached_sound_bind_scene_tables(&source,&tables));
  gh_motion_sound_request attack={.model=&model.model,.actor=&enemy.view,.bank=0,.program=program};
  uint32_t handle=0,frontier=0;
  C(!gh_actor_attached_sound_motion(&source,&attack,&handle,&frontier)&&handle&&!frontier);
  C(!memcmp(&random,&expected,sizeof(random))&&!gh_se_voice_update(voices));
  gh_se_voice_info info;
  C(!gh_se_voice_query(voices,handle,&info)&&!info.pending&&info.native_voice&&
    info.bank==0&&info.program==program&&info.sample_slot==(uint32_t)(program-0x2d)&&info.position_mode==2&&
    info.rate==16000&&info.adsr2==0x5fc0&&info.channel>=11&&info.channel<=13);
  C(submissions==before+1&&submitted[before].frames);
  C(!gh_actor_attached_sound_abort(&source)&&stops==submissions);
 }

 int spawn;
 int16_t saved;
 uint32_t frontier=0;
 C(!gh_enemy_request_from_room_00294ca0(&selected_room,14,GH_EM_NUMERIC_PCSX2_CHOPZERO_REFERENCE,
     &req,&spawn,&saved,&frontier)&&spawn&&req.entry_number==19&&req.dependency_kind==0x203);
 dep=req.dependency_kind;
 memcpy(enemy.view.bytes+0x564,&dep,4);
 C(!gh_actor_attached_sound_bind(&source,&binding));
 C(!gh_actor_attached_sound_bind_scene_tables(&source,&tables));
 gh_motion_sound_request shared_attack={.model=&model.model,.actor=&enemy.view,.bank=0,.program=0x83};
 uint32_t shared_handle=0;
 C(!gh_actor_attached_sound_motion(&source,&shared_attack,&shared_handle,&frontier)&&shared_handle&&!frontier);
 const unsigned before_shared=submissions;
 C(!gh_se_voice_update(voices)&&submissions==before_shared+1&&submitted[before_shared].frames);
 gh_se_voice_info shared_info;
 C(!gh_se_voice_query(voices,shared_handle,&shared_info)&&shared_info.program==0x83&&
   shared_info.sample_slot==86&&shared_info.native_voice&&shared_info.position_mode==2);
 C(!gh_actor_attached_sound_abort(&source)&&stops==submissions);

 C(!gh_actor_attached_sound_bind(&source,&binding));
 C(!gh_actor_attached_sound_bind_scene_tables(&source,&tables));
 gh_motion_sound_request turn={.model=&model.model,.actor=&enemy.view,.bank=0,.program=0x8d};
 uint32_t handle=0,duplicate=99;
 const unsigned before_turn=submissions;
 C(!gh_actor_attached_sound_motion(&source,&turn,&handle,&frontier)&&handle&&!frontier);
 C(!gh_actor_attached_sound_motion(&source,&turn,&duplicate,&frontier)&&!duplicate&&!frontier);
 C(!gh_se_voice_update(voices));
 gh_se_voice_info voice;
 C(!gh_se_voice_query(voices,handle,&voice)&&voice.bank==0&&voice.program==0x8d&&
    voice.sample_slot==92&&voice.rate==16000&&voice.native_voice&&voice.adsr2==0x5fc0);
 C(submissions==before_turn+1&&submitted[before_turn].frames);
 C(!gh_actor_attached_sound_abort(&source)&&stops==submissions);
 C(!gh_enemy_model_release_prefix(&model)&&!gh_game_title_player_release(&enemy));
 C(!gh_enemy_se_banks_destroy(banks));
 gh_se_voice_destroy(voices);
 C(!gh_game_room_pools_release(&pools)&&!gh_task_threads_destroy(threads)&&!gh_heap_destroy(heap));
 C(!gh_scene_actors_reset_0012e800(&tables));
 gh_audio_registry_destroy(registry);
 gh_disc_close(disc);
 free(resource_bytes);
 free(room);
 puts("PASS actual R100 EM00 selector6B and selector15 attached cues, MT, and PCM commands");
 return 0;
}
