/* Recovered MC origin SE slice; upstream eaae18d0dfab1f9a0df276ae5ca1581ad326faf3
 * MIT LICENSE.upstream. Actual ELF and user's snddrv.irx evidence:
 * analysis/se-voice-semantics.json. Native DSP adaptation is explicit. */
#include "se_voice.h"
#include "sound_control.h"
#include "../assets/adx.h"
#include "game_actor_model.h"
#include "game_camera.h"
#include "game_actor_scroll_links.h"
#include "game_ray_query.h"
#include "../assets/se_pitch.h"
#include <math.h>
#include <fenv.h>
#include <stdlib.h>
#include <string.h>
#define NONE 255
struct voice {gh_se_voice_info info;gh_adpcm_pcm pcm;uint32_t age,ee_age;uint16_t base_gain;int next;uint8_t used,priority,group_first,group_count,replace_equal;
 gh_actor_model *model;int32_t part_index;const void *source_owner,*request_key;
 gh_se_attached_probe probe;void *probe_context;gh_se_attached_random random;void *random_context;
 uint8_t descriptor[48],setup_stage,variants;int16_t requested_bank,requested_program;
 uint32_t flags18,flags1c;uint16_t selected_pitch,submitted_pitch;int16_t selected_offset;uint8_t pitch_setup,spatial_slow,combat_profile,external_adx;uint16_t spatial_gain;};
typedef struct {
 gh_disc *disc;
 const gh_sound_control *control;
 gh_adx_key key;
 int16_t decibels[128];
} sequence_audio;
struct gh_se_voice_engine {const gh_audio_registry *registry;gh_audio_output *output;gh_se_mix mix;struct voice voices[64];int channels[51],head;uint32_t serial,frontier;const void *origin_seh;size_t origin_seh_size;uint8_t origin_scene_bound;struct {uint32_t handle;int16_t ticks;} stops[48];unsigned stop_count;gh_se_pitch_reference pitch;sequence_audio sequence;gh_se_attached_failure attached_failure;};
static unsigned h(const uint8_t *p){return (unsigned)p[0]|(unsigned)p[1]<<8;}
static uint32_t w(const uint8_t *p){return (uint32_t)h(p)|(uint32_t)h(p+2)<<16;}
static float sound_float(uint32_t bits){float value;memcpy(&value,&bits,4);return value;}
static int32_t floor_shift(int64_t n,unsigned shift){int64_t d=INT64_C(1)<<shift;return (int32_t)(n>=0?n/d:-((-n+d-1)/d));}
static gh_voice_status unsupported(gh_se_voice_engine *s,uint32_t a){s->frontier=a;return GH_VOICE_UNSUPPORTED;}
gh_voice_status gh_se_mix_boot_defaults_002c9e78(gh_se_mix *mix,uint32_t flags,gh_se_dsp_policy policy)
{
 if(!mix)return GH_VOICE_ARGUMENT;if(policy!=GH_SE_DSP_NATIVE_LINEAR_REFERENCE)return GH_VOICE_UNSUPPORTED;
 *mix=(gh_se_mix){0x5a00,1,flags,policy};return GH_VOICE_OK;
}
static void drop(gh_se_voice_engine *s,int i,int stop)
{
 struct voice *v=s->voices+i;
 if(v->info.native_voice && stop)(void)gh_audio_output_stop(s->output,v->info.native_voice);
 if(v->info.channel<51 && s->channels[v->info.channel]==i)s->channels[v->info.channel]=-1;
 int *link=&s->head;while(*link!=-1){if(*link==i){*link=v->next;break;}link=&s->voices[*link].next;}
 if(v->model){(void)gh_actor_model_sound_return(v->model);v->model=NULL;}
 gh_audio_pcm_release(&v->pcm);v->used=0;v->request_key=NULL;
}
/* Actual 80FF with 5FC0 or 5FCB shares the key-on envelope. The renderer
 * ends at the one-shot sample end. The release rate only applies at key-off.
 * Native linear resampling uses the existing pinned envelope reference. */
static gh_voice_status render(gh_adpcm_pcm *pcm,uint16_t pitch)
{
 uint64_t count;int16_t *out;
 uint32_t env=0,counter=0,phase=1;
 if(pcm->frames>(UINT64_MAX-pitch+1)/4096)return GH_VOICE_BOUNDS;
 count=((uint64_t)pcm->frames*4096+pitch-1)/pitch;
 if(count>SIZE_MAX/sizeof(*out))return GH_VOICE_BOUNDS;
 out=malloc((size_t)count*sizeof(*out));if(!out)return GH_VOICE_MEMORY;
 for(uint64_t i=0;i<count;++i){
  uint64_t pos=i*pitch;size_t index=(size_t)(pos>>12);unsigned fraction=(unsigned)(pos&4095);int32_t a=pcm->samples[index],b=index+1<pcm->frames?pcm->samples[index+1]:0,value;
  if(phase==1){counter+=env>0x6000?8192:32768;if(counter>=32768){counter=0;env+=14336;if(env>=32767){env=32767;phase=2;}}}
  else if(phase==2){counter+=2048;if(counter>=32768){counter=0;env-=(env*8+32767)/32768;}phase=3;}
  else {++counter;if(counter>=32768){counter=0;env=env>=5?env-5:0;}}
  value=floor_shift((int64_t)a*(4096-fraction)+(int64_t)b*fraction,12);
  out[i]=(int16_t)floor_shift((int64_t)value*env,15);
 }
 free(pcm->samples);pcm->samples=out;pcm->frames=(size_t)count;return GH_VOICE_OK;
}
/* These two physical programs are the two original MT-selected
 * recordings of EM0A impact program 2A. The model supplies dependency 20A;
 * the attached adapter has already resolved that dependency in the live bank. */
/* The attached adapter resolves this model's original dependency through
 * the live enemy-bank registry. Check exact EM0A program and sample profiles. */
static int em0a_actor_bank(const struct voice *v,int16_t bank)
{
 return v->source_owner&&bank>=1&&bank!=2&&bank!=3&&
        (v->combat_profile||(v->model&&v->model->root.size>=0x568&&
         h(v->model->root.bytes+0x2fe)==0x202&&w(v->model->root.bytes+0x564)==0x20a));
}

static int em0a_impact_profile(const struct voice *v,int16_t bank,int16_t program,const uint8_t *p)
{
 if(!em0a_actor_bank(v,bank)||v->requested_program!=0x2a||(program!=0x2a&&program!=0x2b))return 0;
 int alternate=program==0x2b;
 return h(p+4)==(unsigned)(0x11+alternate)&&w(p+6)==0x00500406&&
        h(p+0xe)==0x21&&w(p+0x10)==0x006e006e&&w(p+0x14)==0x005a0040&&
        h(p+0x18)==(alternate?0xfe00:0xff80)&&h(p+0x1a)==0x80&&
        w(p+0x1c)==0x00ff0000&&w(p+0x20)==(alternate?5u:0x105u)&&w(p+0x24)==0x300;
}

static int enemy_reaction_profile(const struct voice *v,int16_t bank,int16_t program,
 const uint8_t *p,const uint8_t *sample)
{
 /* Original program 0 and its MT-selected variations in R100 enemy banks.
  * A detached pending voice still has the verified descriptor and sample. */
 static const struct {
  uint32_t dependency,pitch_range;
  uint16_t offsets[3];
  uint8_t gains[3],variations;
 } profiles[]={
  {0x200,0x00c8ff38,{0x5530,0x5b00,0x6350},{127,120,127},1},
  {0x202,0x00c8ff38,{0x5220,0x5970,0x6050},{125,125,125},1},
  {0x203,0x00c8ff38,{0x8bf0,0x9230,0x98f0},{110,125,125},1},
  {0x20a,0,{0xdca0,0xe580,0xf040},{125,120,120},2},
 };
 if(!v->source_owner||bank<1||bank==2||bank==3||v->requested_program!=0||program<0||program>2||
    h(p+4)!=(unsigned)program||w(p+6)!=0x02550228||h(p+0xe)!=0x20||
    w(p+0x14)!=0x005a0040||w(p+0x1c)!=0x00010000||w(p+0x24)!=0x400)return 0;
 uint32_t dependency=0;
 if(v->model){
  if(v->model->root.size<0x568||h(v->model->root.bytes+0x2fe)!=0x202)return 0;
  dependency=w(v->model->root.bytes+0x564);
  if(!dependency)return 0;
 }else if(!v->combat_profile)return 0;
 for(unsigned i=0;i<sizeof(profiles)/sizeof(*profiles);++i){
  if(dependency&&dependency!=profiles[i].dependency)continue;
  const unsigned gain=profiles[i].gains[program];
  const unsigned flags=program?0x10045u:0x10045u|((unsigned)profiles[i].variations<<8);
  if(w(p+0x10)!=gain*0x10001u||w(p+0x18)!=profiles[i].pitch_range||w(p+0x20)!=flags)continue;
  if(!sample||(w(sample)==profiles[i].offsets[program]&&w(sample+4)==11025))return 1;
 }
 return 0;
}
/* Original state71 timed program 11 exists for EM0A. The other R100
 * 0202 dependencies retain their original disabled-program path. */
static int em0a_timed_profile(const struct voice *v,int16_t bank,int16_t program,const uint8_t *p)
{
 return em0a_actor_bank(v,bank)&&v->requested_program==0x11&&program==0x11&&
        h(p+4)==12&&w(p+6)==0x023c0223&&h(p+0xe)==0x20&&
        w(p+0x10)==0x00500050&&w(p+0x14)==0x005a0040&&
        w(p+0x18)==0x0080ff80&&w(p+0x1c)==0x00070000&&
        w(p+0x20)==0x00050005&&w(p+0x24)==0x300;
}

/* EM02 selector71 SEQ frame50 selects logical bank1 program28.
 * Each admitted actor dependency supplies its own authenticated SES sample. */
static int enemy_state71_sequence_profile(const struct voice *v,int16_t bank,int16_t program,
 const uint8_t *p,const uint8_t *sample)
{
 if(!v->source_owner||bank<1||bank==2||bank==3||v->requested_program!=0x28||program!=0x28||
    !v->model||v->model->root.size<0x568||v->model->root.original_vtable214!=0x4479c8||
    h(v->model->root.bytes+0x2fe)!=0x202||h(p)!=(unsigned)bank||h(p+2)!=0x28||
    w(p+6)!=0x02500028||h(p+0xe)!=0x20||w(p+0x14)!=0x005a0040||
    w(p+0x18)!=0x00c8ff38||w(p+0x1c)!=0x00030000||w(p+0x24)!=0x401||
    w(p+0x28)!=0xffff||w(p+0x2c))return 0;
 unsigned slot, gain, age;
 uint32_t offset;
 switch(w(v->model->root.bytes+0x564)){
 case 0x200:slot=15;gain=127;age=1;offset=13360;break;
 case 0x202:slot=16;gain=125;age=3;offset=15232;break;
 case 0x203:slot=17;gain=125;age=3;offset=23392;break;
 default:return 0;
 }
 if(h(p+4)!=slot||w(p+0x10)!=gain*0x10001u||w(p+0x20)!=(age<<16|0x85u))return 0;
 return !sample||(w(sample)==offset&&w(sample+4)==11025&&w(sample+8)==0x5fcb80ff&&!w(sample+12));
}

