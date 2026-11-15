#include "se_voice.h"
#include "sound_control.h"
#include "adx.h"
#include "game_actor_model.h"

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
    float gain,right_gain;
    unsigned active,stops,channels;
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

static gh_voice_status record_play(gh_audio_output *output,const gh_adpcm_pcm *pcm,
    uint32_t rate,float left,float right,uint32_t *handle,unsigned channels)
{
    CHECK(output==(gh_audio_output *)&transport&&pcm&&pcm->samples&&pcm->frames&&rate==48000);
    if(transport.fail_next){transport.fail_next=0;return GH_VOICE_PLATFORM;}
    CHECK(transport.count<sizeof(transport.voices)/sizeof(transport.voices[0]));
    transport.voices[transport.count]=(recorded_voice){pcm->frames,
        pcm_hash(pcm->samples,pcm->frames*channels),left,right,1,0,channels};
    *handle=++transport.count;
    return GH_VOICE_OK;
}

gh_voice_status __wrap_gh_audio_output_play(gh_audio_output *output,const gh_adpcm_pcm *pcm,
    uint32_t rate,float left,float right,uint32_t *handle)
{
    return record_play(output,pcm,rate,left,right,handle,1);
}

gh_voice_status __wrap_gh_audio_output_play_q12(gh_audio_output *output,const gh_adpcm_pcm *pcm,
    uint16_t pitch,float left,float right,uint32_t *handle)
{
    CHECK(pcm->loop_first==815u*28u&&pcm->loop_frames==814u*28u&&pitch==1365);
    return record_play(output,pcm,48000,left,right,handle,1);
}

