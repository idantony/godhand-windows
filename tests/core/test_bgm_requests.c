#include "bgm_requests.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fenv.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"line%d %s\n",__LINE__,#x);exit(1);}}while(0)
static uint32_t u32(const uint8_t*p){return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
int main(int argc,char**argv){
 gh_disc*d;gh_audio_registry*r;gh_sound_control control={0};gh_heap*h;gh_bgm_requests*s=NULL;
 CHECK(argc==2&&gh_disc_open(argv[1],&d)==GH_DISC_OK&&gh_audio_registry_create(&r)==GH_AUDIO_OK&&gh_heap_create(16*1024*1024,&h)==GH_MEM_OK);
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
 gh_bgm_requests_destroy(s);CHECK(gh_heap_destroy(h)==GH_MEM_OK);gh_audio_registry_destroy(r);gh_disc_close(d);
 puts("PASS realBGM0/request prefix/AFD-ownedsource/workbuffer/FEnv/error; SDKconstructor329438 remains unsupported, no linked/play claim");return 0;
}