/* Actual dep0203 selector 15 uses EM03 programs 4/6 and their MT variants. */
static int em03_attack_sequence_profile(const struct voice *v,int16_t bank,int16_t program,
 const uint8_t *p,const uint8_t *sample)
{
 const int16_t request=v->requested_program;
 if(!v->source_owner||bank<1||bank==2||bank==3||(request!=4&&request!=6)||
    program<request||program>request+1||h(p)!=(unsigned)bank||h(p+2)!=(unsigned)program)
  return 0;
 if(v->model){
  const gh_title_player_view *root=&v->model->root;
  if(!root->bytes||root->size<0x568||root->original_vtable214!=0x4479c8||
     h(root->bytes+0x2fe)!=0x202||w(root->bytes+0x564)!=0x203)return 0;
 }else if(!v->combat_profile)return 0;
 const unsigned variants=program==request?0x10185u:0x10085u;
 if(h(p+4)!=(unsigned)(program-1)||w(p+6)!=0x0250022au||h(p+0xe)!=0x20||
    w(p+0x10)!=0x007d007d||w(p+0x14)!=0x005a0040||w(p+0x18)!=0x00c8ff38u||
    w(p+0x1c)!=0x00060000u||w(p+0x20)!=variants||w(p+0x24)!=0x400||
    w(p+0x28)!=0xffff||w(p+0x2c))return 0;
 static const uint32_t offsets[]={0x4a20,0x5280,0x5b60,0x7700};
 return !sample||(w(sample)==offsets[program-4]&&w(sample+4)==11025&&
    w(sample+8)==0x5fcb80ffu&&!w(sample+12));
}

/* Actual EM00 selector6B cues. Logical bank1 is already resolved by
 * the live dependency-bank owner. Both exact rows use the existing 5FCB
 * one-shot reference and the same original age/mix/pitch pipeline. */
static int em00_selector6b_profile(const struct voice *v,int16_t bank,int16_t program,
 const uint8_t *p,const uint8_t *sample)
{
 if(!v->source_owner||bank<1||bank==2||bank==3||
    (program!=0x0a&&program!=0x0b)||v->requested_program!=program||
    h(p)!=(unsigned)bank||h(p+2)!=(unsigned)program)return 0;
 if(v->model){
  const gh_title_player_view *root=&v->model->root;
  if(!root->bytes||root->size<0x568||root->original_vtable214!=0x4479c8||
     h(root->bytes+0x2fe)!=0x202||w(root->bytes+0x564)!=0x200)return 0;
 }else if(!v->combat_profile)return 0;
 const int alternate=program==0x0b;
 if(h(p+4)!=9u+(unsigned)alternate||w(p+6)!=0x0255022d||h(p+0xe)!=0x20||
    w(p+0x10)!=0x007d007d||w(p+0x14)!=0x005a0040||
    w(p+0x18)!=(alternate?0x0064ff38u:0x00640000u)||
    w(p+0x1c)!=0x00030000||w(p+0x20)!=0x00010085||
    w(p+0x24)!=0x400||w(p+0x28)!=0xffff||w(p+0x2c))return 0;
 return !sample||(w(sample)==(alternate?0x11bc0u:0xff80u)&&
    w(sample+4)==11025&&w(sample+8)==0x5fcb80ff&&!w(sample+12));
}

/* Actual 288F00 cEma2 impact selects one of three CORE.SES recordings. */
static int core_ema2_impact_profile(const struct voice *v,int16_t bank,int16_t program,const uint8_t *p)
{
 if(!v->source_owner||bank||v->requested_program!=0x96||program<0x96||program>0x98)return 0;
 if(!v->combat_profile&&(!v->model||v->model->root.size<0x568||
    v->model->root.original_vtable214!=0x448ca0||h(v->model->root.bytes+0x2fe)!=0x2a2||
    w(v->model->root.bytes+0x564)<0x2a2||w(v->model->root.bytes+0x564)>0x2a4))return 0;
 return h(p+4)==(unsigned)(program-0x37)&&w(p+6)==0x02500406&&
        h(p+0xe)==0x20&&w(p+0x10)==0x006e006e&&w(p+0x14)==0x005a0040&&
        h(p+0x18)==(program==0x96?0xff80:0x80)&&h(p+0x1a)==0x80&&
        w(p+0x1c)==0x00070000&&w(p+0x20)==(program==0x96?0x205u:5u)&&w(p+0x24)==1;
}
/* EM02's original AA footsteps keep the CORE bank through 2C7FD0.
 * SDE 3751B8 attenuates the first level and sets both levels to that
 * result in this mode, before the 37D558 pan split. */
static int core_enemy_footstep_profile(const struct voice *v,int16_t bank,int16_t program,
 const uint8_t *p,const uint8_t *sample)
{
 if(!v->source_owner||bank||v->requested_program!=0xaa||(program!=0xaa&&program!=0xab))
  return 0;
 if(!v->model||v->model->root.size<0x568||
    v->model->root.original_vtable214!=0x4479c8||h(v->model->root.bytes+0x2fe)!=0x202)
  return 0;
 uint32_t dependency=w(v->model->root.bytes+0x564);
 if(dependency!=0x200&&dependency!=0x202&&dependency!=0x203)return 0;
 unsigned alternate=program==0xab;
 if(h(p+4)!=99u+alternate||w(p+6)!=0x023c021c||h(p+0xe)!=0x10||
    w(p+0x10)!=0x00370019||w(p+0x14)!=0x005a0040||w(p+0x18)!=0x0100ff00||
    w(p+0x1c)!=0x00010000||w(p+0x20)!=(alternate?0x30005u:0x30105u)||
    w(p+0x24)!=0x401||w(p+0x28)!=0xffff||w(p+0x2c))return 0;
 return !sample||(w(sample)==(alternate?563952u:562304u)&&w(sample+4)==14000&&
    w(sample+8)==0x5fc080ff&&!w(sample+12));
}
/* EM00 selector6B SEQ cue, original logical CORE 8D. */
static int core_em00_selector6b_profile(const struct voice *v,int16_t bank,int16_t program,
 const uint8_t *p,const uint8_t *sample)
{
 if(!v->source_owner||bank||program!=0x8d||v->requested_program!=0x8d||
    h(p)||h(p+2)!=0x8d)return 0;
 if(v->model){
  const gh_title_player_view *root=&v->model->root;
  if(!root->bytes||root->size<0x568||root->original_vtable214!=0x4479c8||
     h(root->bytes+0x2fe)!=0x202||w(root->bytes+0x564)!=0x200)return 0;
 }else if(!v->combat_profile)return 0;
 if(h(p+4)!=92||w(p+6)!=0x0246020bu||w(p+0xa)||h(p+0xe)!=0x20||
    w(p+0x10)!=0x00500050||w(p+0x14)!=0x005a0040||w(p+0x18)!=0x00800080||
    w(p+0x1c)!=0x00060000||w(p+0x20)!=0x00030005||w(p+0x24)!=0x300||
    w(p+0x28)!=0xffff||w(p+0x2c))return 0;
 return !sample||(w(sample)==0x95340&&w(sample+4)==16000&&
     w(sample+8)==0x5fc080ff&&!w(sample+12));
}

/* The late EM03 selector15 CORE cues use the same spatial owner as
 * their local-bank voices, with separately authenticated sample rows. */
static int core_em03_attack_profile(const struct voice *v,int16_t bank,int16_t program,
 const uint8_t *p,const uint8_t *sample)
{
 if(!v->source_owner||bank||v->requested_program!=program||
    (program!=0x85&&program!=0x87&&program!=0xa0)||h(p)||h(p+2)!=(unsigned)program)
  return 0;
 if(v->model){
  const gh_title_player_view *root=&v->model->root;
  if(!root->bytes||root->size<0x568||root->original_vtable214!=0x4479c8||
     h(root->bytes+0x2fe)!=0x202||w(root->bytes+0x564)!=0x203)return 0;
 }else if(!v->combat_profile)return 0;
 const int aged=program==0xa0,second=program==0x87;
 const unsigned slot=aged?98u:second?90u:88u;
 const uint32_t gain=aged?0x00780078u:second?0x005a005au:0x00460046u;
 if(h(p+4)!=slot||w(p+6)!=(aged?0x025a020bu:0x0246020bu)||
    h(p+0xe)!=0x20||w(p+0x10)!=gain||w(p+0x14)!=0x005a0040u||
    w(p+0x18)!=(second?0x00800080u:0x0080ff80u)||w(p+0x1c)!=(aged?0x60000u:0)||
    w(p+0x20)!=(aged?0x30005u:4u)||w(p+0x24)!=(aged?0x300u:0x301u)||
    w(p+0x28)!=0xffffu||w(p+0x2c))return 0;
 const uint32_t offset=aged?0x6f7d0u:second?0xa1ed0u:0xa2f60u;
 return !sample||(w(sample)==offset&&w(sample+4)==16000&&
    w(sample+8)==0x5fc080ffu&&!w(sample+12));
}

static int player_fall_voice_model(const struct voice *v)
{
 return v->source_owner&&v->model&&v->model->root.bytes&&v->model->root.size>=0x300&&
    v->model->root.original_vtable214==0x41d220&&h(v->model->root.bytes+0x2fe)==0x100;
}

static int player_fall_grunt_profile(const struct voice *v,int16_t bank,int16_t program,
 const uint8_t *p,const uint8_t *sample)
{
 if(!player_fall_voice_model(v)||v->requested_bank||v->requested_program!=0xd7||
    bank!=3||program<15||program>17||h(p+4)!=(unsigned)(program-1)||w(p+6)!=0x0055000fu||
    h(p+0xe)!=0x21||w(p+0x10)!=0x007f007fu||w(p+0x14)!=0x005a0040u||
    w(p+0x18)||w(p+0x1c)!=0x00ff0000u||w(p+0x20)!=(program==15?0x205u:5u)||
    w(p+0x24)||w(p+0x28)!=0xffffu||w(p+0x2c))return 0;
 static const uint32_t offsets[]={0xf4f0,0x103b0,0x11900};
 return w(sample)==offsets[program-15]&&w(sample+4)==11025&&
    w(sample+8)==0x5fcb80ffu&&!w(sample+12);
}

