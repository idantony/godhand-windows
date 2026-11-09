#include "audio_events.h"
#include "audio_startup.h"
#include "disc.h"
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define C(x) do{if(!(x)){fprintf(stderr,"FAIL %d %s\n",__LINE__,#x);exit(1);}}while(0)
static uint64_t digest(const gh_adpcm_pcm *p){uint64_t h=UINT64_C(14695981039346656037);for(size_t i=0;i<p->frames;++i){uint16_t n=(uint16_t)p->samples[i];h=(h^(n&255))*UINT64_C(1099511628211);h=(h^(n>>8))*UINT64_C(1099511628211);}return h;}
int main(int argc,char **argv){
 gh_disc *d;size_t index;gh_disc_entry e;uint8_t *data;gh_audio_registry *r;gh_audio_token t;
 unsigned slots[]={170,172,173};size_t frames[]={9884,20300,6720};uint64_t hashes[]={UINT64_C(0x9184e3560419c90e),UINT64_C(0xa345d9ee6c020f34),UINT64_C(0x716f9bd0cf4ce443)};
 C(argc>=2 && argc<=3 && gh_disc_open(argv[1],&d)==GH_DISC_OK);
 C(gh_disc_find(d,"se/core.ses",&index)==GH_DISC_OK && gh_disc_entry_at(d,index,&e)==GH_DISC_OK);data=malloc((size_t)e.size);
 C(data && gh_disc_read(d,index,0,data,(size_t)e.size)==GH_DISC_OK && gh_audio_registry_create(&r)==GH_AUDIO_OK);
 C(gh_audio_commit_se(r,0,data,(size_t)e.size,&t)==GH_AUDIO_OK);free(data);
 for(unsigned i=0;i<3;++i){gh_sound_span descriptor,tail;gh_adpcm_pcm pcm;C(gh_audio_sample_at(r,0,slots[i],&descriptor,&tail)==GH_AUDIO_OK);
  C(gh_audio_decode_ps_adpcm(tail.data,tail.size,&pcm)==GH_VOICE_OK && pcm.frames==frames[i] && digest(&pcm)==hashes[i]);
  if(argc==3 && i==1){
   gh_audio_output *output;int32_t native;uint32_t voice;int active=1;uint64_t played=0;
   C(!strcmp(argv[2],"--output"));gh_voice_status status=gh_audio_output_create(&output,&native);
   if(status){fprintf(stderr,"real XAudio2 create failed status%d native0x%08lx\n",status,(unsigned long)(uint32_t)native);return 2;}
   C(gh_audio_output_set_gain(output,0)==GH_VOICE_OK);
   C(gh_audio_output_play(output,&pcm,16000,0.05f,0.05f,&voice)==GH_VOICE_OK);
   /* Platform output test gain only; not original game ADSR/mix semantics. */
   ULONGLONG until=GetTickCount64()+4000;uint64_t maximum=0;
   while(active && GetTickCount64()<until){C(gh_audio_output_active(output,voice,&active,&played)==GH_VOICE_OK);if(played>maximum)maximum=played;Sleep(10);}
   /* XAudio2 resets SamplesPlayed on END_OF_STREAM. Observe real progress
    * before completion and an empty source queue, rather than expecting an
    * impossible retained final count. */
   C(!active && maximum>pcm.frames-1000 && maximum<=pcm.frames);gh_audio_output_reap(output);gh_audio_output_destroy(output);
   printf("PASS actual XAudio2 buffer completion, observed%llu frames before EOS reset (test gain only)\n",(unsigned long long)maximum);
  }
  gh_audio_pcm_release(&pcm);
 }
 {uint8_t block[16]={0};gh_adpcm_pcm pcm;C(gh_audio_decode_ps_adpcm(block,16,&pcm)==GH_VOICE_BOUNDS);block[1]=3;C(gh_audio_decode_ps_adpcm(block,16,&pcm)==GH_VOICE_UNSUPPORTED);block[1]=1;block[0]=0x50;C(gh_audio_decode_ps_adpcm(block,16,&pcm)==GH_VOICE_UNSUPPORTED);block[0]=0;memset(block+2,0x87,14);C(gh_audio_decode_ps_adpcm(block,16,&pcm)==GH_VOICE_OK && pcm.frames==28 && pcm.samples[0]==28672 && pcm.samples[1]==-32768);gh_audio_pcm_release(&pcm);}
 gh_audio_registry_destroy(r);gh_disc_close(d);puts("PASS three actual embedded ADPCM samples match independent fullPCM digests; bounds/loop/codec gates; not full SeCall/ADSR equivalence");return 0;
}
