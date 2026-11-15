#include "sound_spatial_step.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <fenv.h>
#include <windows.h>
#define C(x) do{if(!(x)){fprintf(stderr,"line%d %s\n",__LINE__,#x);exit(1);}}while(0)
static uint8_t*read_file(gh_disc*d,const char*p,size_t*n){size_t i;gh_disc_entry e;C(!gh_disc_find(d,p,&i)&&!gh_disc_entry_at(d,i,&e));*n=(size_t)e.size;uint8_t*b=malloc(*n);C(b&&!gh_disc_read(d,i,0,b,*n));return b;}
static void put(uint8_t*p,uint32_t n){memcpy(p,&n,4);}
typedef struct{gh_sound_scene*s;gh_bundle*b;gh_boot_work*w;gh_dvd_jobs*d;gh_heap*h;gh_task_threads*t;int done;gh_sound_scene_status result;} load_context;
static void load(void*p,int32_t arg){load_context*c=p;(void)arg;c->result=gh_sound_scene_initialize_002cabe8(c->s,c->b,c->w,c->d,c->h,c->t);c->done=1;}
static float value(uint32_t n){float f;memcpy(&f,&n,4);return f;}
int main(int argc,char**argv){C(argc==2||(argc==3&&!strcmp(argv[2],"--hardware-audio")));gh_disc*d;gh_audio_registry*r;C(!gh_disc_open(argv[1],&d)&&!gh_audio_registry_create(&r)&&!gh_audio_startup_load(r,d));gh_sound_control control={0};C(!gh_sound_control_from_startup(&control,d,r));gh_sound_scene scene={0};C(!gh_sound_scene_from_startup(&scene,&control,r));
 size_t n;uint8_t*pack=read_file(d,"st0/r020.dat",&n);gh_bundle bundle;C(!gh_bundle_open(pack,n,&bundle));gh_heap*h;C(!gh_heap_create(2*1024*1024,&h));gh_dvd_jobs dvd;gh_dvd_jobs_init(&dvd,d);gh_task_record32 records[4]={0};gh_task_manager manager={.records=records,.capacity=4,.count=4,.current_index=-1};gh_task_threads*threads;C(!gh_task_threads_create(&manager,&threads));gh_boot_work work={0};work.room=0x20;load_context ctx={&scene,&bundle,&work,&dvd,h,threads,0,0};int32_t slot;C(!gh_task_threads_start(threads,2,load,&ctx,0,&slot));
 for(unsigned i=0;i<12&&!ctx.done;i++){C(gh_tasks_run_matching_002d52a0(&manager).status==GH_TASK_OK);C(!gh_dvd_pump(&dvd));}
 C(ctx.done&&!ctx.result&&scene.ready&&!scene.bmh.bytes&&!scene.ts.bytes&&!control.ts_head2c);
 static gh_game_init_state init;gh_game_init_state_initialize(&init);gh_game_pl_camera camera={0};C(!gh_game_camera_construct_pl_00139bc8(&camera,&init.camera,GH_CAMERA_NATIVE_BINARY32_REFERENCE));init.active_camera=&init.camera;uint8_t raw[GH_TITLE_PLAYER_BYTES]={0};gh_title_player_view player;C(!gh_title_player_construct_00107e40(raw,sizeof(raw),&player));uint32_t frontier;
 /* Scalarhistoryinputs areexplicitfixtures ontheSAMEloadedcanonicalowner. */
 control.event_mask50=0x12345678;control.flags_ac=0;control.flags_b0=0x11223344;put((uint8_t*)&work+0x5e4,0x4000);
 C(!fesetround(FE_DOWNWARD));C(gh_sound_spatial_step_002ca518(&scene,NULL,&work,&init,&camera,&player,GH_CAMERA_NATIVE_BINARY32_REFERENCE,&frontier)==GH_SOUND_SCENE_OK&&!frontier&&fegetround()==FE_DOWNWARD);C(!fesetround(FE_TONEAREST));
 C(control.listener_mode110==1&&control.listener_position_c0[0]==0&&control.listener_position_c0[1]==0x3fc00000&&control.listener_position_c0[2]==0xbdcccccd&&control.listener_position_c0[3]==0x3f800000);
 C(control.previous_mask54==0x12345678&&control.event_mask50==control.event_mask58&&control.event_mask50==0x80000000&&control.flags_ac==control.flags_b0&&scene.ratios74[0]==scene.fade_ratio_bits&&scene.ratios88[0]==scene.default_ratio_bits);
 for(unsigned row=0;row<3;row++){double v=value(control.listener_matrix_d0[12+row]);for(unsigned col=0;col<3;col++)v+=(double)value(control.listener_matrix_d0[col*4+row])*value(control.listener_position_c0[col]);C(fabs(v)<1e-6);}
 put((uint8_t*)&work+0x5e4,0x40);put(camera.fields+0x140,0xdeadbeef);
 C(!gh_sound_listener_update_002cc150(&control,&work,&init,&camera,&player,GH_CAMERA_EXACT_UNAVAILABLE,&frontier)&&control.listener_mode110==0&&!memcmp(control.listener_matrix_d0,camera.fields+0x140,64)&&!memcmp(control.listener_position_c0,camera.fields+0x210,16));
 put((uint8_t*)&work+0x5e4,0);C(!gh_sound_listener_update_002cc150(&control,&work,&init,&camera,NULL,GH_CAMERA_EXACT_UNAVAILABLE,&frontier)&&control.listener_mode110==1); /* localfallbackonly */
 const uint8_t unknown=0;scene.bmh.bytes=&unknown;control.event_mask50=7;C(gh_sound_spatial_step_002ca518(&scene,NULL,&work,&init,&camera,&player,GH_CAMERA_NATIVE_BINARY32_REFERENCE,&frontier)==GH_SOUND_SCENE_UNSUPPORTED&&frontier==0x2d22f0&&control.previous_mask54==7);scene.bmh.bytes=NULL;
 scene.ts.bytes=&unknown;control.ts_head2c=(void*)&unknown;C(gh_sound_spatial_step_002ca518(&scene,NULL,&work,&init,&camera,&player,GH_CAMERA_NATIVE_BINARY32_REFERENCE,&frontier)==GH_SOUND_SCENE_UNSUPPORTED&&frontier==0x2c8ad8);
 put((uint8_t*)&work+0x588,0x80000000);C(gh_sound_spatial_step_002ca518(&scene,NULL,&work,&init,&camera,&player,GH_CAMERA_NATIVE_BINARY32_REFERENCE,&frontier)==GH_SOUND_SCENE_UNSUPPORTED&&frontier==0x2c8ad8);
 put((uint8_t*)&work+0x588,0);put((uint8_t*)&work+0x58c,0x80000000);C(!gh_sound_spatial_step_002ca518(&scene,NULL,&work,&init,&camera,&player,GH_CAMERA_NATIVE_BINARY32_REFERENCE,&frontier));scene.ts.bytes=NULL;control.ts_head2c=NULL;
 C(gh_audio_release_bank(r,2)==GH_AUDIO_OK);
 scene.ready=0;work.room=0x102;work.system_flags=0;ctx.done=0;
 size_t room_size;uint8_t *room=read_file(d,"st1/r102.dat",&room_size);
 C(!gh_bundle_open(room,room_size,&bundle));
 C(!gh_task_threads_start(threads,2,load,&ctx,0,&slot));
 for(unsigned i=0;i<12&&!ctx.done;++i){C(gh_tasks_run_matching_002d52a0(&manager).status==GH_TASK_OK);C(!gh_dvd_pump(&dvd));}
 C(ctx.done&&!ctx.result&&scene.ts_count==8);
 gh_sound_ts_row *source=&scene.ts_rows[0];
 const uint8_t *record=scene.ts.bytes+source->offset;
 C(record[0x0d]==2);
 float center[3];memcpy(center,record+0x30,12);
 float position[3]={center[0],center[1]+3.5f,center[2]};
 memcpy(player.alias_f0,position,12);
 C(!gh_sound_spatial_step_002ca518(&scene,NULL,&work,&init,&camera,&player,GH_CAMERA_NATIVE_BINARY32_REFERENCE,&frontier));
 C(source->state13==0&&!(source->flags0c&2u));
 position[1]=center[1]+3;
 memcpy(player.alias_f0,position,12);
 put((uint8_t*)&work+0x5e4,0x800);
 C(!gh_sound_spatial_step_002ca518(&scene,NULL,&work,&init,&camera,&player,GH_CAMERA_NATIVE_BINARY32_REFERENCE,&frontier));
 C(source->state13==0&&!(source->flags0c&2u));
 put((uint8_t*)&work+0x5e4,0);
 C(!gh_sound_spatial_step_002ca518(&scene,NULL,&work,&init,&camera,&player,GH_CAMERA_NATIVE_BINARY32_REFERENCE,&frontier));
 C(source->state13==1&&source->flags0c==3&&source->volume10==3);
 C(gh_sound_spatial_step_002ca518(&scene,NULL,&work,&init,&camera,&player,GH_CAMERA_NATIVE_BINARY32_REFERENCE,&frontier)==GH_SOUND_SCENE_UNSUPPORTED&&frontier==0x2cb968&&source->state13==1);
 if(argc==3){
  gh_audio_output *output;
  int32_t native;
  gh_se_mix mix;
  gh_se_voice_engine *voices;
  C(!gh_audio_output_create(&output,&native)&&!gh_audio_output_set_gain(output,0));
  C(!gh_se_mix_boot_defaults_002c9e78(&mix,0,GH_SE_DSP_NATIVE_LINEAR_REFERENCE));
  C(!gh_se_voice_create(r,output,&mix,&voices));
  C(!gh_sound_spatial_step_002ca518(&scene,voices,&work,&init,&camera,&player,GH_CAMERA_NATIVE_BINARY32_REFERENCE,&frontier));
  const uint32_t handle=source->voice14;
  C(source->state13==2&&handle);
  gh_se_voice_info voice;
  C(!gh_se_voice_query(voices,handle,&voice)&&voice.bank==2&&voice.program==34&&voice.position_mode==3&&voice.pending);
  C(!gh_se_voice_update(voices));
  C(!gh_se_voice_query(voices,handle,&voice)&&voice.native_voice&&!voice.pending&&voice.frames==(840u+(840u-197u))*28u);
  C(!gh_sound_spatial_step_002ca518(&scene,voices,&work,&init,&camera,&player,GH_CAMERA_NATIVE_BINARY32_REFERENCE,&frontier));
  C(!gh_se_voice_update(voices)&&!gh_se_voice_query(voices,handle,&voice)&&voice.distance==3);
  /* The native stop queue takes its fade branch after XAudio2 has
   * acknowledged playback. Wait for progress on this same muted voice. */
  uint64_t played=0;
  int active=0;
  const ULONGLONG deadline=GetTickCount64()+2000;
  do{
   C(!gh_audio_output_active(output,voice.native_voice,&active,&played)&&active);
   if(played)break;
   Sleep(5);
  }while(GetTickCount64()<deadline);
  C(played>0);
  position[1]=center[1]+4;
  memcpy(player.alias_f0,position,12);
  C(!gh_sound_spatial_step_002ca518(&scene,voices,&work,&init,&camera,&player,GH_CAMERA_NATIVE_BINARY32_REFERENCE,&frontier));
  C(source->state13==3&&source->voice14==handle);
  C(!gh_sound_spatial_step_002ca518(&scene,voices,&work,&init,&camera,&player,GH_CAMERA_NATIVE_BINARY32_REFERENCE,&frontier));
  C(source->state13==0&&!source->voice14);
  C(!gh_se_voice_update(voices));
  C(!gh_se_voice_query(voices,handle,&voice)&&voice.stopping&&
    voice.stop_step==-(0x7f00/30)&&voice.stop_count==1);
  gh_se_voice_destroy(voices);
  gh_audio_output_destroy(output);
  puts("PASS real R102 bank 2 TS loop, mode 3 distance update and 30-frame fade");
 }
 C(!gh_task_threads_destroy(threads)&&!gh_heap_used(h)&&!gh_heap_destroy(h));free(room);free(pack);gh_audio_registry_destroy(r);gh_disc_close(d);
 puts("PASS original R020 listener and R102 authored TS sphere, boundary, suspend gate and voice-start frontier");return 0;}