static int player_fall_impact_profile(const struct voice *v,const uint8_t *p)
{
 return player_fall_voice_model(v)&&!v->requested_bank&&v->requested_program==0x123&&
    !v->info.bank&&v->info.program==0x123&&h(p+4)==149&&w(p+6)==0x00640100u&&
    h(p+0xe)==0x23&&w(p+0x10)==0x00780078u&&w(p+0x14)==0x005a0040u&&
    w(p+0x18)==0x0080ff80u&&w(p+0x1c)==0x00ff0000u&&w(p+0x20)==0x00060005u&&
    !w(p+0x24)&&w(p+0x28)==0xffffu&&!w(p+0x2c);
}

static gh_voice_status prepare_pcm_with_pitch(gh_se_voice_engine*s,int16_t bank,int16_t program,const uint8_t*p,struct voice*candidate,uint16_t selected_pitch){
 gh_audio_pcm_release(&candidate->pcm);
 unsigned slot=h(p+4),rate,level;gh_sound_span descriptor,tail;gh_voice_status result;
 slot=h(p+4);if(gh_audio_sample_at(s->registry,bank,slot,&descriptor,&tail)!=GH_AUDIO_OK)return unsupported(s,0x35fc);
 if(descriptor.size!=16)return unsupported(s,0x35fc);
 uint16_t adsr1=(uint16_t)h(descriptor.data+8),adsr2=(uint16_t)h(descriptor.data+10);
 int square_voice=candidate->source_owner&&bank==3&&program>=1&&program<=3&&adsr2==0x5fcb;
 int hurt_voice=adsr2==0x5fcb&&candidate->source_owner&&bank==3&&(program==7||program==8)&&
    candidate->requested_bank==0&&candidate->requested_program==program+200&&
    candidate->model&&candidate->model->root.size>=0x300&&
    candidate->model->root.original_vtable214==0x41d220&&
    h(candidate->model->root.bytes+0x2fe)==0x100&&h(p+4)==(unsigned)program&&
    w(p+6)==0x0055000f&&h(p+0xe)==0x21&&w(p+0x10)==0x007f007f&&
    w(p+0x14)==0x005a0040&&!w(p+0x18)&&w(p+0x1c)==0x00ff0000&&
    w(p+0x20)==5&&!w(p+0x24)&&
    w(descriptor.data)==(program==7?54128u:57568u)&&w(descriptor.data+4)==11025;
 int fall_grunt=adsr2==0x5fcb&&player_fall_grunt_profile(candidate,bank,program,p,descriptor.data);
 if(player_fall_impact_profile(candidate,p)&&(w(descriptor.data)!=0x35460||
    w(descriptor.data+4)!=18000||w(descriptor.data+8)!=0x5fc080ffu||w(descriptor.data+12)))
  return unsupported(s,0x35fc);
 int impact_voice=adsr2==0x5fcb&&em0a_impact_profile(candidate,bank,program,p)&&
                  w(descriptor.data)==(program==0x2a?0u:0x2b80u)&&w(descriptor.data+4)==16000;
 int reaction_voice=adsr2==0x5fcb&&enemy_reaction_profile(candidate,bank,program,p,descriptor.data);
 int sequence_voice=adsr2==0x5fcb&&enemy_state71_sequence_profile(candidate,bank,program,p,descriptor.data);
 int selector6b_voice=adsr2==0x5fcb&&em00_selector6b_profile(candidate,bank,program,p,descriptor.data);
 int attack_voice=adsr2==0x5fcb&&em03_attack_sequence_profile(candidate,bank,program,p,descriptor.data);
 int timed_voice=adsr2==0x5fcb&&em0a_timed_profile(candidate,bank,program,p)&&
                w(descriptor.data)==0x5be0&&w(descriptor.data+4)==16000;
 if(adsr1!=0x80ff||(adsr2!=0x5fc0&&!square_voice&&!hurt_voice&&!fall_grunt&&!impact_voice&&!reaction_voice&&!timed_voice&&!sequence_voice&&!selector6b_voice&&!attack_voice)||w(descriptor.data+12))return unsupported(s,0x35fc);
 static const uint32_t ema2_impact_offsets[]={0x553c0,0x56490,0x54300};
 int ema2_voice=adsr2==0x5fc0&&core_ema2_impact_profile(candidate,bank,program,p)&&
                w(descriptor.data)==ema2_impact_offsets[program-0x96]&&w(descriptor.data+4)==22050;
 const int footstep=core_enemy_footstep_profile(candidate,bank,program,p,NULL);
 if(footstep&&!core_enemy_footstep_profile(candidate,bank,program,p,descriptor.data))
  return unsupported(s,0x35fc);
 const int selector6b_core=core_em00_selector6b_profile(candidate,bank,program,p,NULL);
 if(selector6b_core&&!core_em00_selector6b_profile(candidate,bank,program,p,descriptor.data))
  return unsupported(s,0x35fc);
 const int attack_core=core_em03_attack_profile(candidate,bank,program,p,NULL);
 if(attack_core&&!core_em03_attack_profile(candidate,bank,program,p,descriptor.data))
  return unsupported(s,0x35fc);
 if(impact_voice||reaction_voice||ema2_voice||timed_voice||footstep||sequence_voice||attack_core||
    selector6b_voice||selector6b_core||attack_voice)candidate->combat_profile=1;
 rate=w(descriptor.data+4);if(rate<1000 || rate>48000)return unsupported(s,0x38e4);
 candidate->info.pitch_q12=selected_pitch?selected_pitch:(uint16_t)((uint64_t)rate*4096/48000);
 result=gh_audio_decode_ps_adpcm(tail.data,tail.size,&candidate->pcm);if(result){s->frontier=0x35fc;return result;}
 result=render(&candidate->pcm,candidate->info.pitch_q12);if(result){gh_audio_pcm_release(&candidate->pcm);return result;}
 level=(unsigned)((uint64_t)s->mix.se_level_q8*(h(p+0x10)<<8)/0x7f00);
 candidate->base_gain=(uint16_t)level;level/=2;
 candidate->info.stop_current=0x7f00;
 candidate->info.level_q14=(uint16_t)level;candidate->info.rate=rate;candidate->info.sample_slot=slot;candidate->info.frames=(uint32_t)candidate->pcm.frames;
 candidate->info.adsr1=adsr1;candidate->info.adsr2=adsr2;candidate->info.program=program;candidate->info.bank=bank;candidate->info.pending=1;candidate->info.channel=NONE;
 return GH_VOICE_OK;
}
static gh_voice_status prepare_pcm(gh_se_voice_engine*s,int16_t bank,int16_t program,const uint8_t*p,struct voice*candidate){return prepare_pcm_with_pitch(s,bank,program,p,candidate,0);}
gh_voice_status gh_se_voice_create(const gh_audio_registry *r,gh_audio_output *output,const gh_se_mix *mix,gh_se_voice_engine **out)
{
 gh_se_voice_engine *s;if(!r || !output || !mix || !out || mix->se_level_q8>0x7f00 || mix->output_mode>1)return GH_VOICE_ARGUMENT;
 if(mix->dsp_policy!=GH_SE_DSP_NATIVE_LINEAR_REFERENCE)return GH_VOICE_UNSUPPORTED;
 s=calloc(1,sizeof(*s));if(!s)return GH_VOICE_MEMORY;s->registry=r;s->output=output;s->mix=*mix;s->head=-1;for(unsigned i=0;i<51;++i)s->channels[i]=-1;*out=s;return GH_VOICE_OK;
}
gh_voice_status gh_se_voice_bind_origin_seh(gh_se_voice_engine *s,const void *bytes,size_t size)
{
 if(!s || (!bytes && size))return GH_VOICE_ARGUMENT;
 s->origin_seh=bytes;s->origin_seh_size=size;s->origin_scene_bound=1;return GH_VOICE_OK;
}
gh_voice_status gh_se_voice_bind_external_adx(gh_se_voice_engine *s,gh_disc *disc,
 const gh_sound_control *control,const void *elf,size_t elf_bytes)
{
 if(!s||!disc||!control||!elf||!control->initialized||
    control->room_heaps[1].registry!=s->registry||!control->room_heaps[1].identity_verified)
  return GH_VOICE_ARGUMENT;
 s->frontier=0;
 if(s->sequence.disc)
  return s->sequence.disc==disc&&s->sequence.control==control?GH_VOICE_OK:GH_VOICE_ARGUMENT;
 sequence_audio config={.disc=disc,.control=control};
 if(gh_adx_key_from_elf(elf,elf_bytes,&config.key))return unsupported(s,0x2ca34c);
 if(gh_adx_volume_table_from_elf(elf,elf_bytes,config.decibels))return unsupported(s,0x373ff0);
 s->sequence=config;
 return GH_VOICE_OK;
}

static int sequence_profile(int16_t program,const uint8_t p[48])
{
 const unsigned afd=program==0x32?4u:6u;
 return w(p+4)==0x01300000&&w(p+8)==0x00000264&&w(p+0xc)==0x00220000&&
  w(p+0x10)==0x007f007f&&w(p+0x14)==0x005a0040&&!w(p+0x18)&&!w(p+0x1c)&&
  w(p+0x20)==0x00030004&&!w(p+0x24)&&w(p+0x28)==afd&&!w(p+0x2c);
}

static gh_voice_status sequence_prepare(gh_se_voice_engine *s,struct voice *v)
{
 const sequence_audio *config=&s->sequence;
 gh_sound_span afd;
 uint64_t offset,bytes;
 const char *path=NULL;
 const unsigned row=v->info.sample_slot, member=row==4?2u:3u;
 const uint32_t frames=row==4?80692u:53375u;
 if(gh_audio_external_path(s->registry,v->info.bank,&path)||strcmp(path,"se/r100.afs")||
    gh_audio_external_sample(s->registry,v->info.bank,row,&afd,&offset,&bytes))
  return unsupported(s,0x373d90);
 const uint8_t *source=afd.data;
 if(afd.size!=32||h(source)!=row||h(source+2)!=member||w(source+4)||
    w(source+8)!=0x0501bb80||w(source+12)!=0x02005a71||
    w(source+16)||w(source+20)||w(source+24)||w(source+28)||
    bytes!=(row==4?45454u:30082u))
  return unsupported(s,0x373298);

 size_t index;
 if(gh_disc_find(config->disc,path,&index))return unsupported(s,0x32bb48);
 uint8_t *encoded=malloc((size_t)bytes);
 if(!encoded)return GH_VOICE_MEMORY;
 if(gh_disc_read(config->disc,index,offset,encoded,(size_t)bytes)){
  free(encoded);
  return unsupported(s,0x32bb48);
 }
 gh_adx_decoder *decoder=NULL;
 gh_adx_info info;
 gh_adx_error error=gh_adx_open(encoded,(size_t)bytes,&config->key,&decoder);
 if(!error)error=gh_adx_inspect(decoder,&info);
 if(error||info.channels!=1||info.sample_rate!=48000||info.sample_count!=frames||info.loop_enabled){
  gh_adx_close(decoder);
  free(encoded);
  return unsupported(s,0x32bb48);
 }
 int16_t *samples=malloc((size_t)frames*sizeof(*samples));
 if(!samples){
  gh_adx_close(decoder);
  free(encoded);
  return GH_VOICE_MEMORY;
 }
 size_t decoded;
 error=gh_adx_decode_mono(decoder,samples,frames,&decoded);
 gh_adx_close(decoder);
 free(encoded);
 if(error||decoded!=frames){
  free(samples);
  return unsupported(s,0x32bb48);
 }
 v->pcm=(gh_adpcm_pcm){samples,frames,(size_t)bytes};
 v->info.frames=frames;
 /* 37C6A0 scales descriptor level 127 by the real AFD weight 113.
  * 37D3C8 uses the existing SE mixer before the separate 373FF0 gain. */
 const unsigned level=h(v->descriptor+0x10)*source[12]/127u;
 v->base_gain=(uint16_t)((uint32_t)s->mix.se_level_q8*(level<<8)/0x7f00u);
 v->info.level_q14=v->base_gain/2u;
 return GH_VOICE_OK;
}