gh_voice_status __wrap_gh_audio_output_play_stereo(gh_audio_output *output,const gh_adpcm_pcm *pcm,
    uint32_t rate,float left,float right,uint32_t *handle)
{
    return record_play(output,pcm,rate,left,right,handle,2);
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
    CHECK(output==(gh_audio_output *)&transport&&handle&&handle<=transport.count);
    transport.voices[handle-1].gain=left;
    transport.voices[handle-1].right_gain=right;
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
    gh_voice_status status=gh_se_voice_request_origin(voices,2,program,-1,-1,0,0,&handle);
    if(status)fprintf(stderr,"origin %x status %d frontier %x\n",program,status,gh_se_voice_frontier(voices));
    CHECK(!status&&handle);
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
    CHECK(actual->channels==1);
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

typedef struct {
    int16_t program;
    unsigned row,member,channels,first_channel;
    uint32_t offset,bytes,frames;
} gate_clip_case;

static void check_gate_clip(gh_se_voice_engine *voices,gh_audio_registry *registry,
    gh_disc *disc,const void *elf,size_t elf_bytes,const char *archive,const gate_clip_case *test)
{
    gh_sound_span afd;
    uint64_t offset,bytes;
    size_t index;
    CHECK(!gh_audio_external_sample(registry,2,test->row,&afd,&offset,&bytes)&&afd.size==32&&
          afd.data[2]==test->member&&offset==test->offset&&bytes==test->bytes);
    CHECK(!gh_disc_find(disc,archive,&index));
    uint8_t *encoded=malloc((size_t)bytes);
    CHECK(encoded&&!gh_disc_read(disc,index,offset,encoded,(size_t)bytes));
    gh_adx_key key;
    gh_adx_decoder *decoder;
    gh_adx_info clip;
    CHECK(!gh_adx_key_from_elf(elf,elf_bytes,&key)&&!gh_adx_open(encoded,(size_t)bytes,&key,&decoder));
    CHECK(!gh_adx_inspect(decoder,&clip)&&clip.channels==test->channels&&
          clip.sample_count==test->frames&&!clip.loop_enabled);
    int16_t *samples=malloc((size_t)clip.sample_count*clip.channels*sizeof(*samples));
    CHECK(samples);
    size_t frames;
    const gh_adx_error status=clip.channels==2?gh_adx_decode(decoder,samples,clip.sample_count,&frames):
        gh_adx_decode_mono(decoder,samples,clip.sample_count,&frames);
    CHECK(!status&&frames==clip.sample_count);
    if(clip.channels==2){
        size_t distinct=0;
        for(size_t i=0;i<frames;i++)distinct+=samples[i*2]!=samples[i*2+1];
        CHECK(distinct>10000);
    }
    const uint64_t expected=pcm_hash(samples,frames*clip.channels);
    gh_adx_close(decoder);
    free(samples);
    free(encoded);

    const unsigned submitted=transport.count;
    const uint32_t handle=request(voices,test->program);
    CHECK(voice_info(voices,handle).pending&&gh_se_voice_external_adx_nodes(voices)==1);
    uint32_t duplicate=99;
    CHECK(!gh_se_voice_request_origin(voices,2,test->program,-1,-1,0,0,&duplicate)&&!duplicate);
    CHECK(!gh_se_voice_update(voices)&&transport.count==submitted+1);
    const gh_se_voice_info info=voice_info(voices,handle);
    CHECK(!info.pending&&info.channel==test->first_channel&&info.sample_slot==test->row&&info.native_voice==submitted+1);
    const recorded_voice *recorded=transport.voices+submitted;
    CHECK(recorded->channels==test->channels&&recorded->frames==frames&&recorded->hash==expected);
}

static gh_voice_status grab_probe(void *context,const float position[4],int update,
    gh_se_attached_environment *environment,uint32_t *frontier)
{
    (void)context;
    (void)position;
    (void)update;
    *environment=(gh_se_attached_environment){.output_mode=1};
    *frontier=0;
    return GH_VOICE_OK;
}

static void check_paired_player_grab(gh_audio_registry *registry,gh_disc *disc,
    const gh_sound_control *control,const void *elf,size_t elf_bytes,uint16_t room)
{
    gh_se_voice_engine *voices=new_engine(registry,disc,control,elf,elf_bytes);
    gh_boot_work work={.room=(int16_t)room};
    uint8_t raw[0x5ac]={0};
    memcpy(raw+0x2f4,&(const uint16_t){0x0a01},2);
    memcpy(raw+0x2fe,&(const uint16_t){0x100},2);
    gh_actor_model model={.bound=1,.parts_ready=1,.work=&work,
        .root={.bytes=raw,.size=sizeof(raw),.alias_f0=raw+0xb0,.original_vtable214=0x41d220}};
    model.native_identity=&model;
    uint32_t handle=0, duplicate=99;
    CHECK(!gh_se_voice_request_attached(voices,&model,&handle,&model,-1,0,7,0,0,
                                      grab_probe,NULL,NULL,NULL,&handle)&&handle);
    const gh_se_voice_info info=voice_info(voices,handle);
    CHECK(info.pending&&info.sample_slot==20&&info.rate==48000&&model.sound_borrowers==1);
    CHECK(!gh_se_voice_request_attached(voices,&model,&duplicate,&model,-1,0,7,0,0,
                                      grab_probe,NULL,NULL,NULL,&duplicate)&&!duplicate&&model.sound_borrowers==1);
    gh_se_voice_destroy(voices);
    CHECK(!model.sound_borrowers);
}

static void check_gate_local_cues(gh_se_voice_engine *voices,gh_sound_control *control)
{
    const unsigned before=transport.count;
    const uint32_t duck=UINT32_C(0x04000000),ac=control->flags_ac|0x10u,b0=control->flags_b0|0x20u;
    control->flags_ac=ac|duck;
    control->flags_b0=b0|duck;
    gh_se_voice_info moving,close;
    const uint32_t p10=request(voices,0x10);
    CHECK(!gh_sound_control_flush_se_002ca5d8(control,voices));
    CHECK(control->flags_ac==(ac&~duck)&&control->flags_b0==(b0&~duck));
    CHECK(!gh_se_voice_query(voices,p10,&moving)&&!moving.pending&&moving.channel==39);
    const uint32_t p11=request(voices,0x11);
    CHECK(!gh_se_voice_query(voices,p11,&close)&&close.pending&&close.sample_slot==6&&
          close.rate==14000&&close.pitch_q12==1194&&close.adsr1==0x80ff&&close.adsr2==0x5fc0);
    CHECK(control->flags_ac==(ac&~duck)&&control->flags_b0==(b0&~duck));
    CHECK(gh_se_voice_update(voices)==GH_VOICE_UNSUPPORTED&&gh_se_voice_frontier(voices)==0x2cda80&&
          transport.count==before+1);
    CHECK(!gh_sound_control_flush_se_002ca5d8(control,voices));
    CHECK(!gh_se_voice_query(voices,p11,&close)&&!close.pending&&close.channel==23);
    CHECK(control->flags_ac==(ac|duck)&&control->flags_b0==(b0|duck));
    CHECK(transport.count==before+2&&transport.voices[moving.native_voice-1].active&&
          !transport.voices[moving.native_voice-1].stops);
    recorded_voice *voice=transport.voices+close.native_voice-1;
    const unsigned left=0x5a00u*(120u<<8)/0x7f00u/2u;
    const unsigned right=0x5a00u*(100u<<8)/0x7f00u/2u;
    CHECK(voice->channels==1&&voice->gain==(float)left/16384.0f&&voice->right_gain==(float)right/16384.0f);
    CHECK(voice->frames==34964&&voice->hash==UINT64_C(0x206a67707738bcc4));
    puts("PASS actual R105 gate movement and close cues coexist, channel gains, PCM, AC/B0 and completion ordering");
    CHECK(!gh_sound_control_flush_se_002ca5d8(control,voices));
    CHECK(control->flags_ac==(ac|duck)&&control->flags_b0==(b0|duck)&&transport.count==before+2);
    voice->active=0;
    CHECK(!gh_sound_control_flush_se_002ca5d8(control,voices));
    CHECK(gh_se_voice_query(voices,p11,&close)==GH_VOICE_BOUNDS);
    CHECK(control->flags_ac==(ac|duck)&&control->flags_b0==(b0|duck));
    CHECK(!gh_sound_control_flush_se_002ca5d8(control,voices));
    CHECK(control->flags_ac==(ac&~duck)&&control->flags_b0==(b0&~duck));
    CHECK(!gh_se_voice_query(voices,p10,&moving)&&transport.voices[moving.native_voice-1].active&&
          !transport.voices[moving.native_voice-1].stops);
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
    const gate_clip_case gate_clips[]={
        {0x0d,2,1,2,49,159744,108166,96079},
        {0x35,10,5,1,48,395264,56218,99818},
        {0x36,12,6,1,49,452608,44248,78542}
    };
    for(unsigned i=0;i<sizeof(gate_clips)/sizeof(gate_clips[0]);i++){
        voices=new_engine(registry,disc,&control,elf,elf_bytes);
        check_gate_clip(voices,registry,disc,elf,elf_bytes,"se/r100.afs",&gate_clips[i]);
        gh_se_voice_destroy(voices);
    }
    check_paired_player_grab(registry,disc,&control,elf,elf_bytes,0x100);
    check_paired_player_grab(registry,disc,&control,elf,elf_bytes,0x105);
    ses=read_asset(disc,"se/r105.ses",&ses_bytes);
    CHECK(!gh_audio_commit_se_with_external_index(registry,2,ses,ses_bytes,disc,&token)&&
        gh_audio_token_complete(registry,token));
    free(ses);
    const gate_clip_case r105_gate={0x32,2,1,1,48,159744,38992,69209};
    voices=new_engine(registry,disc,&control,elf,elf_bytes);
    check_gate_clip(voices,registry,disc,elf,elf_bytes,"se/r105.afs",&r105_gate);
    gh_se_voice_destroy(voices);
    voices=new_engine(registry,disc,&control,elf,elf_bytes);
    check_gate_local_cues(voices,&control);
    gh_se_voice_destroy(voices);
    free(elf);
    gh_audio_registry_destroy(registry);
    gh_disc_close(disc);
    puts("PASS actual R100 stereo AFD2 and mono AFD4/6/10/12, shared queue, sequence links, priority, gain, ramp, retry and EOS");
    return 0;
}
