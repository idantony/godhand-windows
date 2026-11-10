#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "bgm_requests.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fenv.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"line%d %s\n",__LINE__,#x);exit(1);}}while(0)
static uint32_t u32(const uint8_t*p){return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
int main(int argc,char**argv){
 gh_disc*d;gh_audio_registry*r;gh_sound_control control={0};gh_heap*h;gh_bgm_requests*s=NULL;
 CHECK(argc==2&&gh_disc_open(argv[1],&d)==GH_DISC_OK&&gh_audio_registry_create(&r)==GH_AUDIO_OK&&gh_heap_create(64*1024*1024,&h)==GH_MEM_OK);
 CHECK(gh_bgm_requests_create(r,&control,d,h,&s)==GH_BGM_ARGUMENT&&!s);
 CHECK(gh_audio_startup_load(r,d)==GH_AUDIO_OK&&gh_sound_control_from_startup(&control,d,r)==GH_VOICE_OK);
 CHECK(gh_bgm_requests_create(r,&control,d,h,&s)==GH_BGM_OK&&s&&!gh_bgm_requests_pending(s));
 gh_bgm_request_view v;gh_sound_span row;uint64_t off,size;size_t fi;
 CHECK(gh_audio_external_sample(r,-1,0,&row,&off,&size)==GH_AUDIO_OK&&gh_disc_find(d,"bgm/bgm00.afs",&fi)==GH_DISC_OK);
 const int modes[]={FE_TONEAREST,FE_DOWNWARD,FE_UPWARD,FE_TOWARDZERO};
 for(unsigned m=0;m<4;m++){
  CHECK(fesetround(modes[m])==0);int before=fegetround();
  CHECK(gh_bgm_event_set_prefix_002d15a8(s,0,0,0)==GH_BGM_UNSUPPORTED&&fegetround()==before);
  CHECK(gh_bgm_requests_pending(s)==1&&!gh_bgm_requests_active(s)&&gh_bgm_requests_frontier(s)==0x329438);
  CHECK(gh_bgm_requests_inspect(s,0,&v)==GH_BGM_OK&&v.slot==0&&v.event==0&&v.logical_id==128&&v.mode==5&&v.request_flags==1&&v.stream_flags==1);
  CHECK(v.member==0&&v.member_offset==2048&&v.member_bytes==7168000&&v.member_offset==off&&v.member_bytes==size);
  CHECK(v.rate==48000&&v.original_buffer_bytes==252388&&v.afd_channels==1&&v.adx_channels==2&&v.adx_version==4&&v.adx_flags==8);
  CHECK(v.original_scalar_bytes==0xdc&&u32(v.original_scalars+0x34)==0x3f4ccccd&&u32(v.original_scalars+0x48)==1&&u32(v.original_scalars+0xb0)==0);
  CHECK(!v.linked&&!v.prepared&&v.afd_row.data!=row.data&&!memcmp(v.afd_row.data,row.data,32));
  CHECK(v.encoded.data&&v.encoded.size==size&&gh_heap_used(h)>=size+v.original_buffer_bytes);
  uint8_t bytes[16384];for(uint64_t at=0;at<size;){size_t n=size-at>sizeof(bytes)?sizeof(bytes):(size_t)(size-at);CHECK(gh_disc_read(d,fi,off+at,bytes,n)==GH_DISC_OK&&!memcmp(bytes,v.encoded.data+(size_t)at,n));at+=n;}
  CHECK(gh_bgm_event_set_prefix_002d15a8(s,0,0,0)==GH_BGM_BUSY);
  CHECK(gh_bgm_requests_abort_pending(s)==GH_BGM_OK&&!gh_bgm_requests_pending(s)&&!gh_heap_used(h));
 }
 CHECK(fesetround(FE_TONEAREST)==0);
 CHECK(gh_bgm_event_set_prefix_002d15a8(s,56,-1,9)==GH_BGM_BOUNDS&&gh_bgm_requests_frontier(s)==0x373158&&!gh_heap_used(h));
 CHECK(gh_bgm_requests_inspect(s,0,&v)==GH_BGM_OK&&v.mode==5&&v.original_scalars[0xa0]==255&&v.original_scalars[0xa1]==255&&v.original_scalars[0xa2]==9);
 CHECK(gh_bgm_requests_abort_pending(s)==GH_BGM_OK&&gh_bgm_requests_inspect(s,0,&v)==GH_BGM_NOT_READY);
 /* Nativeconstructor completes only after actualkey/sourcevalidation and
  * endpointprepare. Originalmode5 neverrequestsstart byitself. */
 size_t elf_size=(size_t)gh_disc_boot_elf_size(d);uint8_t*elf=malloc(elf_size);gh_adx_key key;
 CHECK(elf&&!gh_disc_read_boot_elf(d,0,elf,elf_size)&&gh_adx_key_from_elf(elf,elf_size,&key)==GH_ADX_OK);free(elf);
 CHECK(!gh_bgm_requests_enable_native(s,&key,1,GH_BGM_GAIN_NATIVE_DB_REFERENCE));
 CHECK(gh_bgm_requests_enable_native(s,&key,1,GH_BGM_GAIN_NATIVE_DB_REFERENCE)==GH_BGM_BUSY);
 CHECK(!gh_bgm_event_set_002d15a8(s,0,0,0)&&!gh_bgm_requests_pending(s)&&gh_bgm_requests_active(s)==1);
 CHECK(!gh_bgm_requests_inspect(s,0,&v)&&v.linked&&v.prepared&&v.transport.prepared&&!v.transport.started&&!v.transport.submitted_frames&&!v.transport.queued_buffers&&v.transport.muted);
 CHECK(u32(v.original_scalars+0xb0)==1&&u32(v.original_scalars+0x14)==0x42900000&&v.stream_flags==3&&v.transport.gain>.4027f&&v.transport.gain<.4028f);
 CHECK(!gh_bgm_event_set_002d15a8(s,0,0,0)&&gh_bgm_requests_active(s)==2&&!gh_bgm_requests_inspect(s,1,&v));
 CHECK(gh_bgm_requests_abort_pending(s)==GH_BGM_BUSY);
 uint32_t old_ac=control.flags_ac,old_b0=control.flags_b0;
 CHECK(!gh_bgm_event_fade_002d1768(s,55,15,0,-1)&&control.flags_ac==old_ac&&control.flags_b0==old_b0);
 CHECK(!gh_bgm_event_fade_002d1768(s,0,15,0,-1)&&!gh_bgm_requests_inspect(s,1,&v)&&u32(v.original_scalars+0x18)==0&&u32(v.original_scalars+0x1c)==0x41700000);
 CHECK(!gh_bgm_requests_inspect(s,0,&v)&&u32(v.original_scalars+0x18)==0x3f800000&&control.flags_ac==old_ac&&control.flags_b0==old_b0);
 CHECK(gh_bgm_event_fade_002d1768(s,0,15,0,1)==GH_BGM_UNSUPPORTED&&gh_bgm_requests_frontier(s)==0x373938);
 CHECK(!gh_bgm_event_fade_002d1768(s,0,1,1,-1));

 gh_boot_work work={0};for(unsigned m=0;m<4;m++){CHECK(!fesetround(modes[m]));int before=fegetround();CHECK(!gh_bgm_requests_update_002d0938(s,&work)&&fegetround()==before);}
 CHECK(!fesetround(FE_TONEAREST)&&!gh_bgm_requests_inspect(s,0,&v)&&!v.transport.started&&!v.transport.completed_frames&&!v.transport.submitted_frames);
 CHECK(!gh_bgm_event_start_002d1610(s,0,0,0)&&!gh_bgm_requests_inspect(s,1,&v)&&v.transport.started&&v.transport.submitted_frames>0&&v.transport.muted&&!v.request_flags);
 CHECK((control.flags_ac&0x10000000)&&(control.flags_b0&0x10000000));
 for(unsigned spin=0;spin<200&&!v.transport.completed_frames;spin++){Sleep(10);CHECK(!gh_bgm_requests_update_002d0938(s,&work)&&!gh_bgm_requests_inspect(s,1,&v));}
 CHECK(v.transport.completed_frames>0&&v.transport.submitted_frames>=v.transport.completed_frames);
 const uint8_t*old_source=v.encoded.data;
 CHECK(!fesetround(FE_DOWNWARD));
 CHECK(!gh_bgm_event_start_002d1610(s,0,0,0)&&!gh_bgm_requests_inspect(s,2,&v)&&v.transport.started&&v.transport.submitted_frames&&v.request_flags==0&&!(v.stream_flags&2));
 CHECK(fegetround()==FE_DOWNWARD&&v.encoded.data!=old_source&&gh_bgm_requests_active(s)==3);
 CHECK(!fesetround(FE_TONEAREST));
 CHECK(!gh_bgm_requests_inspect(s,1,&v)&&(v.request_flags&0x1000)&&u32(v.original_scalars+0x1c)==0x41200000);
 CHECK(!gh_bgm_event_stop_002d1800(s,0)&&!(control.flags_ac&0x10000000)&&!(control.flags_b0&0x10000000));
 /* Actual mask50==0 branch fades tozero over originalrelease60ticks, then
  * tearsdownstream; followingtick removesrequest fromtypedactivelist. */
 CHECK(control.event_mask50==0x80000000);control.event_mask50=0;
 for(unsigned i=0;i<60;i++)CHECK(!gh_bgm_requests_update_002d0938(s,&work));
 CHECK(gh_bgm_requests_active(s)==3&&!gh_heap_used(h));
 CHECK(!gh_bgm_requests_update_002d0938(s,&work)&&!gh_bgm_requests_active(s)&&!gh_bgm_requests_pending(s));
 CHECK(gh_bgm_requests_inspect(s,0,&v)==GH_BGM_NOT_READY);
 control.flags_ac|=0x10000000;CHECK(!gh_bgm_requests_update_002d0938(s,&work)&&!(control.flags_ac&0x10000000));control.flags_ac|=0x200000;CHECK(gh_bgm_requests_update_002d0938(s,&work)==GH_BGM_UNSUPPORTED&&gh_bgm_requests_frontier(s)==0x2d0d70);control.flags_ac&=~0x200000u;
 CHECK(!gh_bgm_requests_abort_pending(s));
 /* Actualcase3tail doesNOTpreserveTitleevent0: room20 has nostaticmaprow,
  * original2D0B88 fallback selects2 andfades event0 over15ticks. */
 control.event_mask50=0x80000000;gh_game_children*children;gh_game_init_state*init=malloc(sizeof(*init));CHECK(init&&!gh_game_children_create(NULL,&children)&&!gh_game_children_initialize_0015fd98(children));gh_game_init_state_initialize(init);work.room=0x20;
 CHECK(!gh_bgm_event_set_002d15a8(s,0,0,0)&&!gh_bgm_requests_scene_begin_002d0850(s,&work,children,init));int32_t selected;
 CHECK(gh_bgm_requests_scene_event(s,&selected)&&selected==2&&!gh_bgm_requests_inspect(s,0,&v)&&u32(v.original_scalars+0x18)==0&&u32(v.original_scalars+0x1c)==0x41700000);
 for(unsigned i=0;i<16;i++)CHECK(!gh_bgm_requests_update_002d0938(s,&work));
 CHECK(!gh_bgm_requests_active(s)&&!gh_heap_used(h));
 CHECK(!gh_bgm_event_start_002d1610(s,0,0,0)&&!gh_bgm_requests_inspect(s,0,&v)&&v.transport.started&&v.transport.submitted_frames&&v.request_flags==0&&v.transport.muted);
 for(unsigned spin=0;spin<200&&!v.transport.completed_frames;spin++){Sleep(10);CHECK(!gh_bgm_requests_update_002d0938(s,&work)&&!gh_bgm_requests_inspect(s,0,&v));}
 CHECK(v.transport.completed_frames>0);
 CHECK(!gh_bgm_event_stop_002d1800(s,0));for(unsigned i=0;i<61;i++)CHECK(!gh_bgm_requests_update_002d0938(s,&work));CHECK(!gh_bgm_requests_active(s)&&!gh_heap_used(h));
 free(init);gh_game_children_destroy(children);
 gh_bgm_requests_destroy(s);CHECK(gh_heap_destroy(h)==GH_MEM_OK);gh_audio_registry_destroy(r);gh_disc_close(d);
 puts("PASS realBGM0 nativeSet/no-autoplay/actual-muted-Start-consumption/Stop/fade/list/tick/source/FEnv");return 0;
}