static gh_voice_status sequence_gain(gh_se_voice_engine *s,const struct voice *v,float *gain)
{
 const gh_sound_control *control=s->sequence.control;
 if(!control||!control->initialized||control->output_mode!=1)return unsupported(s,0x3756f0);
 const float master=sound_float(control->gain_bits[1]), category=sound_float(control->gain_bits[4]);
 if(!isfinite(master)||!isfinite(category)||master<0||master>127||category<0||category>127)
  return unsupported(s,0x373ff0);
 const unsigned weight=((uint32_t)v->base_gain*(unsigned)v->info.stop_current/0x7f00u)>>8;
 volatile float a=(float)weight*master,b=a/127.0f,c=b*category,d=c/127.0f;
 const unsigned index=(unsigned)(d<127.0f?d:127.0f);
 volatile float exponent=(float)s->sequence.decibels[index]/200.0f;
 *gain=powf(10.0f,exponent);
 return GH_VOICE_OK;
}

void gh_se_voice_destroy(gh_se_voice_engine *s){if(s){for(int i=0;i<64;++i)if(s->voices[i].used)drop(s,i,1);free(s);}}
gh_voice_status gh_se_voice_request_origin(gh_se_voice_engine *s,int16_t bank,int16_t program,int16_t arg3,int16_t arg4,uint32_t flags,int32_t delay,uint32_t *handle)
{
 uint8_t p[48];gh_voice_status result;unsigned i;struct voice candidate={0};
 if(!s || !handle)return GH_VOICE_ARGUMENT;*handle=0;s->frontier=0;
 int title=bank==2&&(program==0||(program>=10&&program<=12)||program==15);
 const int sequence=bank==2&&(program==0x32||program==0x33)&&s->sequence.disc;
 const int empty_load=bank==0&&program==0x122;
 if((!title&&!sequence&&!empty_load&&(bank || (program!=0x2d && program!=0x15e && program!=0x160 && program!=0x161))) || arg3!=-1 || arg4!=-1 || flags || delay)return unsupported(s,0x2cb8a0);
 if(s->mix.system_flags&0x08000040)return unsupported(s,0x2ce588);
 if(gh_audio_get_program(s->registry,bank,program,0,p)!=GH_AUDIO_OK)return unsupported(s,0x374cb8);
 /* Exact audited subsets; +21 controls random variants, +22 duplicate age
  * window. +09 is origin/output selection, not variant count. */
 if(sequence){
  if(!s->origin_scene_bound||s->origin_seh||s->origin_seh_size)return unsupported(s,0x2cae10);
  if(!sequence_profile(program,p))return unsupported(s,0x2cdff0);
 }else if(title){
  if(!s->origin_scene_bound||s->origin_seh||s->origin_seh_size)return unsupported(s,0x2cae10);
  if(program==0){if(p[6]!=10||p[7]!=4||p[8]!=120||p[9]||h(p+0xe)!=0x20||h(p+0x10)!=90||w(p+0x20)!=0x30000||w(p+0x24)!=0x300||h(p+0x1e))return unsupported(s,0x2cdff0);}
  else if(p[6]!=20||p[7]!=4||p[8]!=122||p[9]||h(p+0xe)!=0x23||h(p+0x10)!=120||w(p+0x20)||w(p+0x24)||h(p+0x1e)!=255)return unsupported(s,0x2cdff0);
  if(program==15&&h(p+4)!=4)return unsupported(s,0x2cdff0);
 }else if(empty_load){
  if(!s->origin_scene_bound||s->origin_seh||s->origin_seh_size)return unsupported(s,0x2cae10);
  if(h(p+4)!=148||w(p+6)!=0x00640100||w(p+0xa)||h(p+0xe)!=0x23||
     w(p+0x10)!=0x00780078||w(p+0x18)!=0x0080ff80||w(p+0x20)!=0x00060005)
   return unsupported(s,0x2cdff0);
  gh_sound_span descriptor,tail;
  if(gh_audio_sample_at(s->registry,bank,148,&descriptor,&tail)!=GH_AUDIO_OK||
     descriptor.size!=16||w(descriptor.data)!=0x176d0||w(descriptor.data+4)!=20000||
     w(descriptor.data+8)!=0x5fc080ff||w(descriptor.data+12))
   return unsupported(s,0x35fc);
 }else if(program==0x2d){
  if(p[6]!=15||p[7]!=1||p[8]!=104||p[9]!=2||h(p+0xe)!=0x21||w(p+0x20)!=0x00030005||h(p+0x10)!=90)return unsupported(s,0x2cdff0);
  if(!s->origin_scene_bound||s->origin_seh||s->origin_seh_size)return unsupported(s,0x2cae10);
 }else if(p[6]||p[7]!=4||p[8]!=120||p[9]||h(p+0xe)!=0x23||w(p+0x20))return unsupported(s,0x2cdff0);
 if(!sequence&&(h(p+0x10)!=h(p+0x12)||h(p+0x14)!=64||h(p+0x16)!=90||(!empty_load&&w(p+0x18))||h(p+0x1c)||h(p+0x28)!=65535||h(p+0x2a)||w(p+0x2c)))return unsupported(s,0x2cdff0);
 if(!title&&!sequence&&(h(p+0x1e)!=255||w(p+0x24)))return unsupported(s,0x2cdff0);
 /* Actual2CBF78 compares bank AND program; duplicate-age suppression for
  * equal-origin zero-delay is shared by logo and room confirmation0. */
 if(p[0x22])for(int j=s->head;j!=-1;j=s->voices[j].next)if(s->voices[j].info.bank==bank&&s->voices[j].info.program==program&&(s->voices[j].ee_age<=p[0x22]||s->voices[j].ee_age>=UINT32_C(0x80000000)))return GH_VOICE_OK;
 for(i=0;i<64 && s->voices[i].used;++i){}if(i==64)return GH_VOICE_OK;
 if(sequence){
  memcpy(candidate.descriptor,p,sizeof(candidate.descriptor));
  candidate.external_adx=1;
  candidate.info=(gh_se_voice_info){.bank=bank,.program=program,
   .sample_slot=h(p+0x28),.rate=48000,.pitch_q12=4096,.pending=1,.channel=NONE,.stop_current=0x7f00};
 }else{
  result=prepare_pcm(s,bank,program,p,&candidate);if(result)return result;
  if(empty_load){
   memcpy(candidate.descriptor,p,sizeof(candidate.descriptor));
   candidate.info.pitch_q12=0;
  }
 }
 if(!++s->serial)++s->serial;candidate.info.original_handle=s->serial;candidate.next=s->head;candidate.used=1;candidate.priority=p[8];candidate.group_first=p[6];candidate.group_count=p[7]+1;candidate.replace_equal=(p[0xe]&0x20)!=0;s->voices[i]=candidate;s->head=(int)i;*handle=s->serial;return GH_VOICE_OK;
}
/* Actor-attached EE construction; existingPCM/DSP/channel/output owner above. */
static int source_inside(const uint8_t*base,size_t bytes,const uint8_t*ptr,size_t n){return base&&ptr&&(uintptr_t)ptr>=(uintptr_t)base&&(uintptr_t)ptr-(uintptr_t)base<=bytes&&n<=bytes-((uintptr_t)ptr-(uintptr_t)base);}
static gh_voice_status source_position(struct voice*v,int offset){const uint8_t*p;if(!v->model)return GH_VOICE_ARGUMENT;if(v->model->native_identity!=v->model||!v->model->bound)return GH_VOICE_ARGUMENT;if(v->part_index<0){p=v->model->root.alias_f0;if(!source_inside(v->model->root.bytes,v->model->root.size,p,16))return GH_VOICE_BOUNDS;}else{if((unsigned)v->part_index>=v->model->part_count)return GH_VOICE_BOUNDS;gh_actor_part*part=v->model->parts+v->part_index;p=part->alias_f0;if(!source_inside(part->bytes,0x220,p,16))return GH_VOICE_BOUNDS;}float position[4];memcpy(position,p,16);for(unsigned i=0;i<4;i++)if(!isfinite(position[i]))return GH_VOICE_UNSUPPORTED;if(offset){volatile float y=position[1]+1.3f;position[1]=y;if(!isfinite(position[1]))return GH_VOICE_UNSUPPORTED;}memcpy(v->info.source_position,position,16);return GH_VOICE_OK;}
static int remap_bank_program(int16_t program){int p=program;return (p>=200&&p<=232)||(p>=0xee&&p<=0xef)||(p>=0xf2&&p<=0xf3)||(p>=0xf8&&p<=0xf9)||p==0xea||p==0x109||p==0x10b;}
static gh_voice_status attached_profile(gh_se_voice_engine*s,const uint8_t*p,int spatial_levels){if(p[6]>=128||p[7]>=128||p[8]>=128||p[6]+(unsigned)p[7]+1>51)return unsupported(s,0x37cf00);if((!spatial_levels&&h(p+0x10)!=h(p+0x12))||h(p+0x10)>128||h(p+0x12)>128||h(p+0x14)!=64||h(p+0x16)!=90||h(p+0x1c)||h(p+0x28)!=65535||h(p+0x2a)||w(p+0x2c))return unsupported(s,0x2ce588);return GH_VOICE_OK;}
/* 2CBF78 mode 2 compares signed EE ages and keeps the first equal-age node. */
static int youngest_attached_duplicate(const gh_se_voice_engine *s,
 const struct voice *candidate)
{
 int youngest=-1;
 uint32_t youngest_order=0;
 const uint32_t age_limit=UINT32_C(0x80000000)+candidate->descriptor[0x22];
 for(int i=s->head;i!=-1;i=s->voices[i].next){
  const struct voice *old=s->voices+i;
  uint32_t order=old->ee_age^UINT32_C(0x80000000);
  if(old->info.bank!=candidate->info.bank||old->info.program!=candidate->info.program||
     order>age_limit||(youngest>=0&&order>=youngest_order))continue;
  youngest=i;
  youngest_order=order;
 }
 return youngest;
}

