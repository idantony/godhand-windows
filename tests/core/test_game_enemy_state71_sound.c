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
 uint32_t sequence_offset;
 C(em_bytes>=0x424);
 memcpy(&sequence_offset,resource_bytes+0x420,4);
 C(sequence_offset==0x2154c0&&sequence_offset<=em_bytes&&em_bytes-sequence_offset>=0x1b);
 const uint8_t *sequence=resource_bytes+sequence_offset;
 C(!memcmp(sequence,"SEQ",4)&&sequence[0x1a]==1);
 const size_t sound_offset=sequence[12]|(unsigned)sequence[13]<<8;
 const unsigned sound_count=sequence[14]|(unsigned)sequence[15]<<8;
 C(sound_offset<=em_bytes-sequence_offset&&sound_count<=(em_bytes-sequence_offset-sound_offset)/12);
 unsigned sequence28=0;
 for(unsigned i=0;i<sound_count;++i){
  const uint8_t *row=sequence+sound_offset+12*i;
  if(row[0]==50&&!row[1]&&row[2]==0x28&&!row[3]&&!row[4]&&!row[5]&&row[8]==1)++sequence28;
 }
 C(sequence28==1);
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

 const unsigned rows[]={2,3,4,0,14,15};
 for(unsigned i=0;i<sizeof(rows)/sizeof(*rows);++i){
  gh_enemy_request req;
  int spawn;
  int16_t saved;
  uint32_t frontier;
  C(!gh_enemy_request_from_room_00294ca0(&selected_room,rows[i],GH_EM_NUMERIC_PCSX2_CHOPZERO_REFERENCE,
       &req,&spawn,&saved,&frontier)&&spawn&&req.object_kind==0x202);
  fprintf(stdout,"EMS row %u entry %u appearance %u dep %03X\n",rows[i],req.entry_number,req.appearance,req.dependency_kind);
  if(!i)C(req.entry_number==3&&req.appearance==2&&req.dependency_kind==0x200);
  int32_t bank;
  C(!gh_enemy_se_request_002cb278(banks,&work,req.dependency_kind,&bank));
  if(rows[i]>=14)C(bank==-1&&!gh_enemy_se_resolve_002cb418(banks,req.dependency_kind,&bank));
  C(bank>=1);
  int ready=0;
  for(unsigned n=0;n<16&&!ready;++n){
   C(!gh_dvd_pump(&dvd)&&!gh_enemy_se_banks_update_002cf440(banks));
   C(!gh_enemy_se_ready_002cb778(banks,bank,&ready));
  }
  C(ready);
  int32_t resolved;
  C(!gh_enemy_se_resolve_002cb418(banks,req.dependency_kind,&resolved)&&resolved==bank);
  uint8_t program[48];
  int enabled=req.dependency_kind==0x20a;
  gh_audio_status got=gh_audio_get_program(registry,(int16_t)bank,0x11,0,program);
  C(got==(enabled?GH_AUDIO_OK:GH_AUDIO_DISABLED));
  if(enabled)check_program(registry,(int16_t)bank,0x11,12,16000,15456);

  gh_game_title_player enemy={0};
  int original;
  C(!gh_enemy202_allocate_003130e0(&enemy,heap,&work,&original)&&original);
  memcpy(enemy.view.bytes+0x2fe,&(uint16_t){0x202},2);
  uint32_t dep=req.dependency_kind;
  memcpy(enemy.view.bytes+0x640,&(uint16_t){req.entry_number},2);
  if(rows[i]==14)C(req.entry_number==19&&dep==0x203);
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
  gh_motion_sound_request request={.model=&model.model,.actor=&enemy.view,.bank=1,.program=0x11};
  const unsigned before_timed=submissions;
  float anchor[4];
  memcpy(anchor,enemy.view.alias_f0,sizeof(anchor));
  float listener[4]={anchor[0]-3,anchor[1]+1.3f,anchor[2],1};
  float matrix[16]={1,0,0,0,0,1,0,0,0,0,1,0,-listener[0],-listener[1],-listener[2],1};
  memcpy(control.listener_position_c0,listener,sizeof(listener));
  memcpy(control.listener_matrix_d0,matrix,sizeof(matrix));
  control.listener_mode110=1;
  uint32_t handle;
  C(!gh_actor_attached_sound_motion(&source,&request,&handle,&frontier)&&!frontier);
  C(!memcmp(&random,&expected,sizeof(random)));
  if(enabled){
   C(handle&&model.model.sound_borrowers==1);
   uint32_t duplicate=99;
   C(!gh_actor_attached_sound_motion(&source,&request,&duplicate,&frontier)&&!frontier&&!duplicate);
   C(!gh_se_voice_attached_reservations(voices)&&model.model.sound_borrowers==1);
   C(!gh_se_voice_update(voices));
   gh_se_voice_info info;
   C(!gh_se_voice_query(voices,handle,&info)&&!info.pending&&info.native_voice&&info.channel==35);
   C(info.position_mode==2&&info.distance==3&&!info.pan&&info.pitch_q12);
   C(info.left_q14==0x5a00*(95*80/127)/127/2&&!info.right_q14);
   C(submissions==before_timed+1&&submitted[before_timed].frames>15456);
  }else if(rows[i]<14){
   C(!handle&&!model.model.sound_borrowers&&!gh_se_voice_active_nodes(voices));
   C(!gh_se_voice_attached_reservations(voices)&&submissions==before_timed);
   /* Selector 11 uses logical program 0. Both variants have a one-frame
    * duplicate window, and their sample's pitch range is selected by SDE. */
   request.program=0;
   const gh_game_mt before_reaction=random;
   uint32_t draw;
   C(!gh_game_mt_next_0030f348(&expected,&draw));
   const int16_t actual_program=(int16_t)(draw&1u);
   C(!gh_actor_attached_sound_motion(&source,&request,&handle,&frontier)&&handle&&!frontier);
   C(!memcmp(&random,&expected,sizeof(random))&&model.model.sound_borrowers==1);
   random=before_reaction;
   uint32_t duplicate=99;
   C(!gh_actor_attached_sound_motion(&source,&request,&duplicate,&frontier)&&!frontier&&!duplicate);
   C(!memcmp(&random,&expected,sizeof(random))&&!gh_se_voice_attached_reservations(voices));
   gh_voice_status status=gh_se_voice_update(voices);
   if(status)fprintf(stderr,"reaction sound dep%03x status%d frontier%08x\n",dep,status,gh_se_voice_frontier(voices));
   C(!status);
   gh_se_voice_info info;
   C(!gh_se_voice_query(voices,handle,&info)&&!info.pending&&info.native_voice&&info.channel==40);
   C(info.bank==bank&&info.program==actual_program&&info.sample_slot==(unsigned)actual_program&&
     info.rate==11025&&info.pitch_q12&&info.position_mode==2&&info.distance==3&&
     info.left_q14&&!info.right_q14&&submissions==before_timed+1);
  }
  C(!gh_actor_attached_sound_abort(&source));
  if(rows[i]>=14){
   if(rows[i]==15)C(req.entry_number==20&&dep==0x200);
   unsigned seen=0;
   for(unsigned desired=0;desired<2;++desired){
    gh_game_mt_seed_0030f2b0(&random,desired?1:2);
    gh_game_mt before=random;
    expected=before;
    uint32_t draw;
    C(!gh_game_mt_next_0030f348(&expected,&draw)&&(draw&1u)==desired);
    if(rows[i]==15){
     gh_sound_span sample,tail;
     gh_adpcm_pcm pcm={0};
     C(!gh_audio_get_program(registry,(int16_t)bank,(int16_t)(8+desired),0,program));
     C(!gh_audio_sample_at(registry,(int16_t)bank,7+desired,&sample,&tail)&&sample.size==16);
     C(!gh_audio_decode_ps_adpcm(tail.data,tail.size,&pcm)&&pcm.frames==(desired?19740u:18564u));
     gh_audio_pcm_release(&pcm);
    }
    C(!gh_actor_attached_sound_bind(&source,&binding));
    C(!gh_actor_attached_sound_bind_scene_tables(&source,&tables));
    request.program=8;
    gh_actor_motion_status status=gh_actor_attached_sound_motion(&source,&request,&handle,&frontier);
    if(status)fprintf(stderr,"R100 death request status%d pc%08x detail%d\n",status,frontier,source.detail);
    C(!status&&handle&&!frontier&&!memcmp(&random,&expected,sizeof(random)));
    gh_se_voice_info info;
    C(!gh_se_voice_query(voices,handle,&info)&&info.pending&&info.position_mode==2&&
       info.program==(int)(8+desired)&&info.sample_slot==7+desired&&info.adsr2==0x5fcb);
    uint32_t duplicate=99;
    random=before;
    C(!gh_actor_attached_sound_motion(&source,&request,&duplicate,&frontier)&&!duplicate&&!frontier&&
       !memcmp(&random,&expected,sizeof(random)));
    unsigned before_submit=submissions;
    gh_voice_status update=gh_se_voice_update(voices);
    if(update)fprintf(stderr,"R100 death update %d pc%08x\n",update,gh_se_voice_frontier(voices));
    C(!update&&!gh_se_voice_query(voices,handle,&info)&&!info.pending&&info.native_voice&&
       info.channel==46&&info.distance==3&&info.rate==11025&&info.left_q14&&!info.right_q14&&
       submissions==before_submit+1);
    C(!gh_se_voice_detach_actor_002ce488(voices,&model.model)&&!model.model.sound_borrowers);
    C(!gh_actor_attached_sound_abort(&source));
    seen|=1u<<desired;
   }
   C(seen==3);
   printf("PASS actual R100 stamp%u death 08/09 PCM, original MT and duplicate suppression (no device)\n",req.entry_number);
  }
  if(!enabled&&rows[i]<14){
   for(unsigned desired=0;desired<2;++desired){
    uint32_t draw;
    gh_game_mt before;
    gh_game_mt_seed_0030f2b0(&before,desired?1:2);
    expected=before;
    C(!gh_game_mt_next_0030f348(&expected,&draw)&&(draw&1u)==desired);
    random=before;
    C(!gh_actor_attached_sound_bind(&source,&binding));
    C(!gh_actor_attached_sound_bind_scene_tables(&source,&tables));
    request.bank=0;
    request.program=0xaa;
    gh_actor_motion_status status=gh_actor_attached_sound_motion(&source,&request,&handle,&frontier);
    if(status)fprintf(stderr,"CORE footstep request status%d pc%08x detail%d\n",status,frontier,source.detail);
    C(!status&&handle&&!frontier&&!memcmp(&random,&expected,sizeof(random)));
    gh_se_voice_info info;
    C(!gh_se_voice_query(voices,handle,&info)&&info.pending&&info.position_mode==2&&
       info.bank==0&&info.program==(int)(0xaa+desired)&&info.sample_slot==99+desired&&
       info.rate==14000&&info.adsr1==0x80ff&&info.adsr2==0x5fc0);
    random=before;
    uint32_t duplicate=99;
    C(!gh_actor_attached_sound_motion(&source,&request,&duplicate,&frontier)&&!duplicate&&!frontier&&
      !memcmp(&random,&expected,sizeof(random))&&model.model.sound_borrowers==1);
    const unsigned submitted_before=submissions;
    gh_voice_status update=gh_se_voice_update(voices);
    if(update)fprintf(stderr,"CORE footstep update status%d pc%08x\n",update,gh_se_voice_frontier(voices));
    C(!update);
    C(!gh_se_voice_query(voices,handle,&info)&&!info.pending&&info.native_voice&&
       info.channel==28&&info.distance==3&&!info.azimuth&&!info.pan&&info.pitch_q12);
    C(info.left_q14==0x5a00*25/127/2&&!info.right_q14&&submissions==submitted_before+1&&
       submitted[submitted_before].frames>(desired?1316u:2856u));
    const uint16_t normal_pitch=info.pitch_q12;
    store(enemy.view.bytes+0x5a8,0.25f);
    C(!gh_se_voice_update(voices)&&!gh_se_voice_query(voices,handle,&info)&&
       info.pitch_q12<normal_pitch&&info.left_q14==0x5a00*19/127/2);
    for(unsigned age=0;age<4;++age)C(!gh_se_voice_update(voices));
    C(!gh_se_voice_query(voices,handle,&info)&&!info.left_q14&&!info.right_q14&&
       !memcmp(&random,&expected,sizeof(random)));
    store(enemy.view.bytes+0x5a8,1);
    C(!gh_actor_attached_sound_abort(&source)&&!model.model.sound_borrowers);
   }
  }
  if(!enabled&&rows[i]<14){
   const unsigned sample_slot=15+i;
   const size_t sample_frames[]={5684,10108,12348};
   check_program(registry,(int16_t)bank,0x28,sample_slot,11025,sample_frames[i]);
   C(!gh_actor_attached_sound_bind(&source,&binding));
   C(!gh_actor_attached_sound_bind_scene_tables(&source,&tables));
   request.bank=1;
   request.program=0x28;
   expected=random;
   const unsigned before_sequence=submissions;
   gh_actor_motion_status requested=gh_actor_attached_sound_motion(&source,&request,&handle,&frontier);
   if(requested)fprintf(stderr,"SEQ28 request dep%03x status%d pc%08x detail%d bank%d\n",dep,requested,frontier,source.detail,bank);
   C(!requested&&handle&&!frontier);
   C(!memcmp(&random,&expected,sizeof(random))&&model.model.sound_borrowers==1);
   uint32_t duplicate=99;
   C(!gh_actor_attached_sound_motion(&source,&request,&duplicate,&frontier)&&!duplicate&&!frontier);
   C(!memcmp(&random,&expected,sizeof(random))&&!gh_se_voice_attached_reservations(voices));
   gh_voice_status status=gh_se_voice_update(voices);
   if(status)fprintf(stderr,"EM02 sequence28 dep%03x status%d frontier%08x\n",dep,status,gh_se_voice_frontier(voices));
   C(!status);
   gh_se_voice_info info;
   C(!gh_se_voice_query(voices,handle,&info)&&!info.pending&&info.native_voice&&info.channel==40);
   C(info.bank==bank&&info.program==0x28&&info.sample_slot==sample_slot&&info.rate==11025&&
     info.position_mode==2&&info.distance==3&&info.left_q14&&!info.right_q14&&info.pitch_q12&&
     submissions==before_sequence+1&&submitted[before_sequence].frames>sample_frames[i]);
   C(!gh_actor_attached_sound_abort(&source)&&!model.model.sound_borrowers);
  }
  C(!gh_enemy_model_release_prefix(&model)&&!gh_game_title_player_release(&enemy));
 }
 C(submissions==17&&stops==17);
 C(!gh_enemy_se_banks_destroy(banks));
 gh_se_voice_destroy(voices);
 C(!gh_game_room_pools_release(&pools)&&!gh_task_threads_destroy(threads)&&!gh_heap_destroy(heap));
 C(!gh_scene_actors_reset_0012e800(&tables));
 gh_audio_registry_destroy(registry);
 gh_disc_close(disc);
 free(resource_bytes);
 free(room);
 puts("PASS actual R100 timed bank1:11 enemy reaction bank1:0, SEQ bank1:28, CORE AA/AB footsteps, original MT, duplicates and spatial PCM commands (no device)");
 return 0;
}
