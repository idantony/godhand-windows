#include "game_actor_attached_sound.h"
#include "game_enemy_factory.h"
#include "game_enemy_model.h"
#include "game_enemy_2a2.h"
#include "game_scene_tables.h"
#include "enemy_se_banks.h"
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
   descriptor.data[10]==0xc0&&descriptor.data[11]==0x5f);
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
 check_program(registry,0,0x96,95,22050,7504);
 check_program(registry,0,0x97,96,22050,6944);
 check_program(registry,0,0x98,97,22050,7476);
 size_t room_bytes,em_bytes;
 uint8_t *room=read_file(disc,"st1/r100.dat",&room_bytes);
 uint8_t *resource_bytes=read_file(disc,"em/ema2.dat",&em_bytes);
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

 gh_game_title_player enemy={0};
 int original;
 C(!gh_enemy2a2_allocate_00313820(&enemy,heap,&work,&original)&&original);
 memcpy(enemy.view.bytes+0x2fe,&(uint16_t){0x2a2},2);
 memcpy(enemy.view.bytes+0x564,&(uint32_t){0x2a2},4);
 gh_object_view resource={.bytes=resource_bytes,.kind=0x2a2,.size=em_bytes,.file_extent=em_bytes,.file_extent_known=1};
 gh_enemy_model model={0};
 uint32_t fog=0;
 C(!gh_enemy2a2_model_00288da8(&model,&enemy.view,&resource,&work,&pools,heap,&fog,
     GH_STATIC_BINARY32_REFERENCE,&original)&&original);
 store(enemy.view.bytes+0x5a8,1);
 gh_game_mt random;
 gh_game_mt_seed_0030f2b0(&random,1);
 gh_actor_attached_sound source={0};
 gh_actor_attached_sound_bindings binding={voices,&control,&scene,&work,&random};
 C(!gh_actor_attached_sound_bind(&source,&binding));
 C(!gh_actor_attached_sound_bind_scene_tables(&source,&tables));
 gh_motion_sound_request request={.model=&model.model,.actor=&enemy.view,.bank=0,.program=0x96};
 float anchor[4];
 memcpy(anchor,enemy.view.alias_f0,sizeof(anchor));
 float listener[4]={anchor[0]-3,anchor[1]+1.3f,anchor[2],1};
 float matrix[16]={1,0,0,0,0,1,0,0,0,0,1,0,-listener[0],-listener[1],-listener[2],1};
 memcpy(control.listener_position_c0,listener,sizeof(listener));
 memcpy(control.listener_matrix_d0,matrix,sizeof(matrix));
 control.listener_mode110=1;
 unsigned seen=0;
 for(unsigned i=0;i<16&&seen!=7;++i){
  gh_game_mt expected=random;
  uint32_t roll;
  C(!gh_game_mt_next_0030f348(&expected,&roll));
  unsigned variant=roll%3;
  uint32_t handle,frontier;
  gh_actor_motion_status status=gh_actor_attached_sound_motion(&source,&request,&handle,&frontier);
  if(status)fprintf(stderr,"cEma2 request status %d frontier %08x detail %d\n",status,frontier,source.detail);
  C(!status&&handle&&!frontier&&!memcmp(&random,&expected,sizeof(random)));
  gh_se_voice_info info;
  C(!gh_se_voice_query(voices,handle,&info)&&info.pending&&!info.native_voice);
  C(info.bank==0&&info.program==(int)(0x96+variant)&&info.sample_slot==95+variant&&info.position_mode==2);
  gh_voice_status update=gh_se_voice_update(voices);
  if(update)fprintf(stderr,"cEma2 update %d frontier %08x\n",update,gh_se_voice_frontier(voices));
  C(!update&&!gh_se_voice_query(voices,handle,&info)&&info.native_voice&&!info.pending);
  unsigned gain=95*110/127;
  C(info.channel==6&&info.distance==3&&!info.azimuth&&!info.pan&&info.pitch_q12);
  C(info.left_q14==0x5a00*gain/127/2&&!info.right_q14);
  C(submitted[info.native_voice-1].left==(float)info.left_q14/16384&&
    !submitted[info.native_voice-1].right);
  uint32_t native=info.native_voice;
  uint16_t initial=info.pitch_q12,seed,after_seed;
  C(!gh_se_voice_pitch_state(voices,&seed));
  unsigned initial_submissions=submissions,initial_commands=command_count;
  store(enemy.view.bytes+0x5a8,.25f);
  C(!gh_se_voice_update(voices)&&!gh_se_voice_query(voices,handle,&info));
  C(info.native_voice==native&&info.pitch_q12==initial&&submissions==initial_submissions);
  C(command_count==initial_commands+1&&command_order[initial_commands]=='G');

  store(enemy.view.bytes+0x5a8,1);
  C(!gh_se_voice_update(voices)&&!gh_se_voice_query(voices,handle,&info)&&info.pitch_q12==initial);
  C(info.native_voice==native&&!gh_se_voice_pitch_state(voices,&after_seed)&&seed==after_seed);
  C(!memcmp(&random,&expected,sizeof(random)));
  seen|=1u<<variant;
  C(!gh_actor_attached_sound_abort(&source)&&!model.model.sound_borrowers);
  C(!gh_actor_attached_sound_bind(&source,&binding));
 C(!gh_actor_attached_sound_bind_scene_tables(&source,&tables));
 }
 C(seen==7&&submissions==stops&&!pitch_updates&&gain_updates>=6);

 /* Find an actual unlinked R100 face that occludes the original ray.
  * Both sides of the segment use the same canonical query owner. */
 gh_scene_ray_report reports[8];
 gh_scene_ray_request ray={.mask=7,.flags=0x12c,.report_limit=8,.native_reports=reports};
 int found=0;
 for(int x=-30;x<=30&&!found;x+=2)for(int z=-30;z<=30&&!found;z+=2){
  memset(reports,0,sizeof(reports));
  ray.from[0]=ray.to[0]=(float)x;ray.from[2]=ray.to[2]=(float)z;
  ray.from[1]=4;ray.to[1]=-4;ray.from[3]=ray.to[3]=1;
  int hit;
  uint32_t pc;
  gh_scene_ray_status result=gh_scene_ray_query_direct_0012ed60(&tables,&ray,0,&hit,&pc);
  if(!result&&hit){
   for(unsigned k=0;k<tables.ray.triangle17c;++k)
    if(reports[k].attributes&&reports[k].attribute_bytes>=12&&!reports[k].attributes[10])found=1;
  }
 }
 C(found);
 memcpy(enemy.view.alias_f0,ray.from,16);
 store(enemy.view.alias_f0+4,ray.from[1]-1.3f);
 memcpy(control.listener_position_c0,ray.to,16);
 uint32_t blocked,frontier;
 C(!gh_actor_attached_sound_motion(&source,&request,&blocked,&frontier)&&blocked);
 unsigned before=submissions;
 C(!gh_se_voice_update(voices)&&submissions==before&&!model.model.sound_borrowers);
 gh_se_voice_info missing;
 C(gh_se_voice_query(voices,blocked,&missing)==GH_VOICE_BOUNDS);

 C(!gh_actor_attached_sound_abort(&source));

 gh_enemy_se_banks *banks;
 gh_enemy_se_bindings bank_bind={.control=&control,.registry=registry,.disc=disc,.dvd=&dvd,
     .default_heap=heap,.room_heap=heap,.voices=voices};
 int32_t bank;
 C(!gh_enemy_se_banks_from_startup(&bank_bind,&banks));
 C(!gh_enemy_se_request_002cb278(banks,&work,0x2a2,&bank)&&bank>=1);
 int ready=0;
 for(unsigned i=0;i<16&&!ready;++i){
  C(!gh_dvd_pump(&dvd)&&!gh_enemy_se_banks_update_002cf440(banks));
  C(!gh_enemy_se_ready_002cb778(banks,bank,&ready));
 }
 C(ready);
 memcpy(enemy.view.alias_f0,anchor,16);
 memcpy(control.listener_position_c0,listener,16);
 C(!gh_actor_attached_sound_bind(&source,&binding));
 C(!gh_actor_attached_sound_bind_scene_tables(&source,&tables));
 request.bank=1;
 request.program=0x5a;
 seen=0;
 for(unsigned i=0;i<8&&seen!=3;++i){
  gh_game_mt expected=random;
  uint32_t draw,handle;
  C(!gh_game_mt_next_0030f348(&expected,&draw));
  gh_actor_motion_status status=gh_actor_attached_sound_motion(&source,&request,&handle,&frontier);
  if(status)fprintf(stderr,"cEma2 timer cue request %d frontier %08x\n",status,frontier);
  C(!status&&handle&&!frontier&&!memcmp(&expected,&random,sizeof(random)));
  const unsigned variant=draw%2;
  gh_voice_status update=gh_se_voice_update(voices);
  if(update)fprintf(stderr,"cEma2 timer cue update %d frontier %08x\n",update,gh_se_voice_frontier(voices));
  gh_se_voice_info info;
  C(!update&&!gh_se_voice_query(voices,handle,&info)&&info.native_voice);
  C(info.bank==bank&&info.program==(int)(0x5a+variant)&&info.sample_slot==variant&&
    info.rate==11025&&info.adsr2==0x5fcb&&info.distance==3&&info.channel>=45&&info.channel<=47);
  seen|=1u<<variant;
  C(!gh_actor_attached_sound_abort(&source)&&!model.model.sound_borrowers);
  C(!gh_actor_attached_sound_bind(&source,&binding));
  C(!gh_actor_attached_sound_bind_scene_tables(&source,&tables));
 }
 C(seen==3&&submissions==stops);
 C(!gh_actor_attached_sound_abort(&source)&&!gh_enemy_se_banks_destroy(banks));
 gh_se_voice_destroy(voices);
 C(!gh_enemy_model_release_prefix(&model)&&!gh_game_title_player_release(&enemy));
 C(!gh_game_room_pools_release(&pools)&&!gh_task_threads_destroy(threads)&&!gh_heap_destroy(heap));
 C(!gh_scene_actors_reset_0012e800(&tables));
 gh_audio_registry_destroy(registry);
 gh_disc_close(disc);
 free(resource_bytes);
 free(room);
 puts("PASS actual cEma2 model + CORE.SES 96/97/98, preset7 spatial gain, canonical clear and blocked rays, no false slow pitch and scoped stop (no device)");
 return 0;
}