/* 1F7E48 evaluates X²+Y², then adds Z² and takes the VU square root.
 * Keep each step binary32 under the surrounding request's numeric policy. */
static float duplicate_distance(const float listener[4],const float source[4])
{
 volatile float x=listener[0]-source[0];
 volatile float y=listener[1]-source[1];
 volatile float z=listener[2]-source[2];
 volatile float xx=x*x,yy=y*y,zz=z*z;
 volatile float xy=xx+yy;
 volatile float xyz=xy+zz;
 return sqrtf(xyz);
}

static gh_voice_status attached_duplicate(gh_se_voice_engine *s,struct voice *candidate,
 int *suppressed)
{
 *suppressed=0;
 if(!candidate->descriptor[0x22])return GH_VOICE_OK;
 if(candidate->info.bank!=2||candidate->info.program!=10||candidate->descriptor[0x22]!=3){
  if(!enemy_reaction_profile(candidate,candidate->info.bank,candidate->info.program,candidate->descriptor,NULL)&&
     !enemy_state71_sequence_profile(candidate,candidate->info.bank,candidate->info.program,candidate->descriptor,NULL)&&
     !em00_selector6b_profile(candidate,candidate->info.bank,candidate->info.program,candidate->descriptor,NULL)&&
     !em03_attack_sequence_profile(candidate,candidate->info.bank,candidate->info.program,candidate->descriptor,NULL)&&
     !core_em03_attack_profile(candidate,candidate->info.bank,candidate->info.program,candidate->descriptor,NULL)&&
     !core_em00_selector6b_profile(candidate,candidate->info.bank,candidate->info.program,candidate->descriptor,NULL)&&
     !em0a_timed_profile(candidate,candidate->info.bank,candidate->info.program,candidate->descriptor)&&
     !player_fall_impact_profile(candidate,candidate->descriptor)&&
     !core_enemy_footstep_profile(candidate,candidate->info.bank,candidate->info.program,candidate->descriptor,NULL))
   return unsupported(s,0x2cbf78);
 }
 int index=youngest_attached_duplicate(s,candidate);
 if(index<0)return GH_VOICE_OK;
 const struct voice *old=s->voices+index;
 /* Original 2CDA80 sets bit0 after successful 2CE588. All currently
  * admitted producers use delay zero, including origin requests. */
 if(!old->info.pending||
    (old->info.source_position[0]==candidate->info.source_position[0]&&
     old->info.source_position[1]==candidate->info.source_position[1]&&
     old->info.source_position[2]==candidate->info.source_position[2])){
  *suppressed=1;
  return GH_VOICE_OK;
 }
 gh_se_attached_environment env={0};
 gh_voice_status status=candidate->probe(candidate->probe_context,
     candidate->info.source_position,0,&env,&s->frontier);
 if(status)return status;
 float listener[4];
 memcpy(listener,env.listener_position_c0,sizeof(listener));
 float old_distance=duplicate_distance(listener,old->info.source_position);
 float candidate_distance=duplicate_distance(listener,candidate->info.source_position);
 if(!isfinite(old_distance)||!isfinite(candidate_distance)||old->info.native_voice)
  return unsupported(s,0x2cbf78);
 if(old_distance<=candidate_distance)*suppressed=1;
 else drop(s,index,0); /* 2CE3E8 unlinks the older pending voice. */
 return GH_VOICE_OK;
}

static gh_voice_status attached_request_body(gh_se_voice_engine*s,const void*owner,const void*key,gh_actor_model*model,int32_t part,int16_t bank,int16_t program,uint32_t flags,int32_t delay,gh_se_attached_probe probe,void*probe_context,gh_se_attached_random random,void*random_context,uint32_t*handle){
 if(!owner||!key||!model||!probe||model->native_identity!=model||!model->bound||!model->parts_ready||!model->root.bytes||model->root.size<0x5ac||part < -1||(part>=0&&(uint32_t)part>=model->part_count))return GH_VOICE_ARGUMENT;
 if(flags||delay)return unsupported(s,0x2cdff0);
 unsigned index=64;for(unsigned i=0;i<64;i++)if(s->voices[i].used==2&&s->voices[i].source_owner==owner&&s->voices[i].request_key==key){index=i;break;}
 if(index==64){gh_sound_span a,b,c,d;gh_audio_status ready=gh_audio_bank_views(s->registry,bank,&a,&b,&c,&d);if(ready==GH_AUDIO_NOT_FOUND)return GH_VOICE_OK;if(ready)return unsupported(s,0x2cfc68);for(index=0;index<64&&s->voices[index].used;index++){}if(index==64)return GH_VOICE_OK;/* GenuineEEpoolfull BEFOREMT. */
  if(gh_actor_model_sound_borrow(model))return GH_VOICE_ARGUMENT;struct voice*v=s->voices+index;memset(v,0,sizeof(*v));v->used=2;v->info.channel=NONE;v->model=model;v->part_index=part;v->source_owner=owner;v->request_key=key;v->requested_bank=bank;v->requested_program=program;v->probe=probe;v->probe_context=probe_context;v->random=random;v->random_context=random_context;
  gh_voice_status positioned=source_position(v,1);if(positioned){drop(s,(int)index,0);s->frontier=0x2cdd18;return positioned;}
 }
 struct voice*v=s->voices+index;if(v->model!=model||v->part_index!=part||v->requested_bank!=bank||v->requested_program!=program||v->probe!=probe||v->probe_context!=probe_context||v->random!=random||v->random_context!=random_context)return GH_VOICE_ARGUMENT;
 if(!v->setup_stage){v->info.bank=bank;v->info.program=program;if(bank==0&&remap_bank_program(program)){v->info.bank=3;v->info.program=(int16_t)(program-200);}gh_audio_status got=gh_audio_get_program(s->registry,v->info.bank,v->info.program,0,v->descriptor);if(got==GH_AUDIO_BOUNDS||got==GH_AUDIO_DISABLED||got==GH_AUDIO_NOT_FOUND){drop(s,(int)index,0);return GH_VOICE_OK;}if(got)return unsupported(s,0x374cb8);v->variants=v->descriptor[0x21];v->setup_stage=1;}
 if(v->setup_stage==1){if(v->variants){if(!v->random)return unsupported(s,0x31cfe0);uint32_t word;gh_voice_status e=v->random(v->random_context,&word,&s->frontier);if(e){if(!s->frontier)s->frontier=0x31cfe0;return e;}v->info.program=(int16_t)(v->info.program+(int32_t)(word%((uint32_t)v->variants+1)));v->setup_stage=2;/* DRAWcommitted BEFORElookup/retry. */}else v->setup_stage=3;}
 if(v->setup_stage==2){gh_audio_status got=gh_audio_get_program(s->registry,v->info.bank,v->info.program,0,v->descriptor);if(got==GH_AUDIO_BOUNDS||got==GH_AUDIO_DISABLED||got==GH_AUDIO_NOT_FOUND){drop(s,(int)index,0);return GH_VOICE_OK;}if(got)return unsupported(s,0x374cb8);v->setup_stage=3;}
 if(v->setup_stage==3){
  if(v->variants&&v->descriptor[0x23])return unsupported(s,0x2cbd10);
  int suppressed;
  gh_voice_status duplicate=attached_duplicate(s,v,&suppressed);
  if(duplicate)return duplicate;
  if(suppressed){drop(s,(int)index,0);return GH_VOICE_OK;}
  v->setup_stage=4;
 }
 if(v->setup_stage==4){uint8_t*p=v->descriptor;gh_se_attached_environment env={0};gh_voice_status e=v->probe(v->probe_context,v->info.source_position,0,&env,&s->frontier);if(e)return e;v->info.position_mode=2;if(env.output_mode&&p[9]!=2){if(p[9]==3)v->info.position_mode=1;else if(p[9]<2)v->info.position_mode=0;}v->flags18=flags|p[0x20]|((uint32_t)p[0x24]<<8);v->flags1c=(unsigned)p[0x25]|(unsigned)p[0x26]<<8;if(v->flags1c&6)v->flags1c|=1;v->setup_stage=5;}
 if(v->setup_stage==5){gh_voice_status e=attached_profile(s,v->descriptor,core_enemy_footstep_profile(v,
    v->info.bank,v->info.program,v->descriptor,NULL));if(e)return e;e=prepare_pcm(s,v->info.bank,v->info.program,v->descriptor,v);if(e)return e;v->priority=v->descriptor[8];v->group_first=v->descriptor[6];v->group_count=v->descriptor[7]+1;v->replace_equal=(v->descriptor[0xe]&0x20)!=0;/* Range is sampled later by actual37C2E0, never expose this intermediate PCM as finalpitch. */if(h(v->descriptor+0x18)||h(v->descriptor+0x1a))v->info.pitch_q12=0;v->setup_stage=6;}
 if(v->setup_stage==6){if(!++s->serial)++s->serial;v->info.original_handle=s->serial;v->request_key=NULL;v->setup_stage=0;v->random=NULL;v->random_context=NULL;v->used=1;v->next=s->head;s->head=(int)index;*handle=v->info.original_handle;}
 return GH_VOICE_OK;
}
gh_voice_status gh_se_voice_request_attached(gh_se_voice_engine *s,const void *owner,
 const void *key,gh_actor_model *model,int32_t part,int16_t bank,int16_t program,
 uint32_t flags,int32_t delay,gh_se_attached_probe probe,void *probe_context,
 gh_se_attached_random random,void *random_context,uint32_t *handle)
{
 if(!s||!handle)return GH_VOICE_ARGUMENT;
 *handle=0;
 s->frontier=0;
 fenv_t env;
 if(feholdexcept(&env))return GH_VOICE_PLATFORM;
 if(fesetround(FE_TONEAREST)){(void)fesetenv(&env);return GH_VOICE_PLATFORM;}
 gh_voice_status status=attached_request_body(s,owner,key,model,part,bank,program,
     flags,delay,probe,probe_context,random,random_context,handle);
 if(fesetenv(&env))return GH_VOICE_PLATFORM;
 if(status&&s->frontier)
  s->attached_failure=(gh_se_attached_failure){
      .frontier=s->frontier,.flags=flags,.part_index=part,.delay=delay,
      .bank=bank,.program=program,.status=status
  };
 return status;
}

