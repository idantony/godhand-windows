#include "se_voice.h"
#include <stdio.h>
#include <stdlib.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#define C(x) do{if(!(x)){fprintf(stderr,"line%d %s\n",__LINE__,#x);exit(1);}}while(0)
static uint64_t hash(const int16_t*p,size_t n){uint64_t h=UINT64_C(14695981039346656037);for(size_t i=0;i<n;i++){uint16_t v=(uint16_t)p[i];h=(h^(v&255))*UINT64_C(1099511628211);h=(h^(v>>8))*UINT64_C(1099511628211);}return h;}
int main(int argc,char**argv){
 gh_disc*d;gh_audio_registry*r;gh_audio_output*out;int32_t error;gh_se_voice_engine*s;gh_se_mix mix;
 C(argc==2&&!gh_disc_open(argv[1],&d)&&!gh_audio_registry_create(&r)&&!gh_audio_startup_load(r,d)&&!gh_audio_output_create(&out,&error)&&!gh_audio_output_set_gain(out,0));
 size_t index;gh_disc_entry file;C(!gh_disc_find(d,"se/r020.ses",&index)&&!gh_disc_entry_at(d,index,&file));uint8_t*bytes=malloc((size_t)file.size);gh_audio_token token;
 C(bytes&&!gh_disc_read(d,index,0,bytes,(size_t)file.size)&&!gh_audio_commit_se(r,2,bytes,(size_t)file.size,&token));free(bytes);
 C(!gh_se_mix_boot_defaults_002c9e78(&mix,0,GH_SE_DSP_NATIVE_LINEAR_REFERENCE)&&!gh_se_voice_create(r,out,&mix,&s)&&!gh_se_voice_bind_origin_seh(s,NULL,0));
 const int programs[4]={0,10,11,12};const uint32_t frames[4]={92736,106806,70634,112733};const uint64_t hashes[4]={UINT64_C(0x260511e539f8f1cf),UINT64_C(0x0d4216ad7632b321),UINT64_C(0x39bfd5d91f8ce1bf),UINT64_C(0x6b6946836e1e6d7d)};uint32_t handles[4];gh_se_voice_info v;
 for(unsigned i=0;i<4;i++){const int16_t*pcm;size_t count;C(!gh_se_voice_request_origin(s,2,(int16_t)programs[i],-1,-1,0,0,&handles[i])&&handles[i]&&!gh_se_voice_query(s,handles[i],&v)&&v.bank==2&&v.program==programs[i]&&v.sample_slot==i&&v.frames==frames[i]&&v.pitch_q12==(i?1877:2048)&&v.adsr1==0x80ff&&v.adsr2==0x5fc0&&v.level_q14==(i?10885:8163));
 C(!gh_se_voice_pcm_view(s,handles[i],&pcm,&count)&&count==frames[i]&&hash(pcm,count)==hashes[i]);printf("program%d frames%zu fnv%016llx\n",programs[i],count,(unsigned long long)hash(pcm,count));}
 uint32_t duplicate=123;C(!gh_se_voice_request_origin(s,2,0,-1,-1,0,0,&duplicate)&&!duplicate);
 C(!gh_se_voice_update(s));const unsigned channels[4]={10,22,21,20};
 for(unsigned i=0;i<4;i++)C(!gh_se_voice_query(s,handles[i],&v)&&v.native_voice&&!v.pending&&v.channel==channels[i]);
 int active;uint64_t played=0;C(!gh_se_voice_query(s,handles[1],&v));
 for(unsigned i=0;i<100&&!played;i++){Sleep(5);C(!gh_audio_output_active(out,v.native_voice,&active,&played));}C(active&&played);
 C(gh_se_voice_request_origin(s,2,1,-1,-1,0,0,&duplicate)==GH_VOICE_UNSUPPORTED&&gh_se_voice_frontier(s)==0x2cb8a0);
 C(gh_audio_token_complete(r,token));gh_se_voice_destroy(s);gh_audio_output_destroy(out);gh_audio_registry_destroy(r);gh_disc_close(d);
 puts("PASS actual R020 bank2 Title0/difficulty10..12 sample/PCM/priority/channel/duplicate and real muted source consumption");return 0;
}
