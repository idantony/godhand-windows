#include "se_voice.h"
#include "audio_startup.h"
#include <stdio.h>
#include <stdlib.h>
#define C(x) do{if(!(x)){fprintf(stderr,"FAIL %d %s\n",__LINE__,#x);exit(1);}}while(0)

static struct {
    size_t frames;
    uint64_t pcm_hash;
    uint32_t rate;
    float left,right;
    unsigned plays,stops;
} recorded;

static uint64_t hash_pcm(const int16_t *pcm,size_t frames)
{
    uint64_t value=UINT64_C(14695981039346656037);
    for(size_t i=0;i<frames;++i){
        const uint16_t sample=(uint16_t)pcm[i];
        value=(value^(sample&255))*UINT64_C(1099511628211);
        value=(value^(sample>>8))*UINT64_C(1099511628211);
    }
    return value;
}

gh_voice_status __wrap_gh_audio_output_play(gh_audio_output *output,const gh_adpcm_pcm *pcm,
    uint32_t rate,float left,float right,uint32_t *handle)
{
    C(output==(gh_audio_output *)&recorded&&pcm&&pcm->samples&&!recorded.plays);
    recorded.frames=pcm->frames;
    recorded.pcm_hash=hash_pcm(pcm->samples,pcm->frames);
    recorded.rate=rate;recorded.left=left;recorded.right=right;
    *handle=++recorded.plays;
    return GH_VOICE_OK;
}
gh_voice_status __wrap_gh_audio_output_active(gh_audio_output *output,uint32_t handle,int *active,uint64_t *frames)
{
    C(output==(gh_audio_output *)&recorded&&handle==1);
    *active=1;*frames=1;
    return GH_VOICE_OK;
}
gh_voice_status __wrap_gh_audio_output_stop(gh_audio_output *output,uint32_t handle)
{
    C(output==(gh_audio_output *)&recorded&&handle==1);
    ++recorded.stops;
    return GH_VOICE_OK;
}

gh_voice_status __wrap_gh_audio_output_voice_gain(gh_audio_output *output,uint32_t handle,float left,float right)
{
    C(output==(gh_audio_output *)&recorded&&handle==1&&left==right);
    return GH_VOICE_OK;
}

gh_voice_status __wrap_gh_audio_output_voice_pitch_ratio(gh_audio_output *output,uint32_t handle,
    uint16_t target,uint16_t submitted)
{
    (void)output;(void)handle;(void)target;(void)submitted;
    return GH_VOICE_UNSUPPORTED;
}

int main(int argc,char **argv)
{
    gh_disc *disc;
    gh_audio_registry *registry;
    gh_se_voice_engine *engine;
    gh_se_mix mix;
    C(argc==2&&gh_disc_open(argv[1],&disc)==GH_DISC_OK);
    C(gh_audio_registry_create(&registry)==GH_AUDIO_OK&&gh_audio_startup_load(registry,disc)==GH_AUDIO_OK);
    C(gh_se_mix_boot_defaults_002c9e78(&mix,0,GH_SE_DSP_NATIVE_LINEAR_REFERENCE)==GH_VOICE_OK);
    C(gh_se_voice_create(registry,(gh_audio_output *)&recorded,&mix,&engine)==GH_VOICE_OK);
    uint32_t handle;
    C(gh_se_voice_request_origin(engine,0,0x15f,0,-1,0,0,&handle)==GH_VOICE_UNSUPPORTED&&
      !handle&&gh_se_voice_frontier(engine)==0x2cb8a0);
    C(gh_se_voice_request_origin(engine,0,0x15f,-1,-1,0,0,&handle)==GH_VOICE_OK&&handle);
    gh_se_voice_info info;
    C(gh_se_voice_query(engine,handle,&info)==GH_VOICE_OK&&info.pending&&info.sample_slot==171);
    C(info.rate==16000&&info.adsr1==0x80ff&&info.adsr2==0x5fc0&&info.pitch_q12==1365);
    const int16_t *pcm;
    size_t frames;
    C(gh_se_voice_pcm_view(engine,handle,&pcm,&frames)==GH_VOICE_OK);
    uint64_t digest=hash_pcm(pcm,frames);
    C(frames==7982&&digest==UINT64_C(0xdedaa41607d4fbd7)&&info.level_q14==6803);
    C(gh_se_voice_update(engine)==GH_VOICE_OK&&recorded.plays==1);
    C(gh_se_voice_query(engine,handle,&info)==GH_VOICE_OK&&!info.pending&&info.channel==0);
    C(recorded.frames==frames&&recorded.pcm_hash==digest&&recorded.rate==48000&&recorded.left==recorded.right);
    gh_se_voice_destroy(engine);
    C(recorded.stops==1);
    gh_audio_registry_destroy(registry);
    gh_disc_close(disc);
    puts("PASS original map CORE0:15F PCM and native voice dispatch without device");
    return 0;
}