void gh_se_voice_last_attached_failure(const gh_se_voice_engine *s,gh_se_attached_failure *out)
{
 if(out)*out=s?s->attached_failure:(gh_se_attached_failure){0};
}
static int r100_spatial_profile(const struct voice *v)
{
 return v->source_owner&&v->info.position_mode==2&&v->info.bank==2&&v->info.program==10&&
        v->descriptor[0x22]==3;
}
static int combat_voice_profile(const struct voice *v)
{
 return v->combat_profile;
}
static int voice_uses_ee_mix(const struct voice *v)
{
 return r100_spatial_profile(v)||combat_voice_profile(v);
}
static int signed_half(const uint8_t *p){unsigned u=h(p);return u<0x8000?(int)u:(int)u-0x10000;}

/* Actual 2CF058 finite binary32 reference, with the existing 1F8180 leaf.
 * The listener producer supplies the same cSnd+C0/D0/110 fields. */
static gh_voice_status spatial_position(gh_se_voice_engine *s,const struct voice *v,
 const gh_se_attached_environment *env,unsigned *distance,unsigned *azimuth)
{
 float position[4],matrix[16];
 memcpy(position,env->listener_position_c0,sizeof(position));
 memcpy(matrix,env->listener_matrix_d0,sizeof(matrix));
 if(env->listener_mode110!=0&&env->listener_mode110!=1)return unsupported(s,0x2cf058);
 for(unsigned i=0;i<4;i++)if(!isfinite(position[i]))return unsupported(s,0x2cf058);
 for(unsigned i=0;i<16;i++)if(!isfinite(matrix[i]))return unsupported(s,0x2cf058);
 float local[2];
 const unsigned rows[]={0,2};
 for(unsigned i=0;i<2;i++){
  unsigned row=rows[i];
  local[i]=((matrix[row]*v->info.source_position[0]+matrix[row+4]*v->info.source_position[1])+
           matrix[row+8]*v->info.source_position[2])+matrix[row+12]*v->info.source_position[3];
 }
 float angle;
 uint32_t frontier;
 if(gh_game_angle_001f8180(local[1],local[0],&angle,GH_CAMERA_NATIVE_BINARY32_REFERENCE,&frontier))
  return unsupported(s,frontier?frontier:0x1f8180);
 if(angle<0)angle+=6.2831855f;
 unsigned degrees=(unsigned)(angle*57.295776f+.5f)%360;
 float scale=1;
 if(!env->listener_mode110){
  unsigned side=(degrees+270)%360;
  if(side>=51&&side<=309)scale=1+.008f*(float)(side<=180?side-50:310-side);
 }
 float x=position[0]-v->info.source_position[0],y=position[1]-v->info.source_position[1],
       z=position[2]-v->info.source_position[2];
 float length=sqrtf((x*x+y*y)+z*z)*scale;
 if(!isfinite(length))return unsupported(s,0x2cf058);
 *distance=length>=32000?32000:(unsigned)length;
 *azimuth=degrees;
 return GH_VOICE_OK;
}

static int span_fits(size_t size,size_t offset,size_t count)
{return offset<=size&&count<=size-offset;}

/* 3781F8/378398 follow the real 374B70 table pointer and preset selectors.
 * The current stereo path only reads the first two bytes of each preset. */
static gh_voice_status spatial_curve(gh_se_voice_engine *s,const struct voice *v,
 unsigned distance,unsigned pitch,int *out)
{
 uint32_t entry=pitch?0x378398:0x3781f8;
 gh_sound_span programs,sys,samples,afd;
 if(gh_audio_bank_views(s->registry,v->info.bank,&programs,&sys,&samples,&afd)||programs.size<32)
  return unsupported(s,entry);
 size_t offset=w(programs.data+0x10);
 if(!offset){*out=pitch?0:127;return GH_VOICE_OK;}
 if(!span_fits(programs.size,offset,16))return unsupported(s,entry);
 const uint8_t *curve=programs.data+offset;
 size_t size=programs.size-offset;
 unsigned preset=v->descriptor[0x1e];
 if(preset>=128||preset>=curve[0])return unsupported(s,entry);
 size_t preset_offset=(size_t)w(curve+4)+preset*4+pitch;
 if(!span_fits(size,preset_offset,1))return unsupported(s,entry);
 unsigned index=curve[preset_offset];
 if(index>=128){*out=pitch?0:127;return GH_VOICE_OK;}
 if(index>=curve[2+pitch])return unsupported(s,entry);
 size_t table=(size_t)w(curve+8+pitch*4)+index*8;
 if(!span_fits(size,table,8))return unsupported(s,entry);
 size_t points=w(curve+table);
 unsigned count=h(curve+table+6);
 if(!count||!span_fits(size,points,(size_t)count*4))return unsupported(s,entry);
 int previous_x=-1,previous_y=0;
 int found=0,value=0;
 for(unsigned i=0;i<count;i++){
  const uint8_t *point=curve+points+i*4;
  int x=signed_half(point),y=signed_half(point+2);
  if(x<=previous_x||(!i&&x)||(!pitch&&(y<0||y>127)))return unsupported(s,entry);
  if(!found&&(int)distance==x){found=1;value=y;}
  if(!found&&(int)distance<x){
   float slope=(float)(previous_y-y)/(float)(x-previous_x);
   value=previous_y-(int)(slope*(float)((int)distance-previous_x));
   found=1;
  }
  previous_x=x;
  previous_y=y;
 }
 *out=found?value:previous_y;
 return GH_VOICE_OK;
}
static unsigned approach(unsigned current,unsigned target,unsigned step)
{
 if(current<target)return target-current>step?current+step:target;
 return current-target>step?current-step:target;
}

/* Same integer master scaling and stereo split as 37D558. The output
 * still uses the documented native DSP envelope/resampler. */
static void apply_spatial_mix(const gh_se_voice_engine *s,struct voice *v)
{
 unsigned base=(unsigned)s->mix.se_level_q8*v->spatial_gain/127;
 v->base_gain=(uint16_t)base;
 unsigned level=base*(unsigned)v->info.stop_current/0x7f00/2;
 v->info.level_q14=(uint16_t)level;
 unsigned pan=v->info.pan;
 v->info.left_q14=(uint16_t)(pan>64?level*(127-pan)/64:level);
 v->info.right_q14=(uint16_t)(pan<64?level*pan/64:level);
}

/* 37C2E0 preserves the SDK random offset. 37D728 adds the current
 * command offset. A slow actor contributes -0xF00, not another draw. */
static gh_voice_status resolve_ee_pitch(gh_se_voice_engine *s,const struct voice *v,
 int slow,uint16_t *out)
{
 uint16_t bits=(uint16_t)((uint16_t)v->selected_offset+h(v->descriptor+0x1c)-(slow?0xf00u:0u));
 int16_t offset;
 memcpy(&offset,&bits,2);
 gh_voice_status status=gh_se_pitch_q12_000038e4(&s->pitch,offset,v->info.rate,out);
 if(status)s->frontier=0x38e4;
 return status;
}

/* 308D68 compares the pointer word with the same 747B20 list built
 * by scene visibility. Typed scroll handles preserve that identity. */
static int contains_scroll(const gh_game_scene_tables *tables,uint32_t handle)
{
 for(unsigned i=0;i<32&&tables->scroll_link_words[i];i++)
  if(tables->scroll_link_words[i]==handle)return 1;
 return 0;
}

/* Original 308DA8 static segment, with its own mask, flags and 8 reports. */
static gh_voice_status sound_occluded(gh_se_voice_engine *s,const struct voice *v,
 const gh_se_attached_environment *env,int *blocked)
{
 gh_game_scene_tables *tables=env->scene_tables;
 if(!tables||!tables->actors_ready)return unsupported(s,0x308da8);
 gh_scene_ray_report reports[8]={0};
 gh_scene_ray_request request={.mask=7,.flags=0x12c,.report_limit=8,.native_reports=reports};
 memcpy(request.from,v->info.source_position,sizeof(request.from));
 memcpy(request.to,env->listener_position_c0,sizeof(request.to));
 int hit;
 uint32_t frontier;
 gh_scene_ray_status status=gh_scene_ray_query_direct_0012ed60(tables,&request,0,&hit,&frontier);
 if(tables->ray.reports18c==reports)tables->ray.reports18c=NULL;
 if(status)return unsupported(s,frontier?frontier:0x12ed60);
 *blocked=0;
 if(!hit)return GH_VOICE_OK;
 unsigned count=tables->ray.triangle17c;
 if(count>8)return unsupported(s,0x308da8);
 for(unsigned i=0;i<count;i++){
  if(!reports[i].attributes||reports[i].attribute_bytes<12)return unsupported(s,0x308da8);
  const uint8_t *attributes=reports[i].attributes;
  unsigned first=h(attributes+8),links=attributes[10];
  if(!links){*blocked=1;return GH_VOICE_OK;}
  for(unsigned j=0;j<links;j++){
   gh_actor_scroll_entry entry;
   if(first+j>UINT16_MAX||gh_scene_actor_scroll_entry_001333b8(tables,(uint16_t)(first+j),&entry))
    return unsupported(s,0x1333b8);
   if((uint32_t)entry.current_key)continue;
   if(!entry.resolved){
    if(entry.current_key)return unsupported(s,0x308d68);
    continue;
   }
   if(entry.scroll_handle&&!contains_scroll(tables,entry.scroll_handle)){
    *blocked=1;
    return GH_VOICE_OK;
   }
  }
 }
 return GH_VOICE_OK;
}

