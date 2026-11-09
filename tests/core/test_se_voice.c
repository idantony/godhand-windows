#include "se_voice.h"
#include "../platform/disc.h"
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#define C(x) do{if(!(x)){fprintf(stderr,"FAIL %d %s\n",__LINE__,#x);exit(1);}}while(0)
static void pcm(gh_se_voice_engine *s,uint32_t handle,size_t expected,uint64_t digest){const int16_t *data;size_t frames;uint64_t hash=UINT64_C(14695981039346656037);C(gh_se_voice_pcm_view(s,handle,&data,&frames)==GH_VOICE_OK && frames==expected);for(size_t i=0;i<frames;++i){uint16_t n=(uint16_t)data[i];hash=(hash^(n&255))*UINT64_C(1099511628211);hash=(hash^(n>>8))*UINT64_C(1099511628211);}C(hash==digest);}
int main(int argc,char **argv){
 gh_disc *disc;gh_disc_entry file;size_t index;void *data;gh_audio_registry *registry;gh_audio_token token;gh_audio_output *output;int32_t native;gh_se_voice_engine *engine;gh_se_mix mix={0x7f00,1,0,GH_SE_DSP_NATIVE_LINEAR_REFERENCE};gh_se_voice_info info;uint32_t handle[8],unknown;
 C(argc==2 && gh_disc_open(argv[1],&disc)==GH_DISC_OK && gh_disc_find(disc,"se/core.ses",&index)==GH_DISC_OK && gh_disc_entry_at(disc,index,&file)==GH_DISC_OK);
 data=malloc((size_t)file.size);C(data && gh_disc_read(disc,index,0,data,(size_t)file.size)==GH_DISC_OK && gh_audio_registry_create(&registry)==GH_AUDIO_OK && gh_audio_commit_se(registry,0,data,(size_t)file.size,&token)==GH_AUDIO_OK);free(data);
 gh_voice_status status=gh_audio_output_create(&output,&native);if(status){fprintf(stderr,"real XAudio2 unavailable0x%08lx\n",(unsigned long)(uint32_t)native);return 2;}
 C(gh_audio_output_set_gain(output,0)==GH_VOICE_OK); /* actual voices process, no repeated QA sound */
 gh_se_mix defaults;C(gh_se_mix_boot_defaults_002c9e78(&defaults,0x4000,GH_SE_DSP_NATIVE_LINEAR_REFERENCE)==GH_VOICE_OK && defaults.se_level_q8==0x5a00 && defaults.output_mode==1 && defaults.system_flags==0x4000);
 C(gh_se_voice_create(registry,output,&mix,&engine)==GH_VOICE_OK);
 C(gh_se_voice_request_origin(engine,0,0x160,-1,-1,0,0,&handle[0])==GH_VOICE_OK && handle[0]);
 C(gh_se_voice_query(engine,handle[0],&info)==GH_VOICE_OK && info.pending && !info.native_voice && info.sample_slot==172 && info.pitch_q12==1365 && info.rate==16000 && info.adsr1==0x80ff && info.adsr2==0x5fc0 && info.level_q14==16383);
 pcm(engine,handle[0],60915,UINT64_C(0xfb441e2ef9433f43));
 C(gh_se_voice_request_origin(engine,0,0x161,-1,-1,0,0,&handle[1])==GH_VOICE_OK);
 pcm(engine,handle[1],23053,UINT64_C(0xd4d01775af4b4818));
 C(gh_se_voice_update(engine)==GH_VOICE_OK);
 C(gh_se_voice_query(engine,handle[1],&info)==GH_VOICE_OK && !info.pending && info.channel==0 && info.native_voice && info.sample_slot==173 && info.pitch_q12==1194);
 C(gh_se_voice_query(engine,handle[0],&info)==GH_VOICE_OK && info.channel==1);
 C(gh_se_voice_request_origin(engine,0,1,-1,-1,0,0,&unknown)==GH_VOICE_UNSUPPORTED && !unknown && gh_se_voice_frontier(engine)==0x2cb8a0);
 /* Five channels match original group0..4. Equal priority may replace oldest;
  * newest-first list means the oldest of six queued handles is consumed last. */
 for(unsigned i=2;i<8;++i)C(gh_se_voice_request_origin(engine,0,0x15e,-1,-1,0,0,&handle[i])==GH_VOICE_OK);
 pcm(engine,handle[2],29660,UINT64_C(0x58f608fb773f8d88));
 C(gh_se_voice_update(engine)==GH_VOICE_OK);
 unsigned active_count=0;for(unsigned i=0;i<8;++i)if(gh_se_voice_query(engine,handle[i],&info)==GH_VOICE_OK)++active_count;
 C(active_count==5);
 C(gh_se_voice_query(engine,handle[0],&info)==GH_VOICE_BOUNDS && gh_se_voice_query(engine,handle[1],&info)==GH_VOICE_BOUNDS && gh_se_voice_query(engine,handle[5],&info)==GH_VOICE_BOUNDS);
 for(unsigned i=2;i<8;++i)if(i!=5)C(gh_se_voice_query(engine,handle[i],&info)==GH_VOICE_OK);
 ULONGLONG end=GetTickCount64()+4000;uint64_t progress=0;
 do{C(gh_se_voice_update(engine)==GH_VOICE_OK);active_count=0;
  for(unsigned i=0;i<8;++i)if(gh_se_voice_query(engine,handle[i],&info)==GH_VOICE_OK){int active;uint64_t played;++active_count;C(gh_audio_output_active(output,info.native_voice,&active,&played)==GH_VOICE_OK);if(played>progress)progress=played;}
  if(!active_count)break;Sleep(10);
 }while(GetTickCount64()<end);
 C(!active_count && progress>1000);
 /* Reuse after deletion must not leave stale active-list links/cycles. */
 C(gh_se_voice_request_origin(engine,0,0x160,-1,-1,0,0,&handle[0])==GH_VOICE_OK && gh_se_voice_update(engine)==GH_VOICE_OK);
 gh_se_voice_destroy(engine);
 C(gh_se_voice_create(registry,output,&mix,&engine)==GH_VOICE_OK);
 uint32_t queued[64];for(unsigned i=0;i<64;++i)C(gh_se_voice_request_origin(engine,0,0x161,-1,-1,0,0,&queued[i])==GH_VOICE_OK && queued[i]);
 C(gh_se_voice_request_origin(engine,0,0x161,-1,-1,0,0,&unknown)==GH_VOICE_OK && !unknown);
 C(gh_se_voice_update(engine)==GH_VOICE_OK);active_count=0;
 for(unsigned i=0;i<64;++i)if(gh_se_voice_query(engine,queued[i],&info)==GH_VOICE_OK)++active_count;
 C(active_count==21); /* five live channels and16 queue-overflow EE voices */
 C(gh_se_voice_update(engine)==GH_VOICE_OK);active_count=0;
 for(unsigned i=0;i<64;++i)if(gh_se_voice_query(engine,queued[i],&info)==GH_VOICE_OK)++active_count;
 C(active_count==5);
 gh_se_voice_destroy(engine);
 C(gh_se_voice_create(registry,output,&defaults,&engine)==GH_VOICE_OK);
 C(gh_se_voice_request_origin(engine,0,0x160,-1,-1,0,0,&unknown)==GH_VOICE_OK && unknown);
 C(gh_se_voice_query(engine,unknown,&info)==GH_VOICE_OK && info.level_q14==11610);
 gh_se_voice_destroy(engine);gh_audio_output_destroy(output);gh_audio_registry_destroy(registry);gh_disc_close(disc);
 printf("PASS actualMC program->sample->nativePCM/ADSR->muted realXAudio2 voices; LIFO, five channels, equalpriority replacement, completion/reuse; progress%llu\n",(unsigned long long)progress);return 0;
}
