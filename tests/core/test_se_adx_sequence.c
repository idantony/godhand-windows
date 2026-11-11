#include "se_voice.h"
#include "sound_control.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(expr) do { \
    if (!(expr)) { fprintf(stderr,"line %d: %s\n",__LINE__,#expr); exit(1); } \
} while (0)

typedef struct {
    size_t frames;
    uint64_t hash;
    float gain;
    unsigned active,stops;
} recorded_voice;

static struct {
    recorded_voice voices[160];
    unsigned count,fail_next;
} transport;

static uint64_t pcm_hash(const int16_t *pcm,size_t frames)
{
    uint64_t hash=UINT64_C(14695981039346656037);
    for(size_t i=0;i<frames;i++){
        const uint16_t word=(uint16_t)pcm[i];
        hash=(hash^(word&255u))*UINT64_C(1099511628211);
        hash=(hash^(word>>8))*UINT64_C(1099511628211);
    }
    return hash;
}

gh_voice_status __wrap_gh_audio_output_play(gh_audio_output *output,const gh_adpcm_pcm *pcm,
    uint32_t rate,float left,float right,uint32_t *handle)
{
    CHECK(output==(gh_audio_output *)&transport&&pcm&&pcm->samples&&pcm->frames&&rate==48000&&left==right);
    if(transport.fail_next){transport.fail_next=0;return GH_VOICE_PLATFORM;}
    CHECK(transport.count<sizeof(transport.voices)/sizeof(transport.voices[0]));
    transport.voices[transport.count]=(recorded_voice){pcm->frames,pcm_hash(pcm->samples,pcm->frames),left,1,0};
    *handle=++transport.count;
    return GH_VOICE_OK;
}

gh_voice_status __wrap_gh_audio_output_active(gh_audio_output *output,uint32_t handle,int *active,uint64_t *played)
{
    CHECK(output==(gh_audio_output *)&transport&&handle&&handle<=transport.count);
    const recorded_voice *voice=transport.voices+handle-1;
    *active=(int)voice->active;
    *played=*active?1u:voice->frames;
    return GH_VOICE_OK;
}

gh_voice_status __wrap_gh_audio_output_stop(gh_audio_output *output,uint32_t handle)
{
    CHECK(output==(gh_audio_output *)&transport&&handle&&handle<=transport.count);
    recorded_voice *voice=transport.voices+handle-1;
    ++voice->stops;
    voice->active=0;
    return GH_VOICE_OK;
}

gh_voice_status __wrap_gh_audio_output_voice_gain(gh_audio_output *output,uint32_t handle,float left,float right)
{
    CHECK(output==(gh_audio_output *)&transport&&handle&&handle<=transport.count&&left==right);
    transport.voices[handle-1].gain=left;
    return GH_VOICE_OK;
}

gh_voice_status __wrap_gh_audio_output_voice_pitch_ratio(gh_audio_output *output,uint32_t handle,
    uint16_t target,uint16_t submitted)
{
    (void)output;(void)handle;(void)target;(void)submitted;
    return GH_VOICE_UNSUPPORTED;
}

static gh_se_voice_engine *new_engine(const gh_audio_registry *registry,gh_disc *disc,
    const gh_sound_control *control,const void *elf,size_t elf_bytes)
{
    gh_se_voice_engine *voices=NULL;
    gh_se_mix mix;
    CHECK(!gh_se_mix_boot_defaults_002c9e78(&mix,0,GH_SE_DSP_NATIVE_LINEAR_REFERENCE));
    CHECK(!gh_se_voice_create(registry,(gh_audio_output *)&transport,&mix,&voices));
    CHECK(!gh_se_voice_bind_origin_seh(voices,NULL,0));
    uint32_t handle=99;
    CHECK(gh_se_voice_request_origin(voices,2,0x32,-1,-1,0,0,&handle)==GH_VOICE_UNSUPPORTED&&
        !handle&&gh_se_voice_frontier(voices)==0x2cb8a0);
    CHECK(!gh_se_voice_bind_external_adx(voices,disc,control,elf,elf_bytes));
    CHECK(!gh_se_voice_bind_external_adx(voices,disc,control,elf,elf_bytes));
    return voices;
}

static uint32_t request(gh_se_voice_engine *voices,int16_t program)
{
    uint32_t handle=0;
    CHECK(!gh_se_voice_request_origin(voices,2,program,-1,-1,0,0,&handle)&&handle);
    return handle;
}

static gh_se_voice_info voice_info(const gh_se_voice_engine *voices,uint32_t handle)
{
    gh_se_voice_info result;
    CHECK(!gh_se_voice_query(voices,handle,&result));
    return result;
}

static void check_gain(uint32_t native,float gain)
{
    CHECK(native&&native<=transport.count&&fabsf(transport.voices[native-1].gain-gain)<0.000001f);
}

static void check_started(const gh_se_voice_engine *voices,uint32_t handle,int16_t program,unsigned slot)
{
    const gh_se_voice_info info=voice_info(voices,handle);
    CHECK(!info.pending&&info.channel==slot&&info.bank==2&&info.program==program&&
        info.native_voice&&info.rate==48000&&info.pitch_q12==4096&&!info.adsr1&&!info.adsr2);
    const recorded_voice *actual=transport.voices+info.native_voice-1;
    if(program==0x32)
        CHECK(info.sample_slot==4&&actual->frames==80692&&actual->hash==UINT64_C(0x212c969b8d1926a8));
    else
        CHECK(program==0x33&&info.sample_slot==6&&actual->frames==53375&&actual->hash==UINT64_C(0x639e7618e6d5eaa4));
}

static void exercise(gh_se_voice_engine *voices,gh_sound_control *control)
{
    uint32_t unknown=0;
    CHECK(gh_se_voice_request_origin(voices,2,0x34,-1,-1,0,0,&unknown)==GH_VOICE_UNSUPPORTED&&
        !unknown&&gh_se_voice_frontier(voices)==0x2cb8a0);
    const uint32_t first=request(voices,0x32),second=request(voices,0x33);
    CHECK(!transport.count&&voice_info(voices,first).pending&&voice_info(voices,second).pending);
    CHECK(!gh_se_voice_request_origin(voices,2,0x32,-1,-1,0,0,&unknown)&&!unknown);
    transport.fail_next=1;
    CHECK(gh_se_voice_update(voices)==GH_VOICE_PLATFORM&&gh_se_voice_frontier(voices)==0x374df8);
    CHECK(!transport.count&&voice_info(voices,first).pending&&voice_info(voices,second).pending);
    CHECK(!gh_se_voice_update(voices)&&transport.count==2);
    check_started(voices,second,0x33,48);
    check_started(voices,first,0x32,49);
    const uint32_t first_native=voice_info(voices,first).native_voice;
    const uint32_t second_native=voice_info(voices,second).native_voice;
    check_gain(first_native,0.63095734f);
    check_gain(second_native,0.63095734f);
    CHECK(!gh_se_voice_request_origin(voices,2,0x33,-1,-1,0,0,&unknown)&&!unknown);
    for(unsigned i=0;i<3;i++)CHECK(!gh_se_voice_update(voices));

    /* Equal priority, both links equally old. 37CF00 selects the last tie. */
    const uint32_t saved_category=control->gain_bits[4];
    control->gain_bits[4]=0x42800000u; /* The live group-1 category becomes 64. */
    const uint32_t replacement=request(voices,0x32);
    CHECK(!gh_se_voice_update(voices)&&transport.count==3);
    CHECK(gh_se_voice_query(voices,first,&(gh_se_voice_info){0})==GH_VOICE_BOUNDS);
    CHECK(transport.voices[first_native-1].stops==1&&!transport.voices[first_native-1].active);
    check_started(voices,replacement,0x32,49);
    check_started(voices,second,0x33,48);
    check_gain(voice_info(voices,replacement).native_voice,0.31622776f);
    control->gain_bits[4]=saved_category;

    const uint32_t replacement_native=voice_info(voices,replacement).native_voice;
    int accepted=0;
    CHECK(!gh_se_voice_request_stop_00375050(voices,replacement,2,&accepted)&&accepted);
    CHECK(!gh_se_voice_update(voices)&&voice_info(voices,replacement).stopping);
    check_gain(replacement_native,0.31622776f);
    CHECK(!gh_se_voice_update(voices));
    check_gain(replacement_native,0.00001584893f);
    CHECK(!gh_se_voice_update(voices)&&!transport.voices[replacement_native-1].active);
    CHECK(!gh_se_voice_update(voices)&&gh_se_voice_bank_nodes(voices,2)==1);
    transport.voices[second_native-1].active=0;
    CHECK(!gh_se_voice_update(voices)&&!gh_se_voice_bank_nodes(voices,2));
}

static void check_shared_queue(gh_se_voice_engine *voices)
{
    const unsigned initial=transport.count;
    const uint32_t stream=request(voices,0x32);
    for(unsigned i=0;i<48;i++){
        uint32_t handle=0;
        CHECK(!gh_se_voice_request_origin(voices,0,0x15e,-1,-1,0,0,&handle)&&handle);
    }
    CHECK(!gh_se_voice_update(voices));
    const gh_se_voice_info info=voice_info(voices,stream);
    CHECK(!info.pending&&!info.native_voice&&info.channel==255);
    CHECK(!gh_se_voice_update(voices)&&!gh_se_voice_bank_nodes(voices,2));
    CHECK(transport.count>initial);
    for(unsigned i=initial;i<transport.count;i++)CHECK(transport.voices[i].frames!=80692);
}

static void check_stop_order(gh_se_voice_engine *voices)
{
    const uint32_t older=request(voices,0x32),newer=request(voices,0x33);
    for(unsigned i=0;i<4;i++)CHECK(!gh_se_voice_update(voices));
    check_started(voices,newer,0x33,48);
    check_started(voices,older,0x32,49);
    const uint32_t stopped=voice_info(voices,newer).native_voice;
    const uint32_t kept=voice_info(voices,older).native_voice;
    int accepted=0;
    CHECK(!gh_se_voice_request_stop_00375050(voices,newer,0,&accepted)&&accepted);
    const uint32_t next=request(voices,0x33);
    CHECK(!gh_se_voice_update(voices));
    check_started(voices,next,0x33,48);
    check_started(voices,older,0x32,49);
    CHECK(voice_info(voices,older).native_voice==kept&&transport.voices[kept-1].active&&
        transport.voices[stopped-1].stops==1&&!transport.voices[stopped-1].active);
}

static uint8_t *read_asset(gh_disc *disc,const char *path,size_t *size)
{
    size_t index;
    gh_disc_entry entry;
    CHECK(!gh_disc_find(disc,path,&index)&&!gh_disc_entry_at(disc,index,&entry));
    uint8_t *bytes=malloc((size_t)entry.size);
    CHECK(bytes&&!gh_disc_read(disc,index,0,bytes,(size_t)entry.size));
    *size=(size_t)entry.size;
    return bytes;
}

int main(int argc,char **argv)
{
    CHECK(argc==2);
    gh_disc *disc=NULL;
    gh_audio_registry *registry=NULL;
    CHECK(!gh_disc_open(argv[1],&disc)&&!gh_audio_registry_create(&registry));
    CHECK(!gh_audio_startup_load(registry,disc));
    gh_sound_control control={0};
    CHECK(!gh_sound_control_from_startup(&control,disc,registry));
    size_t ses_bytes;
    uint8_t *ses=read_asset(disc,"se/r100.ses",&ses_bytes);
    gh_audio_token token;
    CHECK(!gh_audio_commit_se_with_external_index(registry,2,ses,ses_bytes,disc,&token)&&
        gh_audio_token_complete(registry,token));
    free(ses);
    const size_t elf_bytes=(size_t)gh_disc_boot_elf_size(disc);
    uint8_t *elf=malloc(elf_bytes);
    CHECK(elf&&!gh_disc_read_boot_elf(disc,0,elf,elf_bytes));

    gh_se_voice_engine *voices=new_engine(registry,disc,&control,elf,elf_bytes);
    exercise(voices,&control);
    gh_se_voice_destroy(voices);
    voices=new_engine(registry,disc,&control,elf,elf_bytes);
    check_stop_order(voices);
    gh_se_voice_destroy(voices);
    voices=new_engine(registry,disc,&control,elf,elf_bytes);
    check_shared_queue(voices);
    gh_se_voice_destroy(voices);
    free(elf);
    gh_audio_registry_destroy(registry);
    gh_disc_close(disc);
    puts("PASS actual R100 external mono AFD4/6, shared queue, two sequence links, priority, live gain, ramp, retry and EOS");
    return 0;
}