static gh_voice_status spatial_update(gh_se_voice_engine *s,struct voice *v,
 const gh_se_attached_environment *env)
{
 uint32_t entry=v->info.pending?0x2ce588:0x2cea00;
 if(env->output_mode!=1||env->se_level_q8!=s->mix.se_level_q8)return unsupported(s,0x375630);
 int combat=combat_voice_profile(v);
 if((!combat&&(v->flags18!=4||v->flags1c!=3))||env->reverb||
    (env->system_flags_5e4&0x08000000u)||(env->game_flags_5d8&0x40u))
  return unsupported(s,entry);
 if(v->flags18&0x40u){
  if(!env->sound_flags_ac||!env->sound_flags_b0)return unsupported(s,0x2cda80);
  *env->sound_flags_ac|=0x04000000u;
  *env->sound_flags_b0|=0x04000000u;
 }
 if(v->info.pending&&(v->flags18&0x100u)){
  int blocked;
  gh_voice_status occlusion=sound_occluded(s,v,env,&blocked);
  if(occlusion)return occlusion;
  if(blocked){v->info.pending=0;return GH_VOICE_OK;}
 }
 unsigned distance=0,angle=0;
 int gain=(int)h(v->descriptor+0x10),pitch=0;
 gh_voice_status e;
 if(v->info.position_mode){
  e=spatial_position(s,v,env,&distance,&angle);
  if(e)return e;
  int attenuation;
  e=spatial_curve(s,v,distance,0,&attenuation);
  if(e)return e;
  gain=gain*attenuation/127;
  e=spatial_curve(s,v,distance,1,&pitch);
  if(e)return e;
 }
 /* Both audited distance-pitch curves are identically zero. */
 if(pitch)return unsupported(s,0x378398);
 int slow=0;
 if(v->model&&(v->flags1c&1)){
  float speed;
  memcpy(&speed,v->model->root.bytes+0x5a8,4);
  if(!isfinite(speed))return unsupported(s,entry);
  /* 2CDA80 refreshes the bit at .5 every visit; pending 2CE588 can
   * additionally set it below 1 when Work58C is nonnegative. */
  slow=speed<.5f||(v->info.pending&&env->debug58c>=0&&speed<1);
 }
 if(!v->info.pending){
  float ratio=sound_float(env->gain64_bits);
  if(!(ratio>=0&&ratio<=1))return unsupported(s,entry);
  gain=(int)((float)gain*ratio);
 }
 if(slow&&(v->flags1c&2)){gain=(int)((float)gain*1.4f);if(gain>128)gain=128;}
 if(slow&&(v->flags1c&4))gain=0;
 unsigned folded=angle>=180?360-angle:angle;
 unsigned pan=v->info.position_mode?(unsigned)((float)folded*127.0f/180.0f+.5f):h(v->descriptor+0x14);
 if(!v->info.pending){
  gain=(int)approach(v->spatial_gain,(unsigned)gain,6);
  pan=approach(v->info.pan,pan,3);
 }
 /* Keep all visible parameters transactional if the native endpoint fails. */
 struct voice mixed={.info=v->info,.spatial_gain=(uint16_t)gain};
 mixed.info.pan=(uint16_t)pan;
 apply_spatial_mix(s,&mixed);
 if(!v->info.pending&&v->info.native_voice){
  /* Mode 2 sets command mask 0xF and can update pitch. Mode 0's
   * gain/pan-only mask 0xB leaves the initial pitch intact. */
  if(v->info.position_mode==2&&slow!=v->spatial_slow){
   uint16_t next_pitch;
   e=resolve_ee_pitch(s,v,slow,&next_pitch);
   if(e)return e;
   if(next_pitch!=v->info.pitch_q12){
    e=gh_audio_output_voice_pitch_ratio(s->output,v->info.native_voice,next_pitch,v->submitted_pitch);
    if(e){s->frontier=0x374f30;return e;}
    mixed.info.pitch_q12=next_pitch;
   }
  }
  e=gh_audio_output_voice_gain(s->output,v->info.native_voice,
        (float)mixed.info.left_q14/16384.0f,(float)mixed.info.right_q14/16384.0f);
  if(e){s->frontier=0x374f30;return e;}
 }
 mixed.info.distance=(uint16_t)distance;
 mixed.info.azimuth=(uint16_t)angle;
 v->spatial_slow=(uint8_t)slow;
 v->spatial_gain=(uint16_t)gain;
 v->base_gain=mixed.base_gain;
 v->info=mixed.info;
 return GH_VOICE_OK;
}

static gh_voice_status attached_update(gh_se_voice_engine*s,struct voice*v){if(!v->source_owner)return GH_VOICE_OK;gh_voice_status e;if(!v->info.pending&&v->model){e=source_position(v,1);if(e){s->frontier=0x2cea00;return e;}}gh_se_attached_environment env={0};e=v->probe(v->probe_context,v->info.source_position,v->info.pending&&(v->flags18&1)!=0,&env,&s->frontier);if(e)return e;if(voice_uses_ee_mix(v))return spatial_update(s,v,&env);if(v->info.position_mode)return unsupported(s,0x2cf058);if(env.se_level_q8!=s->mix.se_level_q8)return unsupported(s,0x375630);if(env.output_mode!=1)return unsupported(s,0x37d680);if(env.system_flags_5e4&0x08000000u)return unsupported(s,0x2ce588);if(env.game_flags_5d8&0x40u)return unsupported(s,0x2cda80);if(v->flags18&~5u)return unsupported(s,v->info.pending?0x2ce588:0x2cea00);if(v->info.pending&&env.reverb)return unsupported(s,0x2ce588);if(!v->info.pending&&(v->flags18&4)&&env.gain64_bits!=0x3f800000u)return unsupported(s,0x2cea00);
 if(v->model){float speed;memcpy(&speed,v->model->root.bytes+0x5a8,4);if(!isfinite(speed)||((v->flags1c&1)&&(speed<.5f||(v->info.pending&&env.debug58c>=0&&speed<1))))return unsupported(s,v->info.pending?0x2ce588:0x2cea00);}return GH_VOICE_OK;}
gh_voice_status gh_se_voice_detach_actor_002ce488(gh_se_voice_engine*s,gh_actor_model*model){if(!s||!model)return GH_VOICE_ARGUMENT;s->frontier=0;for(unsigned i=0;i<64;i++){struct voice*v=s->voices+i;if(v->used!=1||v->model!=model)continue;if(v->info.pending){gh_voice_status e=source_position(v,0);if(e){s->frontier=0x2ce488;return e;}}if(gh_actor_model_sound_return(model))return GH_VOICE_ARGUMENT;v->model=NULL;v->part_index=-1;}return GH_VOICE_OK;}
gh_voice_status gh_se_voice_abort_attached_owner(gh_se_voice_engine *s,const void *owner)
{
 if(!s||!owner)return GH_VOICE_ARGUMENT;
 for(unsigned i=0;i<64;++i){
  struct voice *v=s->voices+i;
  if(!v->used||v->source_owner!=owner)continue;
  if(v->info.native_voice){
   gh_voice_status error=gh_audio_output_stop(s->output,v->info.native_voice);
   if(error&&error!=GH_VOICE_BOUNDS){s->frontier=0x2ce488;return error;}
  }
  drop(s,(int)i,0);
 }
 return GH_VOICE_OK;
}
unsigned gh_se_voice_attached_reservations(const gh_se_voice_engine*s){unsigned n=0;if(s)for(unsigned i=0;i<64;i++)n+=s->voices[i].used==2;return n;}
static gh_voice_status stop_command(gh_se_voice_engine *s,int index,int ticks)
{
 struct voice *v=s->voices+index;
 int active=0;
 uint64_t played=0;
 gh_voice_status error=gh_audio_output_active(s->output,v->info.native_voice,&active,&played);
 if(error&&error!=GH_VOICE_BOUNDS)return error;
 if(error==GH_VOICE_BOUNDS||!active){drop(s,index,error==GH_VOICE_OK);return GH_VOICE_OK;}
 if(ticks<=0||!played){
  error=gh_audio_output_stop(s->output,v->info.native_voice);
  if(error&&error!=GH_VOICE_BOUNDS)return error;
  if(v->info.channel<51&&s->channels[v->info.channel]==index)s->channels[v->info.channel]=-1;
  v->info.channel=NONE;
  v->info.native_voice=0;
  v->info.retiring=1;
  v->info.stopping=0;
 }else if(v->info.stop_current){
  const int step=-(int)v->info.stop_current/ticks;
  v->info.stop_step=(int16_t)(step?step:-1);
  v->info.stop_count=0;
  v->info.stopping=1;
 }
 return GH_VOICE_OK;
}

static gh_voice_status sequence_queued_stops(gh_se_voice_engine *s)
{
 /* Original 375050 reserves its shared command before this frame's EE
  * start commands. 37BE98 observes that order for the ADX link allocator. */
 for(unsigned q=0;q<s->stop_count;q++){
  for(int j=s->head;j!=-1;j=s->voices[j].next){
   struct voice *v=s->voices+j;
   if(!v->external_adx||v->info.original_handle!=s->stops[q].handle)continue;
   if(v->info.native_voice){
    gh_voice_status error=stop_command(s,j,s->stops[q].ticks);
    if(error){s->frontier=0x37d820;return error;}
   }
   s->stops[q].handle=0;
   break;
  }
 }
 return GH_VOICE_OK;
}

