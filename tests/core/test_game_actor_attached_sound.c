#include "game_actor_attached_sound.h"
#include "game_enemy_factory.h"
#include "game_enemy_model.h"
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fenv.h>
#define C(x) do{if(!(x)){fprintf(stderr,"line%d %s\n",__LINE__,#x);exit(1);}}while(0)
static uint8_t *read_file(gh_disc*d,const char*p,size_t*n){size_t i;gh_disc_entry e;C(!gh_disc_find(d,p,&i)&&!gh_disc_entry_at(d,i,&e));*n=(size_t)e.size;uint8_t*b=malloc(*n);C(b&&!gh_disc_read(d,i,0,b,*n));return b;}
typedef struct {gh_sound_scene*s;gh_bundle*b;gh_boot_work*w;gh_dvd_jobs*d;gh_heap*h;gh_task_threads*t;int done;} loader;
static void load(void*p,int32_t unused){loader*l=p;(void)unused;C(!gh_sound_scene_initialize_002cabe8(l->s,l->b,l->w,l->d,l->h,l->t));l->done=1;}
static void store(void*p,float f){memcpy(p,&f,4);}
static void square_voice_samples(gh_audio_registry *registry,gh_se_voice_engine *voices,
 gh_actor_attached_sound *source,const gh_actor_attached_sound_bindings *bindings,
 gh_actor_model *model,gh_title_player_view *player)
{
 static const size_t adpcm_frames[]={11816,2884,2380};
 gh_motion_sound_request request={.model=model,.actor=player,.bank=0};
 C(!gh_actor_attached_sound_bind(source,bindings));
 for(unsigned i=0;i<3;++i){
  uint8_t program[48];
  gh_sound_span descriptor,tail;
  gh_adpcm_pcm decoded={0};
  unsigned physical=1+i;
  C(!gh_audio_get_program(registry,3,(int16_t)physical,0,program));
  C(program[4]==physical&&!program[5]);
  C(!gh_audio_sample_at(registry,3,physical,&descriptor,&tail)&&descriptor.size==16);
  C(descriptor.data[8]==0xff&&descriptor.data[9]==0x80&&descriptor.data[10]==0xcb&&descriptor.data[11]==0x5f);
  C(!gh_audio_decode_ps_adpcm(tail.data,tail.size,&decoded)&&decoded.frames==adpcm_frames[i]);
  for(size_t block=0;block<decoded.source_bytes;block+=16)
   C(tail.data[block+1]==(block+16==decoded.source_bytes?1:0));
  gh_audio_pcm_release(&decoded);

  request.program=(int16_t)(201+i);
  uint32_t handle,frontier;
  C(!gh_actor_attached_sound_motion(source,&request,&handle,&frontier)&&handle&&!frontier);
  gh_se_voice_info info;
  C(!gh_se_voice_query(voices,handle,&info)&&info.pending&&!info.native_voice);
  C(info.bank==3&&info.program==(int16_t)physical&&info.sample_slot==physical&&info.rate==11025);
  C(info.adsr1==0x80ff&&info.adsr2==0x5fcb&&info.pitch_q12==940);
  const int16_t *pcm;
  size_t frames;
  C(!gh_se_voice_pcm_view(voices,handle,&pcm,&frames)&&frames==(adpcm_frames[i]*4096+939)/940);
  size_t nonzero=0;
  for(size_t n=0;n<frames;++n)nonzero+=pcm[n]!=0;
  C(nonzero>0);
  int accepted=1;
  C(gh_se_voice_request_stop_00375050(voices,handle,-1,&accepted)==GH_VOICE_UNSUPPORTED&&
    !accepted&&gh_se_voice_frontier(voices)==0x37d820);
 }
 C(model->sound_borrowers==3&&!gh_se_voice_attached_reservations(voices));
 C(!gh_actor_attached_sound_abort(source)&&!model->sound_borrowers);
 C(!gh_actor_attached_sound_bind(source,bindings));

 /* The fixture owns these loaded bytes. Change just the release shift. */
 gh_sound_span descriptor,tail;
 C(!gh_audio_sample_at(registry,3,3,&descriptor,&tail)&&descriptor.size==16);
 uint8_t *adsr2=(uint8_t *)descriptor.data+10;
 uint8_t saved=*adsr2;
 *adsr2=0xca;
 uint32_t handle=99,frontier=0;
 gh_actor_motion_status error=gh_actor_attached_sound_motion(source,&request,&handle,&frontier);
 *adsr2=saved;
 C(error==GH_ACTOR_MOTION_UNSUPPORTED&&!handle&&frontier==0x35fc);
 C(model->sound_borrowers==1&&gh_se_voice_attached_reservations(voices)==1);
 C(!gh_actor_attached_sound_abort(source)&&!model->sound_borrowers);
}
static void shared_voice_owners(gh_disc *disc,gh_heap *heap,gh_room_pools *pools,gh_boot_work *work,
 gh_se_voice_engine *voices,gh_actor_attached_sound *source,const gh_actor_attached_sound_bindings *bindings,
 gh_actor_model *model,gh_title_player_view *player)
{
 size_t size;
 uint8_t *data=read_file(disc,"em/em02.dat",&size);
 gh_game_title_player enemy={0};
 int original;
 C(!gh_enemy202_allocate_003130e0(&enemy,heap,work,&original)&&original);
 memcpy(enemy.view.bytes+0x2fe,&(uint16_t){0x202},2);
 memcpy(enemy.view.bytes+0x564,&(uint32_t){0x202},4);
 gh_object_view resource={.bytes=data,.kind=0x202,.size=size,.file_extent=size,.file_extent_known=1};
 gh_enemy_model enemy_model={0};
 uint32_t fog=0;
 C(!gh_enemy_model_initialize_00208268(&enemy_model,&enemy.view,&resource,work,pools,heap,&fog,
   GH_STATIC_BINARY32_REFERENCE,&original)&&original);
 store(enemy.view.alias_f0,13);
 store(enemy.view.alias_f0+4,14);
 store(enemy.view.alias_f0+8,15);
 store(enemy.view.alias_f0+12,1);
 store(enemy.view.bytes+0x5a8,1);
 gh_actor_attached_sound foe={0};
 C(!gh_actor_attached_sound_bind(source,bindings)&&!gh_actor_attached_sound_bind(&foe,bindings));
 uint32_t origin,player_foot,enemy_foot,player_voice,enemy_voice,frontier;
 C(!gh_se_voice_request_origin(voices,0,0x160,-1,-1,0,0,&origin)&&origin);

 /* The exact same request address may belong to either owner on retry.
  * The valid original footstep variant draws each commit before probe. */
 gh_game_mt expected=*bindings->random;
 uint32_t player_roll,enemy_roll;
 C(!gh_game_mt_next_0030f348(&expected,&player_roll)&&!gh_game_mt_next_0030f348(&expected,&enemy_roll));
 uint32_t gain=bindings->control->gain_bits[0];
 bindings->control->gain_bits[0]=0;
 gh_motion_sound_request request={.model=model,.actor=player,.bank=0,.program=0x1e};
 C(gh_actor_attached_sound_motion(source,&request,&player_foot,&frontier)==GH_ACTOR_MOTION_UNSUPPORTED&&
   frontier==0x375630&&!player_foot);
 request.model=&enemy_model.model;
 request.actor=&enemy.view;
 C(gh_actor_attached_sound_motion(&foe,&request,&enemy_foot,&frontier)==GH_ACTOR_MOTION_UNSUPPORTED&&
   frontier==0x375630&&!enemy_foot);
 C(gh_se_voice_attached_reservations(voices)==2&&model->sound_borrowers==1&&enemy_model.model.sound_borrowers==1);
 C(!memcmp(bindings->random,&expected,sizeof(expected)));
 bindings->control->gain_bits[0]=gain;
 C(!gh_actor_attached_sound_motion(&foe,&request,&enemy_foot,&frontier)&&enemy_foot);
 request.model=model;
 request.actor=player;
 C(!gh_actor_attached_sound_motion(source,&request,&player_foot,&frontier)&&player_foot&&player_foot!=enemy_foot);
 C(!gh_se_voice_attached_reservations(voices)&&!memcmp(bindings->random,&expected,sizeof(expected)));

 gh_se_voice_info info;
 C(!gh_se_voice_query(voices,player_foot,&info)&&info.attached_model==model&&info.program==0x1e + (int)(player_roll%2));
 C(!gh_se_voice_query(voices,enemy_foot,&info)&&info.attached_model==&enemy_model.model&&info.program==0x1e + (int)(enemy_roll%2));
 C(info.source_position[0]==13&&info.source_position[1]>15.29f&&info.source_position[1]<15.31f);
 request.program=203;
 C(!gh_actor_attached_sound_motion(source,&request,&player_voice,&frontier)&&player_voice);
 request.model=&enemy_model.model;
 request.actor=&enemy.view;
 request.program=0x161;
 C(!gh_actor_attached_sound_motion(&foe,&request,&enemy_voice,&frontier)&&enemy_voice&&enemy_voice!=player_voice);
 C(gh_se_voice_active_nodes(voices)==5&&model->sound_borrowers==2&&enemy_model.model.sound_borrowers==2);

 C(!gh_actor_attached_sound_abort(source)&&!model->sound_borrowers&&enemy_model.model.sound_borrowers==2);
 C(!gh_actor_attached_sound_abort(source));
 C(gh_se_voice_query(voices,player_foot,&info)==GH_VOICE_BOUNDS&&gh_se_voice_query(voices,player_voice,&info)==GH_VOICE_BOUNDS);
 C(!gh_se_voice_query(voices,enemy_foot,&info)&&!gh_se_voice_query(voices,enemy_voice,&info)&&info.attached_model==&enemy_model.model);
 C(!gh_se_voice_query(voices,origin,&info)&&!info.attached&&gh_se_voice_active_nodes(voices)==3);
 C(!gh_actor_attached_sound_abort(&foe)&&!enemy_model.model.sound_borrowers);
 C(gh_se_voice_query(voices,enemy_foot,&info)==GH_VOICE_BOUNDS&&gh_se_voice_query(voices,enemy_voice,&info)==GH_VOICE_BOUNDS);
 C(!gh_se_voice_query(voices,origin,&info)&&gh_se_voice_active_nodes(voices)==1);
 C(!gh_se_voice_stop_bank_002cbc58(voices,0,0)&&!gh_se_voice_active_nodes(voices));
 C(!gh_enemy_model_release_prefix(&enemy_model)&&!gh_game_title_player_release(&enemy));
 free(data);
}
int main(int argc,char**argv){
 C(argc==2);gh_disc*d;C(!gh_disc_open(argv[1],&d));gh_audio_registry*r;C(!gh_audio_registry_create(&r)&&!gh_audio_startup_load(r,d));gh_sound_control control={0};C(!gh_sound_control_from_startup(&control,d,r));gh_sound_scene scene={0};C(!gh_sound_scene_from_startup(&scene,&control,r));
 size_t room_size,n;uint8_t*room=read_file(d,"st1/r100.dat",&room_size);gh_bundle bundle;C(!gh_bundle_open(room,room_size,&bundle));gh_heap*heap;C(!gh_heap_create(16*1024*1024,&heap));gh_dvd_jobs dvd;gh_dvd_jobs_init(&dvd,d);gh_task_record32 records[4]={0};gh_task_manager manager={.records=records,.capacity=4,.count=4,.current_index=-1};gh_task_threads*threads;C(!gh_task_threads_create(&manager,&threads));gh_boot_work work={0};work.room=0x100;loader l={&scene,&bundle,&work,&dvd,heap,threads,0};int32_t slot;C(!gh_task_threads_start(threads,2,load,&l,0,&slot));for(unsigned i=0;!l.done&&i<16;i++){C(!gh_tasks_run_matching_002d52a0(&manager).status&&!gh_dvd_pump(&dvd));}C(l.done&&scene.ready&&!scene.seh.bytes&&!scene.seh.size);
 gh_room_pools pools={0};C(!gh_game_room_pools_initialize_002a5d80(&pools,&bundle,heap));uint8_t*resource=read_file(d,"pl/pl00.dat",&n);gh_bundle data;gh_bundle_entry md,texture;C(!gh_bundle_open(resource,n,&data)&&!gh_bundle_find(&data,"MD",0,&md)&&!gh_bundle_find(&data,"TM3",0,&texture));_Alignas(16) uint8_t object[GH_TITLE_PLAYER_BYTES]={0};gh_title_player_view player;C(!gh_title_player_construct_00107e40(object,sizeof(object),&player));gh_actor_model model={0};int original;C(gh_actor_model_initialize_00149738(&model,&player,&work,&pools,&md,&texture,0,0,GH_STATIC_BINARY32_REFERENCE,&original)==GH_ACTOR_MODEL_UNSUPPORTED&&model.parts_ready);store(object+0x5a8,1);store(player.alias_f0,3);store(player.alias_f0+4,4);store(player.alias_f0+8,5);store(player.alias_f0+12,1);
 gh_audio_output*output;int32_t native;C(!gh_audio_output_create(&output,&native)&&!gh_audio_output_set_gain(output,0));gh_se_mix mix;C(!gh_se_mix_boot_defaults_002c9e78(&mix,0,GH_SE_DSP_NATIVE_LINEAR_REFERENCE));gh_se_voice_engine*voices;C(!gh_se_voice_create(r,output,&mix,&voices));gh_game_mt mt;gh_game_mt_seed_0030f2b0(&mt,1);gh_actor_attached_sound source={0};gh_actor_attached_sound_bindings bindings={voices,&control,&scene,&work,&mt};C(!gh_actor_attached_sound_bind(&source,&bindings));gh_motion_sound_request request={.model=&model,.actor=&player,.bank=0,.program=0x1e};uint32_t handle,f;gh_se_voice_info info;
 gh_game_mt expected=mt;uint32_t random;C(!gh_game_mt_next_0030f348(&expected,&random));int round=fegetround();C(!fesetround(FE_UPWARD)&&!feclearexcept(FE_ALL_EXCEPT)&&!feraiseexcept(FE_DIVBYZERO));gh_actor_motion_status first_status=gh_actor_attached_sound_motion(&source,&request,&handle,&f);if(first_status)fprintf(stderr,"attached status%d frontier%08x detail%d\n",first_status,f,source.detail);C(!first_status&&handle);C(fegetround()==FE_UPWARD&&fetestexcept(FE_ALL_EXCEPT)==FE_DIVBYZERO);C(!fesetround(round));C(!memcmp(&mt,&expected,sizeof(mt))&&model.sound_borrowers==1&&!gh_se_voice_attached_reservations(voices));C(gh_actor_model_release(&model)==GH_ACTOR_MODEL_BUSY);C(!gh_se_voice_query(voices,handle,&info)&&info.pending&&info.attached&&info.attached_model==&model&&info.program==0x1e + (int)(random%2)&&info.rate==14000&&info.source_position[0]==3&&info.source_position[1]>5.29f&&info.source_position[1]<5.31f&&info.position_mode==0&&info.pitch_q12==0);const int16_t*unresolved_pcm;size_t unresolved_frames;C(gh_se_voice_pcm_view(voices,handle,&unresolved_pcm,&unresolved_frames)==GH_VOICE_UNSUPPORTED);uint32_t first=handle; C(gh_se_voice_update(voices)==GH_VOICE_UNSUPPORTED&&gh_se_voice_frontier(voices)==0x37d760);C(!gh_se_voice_query(voices,first,&info)&&info.pending&&!info.native_voice);C(!gh_actor_attached_sound_abort(&source)&&!model.sound_borrowers); C(!gh_actor_attached_sound_bind(&source,&bindings));request.program=0x161;C(!gh_actor_attached_sound_motion(&source,&request,&first,&f)&&first);
 /* Same address is a new original call after completion, never a cached token. */
 C(!gh_actor_attached_sound_motion(&source,&request,&handle,&f)&&handle!=first&&model.sound_borrowers==2);C(!gh_se_voice_update(voices));gh_voice_status qs=gh_se_voice_query(voices,first,&info);C(!qs&&info.native_voice&&!info.pending&&info.channel<=4);store(player.alias_f0+4,8);C(!gh_se_voice_update(voices)&&!gh_se_voice_query(voices,first,&info)&&info.source_position[1]>9.29f&&info.source_position[1]<9.31f);
 /* Started detach retains cached coordinates and real playback, releases borrow. */
 C(!gh_se_voice_detach_actor_002ce488(voices,&model)&&!model.sound_borrowers);C(!gh_se_voice_query(voices,first,&info)&&!info.attached&&info.native_voice&&info.source_position[1]>9.29f);ULONGLONG until=GetTickCount64()+3000;uint64_t progress=0;while(GetTickCount64()<until){C(!gh_se_voice_update(voices));if(gh_se_voice_query(voices,first,&info)==GH_VOICE_BOUNDS)break;int active;uint64_t played;C(!gh_audio_output_active(output,info.native_voice,&active,&played));if(played>progress)progress=played;Sleep(5);}C(progress>0&&gh_se_voice_query(voices,first,&info)==GH_VOICE_BOUNDS);
 /* Pending detach takes raw actorF0, without the construction +1.3. */
 C(!gh_actor_attached_sound_motion(&source,&request,&handle,&f)&&model.sound_borrowers==1);C(!gh_se_voice_detach_actor_002ce488(voices,&model)&&!model.sound_borrowers&&!gh_se_voice_query(voices,handle,&info)&&info.source_position[1]==8);C(!gh_actor_attached_sound_abort(&source));
 /* Failure after a committed variant draw keeps one reservation and no redraw. */ request.program=0x1e;
 C(!gh_actor_attached_sound_bind(&source,&bindings));control.gain_bits[0]=0;C(gh_actor_attached_sound_motion(&source,&request,&handle,&f)==GH_ACTOR_MOTION_UNSUPPORTED&&f==0x375630&&!handle&&model.sound_borrowers==1&&gh_se_voice_attached_reservations(voices)==1);expected=mt;C(gh_actor_attached_sound_motion(&source,&request,&handle,&f)==GH_ACTOR_MOTION_UNSUPPORTED&&!memcmp(&mt,&expected,sizeof(mt)));control.gain_bits[0]=0x42fe0000;C(!gh_actor_attached_sound_motion(&source,&request,&handle,&f)&&handle&&!memcmp(&mt,&expected,sizeof(mt))&&!gh_se_voice_attached_reservations(voices));C(!gh_actor_attached_sound_abort(&source)&&!model.sound_borrowers);
 /* Part source is the SAME canonical payload; a copied part descriptor is rejected. */ C(!gh_actor_attached_sound_bind(&source,&bindings));request.program=0x161;request.part=model.parts+2;store(model.parts[2].alias_f0,17);store(model.parts[2].alias_f0+4,19);C(!gh_actor_attached_sound_motion(&source,&request,&handle,&f)&&!gh_se_voice_query(voices,handle,&info)&&info.attached_part==2&&info.source_position[0]==17&&info.source_position[1]>20.29f);gh_actor_part foreign=*request.part;request.part=&foreign;C(gh_actor_attached_sound_motion(&source,&request,&first,&f)==GH_ACTOR_MOTION_ARGUMENT&&!first&&model.sound_borrowers==1);C(!gh_actor_attached_sound_abort(&source)&&!model.sound_borrowers);square_voice_samples(r,voices,&source,&bindings,&model,&player);
 shared_voice_owners(d,heap,&pools,&work,voices,&source,&bindings,&model,&player);
 request.part=NULL;request.program=0x1e; /* Same 64 EE slots as origin requests; exhaustion precedes MT and borrowing. */
 for(unsigned i=0;i<64;i++)C(!gh_se_voice_request_origin(voices,0,0x160,-1,-1,0,0,&handle)&&handle);C(!gh_actor_attached_sound_bind(&source,&bindings));expected=mt;C(!gh_actor_attached_sound_motion(&source,&request,&handle,&f)&&!handle&&!model.sound_borrowers&&!memcmp(&expected,&mt,sizeof(mt)));C(!gh_actor_attached_sound_abort(&source));gh_se_voice_destroy(voices);gh_audio_output_destroy(output);
 C(!gh_actor_model_release(&model)&&!gh_game_room_pools_release(&pools)&&!gh_task_threads_destroy(threads)&&!gh_heap_destroy(heap));free(resource);free(room);gh_audio_registry_destroy(r);gh_disc_close(d);puts("PASS actualR100/PL00 attachedFoot sameMT variant, shared64EEpool, pitchrange exact37D760, fixedpitch real mutedXAudio2 progress/EOS, same-key reuse, modelborrow/detach/abort, lateerror retry/FEnv");return 0;
}
