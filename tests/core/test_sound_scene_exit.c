#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "sound_scene_exit.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define C(x) do{if(!(x)){fprintf(stderr,"line%d %s\n",__LINE__,#x);exit(1);}}while(0)
static uint32_t w(const uint8_t*p){uint32_t n;memcpy(&n,p,4);return n;}
int main(int argc,char**argv){
 gh_disc*d;gh_audio_registry*r;gh_heap*h;gh_sound_control control={0};gh_sound_scene scene={0};gh_audio_output*out;int32_t native_error;
 C(argc==2&&!gh_disc_open(argv[1],&d)&&!gh_audio_registry_create(&r)&&!gh_heap_create(64*1024*1024,&h));
 C(!gh_audio_startup_load(r,d)&&!gh_sound_control_from_startup(&control,d,r)&&!gh_sound_scene_from_startup(&scene,&control,r));
 C(!gh_audio_output_create(&out,&native_error)&&!gh_audio_output_set_gain(out,0));
 gh_se_mix mix;gh_se_voice_engine*voices;C(!gh_se_mix_boot_defaults_002c9e78(&mix,0,GH_SE_DSP_NATIVE_LINEAR_REFERENCE)&&!gh_se_voice_create(r,out,&mix,&voices)&&!gh_se_voice_bind_origin_seh(voices,NULL,0));
 gh_bgm_requests*bgm;C(!gh_bgm_requests_create(r,&control,d,h,&bgm));size_t en=(size_t)gh_disc_boot_elf_size(d);uint8_t*elf=malloc(en);gh_adx_key key;
 C(elf&&!gh_disc_read_boot_elf(d,0,elf,en)&&!gh_adx_key_from_elf(elf,en,&key)&&!gh_bgm_requests_enable_native(bgm,&key,1,GH_BGM_GAIN_NATIVE_DB_REFERENCE));free(elf);
 gh_boot_work work={0};C(!gh_bgm_event_set_002d15a8(bgm,0,0,0)&&!gh_bgm_event_start_002d1610(bgm,0,0,0));
 gh_bgm_request_view v;C(!gh_bgm_requests_inspect(bgm,0,&v)&&v.transport.started&&v.transport.submitted_frames&&v.transport.muted);const uint8_t*encoded=v.encoded.data;
 for(unsigned i=0;i<10;i++)C(!gh_bgm_requests_update_002d0938(bgm,&work));
 /* Real pending PCM requests: cancel original EE nodes before SDE update;
  * no voice is falsely said to have played or completed. */
 uint32_t active_handle;gh_se_voice_info active_info;int native_active;uint64_t played=0;
 C(!gh_se_voice_request_origin(voices,0,0x2d,-1,-1,0,0,&active_handle)&&active_handle&&!gh_se_voice_update(voices)&&!gh_se_voice_query(voices,active_handle,&active_info));
 for(unsigned i=0;i<100&&!played;i++){Sleep(5);C(!gh_audio_output_active(out,active_info.native_voice,&native_active,&played));}C(native_active&&played);
 uint32_t a,b;C(!gh_se_voice_request_origin(voices,0,0x15e,-1,-1,0,0,&a)&&a&&!gh_se_voice_request_origin(voices,0,0x160,-1,-1,0,0,&b)&&b&&gh_se_voice_active_nodes(voices)==3);
 /* Test setup commits actual r020 SES through the same native registry. */
 size_t fi;gh_disc_entry de;C(!gh_disc_find(d,"se/r020.ses",&fi)&&!gh_disc_entry_at(d,fi,&de));uint8_t*ses=malloc((size_t)de.size);C(ses&&!gh_disc_read(d,fi,0,ses,(size_t)de.size)&&!gh_audio_commit_se(r,2,ses,(size_t)de.size,&scene.bank2));free(ses);gh_audio_token token=scene.bank2;
 gh_sound_scene_exit state;C(!gh_sound_scene_exit_bind(&state,&scene,bgm,voices));control.flags_ac=control.flags_b0=0xffffffff;
 C(!gh_sound_scene_exit_begin_002caa00(&state)&&gh_se_voice_active_nodes(voices)==1);C(control.flags_ac==0xffc7ffffu&&control.flags_b0==0xffc7ffffu);
 C(!gh_bgm_requests_inspect(bgm,0,&v)&&(v.request_flags&0x100000)&&w(v.original_scalars+0xc8)==0x41a00000&&w(v.original_scalars+0x18)==0x3c23d70a);
 int ready=-1;C(!gh_sound_scene_exit_wait_002cab58(&state,&ready)&&!ready);C(!(v.stream_flags&2));
 /* Pump this real SE backend independently to inspect the exact wait
  * boundary; production caller also pumps its BGM every timed frame. */
 for(unsigned i=0;i<12;i++){C(!gh_se_voice_update(voices)&&!gh_sound_scene_exit_wait_002cab58(&state,&ready)&&!ready);}
 C(!gh_se_voice_update(voices)&&!gh_sound_scene_exit_wait_002cab58(&state,&ready)&&ready);
 /* Keep only real startup masks; unrelated forced flags would request an
  * unported BGM branch. Countdown pause occurs on21st, not20th update. */
 control.flags_ac=control.flags_b0=0x800000;
 for(unsigned i=0;i<20;i++)C(!gh_bgm_requests_update_002d0938(bgm,&work));C(!gh_bgm_requests_inspect(bgm,0,&v)&&!(v.stream_flags&2)&&w(v.original_scalars+0xc8)==0);
 C(!gh_bgm_requests_update_002d0938(bgm,&work)&&!gh_bgm_requests_inspect(bgm,0,&v)&&(v.stream_flags&2)&&v.encoded.data==encoded&&v.prepared);
 /* Positive native ownership cannot be hidden behind a false emptyheap. */
 ses=malloc((size_t)de.size);gh_audio_token extra;C(ses&&!gh_disc_read(d,fi,0,ses,(size_t)de.size)&&!gh_audio_commit_se(r,4,ses,(size_t)de.size,&extra));free(ses);control.unload_a4=0x12345678;
 C(gh_sound_scene_exit_finalize_002ca910(&state)==GH_SOUND_EXIT_UNSUPPORTED&&state.frontier==0x2cd890&&state.detail==4&&state.final_stage==3&&control.unload_a4==0x12345678&&gh_audio_token_complete(r,extra));
 C(!gh_audio_release_bank(r,4));
 C(gh_sound_scene_exit_finalize_002ca910(&state)==GH_SOUND_EXIT_OK&&state.final_stage==5&&control.room_heaps[0].closed&&control.room_heaps[1].closed&&!control.unload_a4);
 C(!gh_audio_token_complete(r,token)&&gh_audio_startup_ready(r,NULL,NULL,NULL)&&!(control.flags_ac&0x800000));
 C(!gh_bgm_requests_inspect(bgm,0,&v)&&v.encoded.data==encoded&&v.prepared); /* bank2 unload preserves BGM0 owner */
 gh_bgm_requests_destroy(bgm);gh_se_voice_destroy(voices);gh_audio_output_destroy(out);C(!gh_heap_used(h)&&!gh_heap_destroy(h));gh_audio_registry_destroy(r);gh_disc_close(d);
 puts("PASS real muted BGM/PCM pending cancel, exact preserved fade/countdown/ready predicate, actual bank2 release/native registry heap closure and preserved parent banks");return 0;
}