static gh_voice_status voice_update_body(gh_se_voice_engine *s)
{
 int i;unsigned issued=s?s->stop_count:0;if(!s)return GH_VOICE_ARGUMENT;s->frontier=0;i=s->head;
 /* 375380 ages existing links before37BE98 consumes this frame's queue.
  * Newly constructed37C160 links start at0 and must stay0 this visit. */
 for(unsigned c=0;c<51;++c)if(s->channels[c]!=-1){struct voice *old=s->voices+s->channels[c];if(old->age!=UINT32_MAX)++old->age;}
 gh_voice_status queued=sequence_queued_stops(s);
 if(queued)return queued;
 while(i!=-1){struct voice *v=s->voices+i;int next=v->next;
  if(!v->used){i=next;continue;}
  /* Submitting a newer voice reaps finished XAudio2 sources. Resolve the
   * endpoint lifetime before updating the older voice's attached gains. */
  if(!v->info.pending&&v->info.native_voice){
   int active;uint64_t played;
   gh_voice_status error=gh_audio_output_active(s->output,v->info.native_voice,&active,&played);
   if(error&&error!=GH_VOICE_BOUNDS)return error;
   if(error==GH_VOICE_BOUNDS||!active){drop(s,i,error==GH_VOICE_OK);i=next;continue;}
  }
  if(!v->info.retiring){gh_voice_status attached=attached_update(s,v);if(attached)return attached;}
  if(v->info.pending){
   /* Original shared SDE queue has48 entries. Overflowed EE voices observe
    * no SDE handle on the next update; don't invent extra queue capacity. */
   if(issued>=48){v->info.pending=0;i=next;continue;}++issued;
   unsigned first=v->group_first,end=first+v->group_count,channel;
   int replaced=-1;
   for(channel=first;channel<end && s->channels[channel]!=-1;++channel){}
   if(channel==end){
    unsigned minimum=255,picked=first;uint32_t age=0;
    for(unsigned c=first;c<end;++c){struct voice *old=s->voices+s->channels[c];if(old->priority<minimum)minimum=old->priority;}
    if(v->priority<minimum || (v->priority==minimum && !v->replace_equal)){v->info.pending=0;++v->ee_age;i=next;continue;}
    for(unsigned c=first;c<end;++c){struct voice *old=s->voices+s->channels[c];if(old->priority==minimum && age<=old->age){age=old->age;picked=c;}}
    channel=picked;
    replaced=s->channels[channel];
    if(!v->external_adx){drop(s,replaced,1);replaced=-1;}
   }
   if((h(v->descriptor+0x18)||h(v->descriptor+0x1a)||v->spatial_slow)&&v->pitch_setup<2){
    if(!s->pitch.loaded)return unsupported(s,0x37d760);
    if(!v->pitch_setup){
     int16_t low,high;
     uint16_t a=(uint16_t)h(v->descriptor+0x18),b=(uint16_t)h(v->descriptor+0x1a);
     memcpy(&low,&a,2);memcpy(&high,&b,2);
     gh_voice_status e=gh_se_pitch_choose_0037d760(&s->pitch,low,high,&v->selected_offset);
     if(e){s->frontier=0x37d760;return e;}
     v->pitch_setup=1;
    }
    gh_voice_status pitch=resolve_ee_pitch(s,v,v->spatial_slow,&v->selected_pitch);
    if(pitch)return pitch;
    if(!v->selected_pitch)return unsupported(s,0x38e4);gh_voice_status e=prepare_pcm_with_pitch(s,v->info.bank,v->info.program,v->descriptor,v,v->selected_pitch);if(e)return e;v->pitch_setup=2;
   }
   uint16_t left=v->info.level_q14,right=left;
   if(voice_uses_ee_mix(v)){apply_spatial_mix(s,v);left=v->info.left_q14;right=v->info.right_q14;}
   float left_gain=(float)left/16384.0f,right_gain=(float)right/16384.0f;
   if(v->external_adx){
    /* 37CF00 uses the existing link table. 37BF10 interprets its final
     * three links as ADX sequence owners, separate from SPU voice DSP. */
    if(channel<48||channel>=51)return unsupported(s,0x37bf10);
    gh_voice_status error=GH_VOICE_OK;
    if(!v->pcm.samples)error=sequence_prepare(s,v);
    if(!error)error=sequence_gain(s,v,&left_gain);
    if(error)return error;
    right_gain=left_gain;
    if(replaced>=0){
     error=gh_audio_output_stop(s->output,s->voices[replaced].info.native_voice);
     if(error&&error!=GH_VOICE_BOUNDS){s->frontier=0x37d820;return error;}
     drop(s,replaced,0);
    }
   }
   gh_voice_status error=gh_audio_output_play(s->output,&v->pcm,48000,left_gain,right_gain,&v->info.native_voice);
   if(error){s->frontier=0x374df8;return error;}
   gh_audio_pcm_release(&v->pcm);v->submitted_pitch=v->info.pitch_q12;v->info.pending=0;v->info.channel=(uint8_t)channel;s->channels[channel]=i;
  }else if(!v->info.native_voice)drop(s,i,0);
  if(v->used)++v->ee_age; /* actual EE+24 ADDIU/SW, modulo2^32 */
  i=next;
 }
 /* QueueStop is consumed before the per-node ramp scan, matching
  *375F68/37BE98 before37CAE0. SamplesPlayed is the explicit native busy
  * acknowledgement replacing asynchronous IOP feedback, not an EE oracle. */
 for(unsigned q=0;q<s->stop_count;q++)for(int j=s->head;j!=-1;j=s->voices[j].next){struct voice*v=s->voices+j;
  if(v->info.original_handle!=s->stops[q].handle||!v->info.native_voice)continue;
  gh_voice_status e=stop_command(s,j,s->stops[q].ticks);if(e)return e;
  break;
 }
 s->stop_count=0;
 for(int j=s->head;j!=-1;j=s->voices[j].next){struct voice*v=s->voices+j;if(!v->info.stopping||!v->info.native_voice)continue;
  if(!v->info.stop_current){gh_voice_status e=gh_audio_output_stop(s->output,v->info.native_voice);if(e&&e!=GH_VOICE_BOUNDS)return e;
   if(v->info.channel<51&&s->channels[v->info.channel]==j)s->channels[v->info.channel]=-1;
   v->info.channel=NONE;v->info.native_voice=0;v->info.retiring=1;v->info.stopping=0;
  }else{int next=v->info.stop_current+v->info.stop_step;if(next<0)next=0;v->info.stop_current=(int16_t)next;v->info.stop_count++;
   v->info.level_q14=(uint16_t)(((uint32_t)v->base_gain*(unsigned)next/0x7f00)/2);
   uint16_t left=v->info.level_q14,right=left;
   if(voice_uses_ee_mix(v)){apply_spatial_mix(s,v);left=v->info.left_q14;right=v->info.right_q14;}
   float left_gain=(float)left/16384.0f,right_gain=(float)right/16384.0f;
   if(v->external_adx){
    gh_voice_status e=sequence_gain(s,v,&left_gain);if(e)return e;
    right_gain=left_gain;
   }
   gh_voice_status e=gh_audio_output_voice_gain(s->output,v->info.native_voice,left_gain,right_gain);
   if(e==GH_VOICE_BOUNDS)drop(s,j,0);else if(e)return e;
  }
 }
 return GH_VOICE_OK;
}
gh_voice_status gh_se_voice_update(gh_se_voice_engine*s){if(!s)return GH_VOICE_ARGUMENT;fenv_t env;if(feholdexcept(&env))return GH_VOICE_PLATFORM;if(fesetround(FE_TONEAREST)){(void)fesetenv(&env);return GH_VOICE_PLATFORM;}gh_voice_status e=voice_update_body(s);if(fesetenv(&env))return GH_VOICE_PLATFORM;return e;}
gh_voice_status gh_se_voice_query(const gh_se_voice_engine *s,uint32_t handle,gh_se_voice_info *out)
{if(!s || !handle || !out)return GH_VOICE_ARGUMENT;for(unsigned i=0;i<64;++i)if(s->voices[i].used && s->voices[i].info.original_handle==handle){*out=s->voices[i].info;out->attached_model=s->voices[i].model;out->attached=s->voices[i].model!=NULL;out->attached_part=s->voices[i].model?s->voices[i].part_index:-1;return GH_VOICE_OK;}return GH_VOICE_BOUNDS;}
uint32_t gh_se_voice_frontier(const gh_se_voice_engine *s){return s?s->frontier:0;}
gh_voice_status gh_se_voice_pcm_view(const gh_se_voice_engine *s,uint32_t handle,const int16_t **samples,size_t *frames)
{
 if(!s || !handle || !samples || !frames)return GH_VOICE_ARGUMENT;
 for(unsigned i=0;i<64;++i)if(s->voices[i].used && s->voices[i].info.original_handle==handle){
  if(!s->voices[i].info.pending)return GH_VOICE_BOUNDS;
  if(!s->voices[i].info.pitch_q12)return GH_VOICE_UNSUPPORTED;
  if(s->voices[i].external_adx&&!s->voices[i].pcm.samples)return GH_VOICE_BOUNDS;
  *samples=s->voices[i].pcm.samples;*frames=s->voices[i].pcm.frames;return GH_VOICE_OK;
 }
 return GH_VOICE_BOUNDS;
}

unsigned gh_se_voice_external_adx_nodes(const gh_se_voice_engine *s)
{
 unsigned count=0;
 if(s)for(unsigned i=0;i<64;i++)
  count+=s->voices[i].used&&s->voices[i].external_adx;
 return count;
}
unsigned gh_se_voice_active_nodes(const gh_se_voice_engine*s){unsigned n=0;if(s)for(unsigned i=0;i<64;i++)n+=s->voices[i].used==1;return n;}
gh_voice_status gh_se_voice_request_stop_00375050(gh_se_voice_engine*s,uint32_t handle,int16_t ticks,int*accepted){
 if(!s||!handle||!accepted)return GH_VOICE_ARGUMENT;*accepted=0;s->frontier=0;
 if(ticks < -1)return unsupported(s,0x378020);
 /* Zero/finished ramp sets the IOP 3998 fast-release override. A -1
  * key-off keeps the sample's release rate and needs a native tail. */
 if(ticks==-1)for(int i=s->head;i!=-1;i=s->voices[i].next){
  const struct voice *voice=s->voices+i;
  if(voice->info.original_handle==handle&&voice->info.adsr2==0x5fcb&&!voice->info.retiring)
   return unsupported(s,0x37d820);
 }
 if(s->stop_count==48)return GH_VOICE_OK;
 s->stops[s->stop_count].handle=handle;s->stops[s->stop_count].ticks=ticks;s->stop_count++;*accepted=1;return GH_VOICE_OK;
}
gh_voice_status gh_se_voice_stop_bank_002cbc58(gh_se_voice_engine*s,int16_t bank,int16_t ticks){
 if(!s||bank<0||bank>=52)return GH_VOICE_ARGUMENT;if(ticks < -1)return unsupported(s,0x378020);s->frontier=0;
 /* Check before dropping pending nodes or publishing any stop commands. */
 if(ticks==-1)for(int i=s->head;i!=-1;i=s->voices[i].next){
  const struct voice *voice=s->voices+i;
  if(voice->info.bank==bank&&voice->info.native_voice&&voice->info.adsr2==0x5fcb)
   return unsupported(s,0x37d820);
 }
 for(int i=s->head;i!=-1;){struct voice*v=s->voices+i;int next=v->next;
  if(v->info.bank==bank){if(v->info.native_voice||v->info.retiring){int accepted;gh_voice_status e=gh_se_voice_request_stop_00375050(s,v->info.original_handle,ticks,&accepted);if(e)return e;/* Originalvoid ignores queuefull. */}else drop(s,i,0);}i=next;
 }return GH_VOICE_OK;
}
gh_voice_status gh_se_voice_scene_exit_begin(gh_se_voice_engine*s){
 if(!s)return GH_VOICE_ARGUMENT;s->frontier=0;
 for(int bank=0;bank<52;bank++){gh_voice_status e=gh_se_voice_stop_bank_002cbc58(s,(int16_t)bank,10);if(e)return e;}
 for(int i=s->head;i!=-1;){struct voice*v=s->voices+i;int next=v->next;if(!v->info.native_voice&&!v->info.retiring)drop(s,i,0);i=next;}
 return GH_VOICE_OK;
}

int gh_se_voice_uses_registry(const gh_se_voice_engine*s,const gh_audio_registry*r){return s&&r&&s->registry==r;}

unsigned gh_se_voice_bank_nodes(const gh_se_voice_engine*s,int16_t bank){unsigned n=0;if(s)for(unsigned i=0;i<64;i++)n+=s->voices[i].used==1&&s->voices[i].info.bank==bank;return n;}

/* Native sound SDK setup, once before voices: own tables and original RNG,
 * loaded through the SAME disc; no module execution or DSP emulation. */
gh_voice_status gh_se_voice_bind_pitch_reference(gh_se_voice_engine*s,gh_disc*d){if(!s||!d)return GH_VOICE_ARGUMENT;if(s->pitch.loaded||s->head!=-1||gh_se_voice_attached_reservations(s))return GH_VOICE_UNSUPPORTED;return gh_se_pitch_load_reference(&s->pitch,d);}
gh_voice_status gh_se_voice_pitch_state(const gh_se_voice_engine*s,uint16_t*out){if(!s||!out)return GH_VOICE_ARGUMENT;if(!s->pitch.loaded)return GH_VOICE_UNSUPPORTED;*out=s->pitch.random_state;return GH_VOICE_OK;}
